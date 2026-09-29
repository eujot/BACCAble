import contextlib
import copy
import hashlib
import io
import json
from pathlib import Path
import re
import sqlite3
import tempfile
import unittest

from baccable_lab.cli import main
from baccable_lab.knowledge.catalog import crc8_j1850, decode_frame, load_dictionary, validate_dictionary
from baccable_lab.knowledge.review import display_evidence, fingerprints, open_readonly, review_session
from baccable_lab.knowledge.profiles import profile_frames
from baccable_lab.storage.schema import SCHEMA


class KnowledgeTests(unittest.TestCase):
    def setUp(self):
        self.catalog = load_dictionary()

    def values(self, role, can_id, data):
        return {s['name']: s for s in decode_frame(self.catalog, role, can_id, bytes.fromhex(data))['signals']}

    def test_source_snapshot_and_dispatcher_coverage(self):
        root = Path(__file__).resolve().parents[2]
        for path, expected in self.catalog['source_sha256'].items():
            self.assertEqual(hashlib.sha256((root / path).read_bytes()).hexdigest(), expected, path)
        dispatcher = (root / 'firmware/baccable/vehicle/standard_frames.c').read_text()
        ids = {int(value, 16) for value in re.findall(r'case\s+(0x[0-9a-fA-F]+)', dispatcher)}
        self.assertEqual(len(ids), 31)
        self.assertLessEqual(ids, {int(m['can_id'], 16) for m in self.catalog['messages']})

    def test_duplicate_bus_identity_and_bad_bit_layout_rejected(self):
        bad = copy.deepcopy(self.catalog)
        bad['messages'].append(bad['messages'][0])
        with self.assertRaises(ValueError):
            validate_dictionary(bad)
        bad = copy.deepcopy(self.catalog)
        bad['messages'][0]['signals'][0]['parts'] = [[7, 7, 2]]
        with self.assertRaises(ValueError):
            validate_dictionary(bad)

    def test_split_fields_and_physical_conversions(self):
        self.assertEqual(self.values('C1', 0xFC, '0FA0000000000000')['engine_rpm']['value'], 1000)
        self.assertEqual(self.values('C1', 0x101, '0064000000000000')['vehicle_speed']['value'], 50)
        self.assertEqual(self.values('C1', 0x4B2, '00001B8000000000')['oil_temperature']['value'], 70)
        self.assertAlmostEqual(self.values('C1', 0x41A, '004E00009C400000')['battery_current']['value'], 0)
        self.assertEqual(self.values('C1', 0x41A, '007F000000000000')['battery_soc']['value'], 127)

    def test_bus_specific_light_fields_and_origin(self):
        c1 = self.values('C1', 0x73E, '0080020000000000')
        c2 = self.values('C2', 0x73E, '0080020000000000')
        self.assertIn('left_lamp_report', c1)
        self.assertNotIn('left_activation_report', c1)
        self.assertIn('left_activation_report', c2)
        self.assertNotIn('left_lamp_report', c2)
        self.assertEqual(self.values('BH', 0x354, '0080000000000000')['left_lamp_report']['origin'], 'comment')
        self.assertFalse(decode_frame(self.catalog, 'BH', 0x41A, bytes(8))['signals'])

    def test_payload_guards_and_unknown_frames(self):
        self.assertFalse(self.values('C1', 0x2FA, '5000'))
        self.assertFalse(self.values('C1', 0x412, '00000033'))
        self.assertIsNone(self.values('C1', 0x412, '0000000000')['accelerator_percent']['value'])
        self.assertEqual(decode_frame(self.catalog, 'C1', 0x123, bytes(8))['signals'], [])
        with self.assertRaises(ValueError):
            decode_frame(self.catalog, 'C1', 0x2FA, bytes(9))

    def test_diagnostic_soc_sources_and_intercooler(self):
        bcm = decode_frame(self.catalog, 'C1', 0x18DAF140, bytes.fromhex('0662100500004E00'))['diagnostics']
        ecm = decode_frame(self.catalog, 'C1', 0x18DAF110, bytes.fromhex('046219BD4D000000'))['diagnostics']
        ic = decode_frame(self.catalog, 'C1', 0x18DAF110, bytes.fromhex('0462193546000000'))['diagnostics']
        self.assertEqual([(p['parameter_id'], p['value']) for p in bcm], [(34, 78)])
        self.assertEqual([(p['parameter_id'], p['value']) for p in ecm], [(21, 77)])
        self.assertEqual([(p['parameter_id'], p['value']) for p in ic], [(22, 30)])
        self.assertFalse(decode_frame(self.catalog, 'C2', 0x18DAF110, bytes.fromhex('046219BD4D000000'))['diagnostics'])

    def test_diagnostics_refuse_nrc_multiframe_mismatch_and_truncation(self):
        for data in ('037F223100000000', '10096219BD4D0000', '214D000000000000', '046219BE4D000000',
                     '046219BD', '086219BD4D000000', '036219BD4D000000'):
            self.assertFalse(decode_frame(self.catalog, 'C1', 0x18DAF110, bytes.fromhex(data))['diagnostics'], data)
        self.assertEqual([p['parameter_id'] for p in self.catalog['diagnostic_parameters'] if not p['request_shape_valid']],
                         [50, 51, 52, 53])

    def test_checksum_standard_vector(self):
        self.assertEqual(crc8_j1850(b'123456789'), 0x4B)

    def test_empirical_template_match_does_not_name_unknown_signal(self):
        catalog = copy.deepcopy(self.catalog)
        catalog['frame_templates'] = [{'role':'C1','can_id':'0x123','dlc':1,
                                      'stable_bit_masks':'FD','stable_values':'80','sessions':['a','b']}]
        matching = decode_frame(catalog,'C1',0x123,b'\x82')
        self.assertEqual(matching['baseline']['status'],'matches_observed_stable_bits')
        self.assertEqual(matching['signals'],[])
        self.assertIsNone(matching['title'])
        self.assertEqual(decode_frame(catalog,'C1',0x123,b'\x83')['baseline']['status'],
                         'differs_from_observed_stable_bits')
        self.assertIsNone(decode_frame(catalog,'C2',0x123,b'\x82')['baseline'])
        catalog['frame_templates'][0].update(stable_bit_masks='00',stable_values='00')
        self.assertEqual(decode_frame(catalog,'C1',0x123,b'\x82')['baseline']['status'],
                         'no_observed_stable_bits')

    def test_profiles_find_steps_but_do_not_infer_semantics(self):
        rows = [('C1',0x123,1,ns*1_000_000_000,ns*1000,bytes([value]))
                for ns,value in [(3,128),(4,128),(5,128),(8,130),(9,130),(9.5,130)]]
        class Database:
            def execute(self, sql):
                return iter(rows)
        events = [{'id':1,'host_ns':8_000_000_000,'label':'window_open','source':'keyboard'},
                  {'id':2,'host_ns':8_000_000_000,'label':'window_open','source':'analysis:can'}]
        result = profile_frames(Database(),self.catalog,events)
        profile = result['frame_profiles'][0]
        self.assertEqual(profile['volatile_bit_masks'],'02')
        self.assertEqual(profile['stable_bit_masks'],'FD')
        self.assertEqual(profile['stable_values'],'80')
        self.assertFalse(profile['payload_constant'])
        step = result['candidate_bit_steps'][0]
        self.assertEqual((step['byte'],step['bit'],step['before'],step['after']),(0,1,0,1))
        self.assertEqual(step['status'],'exploratory_bit_step')
        self.assertEqual(step['known_fields'],[])
        self.assertEqual(len(result['candidate_bit_steps']),1)

    def test_packaged_data_and_generated_catalog_are_current(self):
        import importlib.util
        import tomllib
        root = Path(__file__).resolve().parents[1]
        patterns = tomllib.loads((root/'pyproject.toml').read_text())['tool']['setuptools']['package-data']['baccable_lab']
        for name in ('giulia.json','hypotheses.json'):
            self.assertTrue(any(Path('knowledge',name).match(pattern) for pattern in patterns))
        spec = importlib.util.spec_from_file_location('render',root/'scripts/render_can_dictionary.py')
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        self.assertEqual((root/'docs/CAN_DICTIONARY.md').read_text(),module.render(self.catalog))
        report = root/self.catalog['session_evidence']['report']
        self.assertEqual(hashlib.sha256(report.read_bytes()).hexdigest(),self.catalog['session_evidence']['report_sha256'])
        evidence = json.loads(report.read_text())
        hypotheses = json.loads((root/'baccable_lab/knowledge/hypotheses.json').read_text())['hypotheses']
        self.assertEqual(len({h['id'] for h in hypotheses}),len(hypotheses))
        sessions = {s['session']:{e['id'] for e in s['events']} for s in evidence['sessions']}
        for hypothesis in hypotheses:
            for item in hypothesis['evidence']:
                self.assertLessEqual(set(item['event_ids']),sessions[item['session']])
        spec = importlib.util.spec_from_file_location('attach',root/'scripts/attach_dictionary_evidence.py')
        attach = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(attach)
        # Historical reports keep their original version. Exercise both guards
        # independently of later firmware/dictionary snapshot updates.
        mismatched = {**evidence, 'dictionary_version': 'different-snapshot'}
        with self.assertRaisesRegex(ValueError, 'source versions differ'):
            attach.attach(copy.deepcopy(self.catalog), mismatched, 'unused')
        partial = {**evidence, 'sessions': evidence['sessions'][-1:],
                   'dictionary_version': self.catalog['dictionary_version'],
                   'firmware_source_commit': self.catalog['firmware_source_commit']}
        with self.assertRaisesRegex(ValueError,'discard previous session evidence'):
            attach.attach(copy.deepcopy(self.catalog),partial,'unused')

    def test_cli_passive_decode_and_validation(self):
        with contextlib.redirect_stdout(io.StringIO()) as output:
            self.assertEqual(main(['dictionary', '--role', 'BH', '--id', '0x46C', '--data', '0000000000000400']), 0)
        decoded = json.loads(output.getvalue())
        self.assertEqual(next(s['enum'] for s in decoded['signals'] if s['name'] == 'indicator_request'), 'left')
        with contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(main(['dictionary', '--data', '00']), 2)

    def test_session_review_preserves_inputs_and_separates_derived_markers(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp) / 'test'
            directory.mkdir()
            database = sqlite3.connect(directory / 'session.sqlite3')
            database.executescript(SCHEMA)
            database.execute("INSERT INTO sessions VALUES ('test','now','later',3,'complete','monotonic')")
            sequence = 0
            def frame(role, can_id, host, data, device_ms=None):
                nonlocal sequence
                sequence += 1
                database.execute('INSERT INTO can_frames VALUES (?,?,?,?,?,?,?,?,?,?,?)',
                                 (sequence,'test',role,sequence,host,0,device_ms if device_ms is not None else host//1_000_000,
                                  can_id,len(data),data,0))
            frame('BH', 0x46C, 500_000_000, bytes(8))
            frame('BH', 0x46C, 1_000_000_000, bytes.fromhex('0000000000000400'))
            frame('BH', 0x90, 2_000_000_000, bytes.fromhex('08060041000D0042'))
            frame('BH', 0x90, 2_100_000_000, bytes.fromhex('084600B000000000'))
            for ms in range(0, 1601, 100):
                code = 0x10 if ms in (0, 1600) else 0x50
                payload = bytes((code, ms//100 & 15))
                frame('C1', 0x2FA, ms*1_000_000, payload+bytes((crc8_j1850(payload),)), ms)
            database.execute("INSERT INTO events VALUES (1,'test',1200000000,'left_indicator_on','keyboard','')")
            database.execute("INSERT INTO events VALUES (2,'test',1200000000,'left_indicator_on','analysis:can','derived')")
            database.commit()
            database.close()
            before = fingerprints(directory)
            result = review_session(directory, self.catalog)
            self.assertEqual(fingerprints(directory), before)
            correlations = [c for c in result['marker_correlations'] if c['signal']=='indicator_request']
            self.assertEqual(len(correlations), 1)
            self.assertEqual(correlations[0]['transition']['delta_seconds'], -0.2)
            self.assertEqual(len(result['events']), 2)
            self.assertEqual(result['steering']['long_holds'][0]['device_duration_ms'], 1500)
            message = result['ipc']['complete_messages'][0]
            self.assertEqual(message['text_code_units'], 4)
            self.assertEqual(message['line_separator_positions'], [1])
            self.assertEqual(message['non_ascii_code_units'], [0xB0])
            with contextlib.redirect_stdout(io.StringIO()) as output:
                self.assertEqual(main(['session','decode','test','--sessions',str(directory.parent),
                                       '--role','BH','--id','0x46C']),0)

    def test_review_output_cannot_overwrite_capture_or_existing_file(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp) / 'sessions'
            root.mkdir()
            output = Path(temp) / 'report.json'
            output.write_text('preserve')
            with contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(main(['review','--sessions',str(root),'--output',str(root/'report.json')]), 2)
                self.assertEqual(main(['review','--sessions',str(root),'--output',str(output)]), 2)
            self.assertEqual(output.read_text(), 'preserve')

    def test_readonly_includes_committed_wal_and_rejects_writes(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            writer = sqlite3.connect(directory/'session.sqlite3')
            try:
                writer.executescript(SCHEMA)
                writer.execute("INSERT INTO sessions VALUES ('wal','now','later',1,'complete','monotonic')")
                writer.commit()
                self.assertGreater((directory/'session.sqlite3-wal').stat().st_size,0)
                before = fingerprints(directory)
                reader = open_readonly(directory)
                try:
                    self.assertEqual(reader.execute('SELECT id FROM sessions').fetchone()[0],'wal')
                    with self.assertRaises(sqlite3.OperationalError):
                        reader.execute('DELETE FROM sessions')
                finally:
                    reader.close()
                self.assertEqual(fingerprints(directory),before)
            finally:
                writer.close()

    def test_ongoing_session_is_not_a_review_baseline(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            database = sqlite3.connect(directory/'session.sqlite3')
            database.executescript(SCHEMA)
            database.execute("INSERT INTO sessions VALUES ('active','now',NULL,NULL,'recording','monotonic')")
            database.commit()
            database.close()
            before = fingerprints(directory)
            with self.assertRaisesRegex(ValueError,'finish the session'):
                review_session(directory,self.catalog)
            self.assertEqual(fingerprints(directory),before)

    def test_incomplete_or_malformed_ipc_transfer_is_not_a_complete_message(self):
        class Database:
            def __init__(self, rows):
                self.rows = rows
            def execute(self, sql):
                return iter(self.rows)
        # First fragment promises indexes 0,1,2. Index 1 is missing or malformed.
        first = (0,0,8,bytes.fromhex('1006004100420043'))
        last = (100_000_000,100,8,bytes.fromhex('1086004700480049'))
        missing = display_evidence(Database([first,last]))
        self.assertEqual(missing['complete_messages'],[])
        self.assertGreater(missing['incomplete_sequences'],0)
        bad = (50_000_000,50,7,bytes.fromhex('10460044004500'))
        malformed = display_evidence(Database([first,bad,last]))
        self.assertEqual(malformed['complete_messages'],[])
        self.assertEqual(malformed['malformed_frames'],1)


if __name__ == '__main__':
    unittest.main()

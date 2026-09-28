import contextlib
import io
import itertools
import json
import sqlite3
import tempfile
import time
import unittest
from pathlib import Path
from unittest.mock import patch

from baccable_lab.capture import capture
from baccable_lab.cli import _export_basic, _session_info, main

FRAME = bytes([0xA1, 1, 0, 0, 0x34, 0x12, 0, 0, 0xAA] + [0] * 7)
LOSS = bytes([0xAF, 1, 0, 0, 5, 0] + [0] * 10)


class CaptureTests(unittest.TestCase):
    def test_marker_palette_custom_label_and_undo(self):
        with tempfile.TemporaryDirectory() as temp:
            class Reader:
                error = None
                discarded_bytes = 0
                def __init__(self, role, device, output, stop, abort): self.role, self.output = role, output
                def start(self): pass
                def is_alive(self): return False
                def join(self, timeout): pass
            with patch('baccable_lab.capture._Reader', Reader), \
                    patch('baccable_lab.capture._Keyboard') as keyboard, \
                    contextlib.redirect_stdout(io.StringIO()):
                keyboard.return_value.read.side_effect = ['c', 'u', '7', 'q']
                keyboard.return_value.read_line.return_value = 'hood_open'
                self.assertEqual(capture({'C1': '/dev/test-C1'}, Path(temp), ['capture']), 0)
            directory, = Path(temp).iterdir()
            with contextlib.closing(sqlite3.connect(directory / 'session.sqlite3')) as db:
                events = db.execute('SELECT label FROM events ORDER BY id').fetchall()
            self.assertEqual(events, [('left_indicator_on',)])

    def test_grouped_marker_selection_is_saved_with_stable_label(self):
        with tempfile.TemporaryDirectory() as temp:
            class Reader:
                error = None
                discarded_bytes = 0
                def __init__(self, role, device, output, stop, abort): self.role = role
                def start(self): pass
                def is_alive(self): return False
                def join(self, timeout): pass
            with patch('baccable_lab.capture._Reader', Reader), \
                    patch('baccable_lab.capture._Keyboard') as keyboard, \
                    contextlib.redirect_stdout(io.StringIO()):
                keyboard.return_value.read.side_effect = ['g', 'i', '2', 'q']
                self.assertEqual(capture({'C1': '/dev/test-C1'}, Path(temp), ['capture']), 0)
            directory, = Path(temp).iterdir()
            with contextlib.closing(sqlite3.connect(directory / 'session.sqlite3')) as db:
                label, source = db.execute('SELECT label, source FROM events').fetchone()
            self.assertEqual((label, source), ('ipc_menu_overwritten', 'keyboard'))

    def test_search_finds_alias_and_saves_stable_marker(self):
        with tempfile.TemporaryDirectory() as temp:
            class Reader:
                error = None
                discarded_bytes = 0
                def __init__(self, role, device, output, stop, abort): self.role = role
                def start(self): pass
                def is_alive(self): return False
                def join(self, timeout): pass
            with patch('baccable_lab.capture._Reader', Reader), \
                    patch('baccable_lab.capture._Keyboard') as keyboard, \
                    contextlib.redirect_stdout(io.StringIO()):
                keyboard.return_value.read.side_effect = ['f', 'q']
                keyboard.return_value.read_line.return_value = 'ABS'
                self.assertEqual(capture({'C1': '/dev/test-C1'}, Path(temp), ['capture']), 0)
            directory, = Path(temp).iterdir()
            with contextlib.closing(sqlite3.connect(directory / 'session.sqlite3')) as db:
                event = db.execute('SELECT label, source FROM events').fetchone()
            self.assertEqual(event, ('abs_intervention', 'keyboard'))

    def test_search_adds_unmatched_text_as_custom_marker(self):
        with tempfile.TemporaryDirectory() as temp:
            class Reader:
                error = None
                discarded_bytes = 0
                def __init__(self, role, device, output, stop, abort): self.role = role
                def start(self): pass
                def is_alive(self): return False
                def join(self, timeout): pass
            with patch('baccable_lab.capture._Reader', Reader), \
                    patch('baccable_lab.capture._Keyboard') as keyboard, \
                    contextlib.redirect_stdout(io.StringIO()):
                keyboard.return_value.read.side_effect = ['f', 'q']
                keyboard.return_value.read_line.return_value = 'intercooler temperature peak'
                self.assertEqual(capture({'C1': '/dev/test-C1'}, Path(temp), ['capture']), 0)
            directory, = Path(temp).iterdir()
            with contextlib.closing(sqlite3.connect(directory / 'session.sqlite3')) as db:
                event = db.execute('SELECT label, source FROM events').fetchone()
            self.assertEqual(event, ('intercooler temperature peak', 'typed'))

    def test_search_can_select_one_of_multiple_matches(self):
        with tempfile.TemporaryDirectory() as temp:
            class Reader:
                error = None
                discarded_bytes = 0
                def __init__(self, role, device, output, stop, abort): self.role = role
                def start(self): pass
                def is_alive(self): return False
                def join(self, timeout): pass
            with patch('baccable_lab.capture._Reader', Reader), \
                    patch('baccable_lab.capture._Keyboard') as keyboard, \
                    contextlib.redirect_stdout(io.StringIO()):
                keyboard.return_value.read.side_effect = ['f', 'q']
                keyboard.return_value.read_line.side_effect = ['ACC', '1']
                self.assertEqual(capture({'C1': '/dev/test-C1'}, Path(temp), ['capture']), 0)
            directory, = Path(temp).iterdir()
            with contextlib.closing(sqlite3.connect(directory / 'session.sqlite3')) as db:
                label, = db.execute('SELECT label FROM events').fetchone()
            self.assertIn(label, {'acc_set', 'acc_following', 'acc_resume', 'acc_cancelled'})

    def test_search_enter_saves_ambiguous_query_as_custom_text(self):
        with tempfile.TemporaryDirectory() as temp:
            class Reader:
                error = None
                discarded_bytes = 0
                def __init__(self, role, device, output, stop, abort): self.role = role
                def start(self): pass
                def is_alive(self): return False
                def join(self, timeout): pass
            with patch('baccable_lab.capture._Reader', Reader), \
                    patch('baccable_lab.capture._Keyboard') as keyboard, \
                    contextlib.redirect_stdout(io.StringIO()):
                keyboard.return_value.read.side_effect = ['f', 'q']
                keyboard.return_value.read_line.side_effect = ['ACC', '']
                self.assertEqual(capture({'C1': '/dev/test-C1'}, Path(temp), ['capture']), 0)
            directory, = Path(temp).iterdir()
            with contextlib.closing(sqlite3.connect(directory / 'session.sqlite3')) as db:
                event = db.execute('SELECT label, source FROM events').fetchone()
            self.assertEqual(event, ('ACC', 'typed'))

    def test_all_bus_selections_and_empty_bus(self):
        all_roles = ('C1', 'C2', 'BH')
        selections = [roles for count in range(1, 4)
                      for roles in itertools.combinations(all_roles, count)]
        for roles, has_frames in [(roles, True) for roles in selections] + [(('BH',), False)]:
            with self.subTest(roles=roles, has_frames=has_frames), tempfile.TemporaryDirectory() as temp:
                opened = []

                class Reader:
                    error = None
                    discarded_bytes = 0

                    def __init__(self, role, device, output, stop, abort):
                        self.role, self.device, self.output = role, device, output

                    def start(self):
                        opened.append((self.role, self.device))
                        if has_frames:
                            self.output.put((self.role, time.monotonic_ns(), FRAME))

                    def is_alive(self):
                        return False

                    def join(self, timeout):
                        pass

                ports = {role: f'/dev/test-{role}' for role in roles}
                with patch('baccable_lab.capture._Reader', Reader), \
                        patch('baccable_lab.capture._Keyboard') as keyboard, \
                        contextlib.redirect_stdout(io.StringIO()):
                    keyboard.return_value.read.side_effect = [None] * len(roles) + ['q']
                    self.assertEqual(capture(ports, Path(temp), ['capture']), 0)
                self.assertEqual(dict(opened), ports)
                directory, = Path(temp).iterdir()
                self.assertEqual({p.stem for p in directory.glob('*.bin')}, set(roles))
                for role in roles:
                    self.assertEqual((directory / f'{role}.bin').read_bytes(), FRAME if has_frames else b'')
                manifest = json.loads((directory / 'manifest.json').read_text())
                summary = json.loads((directory / 'summary.json').read_text())
                self.assertEqual(manifest['roles'], ports)
                self.assertEqual(set(summary['roles']), set(roles))
                with contextlib.closing(sqlite3.connect(directory / 'session.sqlite3')) as db:
                    self.assertEqual(dict(db.execute('SELECT role, count(*) FROM can_frames GROUP BY role')),
                                     dict.fromkeys(roles, 1) if has_frames else {})
                with contextlib.redirect_stdout(io.StringIO()) as info:
                    self.assertEqual(_session_info(directory), 0)
                for role in all_roles:
                    self.assertEqual(f'  {role}:' in info.getvalue(), role in roles)
                with contextlib.redirect_stdout(io.StringIO()) as exported:
                    self.assertEqual(_export_basic(directory, None), 0)
                self.assertEqual(len(exported.getvalue().splitlines()), (len(roles) if has_frames else 0) + 1)

    def test_startup_loss_is_reported_separately(self):
        with tempfile.TemporaryDirectory() as temp:
            class Reader:
                error = None
                discarded_bytes = 0
                def __init__(self, role, device, output, stop, abort): self.role, self.output = role, output
                def start(self): self.output.put(('C1', time.monotonic_ns(), LOSS))
                def is_alive(self): return False
                def join(self, timeout): pass
            with patch('baccable_lab.capture._Reader', Reader), \
                    patch('baccable_lab.capture._Keyboard') as keyboard, \
                    contextlib.redirect_stdout(io.StringIO()) as output:
                keyboard.return_value.read.side_effect = [None, 'q']
                self.assertEqual(capture({'C1': '/dev/test-C1'}, Path(temp), ['capture']), 0)
            directory, = Path(temp).iterdir()
            summary = json.loads((directory / 'summary.json').read_text())
            self.assertEqual(summary['startup_dropped'], {'C1': 5})
            self.assertEqual(summary['in_session_dropped'], {'C1': 0})
            self.assertIn('WARNING C1', output.getvalue())

    def test_invalid_selections_do_not_create_sessions(self):
        for ports in ({}, {'OTHER': '/dev/a'}, {'C1': ''}, {'C1': '/dev/a', 'BH': '/dev/a'}):
            with self.subTest(ports=ports), tempfile.TemporaryDirectory() as temp:
                root = Path(temp) / 'sessions'
                with self.assertRaises(ValueError):
                    capture(ports, root, [])
                self.assertFalse(root.exists())

    def test_cli_requires_a_port(self):
        with contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit) as result:
            main(['capture'])
        self.assertEqual(result.exception.code, 2)

    def test_preview_command_needs_no_ports_and_uses_temporary_demo_roles(self):
        with patch('baccable_lab.capture._Keyboard') as keyboard, \
                contextlib.redirect_stdout(io.StringIO()) as output:
            keyboard.return_value.read.side_effect = ['q']
            self.assertEqual(main(['preview', '--role', 'BH']), 0)
        self.assertIn('Offline preview ended', output.getvalue())
        self.assertIn('no session was saved', output.getvalue())


if __name__ == '__main__':
    unittest.main()

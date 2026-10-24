"""Audit three October-24 control captures using original RX streams and markers."""
import argparse
import bisect
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import sqlite3
import statistics
import struct
import sys

from analyze_rain_lights import FIELDS as RAIN_FIELDS, fingerprints, sample, traces

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from baccable_lab.can.baccable_binary import BinaryCaptureParser
from baccable_lab.knowledge.catalog import crc8_j1850, extract_signal

SESSIONS = ('20261024-105643Z', '20261024-111510Z', '20261024-113104Z')
FIELDS = {key: list(value) for key, value in RAIN_FIELDS.items()}
FIELDS[('BH', 0x25A, 4)] = [('mode_raw', 0, 0x7C), ('calibration_raw', 1, 0x30)]
FIELDS[('BH', 0x46C, 8)] = [('mode_raw', 7, 0x1F)]
for role in ('C1', 'C2'):
    FIELDS[(role, 0x384, 8)] += [('mode_raw', 1, 0x7C), ('softer_report', 6, 0x80)]
    FIELDS[(role, 0x1FC, 8)] = [('mode_calibration_raw', 0, 0x70)]
FIELDS[('C1', 0x0FC, 8)] = []
FIELDS[('C1', 0x5A8, 8)] = [('transmission_mode_raw', 4, 0x78)]


def frame_hash(digest, timestamp, identifier, dlc, data):
    """Hash exactly the timestamp, identity, length and payload preserved in raw RX."""
    digest.update(struct.pack('<IIB', timestamp, identifier, dlc))
    digest.update(data)


def pulse_segments(rows, byte, mask, origin):
    """Retain press/release timing and repeated held reports on one board clock."""
    pulses, active = [], None
    for row in rows:
        value = row[7][byte] & mask
        if active is not None and value != active['raw']:
            active['release'] = sample(row, origin)
            active['duration_ms'] = row[3] - active['start']['device_ms']
            active = None
        if value:
            if active is None:
                active = dict(raw=value, start=sample(row, origin), held_reports=0)
                pulses.append(active)
            active['held_reports'] += 1
    return pulses


def state_near(rows, value, device_ms):
    """Return the last preceding sample without crossing to a different board clock."""
    at = bisect.bisect_right([row[3] for row in rows], device_ms) - 1
    return (rows[at], value(rows[at])) if at >= 0 else (None, None)


def analyze(directory, catalog):
    before = fingerprints(directory)
    connection = sqlite3.connect((directory / 'session.sqlite3').as_uri() + '?mode=ro', uri=True)
    try:
        connection.execute('BEGIN')
        metadata = connection.execute('SELECT status,duration_seconds FROM sessions').fetchone()
        if metadata[0] != 'complete':
            raise ValueError(f'Capture is not complete: {directory}')
        origin = connection.execute('SELECT min(host_ns) FROM can_frames').fetchone()[0]
        events = [dict(zip(('id', 'host_ns', 'label', 'source', 'note'), row)) for row in connection.execute(
            'SELECT id,host_ns,label,source,note FROM events ORDER BY host_ns,id')]
        steps = [dict(zip(('event_id', 'scenario', 'run', 'step_index', 'expect', 'outcome'), row)) for row in
                 connection.execute('SELECT event_id,scenario,run,step_index,expect,outcome FROM scenario_steps ORDER BY id')]
        inventory = [dict(zip(('role', 'can_id', 'dlc', 'frames'), row)) for row in connection.execute(
            'SELECT role,arbitration_id,dlc,count(*) FROM can_frames GROUP BY role,arbitration_id,dlc')]
        groups = defaultdict(list)
        ids = sorted({key[1] for key in FIELDS})
        for row in connection.execute(
                'SELECT id,sequence,host_ns,device_timestamp_ms,role,arbitration_id,dlc,data FROM can_frames '
                f'WHERE arbitration_id IN ({",".join("?" for _ in ids)}) ORDER BY role,sequence', ids):
            key = row[4:7]
            if key in FIELDS:
                groups[key].append(row)
        profiles = []
        for key, rows in sorted(groups.items()):
            intervals = [b[3] - a[3] for a, b in zip(rows, rows[1:]) if a[2] - origin >= 2_000_000_000]
            profiles.append(dict(role=key[0], can_id=f'0x{key[1]:03X}', dlc=key[2], frames=len(rows),
                settled_median_period_ms=statistics.median(intervals) if intervals else None,
                settled_max_gap_ms=max(intervals) if intervals else None,
                fields=traces(rows, FIELDS[key], origin)))
        raw_checks = {}
        for role in ('C1', 'C2', 'BH'):
            parser, raw_digest, sql_digest = BinaryCaptureParser(role), hashlib.sha256(), hashlib.sha256()
            with (directory / f'{role}.bin').open('rb') as source:
                while data := source.read(65536):
                    for record in parser.feed(data, 0):
                        if not record.is_loss:
                            frame_hash(raw_digest, record.raw_timestamp, record.arbitration_id, record.dlc, record.data)
            parser.finish()
            count = 0
            for timestamp, identifier, dlc, data in connection.execute(
                    'SELECT device_timestamp_raw,arbitration_id,dlc,data FROM can_frames WHERE role=? ORDER BY sequence', (role,)):
                frame_hash(sql_digest, timestamp, identifier, dlc, data)
                count += 1
            if parser.stats.frames != count or raw_digest.digest() != sql_digest.digest():
                raise ValueError(f'Raw/SQLite contents differ: {directory.name}/{role}')
            raw_checks[role] = dict(parser=vars(parser.stats), sqlite_frames=count,
                                   canonical_frames_sha256=raw_digest.hexdigest(), content_matches=True)
        snapshots = []
        for event in events:
            states = {}
            for key, rows in sorted(groups.items()):
                if key[1] not in (0x25A, 0x46C, 0x384, 0x1FC, 0xFC, 0x5A8):
                    continue
                index = bisect.bisect_right([row[2] for row in rows], event['host_ns']) - 1
                if index >= 0:
                    row = rows[index]
                    states[f'{key[0]} 0x{key[1]:03X}/{key[2]}'] = dict(sample(row, origin),
                        age_host_ms=round((event['host_ns'] - row[2]) / 1e6, 3))
            snapshots.append(dict(event_id=event['id'], states=states))
        inputs, displays, modes = groups.get(('BH', 0x256, 4), []), groups.get(('BH', 0x25A, 4), []), groups.get(('BH', 0x46C, 8), [])
        buttons = pulse_segments(inputs, 1, 0x80, origin)
        directions = pulse_segments(inputs, 0, 0x60, origin)
        for pulse in buttons:
            at = pulse['start']['device_ms']
            previous, state = state_near(displays, lambda r: (r[7][0] & 0x7C, r[7][1] & 0x30), at - 1)
            pulse['before_display'] = sample(previous, origin) if previous else None
            changes = [r for r in displays if at <= r[3] <= at + 500 and
                       (r[7][0] & 0x7C, r[7][1] & 0x30) != state]
            pulse['first_changed_display'] = sample(changes[0], origin) if changes else None
            pulse['response_ms'] = changes[0][3] - at if changes else None
        for pulse in directions:
            at = pulse['start']['device_ms']
            previous, state = state_near(modes, lambda r: r[7][7] & 0x1F, at - 1)
            pulse['before_mode'] = sample(previous, origin) if previous else None
            changes = [r for r in modes if at <= r[3] <= at + 2000 and r[7][7] & 0x1F != state]
            pulse['first_changed_mode'] = sample(changes[0], origin) if changes else None
            pulse['response_ms'] = changes[0][3] - at if changes else None
        rpm_signal = next(s for m in catalog['messages'] if (m['role'], m['can_id']) == ('C1', '0x0FC')
                          for s in m['signals'] if s['name'] == 'engine_rpm')
        rpms = [extract_signal(rpm_signal, r[7])['value'] for r in groups.get(('C1', 0xFC, 8), []) if r[2] - origin >= 2_000_000_000]
        transmission = groups.get(('C1', 0x5A8, 8), [])
        result = dict(session=directory.name, status=metadata[0], duration_seconds=metadata[1],
            manifest=json.loads((directory / 'manifest.json').read_text()), summary=json.loads((directory / 'summary.json').read_text()),
            input_sha256=before, host_origin_ns=origin, frames=sum(i['frames'] for i in inventory),
            inventory=inventory, events=events, scenario_steps=steps, profiles=profiles,
            raw_checks=raw_checks, marker_snapshots=snapshots, suspension_pulses=buttons,
            selector_pulses=directions, settled_rpm=dict(samples=len(rpms), minimum=min(rpms) if rpms else None, maximum=max(rpms) if rpms else None),
            transmission_crc=dict(samples=len(transmission), valid=sum(crc8_j1850(r[7][:7]) == r[7][7] for r in transmission)))
    finally:
        connection.close()
    if fingerprints(directory) != before:
        raise ValueError(f'Original capture changed during analysis: {directory}')
    return result


def prior_check(result, report):
    """Compare previously published field traces and input fingerprints exactly."""
    old = next(s for s in report['sessions'] if s['session'] == result['session']) if 'sessions' in report else report
    actual = {(p['role'], p['can_id'], p['dlc']): p for p in result['profiles']}
    mismatches = []
    for profile in old['profiles']:
        new = actual[(profile['role'], profile['can_id'], profile['dlc'])]
        for name, field in profile['fields'].items():
            if field != new['fields'][name]:
                mismatches.append(f"{profile['role']} {profile['can_id']} {name}")
    return dict(changed_input_files=[name for name in old['input_sha256'] if old['input_sha256'][name] != result['input_sha256'].get(name)],
                mismatched_field_traces=mismatches, frame_count_matches=old['frames'] == result['frames'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sessions', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    root, output = args.sessions.resolve(), args.output.resolve()
    if output.exists() or output.is_relative_to(root):
        parser.error('Choose a new output path outside the original captures.')
    catalog = json.loads((ROOT / 'baccable_lab/knowledge/giulia.json').read_text())
    results = []
    for sid in SESSIONS:
        print(sid, flush=True)
        results.append(analyze(root / sid, catalog))
    for result, name in zip(results, ['20261024-rain-lights-evidence.json', '20261024-111510-rain-lights-evidence.json']):
        result['prior_evidence_check'] = prior_check(result, json.loads((ROOT / 'docs/research' / name).read_text()))
    output.write_text(json.dumps(dict(method='Three-session RX audit: canonical raw/SQLite hashes, same-board pulses and original human confirmations; no transmission.',
        analysis_script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        dictionary_snapshot=catalog['dictionary_version'], sessions=results), indent=2) + '\n')


if __name__ == '__main__':
    main()

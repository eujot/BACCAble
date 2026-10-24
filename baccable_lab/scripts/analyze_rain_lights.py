"""Correlate recorded lighting and wiper fields without modifying captures."""
import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import sqlite3
import statistics
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from baccable_lab.can.baccable_binary import BinaryCaptureParser

TARGET = '20261024-105643Z'
# Every identity includes its bus and exact recorded length.
FIELDS = {
    ('C2', 0x2EC, 8): [('cycle_phase', 2, 0x20), ('spray_candidate', 4, 0x10)],
    ('BH', 0x354, 4): [('dipped_report', 1, 0x40), ('historical_auto_flag', 3, 0x40)],
    ('C2', 0x73E, 4): [('dipped_report', 1, 0x40), ('historical_auto_flag', 3, 0x40)],
    ('C1', 0x73E, 4): [('left_position_report', 0, 2), ('right_position_report', 2, 0x80)],
    ('C1', 0x384, 8): [('dipped_associated', 3, 0x80)],
    ('C2', 0x384, 8): [('dipped_associated', 3, 0x80)],
    ('BH', 0x738, 8): [('light_channel_1_raw', 1, 0xFF), ('light_channel_2_raw', 2, 0xFF)],
    ('C1', 0x41A, 7): [('non_specific_load_candidate', 2, 0x60)],
    ('BH', 0x256, 4): [('direction', 0, 0x60), ('suspension_button', 1, 0x80)],
}


def fingerprints(directory):
    """Fingerprint original streams, metadata and committed SQLite/WAL content."""
    result = {}
    for name in ('manifest.json', 'summary.json', 'session.sqlite3',
                 'session.sqlite3-wal', 'C1.bin', 'C2.bin', 'BH.bin'):
        path = directory / name
        if path.exists():
            with path.open('rb') as source:
                result[name] = hashlib.file_digest(source, 'sha256').hexdigest()
    return result


def sample(row, start):
    """Keep original frame IDs and both clocks for each selected edge."""
    return dict(frame_id=row[0], sequence=row[1], host_ns=row[2],
                host_seconds=round((row[2] - start) / 1e9, 6),
                device_ms=row[3], data=row[7].hex(' '))


def traces(rows, fields, start):
    """Describe all field changes, retaining pulse widths on a single board clock."""
    result = {}
    for name, byte, mask in fields:
        changes, previous = [], None
        for row in rows:
            value = row[7][byte] & mask
            if previous != value:
                changes.append(dict(sample(row, start), raw=value))
                previous = value
        result[name] = dict(values=dict(Counter(str(r[7][byte] & mask) for r in rows)),
                            transitions=changes)
        if mask == 0x20 and byte == 2 and name == 'cycle_phase':
            pulses = []
            for before, after in zip(changes, changes[1:]):
                if before['raw'] and not after['raw']:
                    pulses.append(dict(start=before, end=after,
                                       width_ms=after['device_ms'] - before['device_ms']))
            result[name]['pulses'] = pulses
            result[name]['rising_edges'] = sum(c['raw'] != 0 for c in changes[1:])
    return result


def analyze(directory):
    """Read one finished session, including its WAL, and retain negative controls."""
    before = fingerprints(directory)
    connection = sqlite3.connect((directory / 'session.sqlite3').as_uri() + '?mode=ro', uri=True)
    try:
        connection.execute('BEGIN')
        metadata = connection.execute('SELECT status,duration_seconds FROM sessions').fetchone()
        if metadata[0] not in ('complete', 'failed'):
            raise ValueError(f'Unfinished capture: {directory.name}')
        start = connection.execute('SELECT min(host_ns) FROM can_frames').fetchone()[0]
        events = [dict(zip(('id', 'host_ns', 'label', 'source', 'note'), row)) for row in
                  connection.execute('SELECT id,host_ns,label,source,note FROM events ORDER BY host_ns,id')]
        inventory = [dict(zip(('role', 'can_id', 'dlc', 'frames'), row)) for row in
                     connection.execute('SELECT role,arbitration_id,dlc,count(*) FROM can_frames '
                                        'GROUP BY role,arbitration_id,dlc')]
        groups = defaultdict(list)
        ids = sorted({key[1] for key in FIELDS})
        for row in connection.execute(
                'SELECT id,sequence,host_ns,device_timestamp_ms,role,arbitration_id,dlc,data '
                f'FROM can_frames WHERE arbitration_id IN ({",".join("?" for _ in ids)}) ORDER BY role,sequence', ids):
            key = (row[4], row[5], row[6])
            if key in FIELDS:
                groups[key].append(row)
        profiles = []
        for key, rows in sorted(groups.items()):
            periods = [b[3] - a[3] for a, b in zip(rows, rows[1:])]
            profiles.append(dict(role=key[0], can_id=f'0x{key[1]:03X}', dlc=key[2], frames=len(rows),
                median_period_ms=statistics.median(periods) if periods else None,
                max_gap_ms=max(periods) if periods else None,
                fields=traces(rows, FIELDS[key], start)))
        summary_path = directory / 'summary.json'
        summary = json.loads(summary_path.read_text()) if summary_path.exists() else None
        result = dict(session=directory.name, status=metadata[0], duration_seconds=metadata[1],
                      host_origin_ns=start, input_sha256=before, events=events,
                      frames=sum(i['frames'] for i in inventory), inventory=inventory,
                      profiles=profiles, summary=summary)
        if directory.name == TARGET:
            raw_check = {}
            for role in ('C1', 'C2', 'BH'):
                parser = BinaryCaptureParser(role)
                with (directory / f'{role}.bin').open('rb') as source:
                    while chunk := source.read(65536):
                        parser.feed(chunk, 0)
                parser.finish()
                database_frames = sum(i['frames'] for i in inventory if i['role'] == role)
                raw_check[role] = dict(vars(parser.stats), database_frames=database_frames)
                if parser.stats.frames != database_frames:
                    raise ValueError(f'Raw/SQLite frame count differs for {role}')
            result['raw_replay'] = raw_check
    finally:
        connection.close()
    if fingerprints(directory) != before:
        raise ValueError(f'Input changed during analysis: {directory}')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sessions', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    root = args.sessions.resolve()
    output = args.output.resolve()
    if output.exists() or output.is_relative_to(root):
        parser.error('Use a new output path outside the original capture directory.')
    # Newer sessions may be recording while this historical review is running.
    directories = [p.parent for p in sorted(root.glob('*/session.sqlite3')) if p.parent.name <= TARGET]
    result = dict(target=TARGET, method='Exact bus/ID/DLC, full-session field edges and same-board pulse timing; no transmission.',
                  source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(), sessions=[])
    for directory in directories:
        print(directory.name, flush=True)
        result['sessions'].append(analyze(directory))
    output.write_text(json.dumps(result, indent=2) + '\n')


if __name__ == '__main__':
    main()

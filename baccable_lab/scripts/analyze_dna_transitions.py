"""Read the two DNA captures without modifying them; emit reproducible evidence."""
import bisect
import collections
import hashlib
import json
from pathlib import Path
import sqlite3
import statistics

ROOT = Path(__file__).resolve().parents[1]
SESSIONS = ("20261011-190746Z", "20261011-191050Z")
FIELDS = {
    ("C1", 0x384): (1, 0x7C, 2),
    ("C2", 0x384): (1, 0x7C, 2),
    ("BH", 0x46C): (7, 0x1F, 0),
    ("BH", 0x25A): (0, 0x7C, 2),
    ("C1", 0x5A8): (4, 0x78, 3),
    ("C1", 0x4AF): (0, 1, 0),
    ("C2", 0x4AF): (0, 1, 0),
    ("C2", 0x547): (6, 2, 1),
    ("BH", 0x4F6): (6, 2, 1),
    ("C1", 0x1FC): (0, 0xFF, 0),
    ("C2", 0x1FC): (0, 0xFF, 0),
    ("C1", 0x4AE): (3, 0xFF, 0),
    ("C2", 0x4AE): (3, 0xFF, 0),
    ("BH", 0x4AE): (3, 0xFF, 0),
    ("C1", 0x73C): (1, 0xFF, 0),
}


def fingerprints(directory):
    names = ("session.sqlite3", "session.sqlite3-wal", "manifest.json",
             "summary.json", "C1.bin", "C2.bin", "BH.bin")
    return {name: hashlib.sha256((directory / name).read_bytes()).hexdigest()
            for name in names if (directory / name).exists()}


def crc(payload):
    """Mirror firmware frame_checksum (SAE J1850 over all but the last byte)."""
    value = 255
    for byte in payload[:-1]:
        value ^= byte
        for _ in range(8):
            value = ((value << 1) ^ (0x1D if value & 128 else 0)) & 255
    return value ^ 255


def analyze(session):
    directory = ROOT / "sessions" / session
    before = fingerprints(directory)
    connection = sqlite3.connect(f"file:{directory}/session.sqlite3?mode=ro", uri=True)
    connection.row_factory = sqlite3.Row
    origin = connection.execute("SELECT min(host_ns) FROM can_frames").fetchone()[0]
    events = [dict(row) for row in connection.execute("SELECT * FROM events ORDER BY id")]
    steps = [dict(row) for row in connection.execute("SELECT * FROM scenario_steps ORDER BY id")]
    for event in events:
        event["relative_seconds"] = (event["host_ns"] - origin) / 1e9
    groups = collections.defaultdict(list)
    for row in connection.execute(
        "SELECT id,role,arbitration_id,dlc,host_ns,device_timestamp_ms,sequence,data "
        "FROM can_frames ORDER BY role,sequence"
    ):
        groups[(row["role"], row["arbitration_id"], row["dlc"])].append(row)
    connection.close()
    timelines = {}
    for (role, identifier), (byte, mask, shift) in FIELDS.items():
        rows = next(rows for (r, i, length), rows in groups.items()
                    if r == role and i == identifier and length > byte)
        changes, payload_changes = [], []
        previous, previous_data = None, None
        for row in rows:
            data = row["data"]
            # Retain raw examples but do not fill the report with counter/CRC churn.
            payload = data[:-2] if identifier in (0x4AE, 0x5A8) else data
            value = (data[byte] & mask) >> shift
            sample = {"frame_id": row["id"], "sequence": row["sequence"],
                      "host_ns": row["host_ns"],
                      "relative_seconds": (row["host_ns"] - origin) / 1e9,
                      "device_ms": row["device_timestamp_ms"],
                      "data": data.hex(" "), "field": value}
            if value != previous:
                changes.append(sample)
            if payload != previous_data:
                payload_changes.append(sample)
            previous, previous_data = value, payload
        timelines[f"{role} 0x{identifier:03X}"] = {
            "count": len(rows), "changes": changes,
            "median_interval_ms": statistics.median(
                b["device_timestamp_ms"] - a["device_timestamp_ms"]
                for a, b in zip(rows, rows[1:])),
            "payload_changes": payload_changes,
            "payload_change_exclusions": [6, 7] if identifier in (0x4AE, 0x5A8) else [],
            "last_byte_counts": dict(collections.Counter(row["data"][-1] for row in rows)),
            "crc_j1850_matches": sum(crc(row["data"]) == row["data"][-1] for row in rows),
        }
    # Search all byte values against settled states, bounded to the DNA campaign.
    # This is an association screen, not a decoder or proof of a control input.
    modes = timelines["C1 0x384"]["changes"]
    times = [x["host_ns"] for x in modes]
    end = max(s["host_ns"] for s in steps if s["scenario"] == "dna_all_modes")
    candidates = []
    for (role, identifier, length), rows in groups.items():
        values = [collections.defaultdict(collections.Counter) for _ in range(length)]
        for row in rows:
            time = row["host_ns"]
            index = bisect.bisect_right(times, time) - 1
            if index < 0 or time > end or time - times[index] < 500_000_000:
                continue
            if index + 1 < len(times) and times[index + 1] - time < 200_000_000:
                continue
            mode = modes[index]["field"]
            for byte, value in enumerate(row["data"]):
                values[byte][mode][value] += 1
        for byte, distributions in enumerate(values):
            if set(distributions) != {0, 2, 4, 12}:
                continue
            common = {mode: counts.most_common(1)[0][0] for mode, counts in distributions.items()}
            purity = {mode: max(counts.values()) / sum(counts.values())
                      for mode, counts in distributions.items()}
            if min(purity.values()) >= .95 and len(set(common.values())) > 1:
                candidates.append({"role": role, "can_id": f"0x{identifier:03X}",
                                   "dlc": length, "byte": byte, "mode_values": common,
                                   "purity": purity})
    after = fingerprints(directory)
    assert before == after, "Capture files changed during read-only analysis"
    return {"session": session, "input_sha256": before, "inputs_unchanged": True,
            "host_origin_ns": origin, "events": events, "steps": steps,
            "health": json.loads((directory / "summary.json").read_text()),
            "frame_count": sum(map(len, groups.values())), "timelines": timelines,
            "settled_byte_associations": candidates}


if __name__ == "__main__":
    print(json.dumps({"method": "analyze_dna_transitions.py; 0.5s settling, 0.2s pre-transition exclusion",
                      "sessions": [analyze(session) for session in SESSIONS]}, indent=2))

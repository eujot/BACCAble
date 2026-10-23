"""Check the BH 0x256 lead against every mode edge and all capture controls."""
import argparse
import bisect
import collections
from dataclasses import asdict
import json
from pathlib import Path
import sqlite3
import statistics
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from baccable_lab.can.baccable_binary import BinaryCaptureParser
from analyze_dna_all_sessions import fingerprints


def analyze(directory, inventory):
    results = []
    for session in inventory["sessions"]:
        sid = session["session"]["id"]
        path = directory / sid
        con = sqlite3.connect(f"file:{path / 'session.sqlite3'}?mode=ro", uri=True)
        rows = con.execute("SELECT id,sequence,device_timestamp_ms,host_ns,data FROM can_frames "
                           "WHERE role='BH' AND arbitration_id=598 AND dlc=4 ORDER BY sequence").fetchall()
        changes, pulses, previous = [], [], None
        for row in rows:
            if row[4] != previous:
                changes.append({"frame_id": row[0], "sequence": row[1], "device_ms": row[2],
                                "host_ns": row[3], "data": row[4].hex(" ")})
            previous = row[4]
        active = None
        for row in rows:
            direction = (row[4][0] >> 5) & 3
            if active is not None and direction != active["direction"]:
                active["release_device_ms"] = row[2]
                active["duration_ms"] = row[2] - active["device_ms"]
                active = None
            if direction and active is None:
                active = {"frame_id": row[0], "sequence": row[1], "device_ms": row[2],
                          "host_ns": row[3], "direction": direction, "data": row[4].hex(" ")}
                pulses.append(active)
        times = [p["device_ms"] for p in pulses]
        modes = session["mode_transitions"]["BH"]
        associations = []
        for old, new in zip(modes, modes[1:]):
            index = bisect.bisect_right(times, new["device_ms"]) - 1
            pulse = pulses[index] if index >= 0 else None
            delay = new["device_ms"] - pulse["device_ms"] if pulse else None
            associations.append({"from": old["mode"], "to": new["mode"],
                                 "mode_frame_id": new["frame_id"], "mode_device_ms": new["device_ms"],
                                 "lead": pulse if delay is not None and delay <= 2000 else None,
                                 "delay_ms": delay if delay is not None and delay <= 2000 else None})
        intervals = [b[2] - a[2] for a, b in zip(rows, rows[1:])]
        # Resolve raw-vs-SQLite count discrepancies, retaining the original health reports.
        raw_check = {}
        sql_count = sum(i["frames"] for i in session["inventory"])
        summary_count = sum(v.get("frames", 0) for v in session["summary"]["roles"].values())
        if sql_count != summary_count:
            for role in session["summary"]["roles"]:
                raw = path / f"{role}.bin"
                if not raw.exists():
                    continue
                parser = BinaryCaptureParser(role)
                mode_values, selector_values = collections.Counter(), collections.Counter()
                with raw.open("rb") as source:
                    while chunk := source.read(65536):
                        for record in parser.feed(chunk, 0):
                            if record.is_loss:
                                continue
                            if role in ("C1", "C2") and record.arbitration_id == 0x384 and record.dlc == 8:
                                mode_values[(record.data[1] >> 2) & 31] += 1
                            if role == "BH" and record.arbitration_id == 0x46C and record.dlc == 8:
                                mode_values[record.data[7] & 31] += 1
                            if role == "BH" and record.arbitration_id == 0x256 and record.dlc == 4:
                                selector_values[record.data.hex(" ")] += 1
                parser.finish()
                raw_check[role] = {"parser": asdict(parser.stats), "mode_values": dict(mode_values),
                                   "selector_values": dict(selector_values)}
        con.close()
        assert fingerprints(path) == session["input_sha256"]
        results.append({"session": sid, "frames": len(rows),
                        "payload_counts": dict(collections.Counter(row[4].hex(" ") for row in rows)),
                        "median_interval_ms": statistics.median(intervals) if intervals else None,
                        "payload_changes": changes, "pulses": pulses,
                        "mode_associations": associations, "raw_count_verification": raw_check})
    return results


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sessions", type=Path)
    parser.add_argument("inventory", type=Path)
    args = parser.parse_args()
    inventory = json.loads(args.inventory.read_text())
    print(json.dumps({"method": "BH device-clock 0x256 pulse onset/release vs every 0x46C transition",
                      "sessions": analyze(args.sessions, inventory)}, indent=2))

"""Inventory every capture and search all frame bits around N/D transitions, read-only."""
import argparse
import bisect
import collections
import hashlib
import json
from pathlib import Path
import sqlite3
import sys

ROOT = Path(__file__).resolve().parents[1]
PRIMARY = {"C1": (0x384, 1, 0x7C, 2), "C2": (0x384, 1, 0x7C, 2),
           "BH": (0x46C, 7, 0x1F, 0)}
RESPONSES = {
    "C1": [(0x384, 1, 0x7C, 0, 8), (0x5A8, 4, 0x78, 0, 0x10),
           (0x1FC, 0, 0xFF, 0x50, 0x40)],
    "C2": [(0x384, 1, 0x7C, 0, 8), (0x1FC, 0, 0xFF, 0x50, 0x40)],
    "BH": [(0x46C, 7, 0x1F, 0, 2), (0x25A, 0, 0x7C, 0x40, 0x20),
           (0x25A, 1, 0xFF, 0, 0x10)],
}


def fingerprints(directory):
    result = {}
    for name in ("session.sqlite3", "session.sqlite3-wal", "manifest.json",
                 "summary.json", "C1.bin", "C2.bin", "BH.bin"):
        path = directory / name
        if path.exists():
            with path.open("rb") as source:
                result[name] = hashlib.file_digest(source, "sha256").hexdigest()
    return result


def sample(row, anchor):
    return {"frame_id": row[0], "sequence": row[1], "device_ms": row[2],
            "host_ns": row[3], "relative_ms": row[2] - anchor,
            "data": row[6].hex(" ")}


def scan_window(rows, anchor, wide=False):
    """Screen persistent changes and returning pulses, retaining raw first changes."""
    center = anchor["device_ms"]
    groups = collections.defaultdict(list)
    for row in rows:
        groups[(row[4], row[5])].append(row)
    results = {"response_order": [], "bit_candidates": [], "appearing_ids": []}
    for (identifier, length), frames in groups.items():
        lower, upper = (-5000, -2000) if wide else (-1500, -30)
        before = [r for r in frames if max(center + lower, anchor["previous_since"] + 20)
                  <= r[2] < center + upper]
        after = [r for r in frames if center + 500 <= r[2] < min(center + 1500, anchor["next_since"])]
        near = [r for r in frames if center - (2000 if wide else 400) <= r[2] <= center + 500]
        if not before and near:
            results["appearing_ids"].append({"can_id": f"0x{identifier:03X}",
                                              "dlc": length, "first": sample(near[0], center)})
        if len(before) < 2 or len(after) < 2:
            continue
        for byte in range(length):
            for bit in range(8):
                pre = [(r[6][byte] >> bit) & 1 for r in before]
                post = [(r[6][byte] >> bit) & 1 for r in after]
                baseline = int(sum(pre) > len(pre) / 2)
                if sum(x == baseline for x in pre) / len(pre) < .95:
                    continue
                excursions = [r for r in near if ((r[6][byte] >> bit) & 1) != baseline]
                if not excursions:
                    continue
                opposite = sum(x != baseline for x in post) / len(post)
                kind = "step" if opposite >= .95 else "pulse" if opposite <= .05 else None
                if kind:
                    results["bit_candidates"].append({"can_id": f"0x{identifier:03X}",
                        "dlc": length, "byte": byte, "bit": bit, "baseline": baseline,
                        "kind": kind, "pre_count": len(before), "post_count": len(after),
                        "before": sample(before[-1], center), "first": sample(excursions[0], center),
                        "after": sample(after[0], center)})
    for identifier, byte, mask, natural, dynamic in RESPONSES[anchor["role"]]:
        previous = None
        for row in rows:
            if row[4] != identifier or row[5] <= byte:
                continue
            value = row[6][byte] & mask
            wanted = (natural, dynamic) if anchor["kind"] == "N_D" else (dynamic, natural)
            if previous is not None and (previous[6][byte] & mask, value) == wanted:
                if abs(row[2] - center) <= 1500:
                    results["response_order"].append({"can_id": f"0x{identifier:03X}",
                        "byte": byte, "mask": mask, "before": sample(previous, center),
                        "first": sample(row, center)})
            previous = row
    results["response_order"].sort(key=lambda item: item["first"]["sequence"])
    return results


def analyze(directory):
    before_hashes = fingerprints(directory)
    con = sqlite3.connect(f"file:{directory / 'session.sqlite3'}?mode=ro", uri=True)
    con.row_factory = sqlite3.Row
    session = dict(con.execute("SELECT * FROM sessions").fetchone())
    if session["status"] not in ("complete", "failed"):
        raise RuntimeError(f"Capture is not finished: {directory}")
    events = [dict(r) for r in con.execute("SELECT * FROM events ORDER BY host_ns")]
    inventory = [dict(r) for r in con.execute(
        "SELECT role,arbitration_id,dlc,count(*) AS frames FROM can_frames GROUP BY role,arbitration_id,dlc")]
    transitions, anchors = {}, []
    for role, (identifier, byte, mask, shift) in PRIMARY.items():
        changes = []
        previous = None
        query = ("SELECT id,sequence,device_timestamp_ms,host_ns,arbitration_id,dlc,data "
                 "FROM can_frames WHERE role=? AND arbitration_id=? AND dlc=8 ORDER BY sequence")
        final_time = 0
        for row in con.execute(query, (role, identifier)):
            final_time = row[2]
            value = (row[6][byte] & mask) >> shift
            if value != previous:
                changes.append({**sample(row, 0), "mode": value})
            previous = value
        transitions[role] = changes
        for index in range(1, len(changes)):
            prior, current = changes[index - 1:index + 1]
            if (prior["mode"], current["mode"]) not in ((0, 2), (2, 0)):
                continue
            following = changes[index + 1]["device_ms"] if index + 1 < len(changes) else final_time
            anchors.append({**current, "role": role,
                "kind": "N_D" if current["mode"] == 2 else "D_N",
                "previous_since": prior["device_ms"], "next_since": following,
                "previous_dwell_ms": current["device_ms"] - prior["device_ms"],
                "nearby_events": [e for e in events if abs(e["host_ns"] - current["host_ns"]) <= 5e9]})
    con.row_factory = None
    # One streaming pass per bus, without indexes or modifications to original databases.
    for role in PRIMARY:
        selected = [a for a in anchors if a["role"] == role]
        if not selected:
            continue
        windows = [[] for _ in selected]
        times = [a["device_ms"] for a in selected]
        for row in con.execute(
            "SELECT id,sequence,device_timestamp_ms,host_ns,arbitration_id,dlc,data "
            "FROM can_frames WHERE session_id=? AND role=? ORDER BY sequence", (session["id"], role)):
            left = bisect.bisect_left(times, row[2] - 5000)
            right = bisect.bisect_right(times, row[2] + 5000)
            for index in range(left, right):
                windows[index].append(row)
        for anchor, rows in zip(selected, windows):
            anchor.update(scan_window(rows, anchor))
            if anchor["previous_dwell_ms"] >= 5000:
                anchor["wide_bit_candidates"] = scan_window(rows, anchor, wide=True)["bit_candidates"]
            # Keep original losses near the observation, using the same board's clock.
            anchor["nearby_loss"] = [list(r) for r in con.execute(
                "SELECT sequence,device_timestamp_ms,dropped_count FROM capture_loss "
                "WHERE role=? AND device_timestamp_ms BETWEEN ? AND ?",
                (role, anchor["device_ms"] - 5000, anchor["device_ms"] + 5000))]
    con.close()
    assert fingerprints(directory) == before_hashes, f"Input changed: {directory}"
    print(f"{session['id']}: {len(anchors)} bus-specific N/D edges", file=sys.stderr, flush=True)
    return {"session": session, "input_sha256": before_hashes, "inputs_unchanged": True,
            "manifest": json.loads((directory / "manifest.json").read_text()),
            "summary": json.loads((directory / "summary.json").read_text()),
            "inventory": inventory, "events": events, "mode_transitions": transitions, "edges": anchors}


def aggregate(sessions):
    grouped = collections.defaultdict(list)
    for session in sessions:
        for edge in session["edges"]:
            for item in edge["bit_candidates"]:
                key = (edge["role"], edge["kind"], item["can_id"], item["byte"],
                       item["bit"], item["kind"], item["baseline"])
                grouped[key].append({"session": session["session"]["id"],
                    "anchor_frame_id": edge["frame_id"], "first_relative_ms": item["first"]["relative_ms"],
                    "previous_dwell_ms": edge["previous_dwell_ms"]})
    return [{"role": key[0], "direction": key[1], "can_id": key[2], "byte": key[3],
             "bit": key[4], "kind": key[5], "baseline": key[6], "matches": len(value),
             "observations": value} for key, value in sorted(grouped.items()) if len(value) >= 2]


def negative_controls(directory, sessions, candidates):
    """Count candidate activity outside every mode-change neighborhood on every bus."""
    fields = collections.defaultdict(set)
    for item in candidates:
        if item["direction"] == "N_D" and item["kind"] == "pulse":
            fields[(item["role"], int(item["can_id"], 16))].add((item["byte"], item["bit"]))
    output = []
    for session in sessions:
        path = directory / session["session"]["id"]
        before = fingerprints(path)
        con = sqlite3.connect(f"file:{path / 'session.sqlite3'}?mode=ro", uri=True)
        for role in PRIMARY:
            ids = [i for r, i in fields if r == role]
            if not ids:
                continue
            modes = session["mode_transitions"][role]
            mode_times = [m["device_ms"] for m in modes]
            previous = {}
            counts = collections.defaultdict(collections.Counter)
            examples = collections.defaultdict(list)
            query = ("SELECT id,sequence,device_timestamp_ms,host_ns,arbitration_id,dlc,data "
                     "FROM can_frames WHERE session_id=? AND role=? AND arbitration_id IN (" +
                     ",".join("?" for _ in ids) + ") ORDER BY sequence")
            for row in con.execute(query, (session["session"]["id"], role, *ids)):
                identifier, payload = row[4], row[6]
                old = previous.get(identifier)
                previous[identifier] = payload
                if old is None or len(old) != len(payload):
                    continue
                index = bisect.bisect_right(mode_times, row[2]) - 1
                away = index >= 0 and row[2] - mode_times[index] > 5000 and (
                    index + 1 == len(mode_times) or mode_times[index + 1] - row[2] > 5000)
                for byte, bit in fields[(role, identifier)]:
                    if byte >= len(payload) or not ((old[byte] ^ payload[byte]) >> bit) & 1:
                        continue
                    key = (identifier, byte, bit)
                    counts[key]["all_flips"] += 1
                    if away:
                        counts[key]["away_from_mode_changes"] += 1
                        counts[key][f"away_mode_{modes[index]['mode']}"] += 1
                        if len(examples[key]) < 3:
                            examples[key].append(sample(row, mode_times[index]))
            for (identifier, byte, bit), values in counts.items():
                output.append({"session": session["session"]["id"], "role": role,
                               "can_id": f"0x{identifier:03X}", "byte": byte, "bit": bit,
                               "counts": dict(values), "control_examples": examples[(identifier, byte, bit)]})
        con.close()
        assert fingerprints(path) == before == session["input_sha256"]
    return output


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sessions", type=Path)
    args = parser.parse_args()
    catalog = json.loads((ROOT / "baccable_lab/knowledge/giulia.json").read_text())
    hypotheses = json.loads((ROOT / "baccable_lab/knowledge/hypotheses.json").read_text())
    sessions = [analyze(path.parent) for path in sorted(args.sessions.glob("*/session.sqlite3"))]
    candidates = aggregate(sessions)
    controls = negative_controls(args.sessions, sessions, candidates)
    print(json.dumps({"dictionary_version": catalog["dictionary_version"],
                      "loaded_hypotheses": [h["id"] for h in hypotheses["hypotheses"]],
                      "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                      "sessions": sessions, "repeated_candidates": candidates,
                      "negative_controls": controls}, indent=2))

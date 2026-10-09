"""Read-only session evidence; manual markers and CAN-derived events stay distinct."""
from __future__ import annotations

from collections import Counter
import hashlib
import json
from pathlib import Path
import sqlite3

from .catalog import crc8_j1850, decode_frame, extract_signal, load_dictionary
from .profiles import profile_frames


def fingerprints(directory: Path) -> dict:
    result = {}
    names = ["session.sqlite3", "session.sqlite3-wal", "manifest.json", "summary.json"]
    names.extend(p.name for p in sorted(directory.glob("*.bin")))
    for name in names:
        path = directory / name
        if path.exists():
            digest = hashlib.sha256()
            with path.open("rb") as stream:
                for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                    digest.update(chunk)
            result[name] = digest.hexdigest()
    return result


def table_exists(database: sqlite3.Connection, name: str) -> bool:
    """True when a table is present; older sessions may lack newer tables."""
    return database.execute("SELECT 1 FROM sqlite_master WHERE type='table' AND name=?", (name,)).fetchone() is not None


def open_readonly(directory: Path) -> sqlite3.Connection:
    """Read committed WAL pages without creating sidecars for checkpointed inputs."""
    wal = directory / "session.sqlite3-wal"
    options = "?mode=ro" if wal.exists() and wal.stat().st_size else "?mode=ro&immutable=1"
    database = sqlite3.connect((directory / "session.sqlite3").resolve().as_uri() + options, uri=True)
    database.execute("PRAGMA query_only=ON")
    return database


def manual_event(source: str) -> bool:
    # Unknown sources must not quietly become independent manual evidence.
    # Scenario confirmations are human observations and count as manual.
    return source in ("keyboard", "cli", "scenario")


def matches(rule: dict, value: float) -> bool:
    if "equals" in rule:
        return value in rule["equals"]
    if "above" in rule:
        return value > rule["above"]
    if "below" in rule:
        return value < rule["below"]
    raise ValueError("unknown marker rule")


def correlate(database: sqlite3.Connection, catalog: dict, events: list[dict]) -> list[dict]:
    results = []
    for event in events:
        if not manual_event(event["source"]):
            continue
        for message in catalog["messages"]:
            for signal in message["signals"]:
                rule = signal.get("marker_rules", {}).get(event["label"])
                if rule is None:
                    continue
                ns = event["host_ns"]
                # Reaction time is unknown: inspect -3..+1 seconds, never reinterpret marker labels.
                rows = database.execute(
                    "SELECT id, sequence, host_ns, device_timestamp_ms, dlc, data FROM can_frames WHERE role=? AND arbitration_id=? "
                    "AND host_ns BETWEEN ? AND ? ORDER BY sequence",
                    (message["role"], int(message["can_id"], 16), ns - 3_000_000_000, ns + 1_000_000_000))
                transitions = []
                states = Counter()
                previous = None
                for frame_id, sequence, host, device_ms, dlc, payload in rows:
                    data = bytes(payload)[:dlc]
                    result = extract_signal(signal, data)
                    if result is None or not result["valid"]:
                        continue
                    value = result["value"]
                    active = matches(rule, value)
                    states[str(result["raw"])] += 1
                    if active and previous is False:
                        transitions.append({"frame_id":frame_id,"sequence":sequence,"host_ns":host,
                                            "device_timestamp_ms":device_ms,
                                            "delta_seconds": round((host-ns)/1e9, 6),
                                            "raw": result["raw"], "data": data.hex().upper()
                                            if int(message["can_id"], 16) != 0x1EF else None})
                    previous = active
                nearest = min(transitions, key=lambda t: abs(t["delta_seconds"])) if transitions else None
                results.append({"event_id": event["id"], "label": event["label"], "role": message["role"],
                                "can_id": message["can_id"], "signal": signal["name"],
                                "origin": signal["origin"], "window_seconds": [-3, 1],
                                "result": "matching_transition" if nearest else "no_matching_transition",
                                "transition": nearest, "raw_counts": dict(states),
                                "limits": signal.get("note", "Time agreement alone is not physical validation.")})
    return results


def display_evidence(database: sqlite3.Connection) -> dict:
    sources = Counter()
    controls = Counter()
    completed = []
    pending = None
    interrupted = 0
    malformed = 0
    for host, device_ms, dlc, payload in database.execute(
            "SELECT host_ns,device_timestamp_ms,dlc,data FROM can_frames "
            "WHERE role='BH' AND arbitration_id=144 ORDER BY sequence"):
        data = bytes(payload)[:dlc]
        if len(data) < 2:
            malformed += 1
            if pending:
                interrupted += 1
                pending = None
            continue
        last, index, source = data[0] >> 3, ((data[0] & 7) << 2) | (data[1] >> 6), data[1] & 63
        sources[f"0x{source:02X}"] += 1
        if len(data) != 8 or index > last:
            malformed += 1
            if pending:
                interrupted += 1
                pending = None
            continue
        if source not in (5, 6, 7, 8, 9, 0x21):
            controls[f"0x{source:02X}"] += 1
            if pending:
                interrupted += 1
                pending = None
            continue
        if index == 0:
            if pending:
                interrupted += 1
            pending = {"source": source, "last": last, "next": 0, "units": [],
                       "start_host_ns": host, "start_device_ms": device_ms}
        if pending is None or (source, last, index) != (pending["source"], pending["last"], pending["next"]):
            interrupted += 1
            pending = None
            continue
        pending["units"].extend(int.from_bytes(data[i:i+2], "big") for i in (2, 4, 6))
        pending["next"] += 1
        if index == last:
            units = pending["units"]
            # Padding is distinct from a line separator. Preserve lengths, never publish song/title text.
            trimmed = units[:units.index(0)] if 0 in units else units
            completed.append({"source": f"0x{source:02X}", "start_seconds": round(pending["start_host_ns"]/1e9, 6),
                              "end_seconds": round(host/1e9, 6), "fragments": last+1,
                              "device_duration_ms": device_ms-pending["start_device_ms"],
                              "text_code_units": len(trimmed), "line_separator_positions":
                              [i for i, unit in enumerate(trimmed) if unit == 13],
                              "non_ascii_code_units": sorted(set(unit for unit in trimmed if unit > 127))})
            pending = None
    return {"source_frame_counts": dict(sources), "control_frame_counts": dict(controls),
            "complete_messages": completed, "incomplete_sequences": interrupted + int(pending is not None),
            "malformed_frames": malformed,
            "limits": "RX factory traffic only. Text context is not proof of live audio source or screen ownership."}


def scenario_candidates(candidate_bit_steps: list[dict]) -> list[dict]:
    """Group exploratory bit steps by named action, keeping repeated evidence.

    Reuses the windowed comparison that profile_frames already performs, then
    keeps only bits that flipped for the same expected action at least twice.
    Single events are dropped: one ON/OFF pair cannot name a field. Each kept
    candidate lists its per-event windows so an analysis can weigh consistency.
    """
    grouped: dict[tuple, dict] = {}
    for step in candidate_bit_steps:
        key = (step["role"], step["can_id"], step["byte"], step["bit"], step["label"])
        entry = grouped.setdefault(key, {"role": step["role"], "can_id": step["can_id"],
                                         "byte": step["byte"], "bit": step["bit"],
                                         "label": step["label"], "events": [],
                                         "known_fields": step["known_fields"]})
        entry["events"].append({"event_id": step["event_id"], "before": step["before"],
                                "after": step["after"], "fractions": step["one_fraction_before_after"],
                                "samples": step["samples_before_after"]})
    result = [entry for entry in grouped.values() if len(entry["events"]) >= 2]
    for entry in result:
        entry["support"] = len(entry["events"])
        entry["event_ids"] = [event["event_id"] for event in entry["events"]]
    result.sort(key=lambda entry: (-entry["support"], str(entry["can_id"]), entry["byte"], entry["bit"]))
    return result


def button_evidence(database: sqlite3.Connection) -> dict:
    codes, crc = Counter(), Counter()
    holds = []
    start = last = None
    for host, device_ms, dlc, payload in database.execute(
            "SELECT host_ns,device_timestamp_ms,dlc,data FROM can_frames "
            "WHERE role='C1' AND arbitration_id=762 ORDER BY sequence"):
        data = bytes(payload)[:dlc]
        if len(data) != 3:
            crc["invalid_dlc"] += 1
            start = last = None
            continue
        valid = crc8_j1850(data[:2]) == data[2]
        crc["valid" if valid else "invalid"] += 1
        codes[f"0x{data[0]:02X}"] += 1
        if not valid or (last is not None and not 0 <= device_ms-last <= 300):
            start = None
        if valid and data[0] in (0x50, 0x90):
            if start is None:
                start = (host, device_ms)
        elif valid and data[0] == 0x10 and start is not None:
            if device_ms-start[1] >= 1200:
                holds.append({"start_seconds": round(start[0]/1e9, 6), "release_seconds": round(host/1e9, 6),
                              "device_duration_ms": device_ms-start[1]})
            start = None
        else:
            start = None
        last = device_ms
    return {"raw_button_counts": dict(codes), "checksum": dict(crc), "long_holds": holds,
            "limits": "Derived raw holds only; CC/ACC gating, release arming, menu actions and visible IPC content are not proven."}


def review_session(directory: Path, catalog: dict) -> dict:
    before = fingerprints(directory)
    # Include committed WAL pages when present. Without a WAL, immutable avoids
    # SQLite creating empty WAL/SHM sidecars just to read a finished database.
    database = open_readonly(directory)
    try:
        database.execute("BEGIN")
        metadata = database.execute("SELECT id,status,duration_seconds FROM sessions").fetchone()
        if metadata is None or metadata[1] in ("active", "recording"):
            raise ValueError(f"finish the session before reviewing: {directory.name}")
        known = {(m["role"], int(m["can_id"], 16)): m for m in catalog["messages"]}
        diagnostic_ids = {(p["role"], int(p["response_id"], 16)) for p in catalog["diagnostic_parameters"]}
        inventory = []
        for role, can_id, dlc, count, first, last in database.execute(
                "SELECT role,arbitration_id,dlc,count(*),min(host_ns),max(host_ns) FROM can_frames "
                "GROUP BY role,arbitration_id,dlc ORDER BY role,arbitration_id,dlc"):
            message = known.get((role, can_id))
            inventory.append({"role": role, "can_id": f"0x{can_id:03X}", "dlc": dlc, "frames": count,
                              "first_seconds": round(first/1e9, 6), "last_seconds": round(last/1e9, 6),
                              "title": message["title"] if message else None,
                              "coverage": "broadcast_dictionary" if message else
                              "diagnostic_response_id" if (role, can_id) in diagnostic_ids else "unmapped"})
        events = [dict(zip(("id", "host_ns", "label", "source", "note"), row)) for row in database.execute(
            "SELECT id,host_ns,label,source,note FROM events ORDER BY host_ns,id")]
        scenario_steps = [dict(zip(("host_ns", "scenario", "step_index", "step_key", "expect", "run", "outcome", "note"), row))
                          for row in database.execute(
            "SELECT host_ns,scenario,step_index,step_key,expect,run,outcome,note FROM scenario_steps ORDER BY host_ns,id")] \
            if table_exists(database, "scenario_steps") else []
        scenario_by_run: dict[tuple, dict] = {}
        for step in scenario_steps:
            entry = scenario_by_run.setdefault((step['scenario'], step['run']),
                                               {"scenario": step['scenario'], "run": step['run'],
                                                "done": 0, "failed": 0})
            entry["done" if step['outcome'] == "done" else "failed"] += 1
        scenario_overview = [scenario_by_run[key] for key in sorted(scenario_by_run)]
        losses = [dict(zip(("role", "host_ns", "device_timestamp_ms", "dropped_count"), row))
                  for row in database.execute(
                      "SELECT role,host_ns,device_timestamp_ms,dropped_count FROM capture_loss ORDER BY host_ns,id")]
        for loss in losses:
            loss["phase"] = "startup" if loss["host_ns"] <= 2_000_000_000 else "in_session"
            loss["saturated"] = loss["dropped_count"] == 65535
        signal_stats = []
        for (role, can_id), message in sorted(known.items()):
            stats = {s["name"]: Counter() for s in message["signals"]}
            examples = {}
            for dlc, payload in database.execute("SELECT dlc,data FROM can_frames WHERE role=? AND arbitration_id=? ORDER BY sequence", (role, can_id)):
                data = bytes(payload)[:dlc]
                if len(data) != message.get("exact_dlc", len(data)):
                    continue
                for signal in message["signals"]:
                    decoded = extract_signal(signal, data)
                    if decoded is not None:
                        stats[signal["name"]][decoded["raw"]] += 1
                        examples.setdefault(signal["name"], data.hex().upper())
            for signal in message["signals"]:
                counts = stats[signal["name"]]
                if counts:
                    signal_stats.append({"role": role, "can_id": message["can_id"], "signal": signal["name"],
                                         "origin": signal["origin"], "samples": sum(counts.values()),
                                         "raw_min": min(counts), "raw_max": max(counts), "distinct_values": len(counts),
                                         "most_common": counts.most_common(8),
                                         # IPC content bytes may contain personal text; only publish lengths above.
                                         "example_data": examples[signal["name"]]
                                         if can_id not in (0x90, 0x1EF) else None})
        diagnostic_counts = Counter()
        for role, can_id in sorted(diagnostic_ids):
            for dlc, payload in database.execute("SELECT dlc,data FROM can_frames WHERE role=? AND arbitration_id=?", (role, can_id)):
                for decoded in decode_frame(catalog, role, can_id, bytes(payload)[:dlc])["diagnostics"]:
                    diagnostic_counts[(decoded["parameter_id"], decoded["did"])] += 1
        steering = button_evidence(database)
        for hold in steering["long_holds"]:
            start_ns = round(hold["start_seconds"] * 1e9)
            end_ns = round(hold["release_seconds"] * 1e9)
            hold["bh_rx_frames"] = database.execute(
                "SELECT count(*) FROM can_frames WHERE role='BH' AND host_ns BETWEEN ? AND ?",
                (start_ns, end_ns)).fetchone()[0]
            hold["factory_text_rx_frames"] = database.execute(
                "SELECT count(*) FROM can_frames WHERE role='BH' AND arbitration_id=144 AND host_ns BETWEEN ? AND ?",
                (start_ns, end_ns)).fetchone()[0]
        result = {"session": metadata[0], "status": metadata[1], "duration_seconds": metadata[2],
                  "input_sha256": before, "frames": sum(i["frames"] for i in inventory),
                  "inventory": inventory, "events": events, "loss_records": losses,
                  "scenario_steps": scenario_steps, "scenario_overview": scenario_overview,
                  "marker_correlations": correlate(database, catalog, events), "signal_statistics": signal_stats,
                  "diagnostic_matches": [{"parameter_id": p, "did": did, "positive_single_frames": n}
                                         for (p, did), n in sorted(diagnostic_counts.items())],
                  "ipc": display_evidence(database), "steering": steering}
        summary = directory / "summary.json"
        if summary.exists():
            captured = json.loads(summary.read_text())
            result["capture_health"] = {k: captured[k] for k in
                ("reader_errors", "errors", "discarded_reader_bytes", "discarded_queued_bytes", "roles")
                if k in captured}
        profiles = profile_frames(database, catalog, events)
        result.update(profiles)
        result["scenario_candidates"] = scenario_candidates(profiles["candidate_bit_steps"])
    finally:
        database.close()
    if fingerprints(directory) != before:
        raise ValueError(f"session changed during review: {directory.name}; stop capture and retry")
    return result


def review_sessions(root: Path) -> dict:
    catalog = load_dictionary()
    directories = sorted(p.parent for p in root.glob("*/session.sqlite3"))
    if not directories:
        raise ValueError(f"no session databases under {root}")
    sessions = [review_session(path, catalog) for path in directories]
    templates = {}
    seen_shapes, seen_labels, seen_dids = set(), set(), set()
    for session in sessions:
        shapes = {(p['role'],p['can_id'],p['dlc']) for p in session['frame_profiles']}
        session['new_frame_shapes'] = [list(k) for k in sorted(shapes-seen_shapes)]
        labels = {e['label'] for e in session['events'] if manual_event(e['source'])}
        session['new_manual_labels'] = sorted(labels-seen_labels)
        dids = {(p['parameter_id'],p['did']) for p in session['diagnostic_matches']}
        session['new_positive_dids'] = [list(k) for k in sorted(dids-seen_dids)]
        seen_shapes.update(shapes)
        seen_labels.update(labels)
        seen_dids.update(dids)
        # A failed run remains evidence, but is not used to learn a complete-run baseline.
        if session['status'] != 'complete':
            continue
        for p in session['frame_profiles']:
            if p['stable_values'] is None:
                continue
            key = (p['role'],p['can_id'],p['dlc'])
            mask, value = int(p['stable_bit_masks'] or '0',16), int(p['stable_values'] or '0',16)
            if key not in templates:
                templates[key] = {'mask':mask,'value':value,'frames':0,'sessions':[]}
            template = templates[key]
            template['mask'] &= mask & ~(template['value'] ^ value)
            template['value'] &= template['mask']
            template['frames'] += p['frames']
            template['sessions'].append(session['session'])
    frame_templates = [{'role':role,'can_id':can_id,'dlc':dlc,
                        'stable_bit_masks':t['mask'].to_bytes(dlc,'big').hex().upper(),
                        'stable_values':t['value'].to_bytes(dlc,'big').hex().upper(),
                        'frames':t['frames'],'sessions':t['sessions'],
                        'status':'repeated_observation' if len(t['sessions']) >= 2 else 'single_session_observation'}
                       for (role,can_id,dlc),t in sorted(templates.items())]
    return {"schema_version": 1, "dictionary_version": catalog["dictionary_version"],
            "firmware_source_commit": catalog["firmware_source_commit"],
            "limits": catalog["capture_limits"], "sessions": sessions, "frame_templates":frame_templates,
            "total_frames": sum(s["frames"] for s in sessions),
            "total_manual_events": sum(manual_event(e["source"]) for s in sessions for e in s["events"]),
            "total_derived_events": sum(e["source"].startswith("analysis:") for s in sessions for e in s["events"])}

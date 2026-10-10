"""One self-contained analysis bundle for a scenario session.

A bundle is the single artifact meant to be handed to an analysis (a person or an
AI): session context, capture health, scenario steps, coverage of mapped versus
unmapped identifiers, ranked per-action candidate bits and bounded raw frame
excerpts around each confirmed step. It is derived read-only from a finished
session and never edits the recording.
"""

from __future__ import annotations

import json
from collections import Counter
from pathlib import Path

from .catalog import crc8_j1850
from .profiles import profile_frames
from .review import fingerprints, open_readonly, scenario_candidates, table_exists

WINDOW_BEFORE_NS = 2_500_000_000
WINDOW_AFTER_NS = 1_000_000_000


def shape_hints(database, shapes: list[dict], *, cap_frames: int = 20000, cap_distinct: int = 256) -> list[dict]:
    """Cheap structure hints for the highest-volume unmapped identifiers.

    Encodes what a first manual pass would compute: is the payload constant, how
    many distinct values appear, which bits ever vary, whether the last byte is a
    CRC-8/SAE-J1850 of the rest, and the most common board-side intervals.
    """
    hints = []
    for shape in shapes:
        role, can_id, dlc = shape["role"], int(shape["can_id"], 16), shape["dlc"]
        seen: set[bytes] = set()
        xor = 0
        crc_ok = crc_total = 0
        periods: Counter = Counter()
        last_device = None
        sampled = 0
        for device_ms, payload in database.execute(
                "SELECT device_timestamp_ms,data FROM can_frames WHERE role=? AND arbitration_id=? AND dlc=? "
                "ORDER BY sequence LIMIT ?", (role, can_id, dlc, cap_frames)):
            data = bytes(payload)[:dlc]
            xor ^= int.from_bytes(data, "big") if data else 0
            if len(seen) < cap_distinct:
                seen.add(data)
            if dlc >= 2 and any(data[:-1]):
                crc_total += 1
                crc_ok += crc8_j1850(data[:-1]) == data[-1]
            if last_device is not None and device_ms >= last_device and device_ms - last_device <= 5000:
                periods[device_ms - last_device] += 1
            last_device = device_ms
            sampled += 1
        mask = ((1 << (8 * dlc)) - 1) ^ xor
        hints.append({"role": role, "can_id": shape["can_id"], "dlc": dlc, "frames": shape["frames"],
                      "sampled": sampled, "distinct": len(seen), "distinct_capped": len(seen) >= cap_distinct,
                      "constant": len(seen) == 1,
                      "volatile_bit_masks": xor.to_bytes(dlc, "big").hex().upper(),
                      "stable_bit_masks": mask.to_bytes(dlc, "big").hex().upper(),
                      "last_byte_crc8_j1850": f"{crc_ok}/{crc_total}",
                      "device_interval_ms_top": periods.most_common(3)})
    return hints


def build_bundle(directory: Path, catalog: dict, *, window_before_ns: int = WINDOW_BEFORE_NS,
                 window_after_ns: int = WINDOW_AFTER_NS, max_frames_per_step_role: int = 30,
                 max_frames_total: int = 20000, include_excerpts: bool = True,
                 include_hints: bool = True, max_hint_shapes: int = 30,
                 scenarios: dict | None = None) -> dict:
    """Assemble the AI-oriented bundle for one finished session."""
    before = fingerprints(directory)
    database = open_readonly(directory)
    try:
        metadata = database.execute("SELECT id,status,duration_seconds,started_at,ended_at FROM sessions").fetchone()
        if metadata is None or metadata[1] in ("active", "recording"):
            raise ValueError(f"finish the session before bundling: {directory.name}")
        known = {(m["role"], int(m["can_id"], 16)): m for m in catalog["messages"]}
        diagnostic_ids = {(p["role"], int(p["response_id"], 16)) for p in catalog["diagnostic_parameters"]}
        events = [dict(zip(("id", "host_ns", "label", "source", "note"), row))
                  for row in database.execute("SELECT id,host_ns,label,source,note FROM events ORDER BY host_ns,id")]
        label_by_event = {event["id"]: event["label"] for event in events}
        steps = [dict(zip(("host_ns", "event_id", "scenario", "step_index", "step_key",
                           "expect", "run", "outcome", "note"), row))
                 for row in database.execute("SELECT host_ns,event_id,scenario,step_index,step_key,expect,"
                                             "run,outcome,note FROM scenario_steps ORDER BY host_ns,id")] \
            if table_exists(database, "scenario_steps") else []
        for step in steps:
            step["action"] = label_by_event.get(step["event_id"], step["expect"] or step["step_key"])
            definition = (scenarios or {}).get(step["scenario"])
            if definition is None:
                continue
            step["scenario_title"] = definition.title
            step["guard"] = list(definition.confounder_guard)
            step["runs"] = definition.runs
            step["min_separation_s"] = definition.min_separation_s
            step["vehicle"] = definition.vehicle
            step["site"] = definition.site
            if 0 <= step["step_index"] < len(definition.steps):
                library_step = definition.steps[step["step_index"]]
                step["library_action"] = library_step.action
                step["library_expect"] = library_step.expect
        step_by_event = {step["event_id"]: step for step in steps}
        roles = [row[0] for row in database.execute("SELECT DISTINCT role FROM can_frames ORDER BY role")]

        mapped_frames = unmapped_frames = 0
        unmapped = []
        for role, can_id, dlc, count in database.execute(
                "SELECT role,arbitration_id,dlc,count(*) FROM can_frames "
                "GROUP BY role,arbitration_id,dlc ORDER BY role,arbitration_id,dlc"):
            if (role, can_id) in known or (role, can_id) in diagnostic_ids:
                mapped_frames += count
            else:
                unmapped_frames += count
                unmapped.append({"role": role, "can_id": f"0x{can_id:03X}", "dlc": dlc, "frames": count})
        unmapped.sort(key=lambda item: -item["frames"])
        hints = shape_hints(database, unmapped[:max_hint_shapes]) if include_hints else []

        profiles = profile_frames(database, catalog, events)
        candidates = scenario_candidates(profiles["candidate_bit_steps"])
        for candidate in candidates:
            for event in candidate["events"]:
                source = step_by_event.get(event["event_id"])
                if source:
                    event.update({"scenario": source["scenario"], "run": source["run"],
                                  "step_index": source["step_index"], "expect": source["expect"]})

        excerpts = []
        total = 0
        if include_excerpts:
            for step in steps:
                if step["outcome"] != "done" or not step["expect"]:
                    continue
                for role in roles:
                    rows = list(database.execute(
                        "SELECT host_ns,arbitration_id,dlc,data FROM can_frames WHERE role=? AND host_ns BETWEEN ? AND ? "
                        "ORDER BY host_ns LIMIT ?",
                        (role, step["host_ns"] - window_before_ns, step["host_ns"] + window_after_ns,
                         max_frames_per_step_role)))
                    if not rows:
                        continue
                    excerpts.append({"scenario": step["scenario"], "run": step["run"],
                                     "step_index": step["step_index"], "expect": step["expect"],
                                     "host_ns": step["host_ns"], "role": role,
                                     "frames": [{"host_ns": host, "can_id": f"0x{can_id:03X}", "dlc": dlc,
                                                 "data": bytes(payload)[:dlc].hex().upper()}
                                                for host, can_id, dlc, payload in rows]})
                    total += len(rows)
                    if total >= max_frames_total:
                        break
                if total >= max_frames_total:
                    break
    finally:
        database.close()
    if fingerprints(directory) != before:
        raise ValueError(f"session changed during bundling: {directory.name}")

    context = {}
    manifest = directory / "manifest.json"
    if manifest.exists():
        try:
            context = json.loads(manifest.read_text(encoding="utf-8")).get("context", {})
        except ValueError:
            context = {}
    health = None
    summary = directory / "summary.json"
    if summary.exists():
        captured = json.loads(summary.read_text(encoding="utf-8"))
        health = {key: captured[key] for key in
                  ("reader_errors", "errors", "discarded_reader_bytes", "discarded_queued_bytes",
                   "startup_dropped", "in_session_dropped") if key in captured}

    bundle = {
        "schema_version": 1,
        "session": {"id": metadata[0], "status": metadata[1], "duration_seconds": metadata[2],
                    "started_at": metadata[3], "ended_at": metadata[4], "roles": roles},
        "dictionary": {"version": catalog["dictionary_version"],
                       "firmware_source_commit": catalog["firmware_source_commit"]},
        "context": context,
        "capture_health": health,
        "input_sha256": before,
        "coverage": {"mapped_frames": mapped_frames, "unmapped_frames": unmapped_frames,
                     "mapped_fraction": round(mapped_frames / max(1, mapped_frames + unmapped_frames), 4),
                     "top_unmapped": unmapped[:25]},
        "shape_hints": hints,
        "scenario": {"steps": steps, "candidates": candidates,
                     "library": {sid: {"title": definition.title, "group": definition.group,
                                       "vehicle": definition.vehicle, "site": definition.site,
                                       "runs": definition.runs, "min_separation_s": definition.min_separation_s,
                                       "guard": list(definition.confounder_guard),
                                       "steps": [{"action": step.action, "expect": step.expect}
                                                 for step in definition.steps]}
                                 for sid, definition in (scenarios or {}).items()
                                 if any(entry["scenario"] == sid for entry in steps)}},
        "excerpts": excerpts,
        "limits": ("RX capture only; host timestamps batch across USB; board clocks have no common epoch. "
                   "Candidate bits are uncorrected exploratory comparisons, not discovered functions. "
                   "Require repeated opposite actions, independent sessions and negative controls."),
    }
    bundle["brief"] = render_brief(bundle)
    return bundle


def render_brief(bundle: dict) -> str:
    """A short Markdown brief that primes an analysis without replacing evidence."""
    session = bundle["session"]
    coverage = bundle["coverage"]
    lines = [f"# Bundle {session['id']} — {session['status']}, {session['duration_seconds']:.0f}s",
             f"Dictionary {bundle['dictionary']['version']} (firmware {bundle['dictionary']['firmware_source_commit'][:10]})."]
    if bundle["context"]:
        lines.append("Context: " + ", ".join(f"{k}={v}" for k, v in bundle["context"].items()))
    total = coverage["mapped_frames"] + coverage["unmapped_frames"]
    lines.append(f"Coverage: {coverage['mapped_frames']}/{total} frames mapped "
                 f"({coverage['mapped_fraction'] * 100:.1f}%); unmapped identifiers: {len(coverage['top_unmapped'])}+.")
    scenarios = {step["scenario"] for step in bundle["scenario"]["steps"]}
    done = sum(1 for step in bundle["scenario"]["steps"] if step["outcome"] == "done")
    failed = sum(1 for step in bundle["scenario"]["steps"] if step["outcome"] == "failed")
    skipped = sum(1 for step in bundle["scenario"]["steps"] if step["outcome"] == "skipped")
    lines.append(f"Scenarios touched: {len(scenarios)}; confirmations: {done} done, {failed} failed, "
                 f"{skipped} skipped (not performed on purpose).")
    top = bundle["scenario"]["candidates"][:10]
    if top:
        lines.append("Top candidate bits (by repeats):")
        for candidate in top:
            lines.append(f"  {candidate['role']} {candidate['can_id']} byte {candidate['byte']} bit {candidate['bit']}"
                         f" <-> {candidate['label']} (x{candidate['support']})")
    else:
        lines.append("No repeated candidate bits yet: run more repetitions of the same action.")
    if coverage["top_unmapped"]:
        worst = ", ".join(f"{i['role']} {i['can_id']} ({i['frames']})" for i in coverage["top_unmapped"][:5])
        lines.append(f"Highest-volume unmapped: {worst}.")
    hints = bundle.get("shape_hints") or []
    if hints:
        constants = [h for h in hints if h["constant"]]
        crcs = []
        for hint in hints:
            ok, _, total = hint["last_byte_crc8_j1850"].partition("/")
            if total.isdigit() and int(total) and ok == total:
                crcs.append(hint)
        lines.append(f"Shape hints: {len(constants)} constant payloads, {len(crcs)} with a confirmed "
                     f"last-byte CRC-8/SAE-J1850 (candidates for a counter+CRC heartbeat).")
    lines.append("Verify candidates against the raw excerpts and the guard windows; keep a claim only with "
                 "repeated, reversible transitions and negative controls.")
    return "\n".join(lines)

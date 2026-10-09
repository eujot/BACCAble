"""Command line entry point for the Milestone 1 recorder."""

from __future__ import annotations

import argparse
import csv
import json
import os
import sqlite3
import sys
import tempfile
import time
from pathlib import Path

from baccable_lab.can.discover import doctor_lines, parse_mapping
from baccable_lab.capture import capture
from baccable_lab.scenarios import (ProgressStore, default_directory, load_scenarios,
                                    location_label, order_scenarios)
from baccable_lab.storage.sqlite_store import list_sessions, resolve_session


def _root(value: str | None) -> Path:
    return Path(value or os.environ.get("BACCABLE_SESSIONS", "sessions"))


def _scenarios_dir(value: str | None) -> Path:
    return Path(value) if value else default_directory()


def _state_path(value: str | None) -> Path:
    return Path(value) if value else Path("config/scenario_state.json")


def _steps_word(count: int) -> str:
    if count == 1:
        return "krok"
    if 2 <= count % 10 <= 4 and not 12 <= count % 100 <= 14:
        return "kroki"
    return "kroków"


def _colors(args) -> bool:
    return not (getattr(args, "no_color", False) or os.environ.get("NO_COLOR"))


def _parse_context(values) -> dict:
    context: dict[str, str] = {}
    for item in values or []:
        key, sep, value = item.partition("=")
        if not sep or not key.strip() or not value.strip():
            raise ValueError(f"context must be KEY=VALUE: {item}")
        context[key.strip()] = value.strip()
    return context


def _dictionary_edit(args) -> int:
    """Build and validate a candidate field, or merge a proposal into a draft."""
    import copy
    from baccable_lab.knowledge.catalog import load_dictionary, validate_dictionary
    if args.accept:
        if not args.output:
            raise ValueError("--accept requires --output (a draft path)")
        payload = json.loads(Path(args.accept).read_text(encoding="utf-8"))
        messages = payload["messages"] if isinstance(payload, dict) and "messages" in payload else [payload]
        catalog = copy.deepcopy(load_dictionary())
        existing = {(m["role"], m["can_id"]) for m in catalog["messages"]}
        for message in messages:
            key = (message.get("role"), message.get("can_id"))
            if key in existing:
                raise ValueError(f"message already exists: {key}")
            catalog["messages"].append(message)
        validate_dictionary(catalog)
        output = Path(args.output)
        if output.exists():
            raise ValueError("draft already exists; choose a new output path")
        output.write_text(json.dumps(catalog, indent=2) + "\n", encoding="utf-8")
        print(f"accepted {len(messages)} message(s) into draft {output}")
        print("review the evidence, copy into knowledge/giulia.json and run scripts/render_can_dictionary.py")
        return 0
    if args.role is None or args.can_id is None or not args.name or not args.parts:
        raise ValueError("--propose requires --role, --id, --name and --parts")
    signal = {"name": args.name, "parts": [[int(x) for x in part.split(",")] for part in args.parts.split(";")],
              "unit": args.unit, "scale": args.scale, "offset": args.offset, "origin": args.origin}
    if args.enum:
        signal["enum"] = {}
        for pair in args.enum.split(","):
            key, _, value = pair.partition("=")
            signal["enum"][str(int(key))] = value.strip()
    if args.note:
        signal["note"] = args.note
    message = {"role": args.role, "can_id": f"0x{args.can_id:03X}", "title": args.message_title or args.name,
               "signals": [signal]}
    if args.dlc:
        message["exact_dlc"] = args.dlc
    catalog = copy.deepcopy(load_dictionary())
    if any(m["role"] == message["role"] and m["can_id"] == message["can_id"] for m in catalog["messages"]):
        status = "valid; merges into an existing message identity"
    else:
        catalog["messages"].append(message)
        validate_dictionary(catalog)
        status = "valid; new message identity"
    print(json.dumps(message, indent=2))
    print(f"# proposal {status}. Add it to knowledge/giulia.json, then run "
          f"scripts/attach_dictionary_evidence.py and scripts/render_can_dictionary.py", file=sys.stderr)
    return 0


def _session_info(directory: Path) -> int:
    database = sqlite3.connect(directory / "session.sqlite3")
    session = database.execute("SELECT id, started_at, ended_at, duration_seconds, status, host_clock FROM sessions").fetchone()
    print(json.dumps(dict(zip(("id", "started_at", "ended_at", "duration_seconds", "status", "host_clock"), session)), indent=2))
    print("\nBuses:")
    manifest = json.loads((directory / "manifest.json").read_text())
    for role in manifest["roles"]:
        frames = database.execute("SELECT count(*) FROM can_frames WHERE role = ?", (role,)).fetchone()[0]
        dropped = database.execute("SELECT coalesce(sum(dropped_count),0) FROM capture_loss WHERE role = ?", (role,)).fetchone()[0]
        startup = database.execute(
            "SELECT coalesce(sum(dropped_count),0) FROM capture_loss WHERE role = ? AND host_ns <= ?",
            (role, 2_000_000_000),
        ).fetchone()[0]
        active = dropped - startup
        print(f"  {role}: {frames} frames, {dropped} dropped (startup {startup}, in-session {active})")
    losses = database.execute("SELECT count(*), coalesce(sum(dropped_count),0) FROM capture_loss").fetchone()
    events = database.execute("SELECT count(*) FROM events").fetchone()[0]
    print(f"  loss records: {losses[0]}, dropped frames: {losses[1]}")
    print(f"  events: {events}")
    database.close()
    return 0


def _export_basic(directory: Path, output: str | None) -> int:
    database = sqlite3.connect(directory / "session.sqlite3")
    destination = open(output, "w", newline="") if output else sys.stdout
    close = destination is not sys.stdout
    try:
        writer = csv.writer(destination)
        writer.writerow(["kind", "role", "host_ns", "device_timestamp_ms", "can_id", "dlc", "data", "dropped", "label", "note"])
        for row in database.execute("SELECT role, host_ns, device_timestamp_ms, arbitration_id, dlc, data FROM can_frames ORDER BY host_ns, id"):
            role, host_ns, device_ms, can_id, dlc, data = row
            writer.writerow(["frame", role, host_ns, device_ms, f"0x{can_id:X}", dlc, bytes(data).hex(), "", "", ""])
        for row in database.execute("SELECT role, host_ns, device_timestamp_ms, dropped_count FROM capture_loss ORDER BY host_ns, id"):
            role, host_ns, device_ms, dropped = row
            writer.writerow(["loss", role, host_ns, device_ms, "", "", "", dropped, "", ""])
        for row in database.execute("SELECT host_ns, label, note FROM events ORDER BY host_ns, id"):
            writer.writerow(["event", "", row[0], "", "", "", "", "", row[1], row[2]])
    finally:
        database.close()
        if close:
            destination.close()
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="baccable", description="BACCAble Lab capture recorder")
    sub = parser.add_subparsers(dest="command", required=True)
    doctor = sub.add_parser("doctor", help="list serial devices without changing vehicle state")
    doctor.add_argument("--usb-status", action="store_true", help="read firmware USB/UART diagnostics over EP0 (requires PyUSB)")
    doctor.add_argument("--samples", type=int, default=1, help="USB diagnostic samples, one second apart (1-60)")
    capture_parser = sub.add_parser("capture", help="record one or more C1/C2/BH binary streams")
    capture_parser.add_argument("--port", action="append", required=True, metavar="ROLE=DEVICE",
                                help="select a CAN port; repeat for additional roles (C1, C2, BH)")
    capture_parser.add_argument("--sessions", default=None, help="session root (default: ./sessions)")
    capture_parser.add_argument("--no-obd", action="store_true", help="document that OBD is disabled")
    capture_parser.add_argument("--no-voice", action="store_true", help="document that voice is disabled")
    capture_parser.add_argument("--scenario", default=None, help="guided scenario id to start immediately")
    capture_parser.add_argument("--scenarios-dir", default=None, help="scenario library directory (default: bundled)")
    capture_parser.add_argument("--state", default=None, help="scenario progress file (default: config/scenario_state.json)")
    capture_parser.add_argument("--caps", action="store_true", help="show the current action in upper case")
    capture_parser.add_argument("--no-color", action="store_true", help="disable ANSI colours")
    capture_parser.add_argument("--no-bell", action="store_true", help="disable the confirmation bell")
    capture_parser.add_argument("--context", action="append", default=None, metavar="KEY=VALUE",
                                help="record a session context entry, e.g. vehicle=Giulia_MY2020 (repeatable)")
    preview = sub.add_parser("preview", help="practice the live interface with simulated CAN traffic; no hardware required")
    preview.add_argument("--role", action="append", choices=("C1", "C2", "BH"),
                         help="simulated bus to show (repeatable; default: C1, C2 and BH)")
    preview.add_argument("--scenario", default=None, help="guided scenario id to practise")
    preview.add_argument("--scenarios-dir", default=None, help="scenario library directory (default: bundled)")
    preview.add_argument("--caps", action="store_true", help="show the current action in upper case")
    preview.add_argument("--no-color", action="store_true", help="disable ANSI colours")
    preview.add_argument("--no-bell", action="store_true", help="disable the confirmation bell")
    scenarios = sub.add_parser("scenarios", help="list guided capture scenarios and progress")
    scenarios.add_argument("--scenarios-dir", default=None, help="scenario library directory (default: bundled)")
    scenarios.add_argument("--state", default=None, help="scenario progress file (default: config/scenario_state.json)")
    scenarios.add_argument("--json", action="store_true", help="emit machine-readable JSON")
    sessions = sub.add_parser("sessions", help="list completed and active sessions")
    sessions.add_argument("--sessions", default=None)
    session = sub.add_parser("session", help="inspect one session")
    session_sub = session.add_subparsers(dest="session_command", required=True)
    info = session_sub.add_parser("info")
    info.add_argument("session")
    info.add_argument("--sessions", default=None)
    decode = session_sub.add_parser("decode", help="annotate saved frames using the CAN dictionary")
    decode.add_argument("session")
    decode.add_argument("--sessions", default=None)
    decode.add_argument("--role", required=True, choices=("C1", "C2", "BH"))
    decode.add_argument("--id", required=True, type=lambda value: int(value, 0), dest="can_id")
    decode.add_argument("--limit", type=int, default=20, help="maximum frames to print (1-1000)")
    event = sub.add_parser("event", help="add an event to an existing session")
    event_sub = event.add_subparsers(dest="event_command", required=True)
    add = event_sub.add_parser("add")
    add.add_argument("session")
    add.add_argument("label")
    add.add_argument("--at-ns", type=int, required=True)
    add.add_argument("--sessions", default=None)
    export = sub.add_parser("export", help="export a basic CSV view")
    export.add_argument("session")
    export.add_argument("--basic", action="store_true", required=True)
    export.add_argument("--output", "-o", default=None)
    export.add_argument("--sessions", default=None)
    dictionary = sub.add_parser("dictionary", help="inspect the passive, evidence-based CAN dictionary")
    dictionary.add_argument("--role", choices=("C1", "C2", "BH"))
    dictionary.add_argument("--id", type=lambda value: int(value, 0), dest="can_id",
                            help="numeric CAN ID, e.g. 0x46C")
    dictionary.add_argument("--data", help="hex payload to decode; requires --role and --id")
    dictionary.add_argument("--propose", action="store_true",
                            help="build and validate a candidate message from the fields below")
    dictionary.add_argument("--accept", metavar="JSON", default=None,
                            help="merge a proposed message JSON into a draft dictionary (needs --output)")
    dictionary.add_argument("--output", "-o", default=None, help="draft dictionary path for --accept")
    dictionary.add_argument("--title", dest="message_title", default=None, help="candidate message title")
    dictionary.add_argument("--dlc", type=int, default=None, help="candidate exact DLC")
    dictionary.add_argument("--name", default=None, help="candidate signal name")
    dictionary.add_argument("--parts", default=None, help="bit layout: b,lsb,width;b,lsb,width (MSB first)")
    dictionary.add_argument("--scale", type=float, default=1.0, help="candidate scale")
    dictionary.add_argument("--offset", type=float, default=0.0, help="candidate offset")
    dictionary.add_argument("--unit", default="raw", help="candidate unit")
    dictionary.add_argument("--origin", choices=("implementation", "comment", "capture"), default="capture")
    dictionary.add_argument("--enum", default=None, help="enum pairs, e.g. 0=Natural,2=Dynamic")
    dictionary.add_argument("--note", default=None, help="candidate note")
    review = sub.add_parser("review", help="review saved sessions read-only against the CAN dictionary")
    review.add_argument("--sessions", default=None)
    review.add_argument("--output", "-o", required=True, help="new JSON report outside the session directory")
    bundle = sub.add_parser("bundle", help="write one self-contained analysis bundle for a session")
    bundle.add_argument("session")
    bundle.add_argument("--sessions", default=None)
    bundle.add_argument("--output", "-o", required=True, help="new JSON bundle path")
    bundle.add_argument("--max-frames", type=int, default=20000, help="cap on raw excerpt frames")
    bundle.add_argument("--no-excerpts", action="store_true", help="omit raw frame excerpts")
    bundle.add_argument("--no-hints", action="store_true", help="omit structural shape hints")
    bundle.add_argument("--scenarios-dir", default=None, help="scenario library directory (default: bundled)")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        if args.command == "dictionary":
            from baccable_lab.knowledge.catalog import decode_frame, load_dictionary
            if args.propose or args.accept:
                return _dictionary_edit(args)
            catalog = load_dictionary()
            if args.data is not None:
                if args.role is None or args.can_id is None:
                    raise ValueError("--data requires --role and --id")
                result = decode_frame(catalog, args.role, args.can_id, bytes.fromhex(args.data))
            else:
                result = {**catalog, "messages": [m for m in catalog["messages"]
                          if (args.role is None or m["role"] == args.role)
                          and (args.can_id is None or int(m["can_id"], 16) == args.can_id)],
                          "diagnostic_parameters": [p for p in catalog["diagnostic_parameters"]
                          if (args.role is None or p["role"] == args.role)
                          and (args.can_id is None or args.can_id in
                               (int(p["request_id"], 16), int(p["response_id"], 16)))]}
                for key in ('frame_templates','observed_inventory'):
                    result[key] = [item for item in catalog.get(key, [])
                                   if (args.role is None or item['role'] == args.role)
                                   and (args.can_id is None or int(item['can_id'],16) == args.can_id)]
            print(json.dumps(result, indent=2))
            return 0
        if args.command == "review":
            from baccable_lab.knowledge.review import review_sessions
            root = _root(args.sessions).resolve()
            output = Path(args.output).resolve()
            if output == root or root in output.parents:
                raise ValueError("write the report outside the captured session directory")
            if output.exists():
                raise ValueError("report already exists; choose a new output path")
            result = review_sessions(root)
            with output.open("x", encoding="utf-8") as stream:
                json.dump(result, stream, indent=2)
                stream.write("\n")
            print(f"reviewed {len(result['sessions'])} sessions, {result['total_frames']} frames: {output}")
            return 0
        if args.command == "bundle":
            from baccable_lab.knowledge.bundle import build_bundle
            from baccable_lab.knowledge.catalog import load_dictionary
            directory = resolve_session(_root(args.sessions), args.session)
            output = Path(args.output)
            if output.exists():
                raise ValueError("bundle already exists; choose a new output path")
            result = build_bundle(directory, load_dictionary(), max_frames_total=args.max_frames,
                                  include_excerpts=not args.no_excerpts, include_hints=not args.no_hints,
                                  scenarios=load_scenarios(_scenarios_dir(args.scenarios_dir)))
            output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
            print(f"bundle {result['session']['id']} ({result['session']['status']}): {output}")
            return 0
        if args.command == "doctor":
            print("\n".join(doctor_lines()))
            if args.usb_status:
                from baccable_lab.can.usb_status import read_usb_status
                if not 1 <= args.samples <= 60:
                    raise ValueError("--samples must be between 1 and 60")
                failed = False
                for sample in range(args.samples):
                    if sample:
                        time.sleep(1)
                    reports = read_usb_status()
                    print(json.dumps({"sample": sample + 1, "usb_status": reports}, indent=2))
                    failed |= not reports or any("error" in report for report in reports)
                return 2 if failed else 0
            return 0
        if args.command == "capture":
            library = load_scenarios(_scenarios_dir(args.scenarios_dir))
            return capture(parse_mapping(args.port), _root(args.sessions), sys.argv,
                           scenarios=library, scenario=args.scenario,
                           progress=ProgressStore(_state_path(args.state)),
                           color=_colors(args), caps=args.caps, bell=not args.no_bell,
                           context=_parse_context(args.context))
        if args.command == "preview":
            roles = args.role or ["C1", "C2", "BH"]
            ports = {role: f"SIMULATED:{role}" for role in roles}
            library = load_scenarios(_scenarios_dir(args.scenarios_dir))
            with tempfile.TemporaryDirectory(prefix="baccable-preview-") as temporary:
                return capture(ports, Path(temporary) / "sessions", ["baccable", "preview"],
                               preview=True, scenarios=library, scenario=args.scenario,
                               color=_colors(args), caps=args.caps, bell=not args.no_bell)
        if args.command == "scenarios":
            library = load_scenarios(_scenarios_dir(args.scenarios_dir))
            state = ProgressStore(_state_path(args.state))
            ordered = order_scenarios(library.values())
            if args.json:
                print(json.dumps([{"id": s.id, "title": s.title, "group": s.group, "vehicle": s.vehicle,
                                   "site": s.site, "location": location_label(s),
                                   "runs": s.runs, "complete": state.complete(s.id, s.total_steps),
                                   "steps": [{"action": step.action, "expect": step.expect}
                                              for step in s.steps]} for s in ordered], indent=2))
            else:
                group = None
                for item in ordered:
                    if item.group != group:
                        group = item.group
                        print(f"[{group or 'general'}]")
                    mark = "x" if state.complete(item.id, item.total_steps) else " "
                    print(f"  [{mark}] {item.id:<26} {location_label(item):<12} {item.total_steps:>2} {_steps_word(item.total_steps)}  {item.title}")
                print(f"{state.completed_count(ordered)}/{len(ordered)} scenariuszy ukończonych")
            return 0
        if args.command == "sessions":
            paths = list_sessions(_root(args.sessions)) if _root(args.sessions).exists() else []
            for path in paths:
                print(path.name)
            return 0
        if args.command == "session":
            if args.session_command == "decode":
                from baccable_lab.knowledge.catalog import decode_frame, load_dictionary
                from baccable_lab.knowledge.review import open_readonly
                if not 1 <= args.limit <= 1000 or not 0 <= args.can_id <= 0x1FFFFFFF:
                    raise ValueError("--limit must be 1-1000 and --id a classical CAN identifier")
                directory = resolve_session(_root(args.sessions), args.session)
                database = open_readonly(directory)
                try:
                    catalog = load_dictionary()
                    for host, device, dlc, data in database.execute(
                        "SELECT host_ns,device_timestamp_ms,dlc,data FROM can_frames WHERE role=? "
                        "AND arbitration_id=? ORDER BY sequence LIMIT ?", (args.role, args.can_id, args.limit)):
                        result = decode_frame(catalog, args.role, args.can_id, bytes(data)[:dlc])
                        print(json.dumps({"host_ns":host,"device_timestamp_ms":device,**result}))
                finally:
                    database.close()
                return 0
            return _session_info(resolve_session(_root(args.sessions), args.session))
        if args.command == "event":
            directory = resolve_session(_root(args.sessions), args.session)
            database = sqlite3.connect(directory / "session.sqlite3")
            database.execute("INSERT INTO events(session_id, host_ns, label, source) VALUES (?, ?, ?, ?)",
                             (directory.name, args.at_ns, args.label, "cli"))
            database.commit()
            database.close()
            print(f"added {args.label} at {args.at_ns} ns")
            return 0
        if args.command == "export":
            return _export_basic(resolve_session(_root(args.sessions), args.session), args.output)
    except (ValueError, OSError, RuntimeError, sqlite3.Error) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2
    return 2


if __name__ == "__main__":
    raise SystemExit(main())

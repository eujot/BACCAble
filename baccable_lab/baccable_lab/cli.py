"""Command line entry point for the Milestone 1 recorder."""

from __future__ import annotations

import argparse
import csv
import json
import os
import sqlite3
import sys
from pathlib import Path

from baccable_lab.can.discover import doctor_lines, parse_mapping
from baccable_lab.capture import capture
from baccable_lab.storage.sqlite_store import list_sessions, resolve_session


def _root(value: str | None) -> Path:
    return Path(value or os.environ.get("BACCABLE_SESSIONS", "sessions"))


def _session_info(directory: Path) -> int:
    database = sqlite3.connect(directory / "session.sqlite3")
    session = database.execute("SELECT id, started_at, ended_at, duration_seconds, status, host_clock FROM sessions").fetchone()
    print(json.dumps(dict(zip(("id", "started_at", "ended_at", "duration_seconds", "status", "host_clock"), session)), indent=2))
    print("\nBuses:")
    manifest = json.loads((directory / "manifest.json").read_text())
    for role in manifest["roles"]:
        frames = database.execute("SELECT count(*) FROM can_frames WHERE role = ?", (role,)).fetchone()[0]
        dropped = database.execute("SELECT coalesce(sum(dropped_count),0) FROM capture_loss WHERE role = ?", (role,)).fetchone()[0]
        print(f"  {role}: {frames} frames, {dropped} dropped")
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
        writer.writerow(["kind", "role", "host_ns", "device_timestamp_ms", "can_id", "dlc", "data", "dropped", "label"])
        for row in database.execute("SELECT role, host_ns, device_timestamp_ms, arbitration_id, dlc, data FROM can_frames ORDER BY host_ns, id"):
            role, host_ns, device_ms, can_id, dlc, data = row
            writer.writerow(["frame", role, host_ns, device_ms, f"0x{can_id:X}", dlc, bytes(data).hex(), "", ""])
        for row in database.execute("SELECT role, host_ns, device_timestamp_ms, dropped_count FROM capture_loss ORDER BY host_ns, id"):
            role, host_ns, device_ms, dropped = row
            writer.writerow(["loss", role, host_ns, device_ms, "", "", "", dropped, ""])
        for row in database.execute("SELECT host_ns, label FROM events ORDER BY host_ns, id"):
            writer.writerow(["event", "", row[0], "", "", "", "", "", row[1]])
    finally:
        database.close()
        if close:
            destination.close()
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="baccable", description="BACCAble Lab capture recorder")
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("doctor", help="list serial devices without changing vehicle state")
    capture_parser = sub.add_parser("capture", help="record one or more C1/C2/BH binary streams")
    capture_parser.add_argument("--port", action="append", required=True, metavar="ROLE=DEVICE",
                                help="select a CAN port; repeat for additional roles (C1, C2, BH)")
    capture_parser.add_argument("--sessions", default=None, help="session root (default: ./sessions)")
    capture_parser.add_argument("--no-obd", action="store_true", help="document that OBD is disabled")
    capture_parser.add_argument("--no-voice", action="store_true", help="document that voice is disabled")
    sessions = sub.add_parser("sessions", help="list completed and active sessions")
    sessions.add_argument("--sessions", default=None)
    session = sub.add_parser("session", help="inspect one session")
    session_sub = session.add_subparsers(dest="session_command", required=True)
    info = session_sub.add_parser("info")
    info.add_argument("session")
    info.add_argument("--sessions", default=None)
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
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        if args.command == "doctor":
            print("\n".join(doctor_lines()))
            return 0
        if args.command == "capture":
            return capture(parse_mapping(args.port), _root(args.sessions), sys.argv)
        if args.command == "sessions":
            paths = list_sessions(_root(args.sessions)) if _root(args.sessions).exists() else []
            for path in paths:
                print(path.name)
            return 0
        if args.command == "session":
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
    except (ValueError, OSError, RuntimeError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2
    return 2


if __name__ == "__main__":
    raise SystemExit(main())

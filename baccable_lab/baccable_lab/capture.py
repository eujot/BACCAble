"""Concurrent three-port capture with a deliberately small terminal UI."""

from __future__ import annotations

import json
import queue
import select
import sys
import termios
import threading
import time
import tty
from datetime import datetime, timezone
from pathlib import Path

from baccable_lab.can.baccable_binary import BinaryCaptureParser
from baccable_lab.storage.manifest import write_manifest
from baccable_lab.storage.raw_writer import RawWriters
from baccable_lab.storage.sqlite_store import SessionStore

MARKERS = {
    "1": "unlock", "2": "driver_door_open", "3": "driver_door_close",
    "4": "engine_start", "5": "brake_on", "6": "brake_off",
    "7": "left_indicator_on", "8": "left_indicator_off",
    "9": "right_indicator_on", "0": "right_indicator_off",
    "l": "lock", "c": "custom",
}


def new_session_id() -> str:
    return datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%SZ")


class _Reader(threading.Thread):
    def __init__(self, role: str, device: str, output: queue.Queue, stop: threading.Event):
        super().__init__(name=f"capture-{role}", daemon=True)
        self.role, self.device, self.output, self.stop = role, device, output, stop
        self.error: str | None = None

    def run(self) -> None:
        try:
            import serial  # type: ignore
            with serial.Serial(self.device, baudrate=115200, timeout=0.2) as port:
                while not self.stop.is_set():
                    data = port.read(4096)
                    if data:
                        self.output.put((self.role, time.monotonic_ns(), data))
        except Exception as exc:  # surfaced in the final summary, never hidden
            self.error = f"{self.role}: {exc}"
            self.stop.set()


class _Keyboard:
    def __init__(self):
        self.enabled = sys.stdin.isatty()
        self.previous = None
        if self.enabled:
            self.previous = termios.tcgetattr(sys.stdin)
            tty.setcbreak(sys.stdin.fileno())

    def read(self) -> str | None:
        if not self.enabled or not select.select([sys.stdin], [], [], 0)[0]:
            return None
        return sys.stdin.read(1)

    def close(self) -> None:
        if self.previous is not None:
            termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self.previous)


def capture(ports: dict[str, str], root: Path, command: list[str]) -> int:
    roles = ["C1", "C2", "BH"]
    missing = [role for role in roles if role not in ports]
    if missing:
        raise ValueError(f"missing role mapping: {', '.join(missing)}")
    root.mkdir(parents=True, exist_ok=True)
    session_id = new_session_id()
    directory = root / session_id
    directory.mkdir()
    write_manifest(directory, session_id, ports, command)
    store = SessionStore(directory, session_id, roles)
    raw = RawWriters(directory, roles)
    parsers = {role: BinaryCaptureParser(role) for role in roles}
    incoming: queue.Queue = queue.Queue(maxsize=4096)
    stop = threading.Event()
    readers = [_Reader(role, ports[role], incoming, stop) for role in roles]
    keyboard = _Keyboard()
    started = time.monotonic_ns()
    events = 0
    status = "complete"
    print(f"Session {session_id}; press 1-0/l for markers, q or Ctrl-C to stop")
    for reader in readers:
        reader.start()
    try:
        last_screen = 0.0
        last_counts = {role: 0 for role in roles}
        while not stop.is_set():
            key = keyboard.read()
            if key in {"q", "Q", "\x03"}:
                break
            if key in MARKERS:
                store.add_event(time.monotonic_ns() - started, MARKERS[key])
                events += 1
                print(f"\nEVENT {MARKERS[key]}")
            try:
                role, host_ns, data = incoming.get(timeout=0.1)
            except queue.Empty:
                role = None
            if role is not None:
                offset = raw.write(role, data)
                for record in parsers[role].feed(data, host_ns - started):
                    store.add_record(record, offset)
            now = time.monotonic()
            if now - last_screen >= 1.0:
                counts = store.counts()
                interval = now - last_screen if last_screen else now - (started / 1_000_000_000)
                if interval <= 0:
                    interval = 1.0
                line = " | ".join(
                    f"{role} {counts[role]['frames'] - last_counts[role]}/{interval:.1f}s "
                    f"({(counts[role]['frames'] - last_counts[role]) / interval:.0f}/s), "
                    f"{counts[role]['dropped']}drop"
                    for role in roles
                )
                elapsed = (time.monotonic_ns() - started) / 1_000_000_000
                print(f"\rRECORDING {elapsed:.0f}s {line}    ", end="", flush=True)
                store.commit()
                last_counts = {role: counts[role]['frames'] for role in roles}
                last_screen = now
    except KeyboardInterrupt:
        pass
    except Exception:
        status = "failed"
        raise
    finally:
        stop.set()
        for reader in readers:
            reader.join(timeout=1.0)
        keyboard.close()
        for parser in parsers.values():
            parser.finish()
        duration = (time.monotonic_ns() - started) / 1_000_000_000
        errors = [reader.error for reader in readers if reader.error]
        stats = {
            "session_id": session_id, "duration_seconds": duration, "events": events,
            "status": status, "roles": {role: parsers[role].stats.__dict__ for role in roles},
            "reader_errors": errors,
        }
        raw.close()
        store.finish("failed" if errors else status, duration, stats)
        print(f"\nSession complete: {directory}")
        print(json.dumps(stats, indent=2))
    return 0 if not errors else 2

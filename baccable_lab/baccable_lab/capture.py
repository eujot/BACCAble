"""Capture selected CAN ports with a deliberately small terminal UI."""

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
    def __init__(self, role: str, device: str, output: queue.Queue, stop: threading.Event, abort: threading.Event):
        super().__init__(name=f"capture-{role}", daemon=True)
        self.role, self.device, self.output, self.stop = role, device, output, stop
        self.abort = abort
        self.discarded_bytes = 0
        self.error: str | None = None

    def run(self) -> None:
        try:
            import serial  # type: ignore
            with serial.Serial(self.device, baudrate=115200, timeout=0.2) as port:
                while not self.stop.is_set():
                    data = port.read(4096)
                    if data:
                        item = (self.role, time.monotonic_ns(), data)
                        # Normal stop still delivers the last read. A failed writer
                        # explicitly aborts delivery so a full queue cannot trap us.
                        while not self.abort.is_set():
                            try:
                                self.output.put(item, timeout=0.1)
                                break
                            except queue.Full:
                                continue
                        else:
                            self.discarded_bytes += len(data)
        except Exception as exc:  # surfaced in the final summary, never hidden
            self.error = f"{self.role}: {exc}"
            self.stop.set()


class _Keyboard:
    def __init__(self):
        self.enabled = sys.stdin.isatty()
        self.previous = None
        if self.enabled:
            self.previous = termios.tcgetattr(sys.stdin)
            try:
                tty.setcbreak(sys.stdin.fileno())
            except BaseException:
                self.close()
                raise

    def read(self) -> str | None:
        if not self.enabled or not select.select([sys.stdin], [], [], 0)[0]:
            return None
        return sys.stdin.read(1)

    def close(self) -> None:
        if self.previous is not None:
            termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self.previous)


def capture(ports: dict[str, str], root: Path, command: list[str]) -> int:
    if not ports or any(role not in {"C1", "C2", "BH"} or not device
                        for role, device in ports.items()):
        raise ValueError("provide at least one port mapping: C1=DEVICE, C2=DEVICE or BH=DEVICE")
    if len(set(ports.values())) != len(ports):
        raise ValueError("each role must use a different serial device")
    roles = [role for role in ("C1", "C2", "BH") if role in ports]
    root.mkdir(parents=True, exist_ok=True)
    session_id = new_session_id()
    directory = root / session_id
    directory.mkdir()
    write_manifest(directory, session_id, ports, command)
    store = raw = keyboard = None
    parsers = {role: BinaryCaptureParser(role) for role in roles}
    incoming: queue.Queue = queue.Queue(maxsize=4096)
    stop, abort = threading.Event(), threading.Event()
    readers = []
    started = time.monotonic_ns()
    events = 0
    discarded_queued_bytes = 0
    errors: list[str] = []

    def consume(item):
        role, host_ns, data = item
        offset = raw.write(role, data)
        for record in parsers[role].feed(data, host_ns - started):
            store.add_record(record, offset)

    def cleanup(label, action):
        try:
            action()
        except Exception as exc:
            errors.append(f"{label}: {exc}")

    try:
        store = SessionStore(directory, session_id, roles)
        raw = RawWriters(directory, roles)
        keyboard = _Keyboard()
        print(f"Session {session_id}; press 1-0/l for markers, q or Ctrl-C to stop")
        for role in roles:
            reader = _Reader(role, ports[role], incoming, stop, abort)
            reader.start()
            readers.append(reader)
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
                item = incoming.get(timeout=0.1)
            except queue.Empty:
                pass
            else:
                consume(item)
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
    except Exception as exc:
        errors.append(f"capture: {exc}")
        abort.set()
    finally:
        stop.set()
        # Producers may still hold a final read, including while blocked on put.
        # Drain concurrently until every producer exits and the queue is empty.
        idle_since = time.monotonic()
        while readers:
            try:
                item = incoming.get(timeout=0.05)
            except queue.Empty:
                if not any(reader.is_alive() for reader in readers):
                    break
                if time.monotonic() - idle_since > 2.0:
                    errors.append("serial reader did not stop within its read timeout")
                    abort.set()
                    break
            else:
                idle_since = time.monotonic()
                if not abort.is_set():
                    try:
                        consume(item)
                    except Exception as exc:
                        errors.append(f"drain: {exc}")
                        abort.set()
                else:
                    discarded_queued_bytes += len(item[2])
        abort.set()
        for reader in readers:
            cleanup(f"join {reader.role}", lambda reader=reader: reader.join(timeout=0.5))
            if reader.is_alive():
                errors.append(f"{reader.role}: reader still running")
        if keyboard is not None:
            cleanup("restore terminal", keyboard.close)
        if raw is not None:
            cleanup("close raw files", raw.close)
        for parser in parsers.values():
            parser.finish()
        duration = (time.monotonic_ns() - started) / 1_000_000_000
        reader_errors = [reader.error for reader in readers if reader.error]
        errors.extend(reader_errors)
        stats = {
            "session_id": session_id, "duration_seconds": duration, "events": events,
            "status": "failed" if errors else "complete",
            "roles": {role: parsers[role].stats.__dict__ for role in roles},
            "reader_errors": reader_errors, "errors": errors,
            "discarded_reader_bytes": sum(reader.discarded_bytes for reader in readers),
            "discarded_queued_bytes": discarded_queued_bytes,
        }
        if store is not None:
            cleanup("finish database", lambda: store.finish(stats["status"], duration, stats))
            cleanup("close database", store.close)
        stats["status"] = "failed" if errors else "complete"
        # Also attempt a failure summary if initialization/finalization failed.
        if store is None or errors:
            cleanup("write summary", lambda: (directory / "summary.json").write_text(
                json.dumps(stats, indent=2) + "\n"))
        print(f"\nSession {'failed' if errors else 'complete'}: {directory}")
        print(json.dumps(stats, indent=2))
    return 2 if errors else 0

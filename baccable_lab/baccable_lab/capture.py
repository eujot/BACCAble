"""Capture selected CAN ports with a deliberately small terminal UI."""

from __future__ import annotations

import json
import queue
import select
import shutil
import sys
import termios
import threading
import time
import tty
from datetime import datetime, timezone
from pathlib import Path

from baccable_lab.can.baccable_binary import BinaryCaptureParser
from baccable_lab.events.catalog import EVENT_GROUPS, MARKERS, label_text
from baccable_lab.events.search import search_markers
from baccable_lab.storage.manifest import write_manifest
from baccable_lab.storage.raw_writer import RawWriters
from baccable_lab.storage.sqlite_store import SessionStore
from baccable_lab.ui import marker_help, render_dashboard
STARTUP_LOSS_WINDOW_NS = 2_000_000_000


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


class _DemoReader(threading.Thread):
    """Generate clearly synthetic binary CAN records for offline UI practice."""

    def __init__(self, role: str, device: str, output: queue.Queue, stop: threading.Event, abort: threading.Event):
        super().__init__(name=f"demo-{role}", daemon=True)
        self.role, self.output, self.stop, self.abort = role, output, stop, abort
        self.error = None
        self.discarded_bytes = 0

    def run(self) -> None:
        counter = 0
        sample_ids = (0x090, 0x2FA, 0x3B0, 0x5A0, 0x610)
        while not self.stop.is_set() and not self.abort.is_set():
            tick = (time.monotonic_ns() // 1_000_000) & 0xFFFFFF
            can_id = sample_ids[counter % len(sample_ids)]
            data = bytes(((counter + index * 17) & 0xFF for index in range(8)))
            record = bytes((0xA8,)) + tick.to_bytes(3, "little") + can_id.to_bytes(4, "little") + data
            try:
                self.output.put((self.role, time.monotonic_ns(), record), timeout=0.1)
            except queue.Full:
                self.discarded_bytes += len(record)
            counter += 1
            self.stop.wait(0.2)


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

    def read_line(self, prompt: str) -> str:
        """Read a line without leaving the terminal in cbreak mode."""
        self.close()
        try:
            return input(prompt).strip()
        finally:
            if self.enabled:
                self.previous = termios.tcgetattr(sys.stdin)
                tty.setcbreak(sys.stdin.fileno())


def capture(ports: dict[str, str], root: Path, command: list[str], *, preview: bool = False) -> int:
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
    last_event_id: int | None = None
    last_event_label: str | None = None
    startup_losses = {role: 0 for role in roles}
    in_session_losses = {role: 0 for role in roles}
    startup_loss_reported: set[str] = set()
    discarded_queued_bytes = 0
    errors: list[str] = []
    latest_frames: dict[str, tuple[float, int, bytes]] = {}
    timeline: list[tuple[float, str, str]] = []
    group_selection: str | None = None
    status_message = ""

    def consume(item):
        role, host_ns, data = item
        offset = raw.write(role, data)
        for record in parsers[role].feed(data, host_ns - started):
            store.add_record(record, offset)
            if record.is_loss:
                if record.host_ns <= STARTUP_LOSS_WINDOW_NS:
                    startup_losses[role] += record.dropped_count
                    if role not in startup_loss_reported:
                        print(f"\nWARNING {role}: {record.dropped_count} frames lost during capture startup; recording continues")
                        startup_loss_reported.add(role)
                else:
                    in_session_losses[role] += record.dropped_count
            elif record.arbitration_id is not None:
                latest_frames[role] = (record.host_ns / 1_000_000_000,
                                       record.arbitration_id, record.data)

    def cleanup(label, action):
        try:
            action()
        except Exception as exc:
            errors.append(f"{label}: {exc}")

    try:
        store = SessionStore(directory, session_id, roles)
        raw = RawWriters(directory, roles)
        keyboard = _Keyboard()
        print(f"{'Offline demo' if preview else 'Session ' + session_id}; {marker_help()}")
        for role in roles:
            reader_type = _DemoReader if preview else _Reader
            reader = reader_type(role, ports[role], incoming, stop, abort)
            reader.start()
            readers.append(reader)
        last_screen = 0.0
        last_commit = 0.0
        last_counts = {role: 0 for role in roles}
        while not stop.is_set():
            key = keyboard.read()
            label = ""
            marker_source = "keyboard"
            selected_from_group = False
            if key in {"q", "Q", "\x03"}:
                break
            if key in {"\x1b", "\x7f", "\b"} and group_selection is not None:
                group_selection = None
                status_message = "Marker selection cancelled"
            elif group_selection == "":
                if key in EVENT_GROUPS:
                    group_selection = key
                    status_message = f"Group selected. Press an item number; 0 selects item 10."
                elif key:
                    status_message = "Choose a group key shown on screen, or Esc to cancel"
            elif group_selection is not None:
                if key and (key.isdigit()):
                    _, entries = EVENT_GROUPS[group_selection]
                    index = 9 if key == "0" else int(key) - 1
                    if 0 <= index < len(entries):
                        label = entries[index][0]
                        group_selection = None
                        selected_from_group = True
                    else:
                        label = ""
                        status_message = "No marker at that number; choose another item or Esc"
                else:
                    label = ""
            elif key in {"?", "m", "M"}:
                status_message = marker_help()
            elif key == "g":
                group_selection = ""
                status_message = "Choose a marker group key shown below"
            elif key == "f" and hasattr(keyboard, "read_line"):
                query = keyboard.read_line("Find marker or type an action: ")
                if query:
                    matches = search_markers(query)
                    if len(matches) == 1:
                        label = matches[0][0]
                        status_message = f"Matched marker: {matches[0][1]}"
                    elif len(matches) > 1:
                        choices = "\n".join(f"{i}. {title}" for i, (_, title) in enumerate(matches, 1))
                        answer = keyboard.read_line(
                            f"Matches for {query!r}:\n{choices}\n"
                            "Choose a number, or press Enter to add the typed action: "
                        )
                        if answer.isdigit() and 1 <= int(answer) <= len(matches):
                            label, title = matches[int(answer) - 1]
                            status_message = f"Matched marker: {title}"
                        elif not answer:
                            label = query
                            marker_source = "typed"
                            status_message = "Added typed action as a custom marker"
                        else:
                            status_message = "Marker search cancelled; choose a listed number or press Enter"
                    else:
                        label = query
                        marker_source = "typed"
                        status_message = "No catalog match; added typed action as a custom marker"
                else:
                    status_message = "Marker search cancelled"
            elif key == "u":
                if last_event_id is None:
                    status_message = "No marker to undo"
                else:
                    store.delete_event(last_event_id)
                    store.commit()
                    status_message = f"Undid {last_event_label}"
                    if timeline:
                        timeline.pop()
                    last_event_id = None
                    last_event_label = None
            elif key == "n":
                if last_event_id is None:
                    status_message = "No marker to annotate"
                elif hasattr(keyboard, "read_line"):
                    note = keyboard.read_line("Note: ")
                    if note:
                        store.update_event_note(last_event_id, note)
                        store.commit()
                        if timeline:
                            at, label, _ = timeline[-1]
                            timeline[-1] = (at, label, note)
                        status_message = f"Note saved: {note}"
            else:
                label = ""
            if not selected_from_group and not group_selection and key in MARKERS:
                label = MARKERS[key]
            if label:
                if label == "custom" and hasattr(keyboard, "read_line"):
                    label = keyboard.read_line("Marker label: ") or "custom"
                at = time.monotonic_ns() - started
                last_event_id = store.add_event(time.monotonic_ns() - started, label, source=marker_source)
                last_event_label = label
                timeline.append((at / 1_000_000_000, label, ""))
                events += 1
                if not status_message.startswith("Matched marker:") and not status_message.startswith("No catalog match") and not status_message.startswith("Added typed action"):
                    status_message = f"Added marker: {label_text(label)}"
            try:
                item = incoming.get(timeout=0.1)
            except queue.Empty:
                item = None
            if item is not None:
                consume(item)
            # Drain the current burst in one pass so the live frame preview
            # does not lag behind when several CAN roles are busy at once.
            while True:
                try:
                    item = incoming.get_nowait()
                except queue.Empty:
                    break
                consume(item)
            now = time.monotonic()
            if now - last_screen >= (0.25 if keyboard.enabled else 1.0):
                counts = {role: {"frames": parsers[role].stats.frames,
                                 "dropped": parsers[role].stats.dropped_frames} for role in roles}
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
                if keyboard.enabled and sys.stdout.isatty():
                    prompt = status_message
                    if group_selection == "":
                        prompt = "Select group: " + "  ".join(f"{key}={title}" for key, (title, _) in EVENT_GROUPS.items())
                    elif group_selection:
                        title, _ = EVENT_GROUPS[group_selection]
                        prompt = f"Select a {title} marker by number"
                    dashboard = render_dashboard(
                        mode="OFFLINE PREVIEW" if preview else "LIVE CAPTURE", elapsed=elapsed,
                        counts=counts, latest=latest_frames, timeline=timeline,
                        width=shutil.get_terminal_size((100, 24)).columns, prompt=prompt,
                        selected_group=group_selection)
                    print("\x1b[2J\x1b[H" + dashboard, end="\x1b[J", flush=True)
                else:
                    print(f"\r{'PREVIEW' if preview else 'RECORDING'} {elapsed:.0f}s {line}    ", end="", flush=True)
                if now - last_commit >= 1.0:
                    store.commit()
                    last_commit = now
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
            "startup_loss_window_seconds": STARTUP_LOSS_WINDOW_NS / 1_000_000_000,
            "startup_dropped": startup_losses,
            "in_session_dropped": in_session_losses,
        }
        if store is not None:
            cleanup("finish database", lambda: store.finish(stats["status"], duration, stats))
            cleanup("close database", store.close)
        stats["status"] = "failed" if errors else "complete"
        # Also attempt a failure summary if initialization/finalization failed.
        if store is None or errors:
            cleanup("write summary", lambda: (directory / "summary.json").write_text(
                json.dumps(stats, indent=2) + "\n"))
        if preview:
            print("\nOffline preview ended. Demo frames and practice markers were discarded; no session was saved.")
        else:
            print(f"\nSession {'failed' if errors else 'complete'}: {directory}")
            print(json.dumps(stats, indent=2))
    return 2 if errors else 0

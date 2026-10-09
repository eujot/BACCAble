"""Capture selected CAN ports with a deliberately small, scenario-first terminal UI."""

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
from baccable_lab.events.catalog import label_text_pl
from baccable_lab.scenarios import (ProgressStore, ScenarioSession, group_label,
                                    location_label)
from baccable_lab.storage.manifest import write_manifest
from baccable_lab.storage.raw_writer import RawWriters
from baccable_lab.storage.sqlite_store import SessionStore
from baccable_lab.ui import render_dashboard
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


def capture(ports: dict[str, str], root: Path, command: list[str], *, preview: bool = False,
            scenarios: dict | None = None, scenario: str | None = None,
            progress: ProgressStore | None = None, color: bool = True, caps: bool = False,
            bell: bool = True, context: dict | None = None) -> int:
    if not ports or any(role not in {"C1", "C2", "BH"} or not device
                        for role, device in ports.items()):
        raise ValueError("provide at least one port mapping: C1=DEVICE, C2=DEVICE or BH=DEVICE")
    if len(set(ports.values())) != len(ports):
        raise ValueError("each role must use a different serial device")
    scenarios = dict(scenarios or {})
    if scenario is not None and scenario not in scenarios:
        raise ValueError(f"unknown scenario: {scenario}")
    progress_store = progress if progress is not None else ProgressStore(None)
    scenario_session = ScenarioSession(scenarios.values(), progress_store)
    roles = [role for role in ("C1", "C2", "BH") if role in ports]
    root.mkdir(parents=True, exist_ok=True)
    session_id = new_session_id()
    directory = root / session_id
    directory.mkdir()
    write_manifest(directory, session_id, ports, command, context=context)
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
    message = ""
    done_count = 0
    failed_count = 0

    def ring() -> None:
        # A short bell so the operator knows a confirmation or a new step
        # happened without looking at the screen.
        if bell and keyboard is not None and keyboard.enabled:
            sys.stdout.write("\x07")
            sys.stdout.flush()

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

    def start_scenario(scenario_id: str | None = None) -> None:
        nonlocal message
        if scenario_session.total() == 0:
            message = "Nie wczytano scenariuszy"
            return
        runner = scenario_session.start(scenario_id)
        if runner is None:
            message = f"Wszystkie scenariusze ukończone ({scenario_session.total()}). Naciśnij s, aby powtórzyć."
        else:
            message = (f"Wznów: {runner.scenario.title}" if runner.handled
                       else f"Start: {runner.scenario.title}")
            ring()

    def confirm(outcome: str) -> None:
        # A confirmation writes a timeline marker plus a structured row so the
        # offline analysis can pair the matching ON/OFF steps across runs.
        nonlocal message, last_event_id, last_event_label, events, done_count, failed_count
        runner = scenario_session.runner
        if runner is None:
            return
        pending = runner.done() if outcome == "done" else runner.failed()
        if pending is None:
            return
        run, index, step = pending
        at = time.monotonic_ns() - started
        label = step.marker_label
        note = f"scenario {runner.scenario.id} run {run} step {index + 1} {outcome}"
        event_id = store.add_event(at, label, source="scenario", note=note)
        store.add_scenario_step(at, event_id, runner.scenario.id, index, step.key, step.expect, run, outcome)
        last_event_id, last_event_label = event_id, label
        events += 1
        done_count += outcome == "done"
        failed_count += outcome == "failed"
        scenario_session.progress.record(runner.scenario.id, runner.handled,
                                         runner.scenario.total_steps, outcome)
        ring()
        if runner.finished:
            finished = runner.scenario.title
            following = scenario_session.advance()
            if following:
                message = f"Ukończono: {finished}. Następny: {following.scenario.title}"
            else:
                message = f"Ukończono: {finished}. Wszystkie scenariusze ukończone."
        else:
            handled, total = runner.progress()
            outcome_pl = "zrobione" if outcome == "done" else "nieudane"
            message = f"{outcome_pl}: {step.action} ({handled}/{total})"

    def undo() -> None:
        nonlocal message, last_event_id, last_event_label, events, done_count, failed_count
        runner = scenario_session.runner
        if last_event_id is None or runner is None or not runner.records:
            message = "Brak do cofnięcia"
            return
        store.delete_event(last_event_id)
        store.delete_scenario_step(last_event_id)
        store.commit()
        events -= 1
        run, index, step, outcome = runner.records.pop()
        runner.handled -= 1
        runner.run, runner.index = run, index
        runner.finished = False
        scenario_session.progress.set_handled(runner.scenario.id, runner.handled,
                                              runner.scenario.total_steps)
        if outcome == "done":
            done_count = max(0, done_count - 1)
        else:
            failed_count = max(0, failed_count - 1)
        outcome_pl = "zrobione" if outcome == "done" else "nieudane"
        message = f"Cofnięto {outcome_pl}: {step.action}"

    def add_note() -> None:
        nonlocal message
        if last_event_id is None or not hasattr(keyboard, "read_line"):
            message = "Brak kroku do oznaczenia"
            return
        note = keyboard.read_line("Notatka do ostatniego kroku: ")
        if note:
            store.update_event_note(last_event_id, note)
            store.update_scenario_note(last_event_id, note)
            store.commit()
            message = f"Notatka: {note}"

    def scenario_panel() -> dict:
        runner = scenario_session.runner
        if runner is None:
            if scenario_session.total() == 0:
                return {"empty": True}
            return {"complete": True, "completed_scenarios": scenario_session.completed(),
                    "total_scenarios": scenario_session.total(), "message": message}
        pending = runner.current()
        if pending is not None:
            run, index, step = pending
        else:
            run, index = runner.run, runner.index
            step = runner.scenario.steps[index]
        steps = runner.scenario.steps
        next_action = ""
        next_index, next_run = index + 1, run
        if next_index >= len(steps):
            next_index, next_run = 0, run + 1
        if next_run <= runner.scenario.runs:
            next_action = steps[next_index].action
        return {"number": scenario_session.current_number(), "total_scenarios": scenario_session.total(),
                "completed_scenarios": scenario_session.completed(),
                "title": runner.scenario.title, "location": location_label(runner.scenario),
                "group": group_label(runner.scenario.group),
                "run": run, "runs": runner.scenario.runs, "step_index": index,
                "steps_in_run": len(runner.scenario.steps), "action": step.action,
                "expected": label_text_pl(step.expect) if step.expect else "", "paused": runner.paused,
                "handled": runner.handled, "total": runner.scenario.total_steps,
                "next_action": next_action,
                "guard": list(runner.scenario.confounder_guard),
                "min_separation_s": runner.scenario.min_separation_s, "message": message}

    try:
        store = SessionStore(directory, session_id, roles)
        raw = RawWriters(directory, roles)
        keyboard = _Keyboard()
        start_scenario(scenario)
        print(f"{'Podgląd offline' if preview else 'Sesja ' + session_id}; "
              "scenariusze: ENTER=zrobione x=nieudane p=zatrzymaj r=wznów u=cofnij s=wybierz q=zakończ")
        for role in roles:
            reader_type = _DemoReader if preview else _Reader
            reader = reader_type(role, ports[role], incoming, stop, abort)
            reader.start()
            readers.append(reader)
        last_screen = 0.0
        last_commit = 0.0
        quit_armed = False
        last_counts = {role: 0 for role in roles}
        while not stop.is_set():
            key = keyboard.read()
            runner = scenario_session.runner
            if key not in {"q", "Q"}:
                quit_armed = False
            if key == "\x03":
                break
            if key in {"q", "Q"}:
                # Confirm quit once so an accidental key cannot end the session.
                if runner is not None and not quit_armed:
                    quit_armed = True
                    message = "Naciśnij q ponownie, aby zakończyć sesję"
                else:
                    break
            elif runner is not None and not runner.paused:
                if key in {"\r", "\n", " ", "y", "Y"}:
                    confirm("done")
                elif key in {"x", "X"}:
                    confirm("failed")
                elif key in {"p", "P"}:
                    runner.pause()
                    message = "Zatrzymane; naciśnij r, aby wznowić"
                elif key in {"u", "U"}:
                    undo()
                elif key in {"n", "N"}:
                    add_note()
            elif runner is not None and runner.paused:
                if key in {"r", "R"}:
                    runner.resume()
                    message = "Wznowione"
                elif key in {"u", "U"}:
                    undo()
                elif key in {"n", "N"}:
                    add_note()
            if key in {"s", "S"} and (runner is None or runner.paused):
                if scenario_session.total():
                    listing = "\n".join(
                        f"{i}. [{item.group}] {item.title} ({item.vehicle}, {item.total_steps} steps)"
                        for i, item in enumerate(scenario_session.order, 1))
                    answer = keyboard.read_line(f"Choose scenario:\n{listing}\nNumber or id: ").strip()
                    choice = None
                    if answer.isdigit() and 1 <= int(answer) <= len(scenario_session.order):
                        choice = scenario_session.order[int(answer) - 1].id
                    elif answer in scenarios:
                        choice = answer
                    if choice:
                        start_scenario(choice)
                    else:
                        message = "Nie wybrano scenariusza"
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
                    dashboard = render_dashboard(
                        mode="OFFLINE PREVIEW" if preview else "LIVE CAPTURE", elapsed=elapsed,
                        counts=counts, latest=latest_frames,
                        width=shutil.get_terminal_size((100, 24)).columns, prompt=message,
                        scenario=scenario_panel(), color=color, caps=caps)
                    print("\x1b[2J\x1b[H" + dashboard, end="\x1b[J", flush=True)
                else:
                    print(f"\r{'PODGLĄD' if preview else 'NAGRYWANIE'} {elapsed:.0f}s {line}    ", end="", flush=True)
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
            "context": dict(context or {}),
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
            print("\nPodgląd offline zakończony. Ramki demo i znaczniki ćwiczeniowe zostały odrzucone; sesja nie została zapisana.")
        else:
            print(f"\nPodsumowanie: scenariusze ukończone {scenario_session.completed()}/{scenario_session.total()}; "
                  f"potwierdzenia w tej sesji: {done_count} zrobione, {failed_count} nieudane; czas {duration:.0f}s")
            print(f"Sesja {'nieudana' if errors else 'zakończona'}: {directory}")
            print(json.dumps(stats, indent=2))
    return 2 if errors else 0

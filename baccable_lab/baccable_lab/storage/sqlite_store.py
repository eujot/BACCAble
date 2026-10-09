"""SQLite persistence for a capture session."""

from __future__ import annotations

import json
import sqlite3
from datetime import datetime, timezone
from pathlib import Path

from baccable_lab.can.baccable_binary import CanRecord
from baccable_lab.storage.schema import SCHEMA


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="milliseconds")


class SessionStore:
    def __init__(self, directory: Path, session_id: str, roles: list[str]):
        self.directory = directory
        self.session_id = session_id
        self.roles = roles
        self.database = sqlite3.connect(directory / "session.sqlite3")
        try:
            self.database.executescript(SCHEMA)
            self.database.execute(
                "INSERT INTO sessions(id, started_at, status, host_clock) VALUES (?, ?, ?, ?)",
                (session_id, utc_now(), "recording", "monotonic_ns"),
            )
            self.database.commit()
            self._sequence = {role: 0 for role in roles}
            self._loss_sequence = {role: 0 for role in roles}
        except BaseException:
            self.close()
            raise

    def add_record(self, record: CanRecord, raw_offset: int) -> None:
        if record.is_loss:
            sequence = self._loss_sequence[record.role]
            self._loss_sequence[record.role] += 1
            self.database.execute(
                "INSERT INTO capture_loss(session_id, role, sequence, host_ns, device_timestamp_raw, "
                "device_timestamp_ms, dropped_count, raw_offset) VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
                (self.session_id, record.role, sequence, record.host_ns, record.raw_timestamp,
                 record.device_timestamp_ms, record.dropped_count, raw_offset),
            )
            return
        sequence = self._sequence[record.role]
        self._sequence[record.role] += 1
        self.database.execute(
            "INSERT INTO can_frames(session_id, role, sequence, host_ns, device_timestamp_raw, "
            "device_timestamp_ms, arbitration_id, dlc, data, raw_offset) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
            (self.session_id, record.role, sequence, record.host_ns, record.raw_timestamp,
             record.device_timestamp_ms, record.arbitration_id, record.dlc, record.data, raw_offset),
        )

    def add_event(self, host_ns: int, label: str, source: str = "keyboard", note: str = "") -> int:
        cursor = self.database.execute(
            "INSERT INTO events(session_id, host_ns, label, source, note) VALUES (?, ?, ?, ?, ?)",
            (self.session_id, host_ns, label, source, note),
        )
        return int(cursor.lastrowid)

    def add_scenario_step(self, host_ns: int, event_id: int, scenario: str, step_index: int,
                          step_key: str, expect: str, run: int, outcome: str, note: str = "") -> int:
        """Record one confirmed scenario step next to its timeline marker."""
        cursor = self.database.execute(
            "INSERT INTO scenario_steps(session_id, host_ns, event_id, scenario, step_index, step_key, "
            "expect, run, outcome, note) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
            (self.session_id, host_ns, event_id, scenario, step_index, step_key, expect, run, outcome, note),
        )
        return int(cursor.lastrowid)

    def delete_scenario_step(self, event_id: int) -> None:
        self.database.execute("DELETE FROM scenario_steps WHERE session_id=? AND event_id=?",
                              (self.session_id, event_id))

    def update_scenario_note(self, event_id: int, note: str) -> None:
        self.database.execute("UPDATE scenario_steps SET note=? WHERE session_id=? AND event_id=?",
                              (note, self.session_id, event_id))

    def update_event_note(self, event_id: int, note: str) -> None:
        self.database.execute("UPDATE events SET note=? WHERE id=? AND session_id=?",
                              (note, event_id, self.session_id))

    def delete_event(self, event_id: int) -> None:
        self.database.execute("DELETE FROM events WHERE id=? AND session_id=?",
                              (event_id, self.session_id))

    def finish(self, status: str, duration_seconds: float, stats: dict) -> None:
        try:
            self.database.execute(
                "UPDATE sessions SET ended_at=?, duration_seconds=?, status=? WHERE id=?",
                (utc_now(), duration_seconds, status, self.session_id),
            )
            self.database.commit()
            (self.directory / "summary.json").write_text(json.dumps(stats, indent=2) + "\n")
        finally:
            self.close()

    def close(self) -> None:
        self.database.close()

    def commit(self) -> None:
        self.database.commit()

    def counts(self) -> dict[str, dict[str, int]]:
        result = {}
        for role in self.roles:
            frames = self.database.execute(
                "SELECT count(*) FROM can_frames WHERE session_id=? AND role=?",
                (self.session_id, role),
            ).fetchone()[0]
            losses = self.database.execute(
                "SELECT coalesce(sum(dropped_count), 0) FROM capture_loss WHERE session_id=? AND role=?",
                (self.session_id, role),
            ).fetchone()[0]
            result[role] = {"frames": frames, "dropped": losses}
        return result


def list_sessions(root: Path) -> list[Path]:
    return sorted((path for path in root.iterdir() if path.is_dir() and (path / "session.sqlite3").exists()), reverse=True)


def resolve_session(root: Path, value: str) -> Path:
    exact = root / value
    if exact.is_dir():
        return exact
    candidates = [path for path in list_sessions(root) if path.name.startswith(value)]
    if len(candidates) != 1:
        raise ValueError(f"session is not unique or does not exist: {value}")
    return candidates[0]

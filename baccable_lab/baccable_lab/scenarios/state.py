"""Persistent guided-scenario progress so a run resumes after a restart.

The progress file records how many confirmations each scenario already handled.
The next launch continues from the first unfinished scenario and step instead of
starting again from the beginning.
"""

from __future__ import annotations

import json
from datetime import datetime, timezone
from pathlib import Path
import tempfile

SCHEMA_VERSION = 1


def _now() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="seconds")


class ProgressStore:
    """Small JSON-backed progress tracker. ``path=None`` keeps it in memory."""

    def __init__(self, path: Path | None = None):
        self.path = Path(path) if path is not None else None
        self.data: dict = {"schema_version": SCHEMA_VERSION, "scenarios": {}}
        if self.path is not None and self.path.exists():
            try:
                loaded = json.loads(self.path.read_text(encoding="utf-8"))
            except (OSError, ValueError):
                loaded = None
            if isinstance(loaded, dict) and isinstance(loaded.get("scenarios"), dict):
                self.data = loaded
                self.data.setdefault("schema_version", SCHEMA_VERSION)
        self.data.setdefault("scenarios", {})

    def handled(self, scenario_id: str) -> int:
        return int(self.data["scenarios"].get(scenario_id, {}).get("handled", 0))

    def complete(self, scenario_id: str, total: int) -> bool:
        return self.handled(scenario_id) >= total

    def completed_count(self, order) -> int:
        return sum(1 for scenario in order if self.complete(scenario.id, scenario.total_steps))

    def record(self, scenario_id: str, handled: int, total: int, outcome: str) -> None:
        entry = self.data["scenarios"].setdefault(
            scenario_id, {"handled": 0, "done": 0, "failed": 0})
        entry["handled"] = handled
        entry["total"] = total
        key = "done" if outcome == "done" else "failed"
        entry[key] = entry.get(key, 0) + 1
        entry["updated_at"] = _now()
        self._save()

    def reset(self, scenario_id: str) -> None:
        self.data["scenarios"].pop(scenario_id, None)
        self._save()

    def set_handled(self, scenario_id: str, handled: int, total: int) -> None:
        entry = self.data["scenarios"].setdefault(
            scenario_id, {"handled": 0, "done": 0, "failed": 0})
        entry["handled"] = max(0, handled)
        entry["total"] = total
        entry["updated_at"] = _now()
        self._save()

    def _save(self) -> None:
        if self.path is None:
            return
        self.data["updated_at"] = _now()
        self.path.parent.mkdir(parents=True, exist_ok=True)
        payload = json.dumps(self.data, indent=2, sort_keys=True) + "\n"
        handle = tempfile.NamedTemporaryFile("w", encoding="utf-8", dir=self.path.parent,
                                             prefix=self.path.name + ".", suffix=".tmp", delete=False)
        try:
            handle.write(payload)
            handle.close()
            Path(handle.name).replace(self.path)
        except BaseException:
            Path(handle.name).unlink(missing_ok=True)
            raise

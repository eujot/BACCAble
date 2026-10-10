"""Pure scenario run/step state machine, independent of the capture loop."""

from __future__ import annotations

from .model import Scenario, Step

OUTCOMES = ("done", "failed", "skipped")


class ScenarioRunner:
    """Track position in a scenario and record confirmed step outcomes.

    The runner holds no I/O. ``current`` reports the pending step; ``done``,
    ``failed`` and ``skip`` record the outcome and advance. ``skip`` marks a step
    the operator deliberately does not perform (for example ABS or traction
    control); it still advances so the procedure is not blocked. Pausing keeps
    the position so the operator can resume and continue marking.
    """

    def __init__(self, scenario: Scenario, handled: int = 0):
        self.scenario = scenario
        self.handled = max(0, min(handled, scenario.total_steps))
        steps = len(scenario.steps)
        self.run = self.handled // steps + 1
        self.index = self.handled % steps
        self.paused = False
        self.finished = self.handled >= scenario.total_steps
        self.records: list[tuple[int, int, Step, str]] = []

    def current(self) -> tuple[int, int, Step] | None:
        if self.finished or self.paused:
            return None
        return self.run, self.index, self.scenario.steps[self.index]

    def record(self, outcome: str) -> tuple[int, int, Step] | None:
        if outcome not in OUTCOMES:
            raise ValueError(f"unknown outcome: {outcome}")
        return self._record(outcome)

    def done(self) -> tuple[int, int, Step] | None:
        return self._record("done")

    def failed(self) -> tuple[int, int, Step] | None:
        return self._record("failed")

    def skip(self) -> tuple[int, int, Step] | None:
        return self._record("skipped")

    def _record(self, outcome: str) -> tuple[int, int, Step] | None:
        pending = self.current()
        if pending is None:
            return None
        run, index, step = pending
        self.records.append((run, index, step, outcome))
        self.handled += 1
        self._advance()
        return pending

    def _advance(self) -> None:
        self.index += 1
        if self.index < len(self.scenario.steps):
            return
        self.index = 0
        self.run += 1
        if self.run > self.scenario.runs:
            self.finished = True

    def pause(self) -> None:
        if not self.finished:
            self.paused = True

    def resume(self) -> None:
        if not self.finished:
            self.paused = False

    def progress(self) -> tuple[int, int]:
        """Handled confirmations and the total expected for the whole scenario."""
        return self.handled, self.scenario.total_steps

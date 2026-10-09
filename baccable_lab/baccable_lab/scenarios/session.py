"""Ordered, resumable scenario queue with automatic advance to the next one."""

from __future__ import annotations

from .model import order_scenarios
from .runner import ScenarioRunner
from .state import ProgressStore


class ScenarioSession:
    """Run scenarios in functional order, resuming from stored progress.

    ``start`` begins the first unfinished scenario (or an explicitly requested
    one). ``advance`` is called after a scenario finishes and moves to the next
    unfinished scenario, so the operator never repeats completed work.
    """

    def __init__(self, scenarios, progress: ProgressStore):
        self.order = order_scenarios(scenarios)
        self.progress = progress
        self.runner: ScenarioRunner | None = None
        self.position = -1
        self.just_finished = None

    def start(self, start_id: str | None = None) -> ScenarioRunner | None:
        if not self.order:
            return None
        if start_id:
            for index, scenario in enumerate(self.order):
                if scenario.id == start_id:
                    handled = 0 if self.progress.complete(scenario.id, scenario.total_steps) \
                        else self.progress.handled(scenario.id)
                    return self._activate(index, handled)
            return None
        for index, scenario in enumerate(self.order):
            handled = self.progress.handled(scenario.id)
            if handled < scenario.total_steps:
                return self._activate(index, handled)
        return None

    def _activate(self, index: int, handled: int) -> ScenarioRunner:
        self.position = index
        self.runner = ScenarioRunner(self.order[index], handled=handled)
        return self.runner

    def advance(self) -> ScenarioRunner | None:
        self.just_finished = self.runner.scenario if self.runner else None
        for index in range(self.position + 1, len(self.order)):
            scenario = self.order[index]
            if self.progress.handled(scenario.id) < scenario.total_steps:
                return self._activate(index, self.progress.handled(scenario.id))
        self.runner = None
        return None

    def completed(self) -> int:
        return self.progress.completed_count(self.order)

    def total(self) -> int:
        return len(self.order)

    def current_number(self) -> int:
        return self.position + 1 if self.position >= 0 else 0

"""Guided capture scenarios: controlled in-car procedures that place structured markers.

A scenario is a small TOML procedure (see ``library/``) that the capture screen
displays step by step. The operator confirms each step as done or failed, may
stop the run, and may resume it later. Scenarios run in a functional order and
advance automatically; progress persists so the next launch continues from the
first unfinished step. Every confirmation writes a normal timeline marker plus
a structured ``scenario_steps`` row so the offline analysis can compare the
correct ON/OFF pairs with repetitions and negative controls.
"""

from .model import (Scenario, Step, default_directory, group_label, load_scenarios,
                    location_label, order_scenarios, parse_scenarios)
from .runner import ScenarioRunner
from .session import ScenarioSession
from .state import ProgressStore

__all__ = [
    "ProgressStore",
    "Scenario",
    "ScenarioRunner",
    "ScenarioSession",
    "Step",
    "default_directory",
    "group_label",
    "load_scenarios",
    "location_label",
    "order_scenarios",
    "parse_scenarios",
]

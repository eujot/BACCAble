"""Guided capture scenarios: controlled in-car procedures that place structured markers.

A scenario is a small TOML procedure (see ``library/``) that the capture screen
displays step by step. The operator confirms each step as done or failed, may
**skip** a step or a whole procedure it does not want to perform (for example ABS
or traction control), and may permanently **exclude** a procedure so it never
returns to the queue. It may stop the run and resume it later. Scenarios run in a
functional order and advance automatically; progress persists so the next launch
continues from the first unfinished step. A performed confirmation (done/failed)
writes a normal timeline marker plus a structured ``scenario_steps`` row so the
offline analysis can compare the correct ON/OFF pairs with repetitions and
negative controls; a skipped step writes only the structured row, never a marker,
so it is not mistaken for evidence that the physical action happened.
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

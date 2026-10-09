"""Scenario definitions and the dependency-free TOML loader."""

from __future__ import annotations

from dataclasses import dataclass
from importlib.resources import files
from pathlib import Path
import tomllib

from baccable_lab.events.catalog import TITLE_BY_LABEL

_VEHICLES = ("stationary", "driving", "any")

# Functional run order so a session never jumps, for example, from a gear test
# straight to opening the boot. Stationary work comes before driving work.
GROUP_ORDER = ("access", "lighting", "wipers", "climate", "media",
               "chassis", "brakes", "drivetrain", "parking", "drive", "adas")
GROUP_TITLE_PL = {
    "access": "Dostęp i drzwi",
    "lighting": "Oświetlenie",
    "wipers": "Wycieraczki i deszcz",
    "climate": "Klimatyzacja i komfort",
    "media": "Multimedia i wyświetlacz",
    "chassis": "Zawieszenie i kierownica",
    "brakes": "Hamulce i stabilność",
    "drivetrain": "Napęd i skrzynia",
    "parking": "Parkowanie",
    "drive": "Jazda na placu",
    "adas": "Wspomaganie jazdy",
}
_VEHICLE_RANK = {"stationary": 0, "any": 0, "driving": 1}
_SITES = ("", "parking", "road")


@dataclass(frozen=True)
class Step:
    """One confirmation asked of the operator during a scenario run."""

    action: str
    expect: str = ""
    key: str = ""

    @property
    def marker_label(self) -> str:
        # A catalog label when the step names a known state, otherwise the raw
        # action text. The analysis only trusts labels that match the catalog.
        return self.expect or self.action


@dataclass(frozen=True)
class Scenario:
    id: str
    title: str
    group: str
    vehicle: str
    runs: int
    min_separation_s: float
    confounder_guard: tuple[str, ...]
    notes: str
    steps: tuple[Step, ...]
    order: int = 0
    site: str = ""

    @property
    def total_steps(self) -> int:
        return self.runs * len(self.steps)


def parse_scenarios(payload: bytes | str, source: str = "<memory>") -> list[Scenario]:
    """Parse one TOML document that may hold several ``[[scenario]]`` tables."""
    text = payload.decode("utf-8") if isinstance(payload, bytes) else payload
    try:
        table = tomllib.loads(text)
    except tomllib.TOMLDecodeError as exc:
        raise ValueError(f"invalid scenario file {source}: {exc}") from exc
    raw = table.get("scenario", [])
    if isinstance(raw, dict):
        raw = [raw]
    if not isinstance(raw, list):
        raise ValueError(f"invalid scenario file {source}: 'scenario' must be a table or array")
    return [_build(item, source) for item in raw]


def _build(item: dict, source: str) -> Scenario:
    scenario_id = _text(item, "id", source)
    steps: list[Step] = []
    for position, step in enumerate(item.get("steps", []), 1):
        where = f"{source}:{scenario_id} step {position}"
        steps.append(Step(action=_text(step, "action", where),
                          expect=str(step.get("expect", "")).strip(),
                          key=str(step.get("key", "")).strip()))
    scenario = Scenario(
        id=scenario_id,
        title=str(item.get("title", scenario_id)).strip() or scenario_id,
        group=str(item.get("group", "")).strip(),
        vehicle=str(item.get("vehicle", "stationary")).strip(),
        runs=int(item.get("runs", 1)),
        min_separation_s=float(item.get("min_separation_s", 3.0)),
        confounder_guard=tuple(str(name) for name in item.get("confounder_guard", [])),
        notes=str(item.get("notes", "")).strip(),
        steps=tuple(steps),
        order=int(item.get("order", 0)),
        site=str(item.get("site", "")).strip(),
    )
    _validate(scenario)
    return scenario


def _text(item: dict, field: str, where: str) -> str:
    value = str(item.get(field, "")).strip()
    if not value:
        raise ValueError(f"scenario entry missing {field!r}: {where}")
    return value


def _validate(scenario: Scenario) -> None:
    if scenario.vehicle not in _VEHICLES:
        raise ValueError(f"scenario {scenario.id}: vehicle must be one of {_VEHICLES}")
    if scenario.site not in _SITES:
        raise ValueError(f"scenario {scenario.id}: site must be one of {_SITES}")
    if scenario.runs < 1:
        raise ValueError(f"scenario {scenario.id}: runs must be >= 1")
    if scenario.min_separation_s < 0:
        raise ValueError(f"scenario {scenario.id}: min_separation_s must not be negative")
    if not scenario.steps:
        raise ValueError(f"scenario {scenario.id}: at least one step is required")
    for step in scenario.steps:
        # An empty expect is allowed (a baseline boundary); a named one must be a
        # real catalog label so it can later be correlated and rendered.
        if step.expect and step.expect not in TITLE_BY_LABEL:
            raise ValueError(f"scenario {scenario.id}: unknown marker label {step.expect!r}")


def default_directory() -> Path:
    """Package-bundled scenario library, usable after an editable install."""
    return Path(str(files("baccable_lab.scenarios").joinpath("library")))


def _group_rank(group: str) -> int:
    return GROUP_ORDER.index(group) if group in GROUP_ORDER else len(GROUP_ORDER)


def location_label(scenario: Scenario) -> str:
    """Polish location wording shown next to the scenario on screen and in lists."""
    if scenario.vehicle == "stationary":
        return "postój"
    if scenario.vehicle == "any":
        return "dowolnie"
    if scenario.site == "parking":
        return "jazda – plac"
    if scenario.site == "road":
        return "jazda – droga"
    return "jazda"


def group_label(group: str) -> str:
    """Polish section title for a scenario group."""
    return GROUP_TITLE_PL.get(group, group or "ogólne")


def order_scenarios(scenarios) -> list[Scenario]:
    """Stationary first, then functional group order, then explicit order and id."""
    return sorted(scenarios,
                  key=lambda s: (_VEHICLE_RANK.get(s.vehicle, 1), _group_rank(s.group), s.order, s.id))


def load_scenarios(directory) -> dict[str, Scenario]:
    """Load every ``*.toml`` in *directory*, keyed by unique scenario id."""
    path = Path(directory)
    if not path.is_dir():
        raise ValueError(f"scenarios directory not found: {path}")
    result: dict[str, Scenario] = {}
    for file in sorted(path.glob("*.toml")):
        for scenario in parse_scenarios(file.read_bytes(), str(file)):
            if scenario.id in result:
                raise ValueError(f"duplicate scenario id: {scenario.id}")
            result[scenario.id] = scenario
    return result

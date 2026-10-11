"""Dependency-free, scenario-first live capture dashboard with ANSI colours.

The live screen shows one guided procedure step at a time, large and prominent,
so the operator always knows what to do next. Colour, an optional upper-case
action, a next-step preview and a progress bar improve readability from inside
the car. The manual marker palette is deliberately absent here: guided scenarios
are the primary workflow and naming evidence comes from their confirmed steps.
"""

from __future__ import annotations

RESET = "\x1b[0m"
BOLD = "\x1b[1m"
DIM = "\x1b[2m"
RED = "\x1b[31m"
GREEN = "\x1b[32m"
YELLOW = "\x1b[33m"
MAGENTA = "\x1b[35m"
CYAN = "\x1b[36m"
GREY = "\x1b[90m"
# Bold black on a bright-yellow bar: the most visible emphasis for the action.
_ACTION = "\x1b[1;30;103m"
_BORDER = CYAN


def _clip(value: str, width: int) -> str:
    if width <= 1:
        return value[:width]
    return value if len(value) <= width else value[: width - 1] + "…"


def _paint(text: str, style: str | None, color: bool) -> str:
    return style + text + RESET if (color and style) else text


def _box(lines: list[tuple[str, str | None]], width: int, color: bool) -> list[str]:
    """Draw a box. Each content line is (plain text, ANSI style or None)."""
    inner = max(8, width - 4)
    rows = [_paint("╔" + "═" * (width - 2) + "╗", _BORDER, color)]
    for text, style in lines:
        clipped = _clip(text, inner)
        padded = clipped + " " * (inner - len(clipped))
        rows.append(_paint("║", _BORDER, color) + " " + _paint(padded, style, color)
                    + " " + _paint("║", _BORDER, color))
    rows.append(_paint("╚" + "═" * (width - 2) + "╝", _BORDER, color))
    return rows


def _bar(handled: int, total: int, size: int = 24) -> str:
    if total <= 0:
        return ""
    filled = max(0, min(size, round(size * handled / total)))
    return "█" * filled + "░" * (size - filled)


def _message_style(message: str) -> str:
    low = message.lower()
    if low.startswith(("zrobione", "ukończono", "wznowione", "cofnięto", "start", "wznów")):
        return GREEN
    if low.startswith(("nieudane", "brak")):
        return RED
    if low.startswith(("pominięte", "pominięto", "wykluczono", "zatrzym")):
        return YELLOW
    return GREY


def render_dashboard(
    *, mode: str, elapsed: float, counts: dict[str, dict[str, int]],
    latest: dict[str, tuple[float, int, bytes]], width: int = 100,
    prompt: str = "", scenario: dict | None = None, color: bool = True, caps: bool = False,
) -> str:
    """Render a terminal-sized snapshot. Pure function for regression tests."""
    width = max(width, 48)
    header = _clip(f"BACCAble Lab  |  {mode}  |  {elapsed:6.1f}s", width)
    if scenario and "total_scenarios" in scenario:
        suffix = f"  |  scenariusze {scenario['completed_scenarios']}/{scenario['total_scenarios']}"
        header = _clip(header, max(4, width - len(suffix))) + suffix
    separator = _paint("═" * width, _BORDER, color)
    rows = [_paint(header, CYAN, color), separator]
    rows.extend(_scenario_block(scenario, width, color, caps))
    rows.append(separator)
    for role in sorted(counts):
        frames = counts[role]["frames"]
        dropped = counts[role]["dropped"]
        frame = latest.get(role)
        if frame:
            at, can_id, data = frame
            description = f"  {role} @{at:.1f}s  0x{can_id:03X} [{len(data)}] {data.hex(' ').upper()}"
        else:
            description = f"  {role}  waiting for frames"
        line = _clip(f"{description}  |  {frames}f {dropped} lost", width)
        rows.append(_paint(line, GREEN if dropped == 0 else RED, color))
    rows.append(separator)
    if prompt:
        rows.append(_paint(_clip(prompt, width), GREY, color))
    return "\n".join(rows)


def _scenario_block(scenario: dict | None, width: int, color: bool, caps: bool) -> list[str]:
    if scenario is None or scenario.get("empty"):
        return _box([("Nie wczytano biblioteki scenariuszy. Naciśnij q, aby zakończyć.", None)], width, color)
    if scenario.get("complete") or "total_scenarios" not in scenario:
        completed = scenario.get("completed_scenarios", 0)
        total = scenario.get("total_scenarios", 0)
        text = f"WSZYSTKIE SCENARIUSZE UKOŃCZONE ({completed}/{total}). Naciśnij s, aby powtórzyć, q aby zakończyć."
        return _box([(text, GREEN + BOLD)], width, color)

    title = (f"SCENARIUSZ {scenario['number']}/{scenario['total_scenarios']} · "
             f"{scenario['title']}  [{scenario['location']}]")
    if scenario.get("paused"):
        pending = scenario.get("action", "")
        return _box([("ZATRZYMANE — naciśnij r, aby wznowić, p pozostawia zatrzymane.", BOLD + YELLOW),
                     (f"następnie: {pending}", GREY),
                     ("[r] wznów   [u] cofnij   [n] notatka   [s] wybierz   [q] zakończ", None)], width, color)

    header = (f"KROK {scenario['step_index'] + 1}/{scenario['steps_in_run']} · "
              f"powtórzenie {scenario['run']}/{scenario['runs']}")
    body: list[tuple[str, str | None]] = []
    if scenario.get("group"):
        body.append((f"— {scenario['group']} —", BOLD))
    body.append((title, None))
    body.append((header, CYAN))
    if "handled" in scenario and "total" in scenario:
        body.append((f"postęp: {scenario['handled']}/{scenario['total']}  {_bar(scenario['handled'], scenario['total'])}",
                     GREEN))
    action = scenario["action"].upper() if caps else scenario["action"]
    body.append((f"WYKONAJ:  {action}", _ACTION))
    if scenario.get("expected"):
        body.append((f"oczekiwany stan:  {scenario['expected']}", MAGENTA))
    if scenario.get("next_action"):
        body.append((f"dalej:  {scenario['next_action']}", GREY))
    lines = _box(body, width, color)
    lines.append(_clip("  [ENTER/SPACJA] zrobione   [x] nieudane   [k] pomiń krok   [p] zatrzymaj", width))
    lines.append(_clip("  [o] pomiń scenariusz   [e] wyklucz   [r] wznów   [u] cofnij   [n] notatka   [s] wybierz   [q] zakończ", width))
    if scenario.get("guard"):
        lines.append(_paint(_clip("  nie zmieniaj: " + ", ".join(scenario["guard"]), width), GREY, color))
    lines.append(_paint(_clip(f"  odstęp między akcjami >= {scenario.get('min_separation_s', 3):g}s", width), GREY, color))
    if scenario.get("message"):
        lines.append(_paint(_clip("  " + scenario["message"], width), _message_style(scenario["message"]), color))
    return lines

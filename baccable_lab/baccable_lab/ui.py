"""Dependency-free live capture dashboard and marker palette."""

from __future__ import annotations

import textwrap

from baccable_lab.events.catalog import EVENT_GROUPS, GROUP_BY_LABEL, QUICK_MARKERS, label_text


def _clip(value: str, width: int) -> str:
    if width <= 1:
        return value[:width]
    return value if len(value) <= width else value[: width - 1] + "…"


def render_dashboard(
    *, mode: str, elapsed: float, counts: dict[str, dict[str, int]],
    latest: dict[str, tuple[float, int, bytes]], timeline: list[tuple[float, str, str]],
    width: int = 100, prompt: str = "", selected_group: str | None = None,
) -> str:
    """Render a terminal-sized snapshot. Pure function for regression tests."""
    width = max(width, 48)
    line = "─" * width
    rows = [
        f"BACCAble Lab  |  {mode}  |  {elapsed:6.1f}s  |  {len(timeline)} markers",
        line,
        "LIVE CAN PREVIEW  (simulated frames are marked DEMO; no vehicle is connected)" if mode == "OFFLINE PREVIEW"
        else "LIVE CAN PREVIEW  (raw frames only; no signal decoding)",
    ]
    for role in sorted(counts):
        frames = counts[role]["frames"]
        dropped = counts[role]["dropped"]
        frame = latest.get(role)
        if frame:
            at, can_id, data = frame
            description = f"{role} @{at:.1f}s  0x{can_id:03X} [{len(data)}] {data.hex(' ').upper()}"
        else:
            description = f"{role}  waiting for frames"
        rows.append(_clip(f"  {description}  |  {frames}f {dropped} lost", width))
    rows.extend((line, "EVENT TIMELINE  (newest last)"))
    if timeline:
        for at, label, note in timeline[-4:]:
            group = GROUP_BY_LABEL.get(label, "Custom")
            text = f"  {at:7.2f}s  {group:<18} {label_text(label)}"
            if note:
                text += f" — {note}"
            rows.append(_clip(text, width))
    else:
        rows.append("  No markers yet. Press a quick key or select a grouped marker with g.")
    rows.extend((line, "MARKER GROUPS  (press g, then choose a group and item)"))
    if selected_group is None:
        overview = "  " + "  |  ".join(
            f"[{key}] {title} ({len(entries)})" for key, (title, entries) in EVENT_GROUPS.items())
        rows.extend("  " + part for part in textwrap.wrap(overview.strip(), width=max(20, width - 4)))
    elif selected_group == "":
        rows.append("  Choose one group key:")
        choices = "  " + "  |  ".join(
            f"[{key}] {title}" for key, (title, _) in EVENT_GROUPS.items())
        rows.extend("  " + part for part in textwrap.wrap(
            choices.strip(), width=max(20, width - 4), break_long_words=False))
    else:
        title, entries = EVENT_GROUPS[selected_group]
        rows.append(f"  {title}: press an item number (0 selects item 10), or Esc to cancel")
        options = "   ".join(f"{index if index < 10 else 0}. {label_text(label)}"
                              for index, (label, _) in enumerate(entries, 1))
        rows.extend("  " + part for part in textwrap.wrap(options, width=max(20, width - 4),
                                                           break_long_words=False))
    rows.extend((line, "QUICK MARKERS  (press the key to timestamp the action)"))
    quick = "  " + "  |  ".join(f"[{key}] {label_text(label)}" for key, label in QUICK_MARKERS.items())
    rows.extend("  " + part for part in textwrap.wrap(quick.strip(), width=max(20, width - 4),
                                                       break_long_words=False))
    rows.extend(textwrap.wrap("ACTIONS  [f] find/type marker  |  [g] grouped marker  |  [l] Lock  |  [c] custom  |  [n] note  |  [u] undo  |  [m/?] help  |  [q] finish",
                              width=max(20, width - 2)))
    if prompt:
        rows.append(_clip(prompt, width))
    return "\n".join(_clip(row, width) for row in rows)


def marker_help() -> str:
    return ("Markers: 1-0 quick actions (shown on screen); f=find a catalog marker or add typed text; "
            "g then group key then item number; c=custom, n=note, u=undo, q=finish")

# Agent starting point

Read [docs/ACTION_PLAN.md](docs/ACTION_PLAN.md) before planning work. It contains
the current backlog, priorities, integration/release status, known local changes
and hardware acceptance gaps. Inspect the actual Git state before relying on its
dated baseline.

Before compiling or testing on this Mac, read the local environment and command
sections of [firmware/baccable/MAKEFILE.md](firmware/baccable/MAKEFILE.md). Select
the documented full ARM toolchain explicitly; the default Homebrew compiler lacks
required embedded C libraries. Host tests use native Apple Clang.

Maintain that single action plan after completing relevant work. Do not create
parallel backlogs or reconstruct old chat sessions when this handoff is sufficient.
Preserve unrelated local edits. Keep project documentation, menu labels and code
comments in English, except for the maintained Polish user guide described below.
Read the linked technical guide for the area being changed.

## Keep both user guides current

Whenever a change affects menu labels or structure, navigation, displayed
messages, settings, readings, actions, or how the device is operated (including
USB capture and diagnostics), update the affected sections of **both**
[the English user guide](manuals/BACCAble_USER_GUIDE_EN.md) and
[the Polish user guide](manuals/BACCAble_USER_GUIDE_PL.md) in the same change.
This applies to additions, removals, renames and behavior or prerequisite changes.
Check the actual code when a description is unclear; do not copy outdated behavior
from the legacy PDF/DOCX manuals.

Keep the two guides equivalent in scope and meaning. Use plain Polish in the
Polish edition while retaining the exact English on-screen labels, messages and
command syntax. Check menu paths, ordering, units, ranges, conditions, examples
and links affected by the change. Update version/scope statements when needed,
without claiming hardware verification that was not performed. Internal changes
with no user-visible effect do not require artificial manual edits.

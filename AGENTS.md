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

For CAN capture analysis or changes to the signal dictionary, read
[the session analysis workflow](baccable_lab/docs/SESSION_ANALYSIS.md).
Load the existing dictionary and research hypotheses before interpreting new
recordings. Preserve original dumps and markers; retain evidence, contradictions
and provenance when updating knowledge. Refresh the readable CAN catalog with
the machine-readable dictionary so subsequent sessions reuse the same meanings.

## Keep the checkout ready for pulls

Update `docs/ACTION_PLAN.md` in the same task branch or isolated worktree as
the implementation. Include the handoff update in that branch's commits and
PR; do not leave agent-authored handoff changes uncommitted in the user's
`master` checkout or copy them back there after opening a PR. Documentation-only
handoff work also belongs on a committed task branch. Inspect `git status` at
completion and report any pre-existing edits that still remain.

If local changes block a pull, inspect them and preserve their full contents
in a named Git backup before restoring any tracked file. Reconcile useful
handoff entries on the task branch, then fast-forward the user's clean checkout.
Do not use `assume-unchanged`, `skip-worktree`, destructive resets, or automatic
stash configuration to hide the issue. Preserve unrelated edits and existing
stashes. This rule concerns changes made by the agent; it does not authorize
discarding the user's work.

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

# Capture screen and acceptance

To practise without a BACCAble or vehicle, run `baccable preview`. The preview
uses synthetic frames, marks them as demo traffic, opens no serial ports, and
deletes its temporary session on exit. Optional `--role C1`, `--role C2`, and
`--role BH` arguments select the simulated buses; by default all three appear.

During a real capture or preview, the terminal dashboard shows the current guided
scenario step in a large box, its expected state, the repetition counter and
progress, the confounder guard, and the live raw CAN frames with per-bus
frame/loss totals. The manual marker palette is deliberately not shown: only a
confirmed scenario step writes a marker (see [SCENARIOS.md](SCENARIOS.md)).
Confirm with `ENTER`/`SPACJA`/`y`, mark a step failed with `x`, skip a step with
`k`, skip the rest of the procedure with `o`, retire it with `e`, pause with `p`,
resume with `r`, undo with `u`, annotate the last step with `n`, jump to another
scenario with `s`, and finish with `q` or `Ctrl-C` (both work during a scenario).
See [the marker catalog and CAN limits](MARKERS.md).

All event labels are manual observations. For example, `Rain detected`,
`Suspension active`, `IPC menu overwritten`, and `A/C on` are not automatically
inferred from CAN. The live preview deliberately shows raw frame data and does
not guess signal meanings.

## Capture acceptance

Before starting, record the firmware release, board identities and port mapping.
Run a stationary 20–30 minute session. The scenario screen is the primary control
surface: `ENTER`/`SPACJA`/`y` confirms the current step, `x` marks it failed,
`k`/`o`/`e` skip or retire work you will not perform, `p`/`r` pause and resume,
`n` adds a note, and `q` or `Ctrl-C` stops cleanly. See
[SCENARIOS.md](SCENARIOS.md) for the complete control list.

Shutdown stops further serial reads and drains queued data plus the final read
before closing raw files and SQLite. This does not drain the device/OS USB buffers.
On storage failure, pending delivery is aborted to release readers and ports;
the session fails rather than claiming a complete recording. Cleanup attempts
all opened resources even if another resource fails to close. Summary output is
best effort when the disk itself is failing. A reader that exceeds the shutdown
timeout is explicitly reported as a failure, not silently treated as stopped.

For a selected-bus capture, acceptance requires a non-empty `<ROLE>.bin` and parsed
SQLite rows for each selected role, host timestamps, a session summary, and an explicit
loss count. A non-zero loss count is not hidden: investigate it before calling
the capture accepted. The first two seconds are reported separately as startup
loss; later loss is reported as in-session loss. Both remain in the raw and
SQLite capture. Reopen the database with `baccable session info` and use
`baccable export SESSION --basic` to inspect the result.

Unselected roles are absent from the manifest, raw files and session info;
selected roles with no traffic remain visible with zero frames. A one-bus
session is sufficient for a targeted investigation but does not satisfy the
Milestone 1 hardware acceptance requirement to verify all three buses.

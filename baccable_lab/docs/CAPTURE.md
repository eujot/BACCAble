# Capture screen and acceptance

To practise without a BACCAble or vehicle, run `baccable preview`. The preview
uses synthetic frames, marks them as demo traffic, opens no serial ports, and
deletes its temporary session on exit. Optional `--role C1`, `--role C2`, and
`--role BH` arguments select the simulated buses; by default all three appear.

During a real capture or preview, the terminal dashboard displays recent raw
CAN frames and IDs, per-bus frame/loss totals, and the four newest events on a
timestamped event timeline. Marker groups stay visible; selecting one displays
all of its available labels. The expanded catalog is grouped by vehicle, driving,
powertrain/transmission, engine/live readings, brakes/stability, driver
assistance/safety, body/access, IPC/media, suspension/chassis, weather/wipers,
lights, lighting/visibility, climate/comfort, parking/manoeuvring and
electrical/diagnostics. To place a
grouped marker, press `g`, the group key shown on screen, then its item number
(`0` selects item 10). Number keys `1`–`0` retain their existing quick markers,
and their exact actions are written on the screen. Press `f` and type an action
to search the catalog: one best match is recorded directly; with multiple
matches choose a number, or press Enter to add the typed phrase; no match adds
the phrase as a custom marker. `c` adds a custom label, `n` adds a note to the
most recent marker, `u` removes that marker, and `q` finishes the capture.
`m` or `?` shows the controls again. See [the marker catalog and CAN limits](MARKERS.md).

All event labels are manual observations. For example, `Rain detected`,
`Suspension active`, `IPC menu overwritten`, and `A/C on` are not automatically
inferred from CAN. The live preview deliberately shows raw frame data and does
not guess signal meanings.

## Capture acceptance

Before starting, record the firmware release, board identities and port mapping.
Run a stationary 20–30 minute session. The capture screen shows the marker
palette. Number keys and `l` retain quick markers for unlock, doors, engine
start, brakes, indicators and lock. Press `m` or `?` to show the palette again,
`c` to enter a custom label, `n` to add a note to the last marker, and `u` to
remove the last marker. Press `q` or Ctrl-C to stop cleanly.

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

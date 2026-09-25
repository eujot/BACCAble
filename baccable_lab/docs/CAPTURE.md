# Capture acceptance

Before starting, record the firmware release, board identities and port mapping.
Run a stationary 20–30 minute session. Use the default marker keys for unlock,
doors, engine start/stop, brake repetitions, indicators and lock. Press `q` or
Ctrl-C to stop cleanly.

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
the capture accepted. Reopen the database with `baccable session info` and use
`baccable export SESSION --basic` to inspect the result.

Unselected roles are absent from the manifest, raw files and session info;
selected roles with no traffic remain visible with zero frames. A one-bus
session is sufficient for a targeted investigation but does not satisfy the
Milestone 1 hardware acceptance requirement to verify all three buses.

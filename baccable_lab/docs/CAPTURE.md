# Capture acceptance

Before starting, record the firmware release, board identities and port mapping.
Run a stationary 20–30 minute session. Use the default marker keys for unlock,
doors, engine start/stop, brake repetitions, indicators and lock. Press `q` or
Ctrl-C to stop cleanly.

Acceptance requires non-empty `C1.bin`, `C2.bin` and `BH.bin`, parsed SQLite
rows for all three roles, host timestamps, a session summary, and an explicit
loss count. A non-zero loss count is not hidden: investigate it before calling
the capture accepted. Reopen the database with `baccable session info` and use
`baccable export SESSION --basic` to inspect the result.

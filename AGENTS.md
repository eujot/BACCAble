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
comments in English. Read the linked technical guide for the area being changed.

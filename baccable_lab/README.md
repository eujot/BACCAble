# BACCAble Lab

Host-side recorder for one, two or three BACCAble binary CAN streams. Milestone 0 and the
software part of Milestone 1 are implemented; real hardware acceptance is still
required before adding OBD, replay UI, voice, or analysis milestones.

## One-command setup

From this directory on macOS:

```sh
python3 -m venv .venv && . .venv/bin/activate
python -m pip install -e '.[hardware,test]'
python -m pytest
```

The core parser and database work without third-party packages. `pyserial` is
needed only for real USB capture and `pytest` only for the test suite.

## First commands

```sh
source .venv/bin/activate
python -m baccable_lab.cli doctor
python -m baccable_lab.cli capture --port C1=/dev/cu.usbmodemXXXX \
  --port C2=/dev/cu.usbmodemYYYY --port BH=/dev/cu.usbmodemZZZZ \
  --no-obd --no-voice
python -m baccable_lab.cli sessions
python -m baccable_lab.cli session info SESSION_ID
python -m baccable_lab.cli export SESSION_ID --basic
```

To capture only C1 (C2 or BH can also be selected alone):

```sh
baccable capture --port C1=/dev/cu.usbmodemXXXX --no-obd --no-voice
```

Only supplied `--port` roles are opened, recorded and shown in session info.
Repeat `--port` to add another bus; each role must have a distinct device path.
The selected ports must already be configured in BACCAble as `USB mode: CAN`.
`doctor` never changes vehicle state. Capture stores each received byte stream
as `<ROLE>.bin` for each selected role, parses complete 16-byte records into
`session.sqlite3`, and preserves `0xAF` loss markers. Port paths are host-assigned
and must be remapped after reconnects.

Hardware setup and the acceptance checklist are in [docs/HARDWARE_SETUP.md](docs/HARDWARE_SETUP.md)
and [docs/CAPTURE.md](docs/CAPTURE.md). The parent project specification is
in [../docs/baccable_lab/PROJECT.md](../docs/baccable_lab/PROJECT.md).

# BACCAble Lab

Host-side recorder for one, two or three BACCAble binary CAN streams. Milestone 0 and the
software part of Milestone 1 are implemented. Offline session review and a
versioned CAN dictionary are available; the full hardware acceptance checklist
remains separate from those analysis results.

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
python -m baccable_lab.cli preview
python -m baccable_lab.cli capture --port C1=/dev/cu.usbmodemXXXX \
  --port C2=/dev/cu.usbmodemYYYY --port BH=/dev/cu.usbmodemZZZZ \
  --no-obd --no-voice
python -m baccable_lab.cli sessions
python -m baccable_lab.cli session info SESSION_ID
python -m baccable_lab.cli export SESSION_ID --basic
python -m baccable_lab.cli dictionary --role BH --id 0x46C
python -m baccable_lab.cli session decode SESSION_ID --role BH --id 0x46C --limit 20
python -m baccable_lab.cli scenarios
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

`preview` runs the same live screen without opening USB serial devices or
requiring a car. It generates clearly labelled synthetic C1/C2/BH frames and
discards the temporary session when you quit. Select roles with repeatable
`--role`, for example `baccable preview --role C1 --role BH`.

The live terminal screen shows recent raw CAN frames, frame/loss counts, and
the four newest event markers as a timeline. It keeps all marker groups visible
and shows a group's complete list when you select it. Press `f` and type an
action to find a catalog marker; if no marker matches, your text is saved as a
custom marker. When several markers match, choose one or press Enter to save
your text. The screen also spells out the stable quick-key mapping (`1`–`0`),
so the actions do not need to be memorized. Press `g`, a group key, then the
marker number to browse the full catalog. Press `c` for a custom label, `n` to
add a note to the latest marker, `u` to undo it, and `q` to finish. The complete
catalog, search terms, and CAN interpretation limits are in
[docs/MARKERS.md](docs/MARKERS.md). Markers are manual observations, not
automatic CAN signal detections; the live frame view shows raw identifiers and
bytes. The first two seconds are reported separately as startup loss so an
attachment backlog is visible without being confused with loss during the test.

## Guided scenarios

Guided scenarios are the primary capture workflow. The screen shows one step of
a controlled in-car procedure at a time as a large `WYKONAJ:` instruction, and
you confirm it with `ENTER`/`SPACJA` (zrobione), `x` (nieudane) or `k`
(pomiń ten krok); a stopped scenario resumes with `r`, `u` undoes the last
confirmation, `s` jumps to another procedure and `q` (or `Ctrl-C`) finishes.
A step you will not perform (for example provoking ABS, ESC or traction control)
is skipped with `k` and writes no marker, so it is not mistaken for evidence;
`o` skips the whole procedure and `e` skips it and stops scheduling it
permanently (`baccable scenarios --exclude ID` / `--include ID`).
On-screen wording is Polish and follows the terms used in the car; the stable
machine keys and the CAN dictionary labels stay English. Colour, a progress bar,
a next-step preview and a short bell make the screen readable and usable from
inside the car (`--caps`, `--no-color`/`NO_COLOR`, `--no-bell`). Procedures run
in a functional order (stationary first, then a parking-lot driving group, then
on-road assistance) and advance automatically to the next one. Progress is
stored in `config/scenario_state.json`, so starting Lab again continues from the
first unfinished step instead of repeating completed work. `baccable scenarios`
marks `[x]` done, `[~]` finished with skips and `[E]` excluded. The manual marker
palette is not shown while a scenario runs, keeping the screen readable. List
the procedures and their progress with `baccable scenarios`, jump to one with
`baccable capture --scenario ID`, or practise it with
`baccable preview --scenario ID`. Record run context with `--context KEY=VALUE`
(for example the vehicle, firmware or conditions) and hand one finished session
to an analysis with `baccable bundle SESSION --output FILE`, which writes a single
self-contained artifact (coverage, ranked per-action candidates and bounded raw
excerpts). See [docs/SCENARIOS.md](docs/SCENARIOS.md).

Hardware setup and the acceptance checklist are in [docs/HARDWARE_SETUP.md](docs/HARDWARE_SETUP.md)
and [docs/CAPTURE.md](docs/CAPTURE.md). The parent project specification is
in [../docs/baccable_lab/PROJECT.md](../docs/baccable_lab/PROJECT.md).

## Offline CAN knowledge

[CAN_DICTIONARY.md](docs/CAN_DICTIONARY.md) describes bus-specific fields,
diagnostic DIDs, code sources and observed coverage. The packaged JSON dictionary
keeps marker evidence separate from code comments and CAN-derived annotations.
`session decode` prints saved frames with known fields and an empirical stable-bit
comparison; it opens no serial port. Unknown identifiers and enum values stay raw.

```sh
baccable review --sessions sessions --output /tmp/baccable-review-NEW-DATE.json
```

Review reads finished recordings, preserves dumps, includes capture errors and
loss, compares manual markers, reconstructs IPC messages and proposes exploratory
bit steps. It refuses an existing output file and output inside the capture root.
Candidates require human review and independent controls before naming a function.
See [the findings](docs/research/20260928-findings.md) and the reusable
[agent analysis workflow](docs/SESSION_ANALYSIS.md) for adding new evidence and
using existing knowledge in the next recording.

## USB transition diagnostics (recovery candidate firmware)

Install the matching candidate C1/C2/BH set and update Lab from its source checkout.
In the existing Lab virtual environment:

```sh
python -m pip install -e '.[hardware,usb-status]'
baccable doctor --usb-status --samples 3
```

PyUSB requires libusb (on macOS, `brew install libusb` if no backend is installed).
The optional read-only EP0 request does not open CDC or consume capture records.
It reports local and C1-cached peer USB stages, mode acknowledgements, reset flags,
UART counters and main-loop snapshot time. Compare samples for progress and heed
`fresh: false` on old peer records. Older firmware can stall this unsupported
request; regular `baccable doctor` still works. No attached device or a failed
control read returns status 2. A successful read does not itself certify all
three capture buses; inspect `pending_ack`, freshness and configured states.
See the [USB audit](../docs/architecture/USB_DIAGNOSTICS.md#msc-to-cdc-failure-and-recovery-candidate-2026-09-27)
for field definitions and vehicle acceptance.

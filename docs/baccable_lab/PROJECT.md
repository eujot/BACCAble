# BACCAble Lab

## Project specification and implementation plan

**Primary goal:** start collecting trustworthy Alfa Romeo Giulia CAN data as quickly as possible, then progressively enrich the same acquisition system with event tagging, OBD reference data, video annotations, automated signal analysis, and an AI-driven experimental advisor.

**Target host:** MacBook Pro with Apple M2 running macOS.

**Vehicle interface:** BACCAble dedicated three-controller hardware using the current `eujot/BACCAble` firmware family, plus a Vgate vLinker MS connected through the second branch of a passive OBD-II splitter.

**Project directory:** `baccable_lab/`

## Current implementation status (2026-09-20)

- **Milestone 0 — complete.** The checkout, firmware capture format, USB
  assumptions and development setup were verified. The host project skeleton
  and one-command setup are present in `baccable_lab/`.
- **Milestone 1 — software ready for hardware acceptance.** The recorder,
  binary parser, raw stream preservation, SQLite session store, loss markers,
  common host timeline, keyboard markers, session inspection and CSV export are
  implemented and covered by host tests.
- **Hardware acceptance — pending.** The three BACCAble ports must still be
  connected to the actual Mac and vehicle for a meaningful stationary capture.
  This is the next action after the PR is merged. Do not mark Milestone 1
  complete until that run passes the checklist in section 14.5 and
  `baccable_lab/docs/CAPTURE.md`.
- **Later milestones — blocked.** OBD, replay UI, voice, video and analysis
  work must wait until hardware capture acceptance passes.

---

# 1. Project intent

This project is a research/data-acquisition environment for reverse engineering the CAN networks of an Alfa Romeo Giulia. The system must make it easy to:

1. capture all available CAN traffic from C1, C2, and BH at the same time;
2. preserve raw data losslessly and detect capture loss;
3. put all observations on one host-based timeline;
4. manually mark actions performed in the vehicle with one key, a large terminal menu, or later by voice;
5. collect OBD/diagnostic reference values from a vLinker MS on the same Mac;
6. attach external sources such as a camera recording after the session and add or refine timeline markers;
7. analyze repeated actions and automatically identify candidate CAN IDs, fields, counters, checksums, state bits, and physical signals;
8. maintain persistent hypotheses with evidence and confidence instead of treating early correlations as facts;
9. let a Codex/AI advisor participate during experiments, detect novelty, propose additional tests, ask for repetitions or negative controls, and exploit unexpected observations;
10. eventually produce a verified, incrementally maintained DBC or equivalent signal catalog for the specific vehicle.

The first iteration must be a **working capture product**, not a large framework that cannot yet record a car. Later features are layered on top of a stable acquisition core.

---

# 2. Scope and safety model

## 2.1 Initial scope

The initial scope is **passive observation and read-only diagnostics**.

The system shall:

- read C1/C2/BH CAN traffic;
- timestamp and store it;
- record event markers;
- query explicitly whitelisted OBD/diagnostic parameters through vLinker MS;
- analyze data offline and, later, near-live;
- suggest experiments to the user.

## 2.2 Out of scope for the initial project

Do not implement arbitrary CAN transmission as part of `baccable_lab` MVP.

Do not implement automatic:

- arbitrary CAN injection;
- actuator tests;
- ECU coding;
- configuration writes;
- UDS `SecurityAccess`;
- `WriteDataByIdentifier`;
- `RoutineControl`;
- ECU reset;
- DTC clearing;
- PROXI operations;
- anything whose purpose is to change safety-critical vehicle behavior.

These boundaries are intentional. Capture and analysis must be mature before any active vehicle testing is considered.

## 2.3 Important BACCAble capture caveat

The current integrated BACCAble `USB mode: CAN` is a capture/output mode, **not a guarantee that the firmware is electrically silent**. According to the current BACCAble USB documentation, normal vehicle feature processing continues during binary USB capture.

Therefore, before field capture:

1. inspect the firmware configuration actually flashed on C1/C2/BH;
2. disable transmit-capable BACCAble features that are not required for the experiment;
3. record the exact firmware commit/build identity and relevant settings in session metadata;
4. do not claim a capture is "passive" merely because USB mode is set to CAN;
5. if strict listen-only behavior is required, evaluate the standalone CAN/SLCAN firmware in `M1` silent mode as a separate deployment option and verify it on the actual hardware before use.

The host application must never send SLCAN transmit commands or arbitrary frames by default.

---

# 3. Verified BACCAble facts the implementation must respect

The coding agent must inspect the actual checkout before implementation. The statements below are verified against the current public `eujot/BACCAble` repository as of 2026-09-20, but source code remains authoritative.

## 3.1 Repository

- Repository: `https://github.com/eujot/BACCAble`
- The verified default branch is `master`, not `main`.
- The repository contains `AGENTS.md`.
- `AGENTS.md` says to read `docs/ACTION_PLAN.md` before planning firmware work and to inspect the actual Git state.
- The action plan is the firmware project's canonical backlog; `baccable_lab` is a separate host-side project and should not create competing firmware backlogs.

## 3.2 BACCAble CAN buses at the OBD connector

Current BACCAble documentation identifies:

- **C1**: OBD pins **6 / 14**
- **C2**: OBD pins **12 / 13**
- **BH**: OBD pins **3 / 11**

Current integrated USB diagnostics documentation states physical bus rates:

- **C1: 500 kbit/s**
- **C2: 500 kbit/s**
- **BH: 125 kbit/s**

Do not silently hard-code these as universal Alfa Romeo facts. Put them in the vehicle profile and verify against the checked-out BACCAble source and the actual car before capture.

## 3.3 Dedicated BACCAble USB port orientation

Current documentation states that, in the manual's orientation on the dedicated board:

- C1 is the right-hand USB port;
- C2 is the middle USB port;
- BH is the left-hand USB port.

Prefer PCB labels over visual orientation. Looking from the opposite side reverses left and right.

## 3.4 Integrated binary CAN capture

For the C1/C2/BH firmware set, select:

`Settings -> Features -> USB mode: CAN`

The current documented behavior is:

- ELM mode is disabled when CAN capture is selected;
- each bus can be connected separately to the host;
- multiple host connections may record different buses simultaneously;
- the host opens each `/dev/cu.usbmodem*` as a **binary stream**;
- it is **not SLCAN text**;
- the capture does not automatically create a CAN log on the USB disk;
- normal firmware feature processing continues during capture;
- each board uses its own local timestamp clock;
- capture can drop records if the host cannot keep up;
- capture-loss markers must be retained.

### Binary record format

Each normal capture record is exactly 16 bytes:

| Bytes | Meaning |
|---|---|
| `0` | `0xA0` through `0xA8`; low nibble is DLC 0..8 |
| `1..3` | 24-bit little-endian device timestamp in milliseconds |
| `4..7` | CAN identifier, little-endian |
| `8..15` | data bytes; unused bytes zero-filled |

A record starting with `0xAF` is a capture-loss record. Current documentation states that bytes `4..5` contain the dropped-frame count, capped at 65535.

The upstream binary record format does **not** explicitly record:

- standard versus extended ID flag;
- RTR frames.

The host must not invent information that is absent from the record.

### Timestamp wrap

The device timestamp is 24 bits in milliseconds and therefore wraps periodically. The parser must unwrap each device's clock independently and preserve both:

- the raw 24-bit value;
- a host-derived unwrapped device timeline.

## 3.5 macOS serial devices

BACCAble documentation says macOS enumerates these USB connections as host-assigned `/dev/cu.usbmodem*` devices.

Do not bind C1/C2/BH permanently to a path such as `/dev/cu.usbmodem101`; paths can change across reconnects. Device discovery must use USB identity and an explicit role-mapping workflow.

## 3.6 BACCAble ELM mode is not needed for this architecture

BACCAble also provides an ELM-compatible USB mode on C1, but it replaces CAN capture on that USB session and temporarily owns the buses for diagnostics. This project uses **vLinker MS as the diagnostic adapter**, therefore the BACCAble C1/C2/BH devices should remain in CAN capture mode during research sessions.

---

# 4. Physical architecture

## 4.1 Hardware available

- Alfa Romeo Giulia
- BACCAble dedicated board / three CAN controllers
- three data-capable USB cables from BACCAble
- powered or high-quality USB hub
- MacBook Pro M2
- passive OBD-II splitter, all relevant pins wired 1:1
- Vgate vLinker MS
- optional camera/phone for video recording
- optional microphone; the MacBook microphone is sufficient initially

## 4.2 Connection diagram

```text
                         ALFA ROMEO GIULIA
                               |
                             OBD-II
                               |
                    passive 16-pin splitter
                         /             \
                        /               \
                       /                 \
              BACCAble 3-CAN          vLinker MS
                   board                 |
             /       |       \           | Bluetooth LE
            /        |        \          |
          C1        C2        BH         |
       pins 6/14  12/13     3/11        |
          |         |         |          |
        USB       USB       USB          |
          \         |         /          |
           \        |        /           |
              USB HUB                       
                  |                         
                  +-----------+-------------+
                              |
                       MacBook Pro M2
                              |
                       baccable_lab
                              |
       +----------------------+----------------------+
       |                      |                      |
   CAN acquisition        OBD acquisition        event input
   C1 / C2 / BH            vLinker BLE          keyboard/voice
       |                      |                      |
       +----------------------+----------------------+
                              |
                       common host timeline
                              |
                    immutable raw + SQLite
                              |
              +---------------+----------------+
              |               |                |
          live status     later analysis   AI advisor
              |               |                |
              +---------------+----------------+
                              |
                    hypotheses / DBC / reports
```

## 4.3 Splitter rules

The OBD splitter must preserve all BACCAble-relevant lines, not only the standard 6/14 CAN pair. Verify continuity for at least:

- 3
- 6
- 11
- 12
- 13
- 14
- ground pins used by the adapters
- pin 16 power

Do not assume every cheap splitter wires all 16 pins.

## 4.4 One OBD diagnostic client at a time

During an integrated `baccable_lab` session, **Car Scanner should not remain connected to vLinker MS**.

The intended ownership is:

```text
vLinker MS -> baccable_lab OBD backend
```

Car Scanner remains useful for:

- confirming vLinker operation;
- exploring supported PIDs;
- validating values;
- checking custom Alfa Romeo/Giulia PID definitions.

But it should release the adapter before `baccable_lab` connects.

---

# 5. Core design principles

## 5.1 Acquisition first

The highest priority is not AI, voice, dashboards, or DBC generation. It is:

> **Never lose data because a non-critical feature is slow.**

CAN readers must remain independent of:

- UI rendering;
- OBD polling delays;
- speech recognition;
- AI/agent latency;
- database queries for analysis;
- video processing.

## 5.2 Preserve raw evidence

Every session must preserve the original BACCAble byte stream for C1/C2/BH.

Parsed database rows are derived data. If a parser bug is found later, the session must be recoverable from the raw stream without repeating the drive/test.

## 5.3 One host timeline

All host-side observations use the same Mac monotonic clock.

Every record should have:

- `host_monotonic_ns`: primary session timeline;
- `host_utc_ns`: wall-clock correlation/export;
- source-specific timestamp when available.

For CAN also preserve:

- raw 24-bit device timestamp;
- unwrapped device timestamp;
- host/device clock-fit metadata where applicable.

`host_monotonic_ns` is authoritative for integrating sources on the Mac. The device timestamp is valuable for accurate spacing within a BACCAble stream.

## 5.4 Separate acquisition from interpretation

Capture must store what was observed, not what the system believes it means.

Examples:

- store CAN ID and payload as observed;
- store OBD raw request/response as well as decoded value;
- store spoken transcript as well as canonical event label;
- store video-local time before mapping it to session time;
- store hypotheses separately from verified signals.

## 5.5 Evidence, not certainty by default

Automatic discoveries are hypotheses.

A candidate signal should move through states such as:

```text
candidate -> probable -> verified
          \-> rejected
```

Each transition must be supported by recorded evidence.

---

# 6. Technology choices

Preferred implementation language for the host application: **Python 3.12+** unless compatibility with an installed dependency requires another supported Python 3 version.

Recommended libraries/components:

| Purpose | Preferred tool |
|---|---|
| BACCAble USB serial | `pyserial` |
| Bluetooth LE | `bleak` |
| Terminal UI | `textual` |
| Rich console output | `rich` |
| Live storage | Python `sqlite3`, WAL mode |
| Offline tabular analysis | `polars` |
| SQL analytics/export | `duckdb` |
| Numerical analysis | `numpy` |
| Statistics/signal correlation | `scipy` |
| CAN/DBC utilities | `cantools` |
| Parquet | `pyarrow` or Polars native support |
| Configuration | YAML via `PyYAML` or `ruamel.yaml` |
| CLI | `typer` or `click` |
| Tests | `pytest` |
| Local voice later | pluggable backend, e.g. `whisper.cpp` or Apple-Silicon-appropriate local STT |

Avoid adding heavy dependencies to Milestone 1 unless they directly help stable CAN capture.

---

# 7. Proposed repository layout

```text
baccable_lab/
├── PROJECT.md                  # this document
├── README.md
├── pyproject.toml
├── uv.lock / requirements lock
├── .gitignore
│
├── baccable_lab/
│   ├── __init__.py
│   ├── cli.py
│   ├── app.py
│   ├── clock.py
│   ├── logging_config.py
│   │
│   ├── can/
│   │   ├── __init__.py
│   │   ├── model.py
│   │   ├── discover.py
│   │   ├── baccable_binary.py
│   │   ├── serial_reader.py
│   │   ├── clock_sync.py
│   │   ├── health.py
│   │   └── replay.py
│   │
│   ├── obd/
│   │   ├── __init__.py
│   │   ├── model.py
│   │   ├── transport.py
│   │   ├── ble_discover.py
│   │   ├── vlinker.py
│   │   ├── elm.py
│   │   ├── sae_pids.py
│   │   ├── custom_pids.py
│   │   ├── scheduler.py
│   │   └── safety.py
│   │
│   ├── events/
│   │   ├── __init__.py
│   │   ├── model.py
│   │   ├── keyboard.py
│   │   ├── palette.py
│   │   ├── voice.py
│   │   └── aliases.py
│   │
│   ├── storage/
│   │   ├── __init__.py
│   │   ├── schema.py
│   │   ├── sqlite_store.py
│   │   ├── raw_writer.py
│   │   ├── manifest.py
│   │   └── export.py
│   │
│   ├── tui/
│   │   ├── __init__.py
│   │   ├── capture_app.py
│   │   ├── status.py
│   │   ├── event_buttons.py
│   │   └── advisor_panel.py
│   │
│   ├── video/
│   │   ├── __init__.py
│   │   ├── model.py
│   │   ├── attach.py
│   │   ├── markers.py
│   │   └── alignment.py
│   │
│   ├── analysis/
│   │   ├── __init__.py
│   │   ├── survey.py
│   │   ├── entropy.py
│   │   ├── periodicity.py
│   │   ├── counters.py
│   │   ├── checksums.py
│   │   ├── event_correlation.py
│   │   ├── obd_correlation.py
│   │   ├── bitfields.py
│   │   ├── cross_bus.py
│   │   └── segmentation.py
│   │
│   ├── knowledge/
│   │   ├── __init__.py
│   │   ├── known_signals.py
│   │   ├── hypotheses.py
│   │   ├── evidence.py
│   │   └── dbc.py
│   │
│   └── advisor/
│       ├── __init__.py
│       ├── observations.py
│       ├── live_summary.py
│       ├── experiment_model.py
│       ├── planner.py
│       ├── novelty.py
│       ├── context_builder.py
│       ├── bridge.py
│       └── protocol.py
│
├── config/
│   ├── vehicle.yaml
│   ├── devices.yaml
│   ├── tags.yaml
│   ├── obd_pids.yaml
│   └── analysis.yaml
│
├── schemas/
│   ├── advisor_context.schema.json
│   ├── advisor_suggestion.schema.json
│   ├── hypotheses.schema.json
│   └── session_manifest.schema.json
│
├── tests/
│   ├── fixtures/
│   ├── test_baccable_parser.py
│   ├── test_device_clock.py
│   ├── test_raw_capture.py
│   ├── test_sqlite_store.py
│   ├── test_events.py
│   ├── test_replay.py
│   ├── test_video_alignment.py
│   └── ...
│
├── sessions/                   # ignored by Git
└── docs/
    ├── HARDWARE_SETUP.md
    ├── CAPTURE.md
    ├── OBD.md
    ├── EVENT_TAGGING.md
    ├── VIDEO.md
    ├── SESSION_FORMAT.md
    ├── ANALYSIS.md
    └── ADVISOR.md
```

Do not create all files before they are needed. The layout is the destination architecture, not a requirement to generate empty modules.

---

# 8. Data model

## 8.1 CAN record model

```python
@dataclass(slots=True)
class CanFrame:
    session_id: str
    bus: Literal["C1", "C2", "BH"]

    host_monotonic_ns: int
    host_utc_ns: int

    device_timestamp_raw_ms: int
    device_timestamp_unwrapped_ms: int | None

    arbitration_id: int
    dlc: int
    data: bytes

    # Binary BACCAble capture does not encode these explicitly.
    id_format: Literal["standard", "extended", "unknown"] = "unknown"
    is_rtr: bool | None = None

    source: str = "baccable_binary"
```

Do not set `standard` merely because ID <= 0x7FF; an extended frame can theoretically use a numerically small identifier. It is acceptable to add an inferred field separately, but preserve the unknown source semantics.

## 8.2 Capture-loss model

```python
@dataclass(slots=True)
class CanCaptureLoss:
    session_id: str
    bus: str
    host_monotonic_ns: int
    host_utc_ns: int
    device_timestamp_raw_ms: int | None
    dropped_frames: int
    raw_record: bytes
```

Loss records are first-class evidence, not parser errors.

## 8.3 Event model

```python
@dataclass(slots=True)
class Event:
    session_id: str
    event_id: str
    host_monotonic_ns: int
    host_utc_ns: int

    label: str
    state: str | None
    source: Literal[
        "keyboard",
        "mouse",
        "voice",
        "video",
        "automatic",
        "imported",
        "advisor"
    ]

    confidence: float | None
    uncertainty_ms: float | None
    note: str | None

    source_time_ns: int | None
    source_id: str | None
```

## 8.4 OBD value model

Every decoded OBD value must retain the raw transaction.

```text
request_id
request_start_monotonic_ns
request_end_monotonic_ns
command
header / addressing mode
raw_request
raw_response
parameter_name
value
unit
decoder/version
status
```

## 8.5 Hypothesis model

```yaml
id: signal.engine_rpm.candidate_001
name: engine_rpm
bus: C1
can_id: 0x123
start_bit: 16
length: 16
endian: little
signed: false
scale: 0.25
offset: 0
unit: rpm
status: probable
confidence: 0.93
sources:
  - type: obd_correlation
    session: 2026-...
    correlation: 0.998
  - type: controlled_experiment
    experiment: rpm_steps_001
conflicts: []
missing_tests:
  - repeat_on_cold_engine
  - compare_with_existing_baccable_decoder
```

Confidence is supporting metadata, not proof.

---

# 9. Session storage

## 9.1 Session directory

```text
sessions/
└── 2026-09-20T120102Z_idle_controls_001/
    ├── manifest.json
    ├── telemetry.sqlite
    │
    ├── raw/
    │   ├── C1.bin
    │   ├── C2.bin
    │   ├── BH.bin
    │   └── obd.jsonl
    │
    ├── logs/
    │   └── app.log
    │
    ├── voice/
    │   └── ... optional ...
    │
    ├── video/
    │   └── sources.json
    │
    ├── exports/
    │   ├── can.parquet
    │   ├── obd.parquet
    │   ├── events.parquet
    │   └── timeline.parquet
    │
    ├── analysis/
    │   ├── survey.json
    │   ├── observations.jsonl
    │   └── hypotheses.yaml
    │
    └── dbc/
        └── vehicle.dbc
```

## 9.2 SQLite mode

Use SQLite as the live authoritative structured store.

Configuration:

- WAL mode;
- batch inserts;
- one dedicated DB writer path;
- sensible busy timeout;
- periodic checkpoints outside the critical read path;
- schema migrations from the beginning, even if a simple integer schema version is enough.

Raw files are separate immutable evidence.

## 9.3 Suggested tables

At minimum:

```text
sessions
can_frames
can_capture_loss
system_events
events
obd_requests
obd_responses
obd_values
voice_transcripts
video_sources
video_anchors
video_markers
observations
hypotheses
hypothesis_evidence
experiments
experiment_runs
relationships
advisor_suggestions
```

Milestone 1 only needs the subset required for capture and events.

---

# 10. Time architecture

Time handling is one of the most important parts of the project.

## 10.1 Host timestamps

At ingestion, call both:

- `time.monotonic_ns()`
- `time.time_ns()`

The monotonic value is used for relative ordering and experiment analysis.

UTC wall time is used for human-readable exports and correlation with external systems.

## 10.2 BACCAble device clocks

Each C1/C2/BH binary stream has an independent local 24-bit millisecond timestamp.

Implement per-device unwrap logic:

```text
raw decreases by a large wrap threshold
    -> increment wrap counter
unwrapped = raw + wrap_count * 2^24
```

Do not mistake USB reconnect/reset for a normal wrap. A reconnect starts a new device-clock epoch and should emit a system event.

## 10.3 Host/device clock fitting

For better cross-bus timing:

1. timestamp receipt of every complete BACCAble record on the host;
2. collect pairs `(device_unwrapped_ms, host_monotonic_ns)`;
3. fit a robust linear mapping per device/epoch:

```text
host_estimated_ns = a * device_ms + b
```

4. account for USB buffering/outliers;
5. never overwrite the original host-arrival timestamp;
6. expose both raw and clock-corrected timestamps to analysis.

This provides a path to compensate for board clock offsets and drift while preserving evidence.

## 10.4 Event timing uncertainty

A human saying "brake on" after physically pressing the pedal has reaction latency. Store uncertainty rather than pretending the voice timestamp is exact.

Example:

```text
label: brake_on
source: voice
uncertainty_ms: 500
```

Keyboard/mouse markers may use a smaller default uncertainty.

---

# 11. BACCAble device discovery

Implement:

```bash
baccable doctor
```

The command should:

1. enumerate `/dev/cu.*` candidates;
2. inspect USB VID/PID where macOS APIs permit;
3. inspect USB serial number/string descriptors where available;
4. identify candidate BACCAble interfaces;
5. show newly connected ports;
6. allow mapping to `C1`, `C2`, `BH`;
7. persist mapping fingerprints in `config/devices.yaml`;
8. warn if identities changed;
9. allow a guided mapping flow such as "unplug/replug C1" when no unique serial identity exists.

Example output:

```text
BACCAble Lab — device check

ROLE  PORT                    USB ID       STATUS
C1    /dev/cu.usbmodem101     ...          READY
C2    /dev/cu.usbmodem103     ...          READY
BH    /dev/cu.usbmodem105     ...          READY

All capture devices available.
```

Do not assume the three devices expose different serial numbers. The implementation must handle identical USB descriptors gracefully.

---

# 12. BACCAble binary parser

## 12.1 Requirements

Parser input is an arbitrary byte stream. USB/serial reads may return:

- part of a record;
- exactly one record;
- multiple records;
- a stream starting mid-record after reconnect/corruption.

Implement a buffering parser that:

- accumulates bytes;
- emits only complete 16-byte records;
- validates byte 0;
- distinguishes `0xA0..0xA8` data and `0xAF` loss records;
- has explicit resynchronization behavior for invalid headers;
- records parser resync/system events;
- never silently discards a valid `0xAF` marker.

## 12.2 DLC validation

Only `0xA0..0xA8` is valid for classic data records.

For a normal frame:

```text
dlc = header & 0x0F
```

Only `data[:dlc]` is semantically meaningful, but preserve all 8 raw data bytes if useful for forensic comparison.

## 12.3 Loss accounting

Expose both:

- per-loss-marker count;
- cumulative lost-frame count per bus;
- session total;
- most recent loss time.

The UI must display drops prominently.

---

# 13. CAN acquisition pipeline

Use independent readers for C1/C2/BH.

Recommended conceptual pipeline:

```text
serial port
   |
reader thread/task
   |----> raw binary file writer (append)
   |
parser
   |
bounded in-memory queue
   |
dedicated batched SQLite writer
   |
optional non-critical live-analysis fanout
```

Important rules:

- capture reads must not wait for database analysis queries;
- UI receives counters/summaries, not every frame;
- advisor receives summaries, not every frame;
- if a non-critical consumer is slow, drop that consumer's derived updates rather than blocking raw acquisition;
- if the core ingest queue is approaching a dangerous threshold, generate a visible system warning;
- raw file writes should be simple, append-only, and buffered carefully.

---

# 14. Milestone 1 — minimum useful product (software ready; hardware acceptance pending)

This milestone has absolute priority. Do not begin Bluetooth, AI, voice, or signal discovery until it works reliably.

## 14.1 Deliverables

Implement:

```bash
baccable doctor
baccable capture
baccable sessions
baccable session info <session>
baccable event add <session> ...
baccable export <session> --basic
```

`baccable capture` must:

- open three BACCAble USB streams;
- record C1/C2/BH simultaneously;
- save raw streams;
- parse 16-byte records;
- store parsed CAN frames in SQLite;
- store capture-loss records;
- use a common host timeline;
- show live bus health and FPS;
- support instant keyboard/manual markers;
- exit cleanly with Ctrl-C;
- provide an end-of-session summary.

## 14.2 Minimum TUI

Use Textual only if it does not delay capture implementation. A first usable version may use Rich plus non-blocking key handling.

Target screen:

```text
 BACCAble Lab                                         RECORDING  00:14:27
 ─────────────────────────────────────────────────────────────────────────
 CAN        rate        frames         drops       state
 C1        1254/s       1,040,123         0        OK
 C2         812/s         670,221         0        OK
 BH         344/s         289,102         0        OK

 LAST EVENTS
 02:31.482  brake_on
 02:32.114  brake_off
 02:39.220  left_indicator_on

 QUICK MARKERS
 [1] Brake ON   [2] Brake OFF   [3] Left ON   [4] Left OFF
 [5] Right ON   [6] Right OFF   [7] Custom    [u] Undo

 Storage: OK       queues: OK       DB: WAL       raw: ON
```

## 14.3 Quick event keys

Initial hard-coded/default keys are acceptable, but the source of truth should quickly move to `config/tags.yaml`.

A marker must be written immediately without confirmation.

Support:

- keyboard shortcut;
- mouse click when Textual is introduced;
- `undo last marker` as a new audit operation, not destructive deletion if possible.

## 14.4 End-of-session output

Example:

```text
Session complete

Duration          00:37:41
C1 frames         2,821,441
C2 frames         1,614,772
BH frames           824,182
Capture losses            0
Events                    84
Parser resyncs             0

Saved:
sessions/2026-.../
```

## 14.5 Milestone 1 definition of done

Milestone 1 is done when the actual MacBook can:

1. detect and map the three BACCAble ports;
2. record all three for at least a meaningful stationary test session;
3. show live rates;
4. add manual event markers without interrupting capture;
5. retain `0xAF` loss records;
6. cleanly stop and reopen the resulting database;
7. replay the raw data through the parser in tests;
8. demonstrate that UI work does not stall acquisition;
9. report any data loss instead of hiding it.

Do not call the milestone done based only on unit tests; perform a hardware capture acceptance run.

---

# 15. Milestone 1.5 — replay and reliability before adding complexity

Implement a replay backend early:

```bash
baccable replay <session>
```

This allows the entire downstream pipeline to be developed without sitting in the car.

Replay should support:

- real-time speed;
- accelerated speed, e.g. `--speed 10`;
- fastest possible mode;
- deterministic event replay;
- choosing one or all buses.

Add failure tests for:

- partial serial reads;
- malformed record headers;
- lost USB connection;
- reconnect and new device-clock epoch;
- capture-loss records;
- full/high queues;
- database write failure;
- abnormal process termination and session recovery.

---

# 16. Event tagging system

## 16.1 Canonical tags

Use stable machine-friendly names.

Example `config/tags.yaml`:

```yaml
groups:
  powertrain:
    - engine_start
    - engine_stop
    - rpm_idle
    - rpm_1500
    - rpm_2000
    - rpm_2500
    - accelerator_0
    - accelerator_25
    - accelerator_50

  chassis:
    - brake_on
    - brake_off
    - parking_brake_on
    - parking_brake_off
    - steering_center
    - steering_left
    - steering_right
    - gear_p
    - gear_r
    - gear_n
    - gear_d

  body:
    - driver_door_open
    - driver_door_close
    - passenger_door_open
    - passenger_door_close
    - rear_left_door_open
    - rear_left_door_close
    - rear_right_door_open
    - rear_right_door_close
    - lock
    - unlock
    - left_indicator_on
    - left_indicator_off
    - right_indicator_on
    - right_indicator_off
    - hazards_on
    - hazards_off
    - low_beam_on
    - low_beam_off
    - high_beam_on
    - high_beam_off
    - wiper_single
    - wiper_low
    - wiper_high
```

## 16.2 Event sessions and repetition

Support experiment/run identity so repeated ON/OFF cycles are grouped.

Example:

```text
experiment: brake_switch_001
run 1: brake_on -> brake_off
run 2: brake_on -> brake_off
...
```

Repeated controlled tests are more informative than a single marker.

---

# 17. Milestone 2 — vLinker MS and OBD reference data

Only begin this after Milestone 1 capture is trusted.

## 17.1 Goal

Use the same Mac as the only timestamp authority for:

- C1/C2/BH CAN;
- OBD requests/responses;
- decoded OBD values;
- event markers.

No cross-computer clock synchronization is required.

## 17.2 Bluetooth/BLE discovery

Because the user's vLinker MS already works with Car Scanner on this Mac, a compatible transport exists. Do not assume the exact BLE GATT UUIDs without discovering them.

Implement:

```bash
baccable obd scan
```

The command should:

- scan BLE advertisements;
- identify candidate vLinker devices;
- print name/address/identifier available through macOS APIs;
- enumerate GATT services and characteristics;
- determine writable and notify/read characteristics;
- optionally run a safe adapter-identification exchange;
- save discovered transport configuration.

Use `bleak` unless hardware tests show another transport is required.

If the user's exact vLinker mode exposes a macOS serial device rather than BLE GATT, implement a `pyserial` transport behind the same interface.

## 17.3 Transport abstraction

```python
class ObdTransport(Protocol):
    async def connect(self) -> None: ...
    async def disconnect(self) -> None: ...
    async def write(self, data: bytes) -> None: ...
    async def read_until_prompt(self, timeout: float) -> bytes: ...
```

Keep ELM/STN command logic above transport.

## 17.4 Adapter initialization

Implement a conservative initialization sequence determined from actual hardware testing.

Support common ELM-style operations such as:

- adapter identification;
- reset where appropriate;
- echo off;
- line formatting normalization;
- headers on/off as required;
- protocol detection/configuration;
- timeouts.

Log every command and raw response.

Do not hide unexpected adapter behavior behind aggressive retries.

## 17.5 Read-only allowlist

Create `obd/safety.py` and explicitly reject commands that are not on the configured read-only allowlist.

Initial safe scope:

- standard Mode 01 current data;
- supported-PID discovery;
- later, explicitly configured UDS ReadDataByIdentifier (`0x22`) only.

Extended Alfa Romeo/Giulia definitions must be data-driven and reviewed before use.

## 17.6 Standard OBD values

Start with supported standard PIDs such as:

- engine RPM;
- vehicle speed;
- coolant temperature;
- throttle position;
- calculated engine load;
- intake air temperature;
- other PIDs explicitly reported as supported by the ECU.

Do not waste bus time continuously querying unsupported parameters.

## 17.7 PID scheduler

Allow different rates:

```yaml
engine_rpm:
  enabled: true
  rate_hz: 10
vehicle_speed:
  enabled: true
  rate_hz: 10
coolant_temperature:
  enabled: true
  rate_hz: 1
```

The scheduler must not flood the diagnostic bus.

Record actual request and response times instead of assuming requested frequency equals achieved frequency.

## 17.8 Alfa Romeo/Giulia custom PIDs

Later import/translate known definitions from:

- current BACCAble `uds_params_array` and related catalogs;
- `danardi78/Alfaromeo-Giulia-Stelvio-PIDs`;
- user-verified MultiECUScan/Car Scanner observations.

Every custom definition should include provenance:

```yaml
name: oil_temperature
source: community_repo
source_url: ...
vehicle_profile: diesel_2.2
verified_on_this_vehicle: false
header: ...
command: ...
formula: ...
unit: C
```

Community definitions are candidates until tested on this exact car.

## 17.9 Diagnostic traffic visible in BACCAble CAN capture

The vLinker requests and ECU diagnostic responses may also appear in BACCAble raw CAN capture.

This is valuable evidence.

The system should later classify CAN frames as:

- likely normal vehicle broadcast;
- likely diagnostic request;
- likely diagnostic response;
- unknown.

Provide analysis switches such as:

```bash
--exclude-diagnostic-traffic
--only-diagnostic-traffic
```

Never delete diagnostic frames from raw capture.

---

# 18. Milestone 3 — voice event marking

Voice is optional. Capture must remain fully usable without it.

## 18.1 Goal

The user should be able to say short commands while performing stationary experiments, for example:

```text
"mark brake on"
"mark brake off"
"mark left on"
"mark driver door open"
"mark rpm two thousand"
```

## 18.2 Wake prefix

Do not interpret arbitrary conversation as an event.

Require a configurable wake prefix such as:

```text
mark ...
```

## 18.3 Local STT

Prefer local speech recognition so vehicle research does not depend on network latency.

The STT engine must be behind a small interface so the implementation can switch between:

- `whisper.cpp`;
- an Apple-Silicon optimized local engine;
- another future backend.

## 18.4 Alias mapping

Example:

```yaml
brake_on:
  aliases:
    - brake on
    - hamulec on
    - wciskam hamulec
    - hamulec
```

Store both:

- raw transcript;
- canonical tag.

Do not discard low-confidence speech. Store it with a low confidence score and make correction easy.

---

# 19. Milestone 4 — camera/video as an external timeline source

The camera does not have to be directly controlled by the Mac.

## 19.1 Attach source

```bash
baccable video attach <session> /path/to/video.mp4
```

Store:

- path;
- file hash;
- file size;
- media metadata;
- optional creation timestamp;
- notes about what the camera sees.

Do not require copying a large video into the session folder.

## 19.2 Video-local markers

Allow:

```bash
baccable video mark <session> <video-id> 00:07:14.320 brake_on
```

Store the marker initially in video-local time.

## 19.3 Alignment anchors

For externally recorded video, support anchors:

```text
video timestamp <-> session event/time
```

With one anchor:

- assume offset only;
- flag drift as unknown.

With two or more anchors:

fit an affine mapping:

```text
session_time = a * video_time + b
```

With multiple anchors, use robust fitting and report residuals.

This permits correction of camera clock drift.

## 19.4 Suggested synchronization action

At the beginning and end of a recording, perform an obvious event visible both in CAN and video, for example a controlled hazards ON/OFF sequence or another clearly observable non-driving-state action.

Do not depend exclusively on EXIF/video creation time for sub-second alignment.

---

# 20. Milestone 5 — offline CAN survey

This is the first analytical layer.

Implement:

```bash
baccable analyze survey <session>
```

For each `(bus, CAN ID)` compute at least:

- frame count;
- first/last timestamp;
- DLC distribution;
- frames per second;
- median/min/max inter-arrival period;
- jitter;
- number of unique payloads;
- byte min/max;
- entropy per byte;
- entropy per bit;
- bit-flip rate;
- value-change rate;
- candidate periodicity;
- candidate rolling counters;
- candidate checksum/CRC-like fields;
- long constant fields;
- state count / clustering hints.

Output machine-readable JSON plus a compact human report.

---

# 21. Event correlation analysis

Implement:

```bash
baccable analyze event <session> brake_on
```

For each event occurrence, analyze configurable windows such as:

```text
pre:  -2.0 s .. event
post: event .. +3.0 s
```

Detect:

- IDs appearing/disappearing;
- frequency changes;
- payload states appearing only near the event;
- bits changing consistently;
- values changing monotonically;
- delayed reactions;
- short bursts;
- repeated follow-up messages.

For binary ON/OFF pairs calculate metrics such as:

```text
P(bit=1 | state=ON)
P(bit=1 | state=OFF)
transition consistency
median transition latency
latency spread
false-positive rate outside event windows
```

Controlled ON/OFF cycles are preferred over a single occurrence.

---

# 22. OBD-to-CAN correlation

Implement:

```bash
baccable analyze obd <session> engine_rpm
```

The analyzer should search candidate signals across C1/C2/BH.

Try, with sensible pruning:

- uint8 / int8;
- uint16 little-endian;
- uint16 big-endian;
- int16 little/big;
- 24/32-bit values where justified;
- non-byte-aligned bit fields;
- common simple scale/offset mappings.

Candidate physical mapping:

```text
physical = raw * scale + offset
```

Evaluate:

- correlation;
- temporal lag;
- coverage;
- monotonicity where expected;
- residual error;
- behavior at zero/engine-off;
- repeatability across sessions.

Do not declare a signal verified solely from high correlation in one normal-driving log.

---

# 23. Cross-bus analysis

Because C1, C2, and BH are captured simultaneously, search for signals that appear to be gatewayed or semantically related.

Potential relationships:

```text
signal A correlates_with signal B
signal A delayed_copy_of signal B
signal A transformed_copy_of signal B
ID X appears_when bus-state Y appears
```

Use these relationships to infer topology and to guide experiments.

---

# 24. Known signals and source harvesting

Create a persistent catalog:

```text
knowledge/known_signals.yaml
```

A coding/analysis agent should mine the checked-out BACCAble source for:

- CAN IDs;
- existing raw decoders;
- masks;
- shifts;
- scales;
- offsets;
- UDS request IDs;
- UDS response IDs;
- DIDs;
- parameter names;
- units;
- engine-profile restrictions;
- comments indicating unverified/research status.

Important: distinguish:

- broadcast/native CAN signals;
- active UDS/diagnostic values;
- values calculated by BACCAble;
- values routed/emulated by BACCAble.

Always record provenance as file/line/commit when possible.

---

# 25. DBC generation policy

Maintain two conceptual layers:

1. hypotheses;
2. verified signal database / DBC.

Do not automatically publish every candidate into `vehicle.dbc`.

By default, DBC export should include only:

- `verified` signals;
- optionally `probable` signals when the user explicitly requests them.

Add comments/provenance to generated definitions where supported.

---

# 26. Live AI / Codex advisor architecture

The AI advisor is a **non-critical consumer** of summaries. It must never sit on the acquisition path.

```text
CAN + OBD + events
        |
        v
  acquisition/storage  ------------------------------+
        |                                             |
        +--> deterministic live statistics            |
                        |                              |
                        v                              |
                 advisor context                      |
                        |                              |
                        v                              |
                 Codex / AI advisor                    |
                        |                              |
                        v                              |
                suggested experiment                  |
                        |                              |
                        v                              |
                    user action                       |
                        |                              |
                        +------------------------------+
```

## 26.1 Do not stream raw CAN to the model

The advisor should receive compact context such as:

- current experiment;
- recent event markers;
- repetition count;
- strongest changed IDs/fields;
- newly observed IDs/states;
- frequency changes;
- OBD reference changes;
- unresolved hypotheses;
- conflicting evidence;
- capture health/drop status.

The model can request a deeper query through tools when needed.

## 26.2 Advisor bridge must be model-agnostic

Do not hard-code a specific OpenAI product interface into acquisition code.

Define a protocol such as:

```text
AdvisorContext JSON -> AdvisorSuggestion JSON
```

Possible integrations can later include:

- a local Codex/agent session with terminal access;
- an API-backed model;
- a deterministic heuristic planner;
- another analysis agent.

The recorder must work if no advisor is connected.

## 26.3 Advisor query tools

Expose safe read-only CLI/query tools that an agent can call:

```bash
baccable advisor status <session>
baccable advisor context <session> --last 30s
baccable query ids <session> --bus C2 --last 60s
baccable query changes <session> --around-event brake_on
baccable query signal <session> --bus C1 --id 0x123
baccable hypotheses list
baccable experiments pending
```

Later expose the same operations through a local Unix-domain socket/JSON-RPC API if useful.

---

# 27. Creative experiment planner

The advisor must not be limited to a predefined checklist.

Its job is to discover new questions.

## 27.1 Novelty detection

Continuously maintain baselines and detect:

- a CAN ID never seen before;
- a new DLC for an existing ID;
- a previously constant bit starting to change;
- a new payload/state cluster;
- an unusual frequency change;
- a frame appearing only in one vehicle state;
- a new temporal dependency;
- a new cross-bus correlation;
- a new relationship with an OBD value;
- a rare burst near a manual event.

Each novelty becomes an `observation`.

Example:

```yaml
type: new_state
bus: BH
can_id: 0x4A2
observed_at: ...
context:
  recent_events:
    - driver_door_open
    - left_indicator_on
```

## 27.2 Multiple competing hypotheses

The advisor should generate competing explanations rather than prematurely selecting one.

Example:

```text
0x4A2 could represent:
- indicator state;
- door/indicator warning interaction;
- cluster notification;
- gateway retransmission.
```

For each hypothesis record:

- evidence for;
- evidence against;
- missing discriminating tests;
- current confidence;
- state.

## 27.3 Adaptive experiments

The advisor may propose an experiment not present in the original plan.

Every suggestion should include:

```yaml
name:
goal:
reason:
steps:
variables_to_hold_constant:
expected_discriminating_result:
risk_level:
estimated_duration:
```

Example:

```yaml
name: indicator_door_interaction
goal: distinguish indicator state from body warning interaction
reason: 0x4A2 only appeared after left_indicator_on while driver door was open
steps:
  - close all doors
  - left indicator ON
  - left indicator OFF
  - open driver door
  - left indicator ON
  - left indicator OFF
  - close driver door
  - hazards ON
  - hazards OFF
variables_to_hold_constant:
  - engine state
  - gear
  - brake
  - headlights
expected_discriminating_result:
  - if 0x4A2 follows indicator with door both open and closed, indicator-related hypothesis gains support
  - if it appears only with door open, warning/interaction hypothesis gains support
risk_level: stationary_low
```

## 27.4 Counterfactual testing

For every strong hypothesis, ask:

> What short test could prove this wrong?

Examples:

If candidate = brake switch:

- toggle headlights without brake;
- parking brake without pedal;
- brake with engine off;
- repeat in P/N/D if stationary and appropriate.

If candidate = left indicator:

- right indicator;
- hazards;
- indicator with different ignition states.

## 27.5 Cross-feature reasoning

Look for interactions such as:

- brake × gear;
- indicator × steering angle;
- door × ignition;
- lock × key/vehicle wake state;
- headlight × ambient state;
- engine start × brake;
- reverse gear × reverse lamps;
- parking brake × gear;
- seat belt × vehicle speed.

When data suggests an interaction, propose a test that changes one variable at a time.

## 27.6 Temporal reasoning

Search beyond the exact marker instant.

Classify patterns as:

- preceding event;
- immediate response;
- delayed response;
- periodic response;
- transient burst;
- persistent state.

If a frame consistently appears ~800 ms after engine start, the advisor should suggest repeating engine-start sequences to test latency stability.

## 27.7 Serendipity

Unexpected events during an experiment are not automatically noise.

If a new pattern appears after an accidental combination of actions, save a `serendipitous_observation` and suggest a clean reproducing experiment.

## 27.8 Information-gain prioritization

Internally rank suggestions using factors such as:

- expected information gain;
- ability to discriminate competing hypotheses;
- number of unresolved signals affected;
- novelty;
- required time;
- complexity;
- repeatability;
- safety.

Prefer short, stationary, one-variable experiments when they provide comparable information.

---

# 28. Advisor UI

Later add a dedicated TUI panel:

```text
 AI ADVISOR
 ───────────────────────────────────────────────────────
 Observation
 New BH 0x4A2 state appeared around left_indicator_on.
 It was present only while driver door was open.

 Hypotheses
 1. door/indicator warning       probable
 2. indicator state              possible
 3. cluster notification         possible

 Suggested next test
 Close all doors and repeat LEFT ON/OFF five times,
 then repeat with driver door open.
 Keep ignition state unchanged.

 [A] Accept   [S] Skip   [D] Done   [N] Note
```

Advisor suggestions must be persisted even if skipped. This prevents repeating the same unhelpful suggestion without context.

---

# 29. Experiment state machine

Persist experiments independently of conversation history.

States:

```text
proposed
accepted
running
completed
invalid
needs_repeat
rejected
```

Store:

- who/what proposed it;
- reason;
- accepted/start/end timestamps;
- expected variables;
- actual event markers;
- result summary;
- data quality;
- affected hypotheses;
- notes.

The project knowledge must survive changing model providers or ending a Codex session.

---

# 30. Suggested initial vehicle experiments

Do not require these for software completion, but ship a template experiment set.

## 30.1 Stationary baseline first

Start with stationary tests before normal driving.

Suggested sequence:

1. vehicle asleep / begin capture before wake if practical;
2. remote unlock;
3. driver door open/close repeated;
4. lock/unlock repeated;
5. ignition states;
6. engine start;
7. idle;
8. brake ON/OFF 10 times;
9. left indicator ON/OFF repeated;
10. right indicator ON/OFF repeated;
11. hazards ON/OFF;
12. low/high beam states;
13. parking brake states;
14. seat belt state if easily controlled;
15. gear P/R/N/D only under safe stationary conditions and according to normal vehicle operation;
16. engine stop;
17. lock;
18. observe transition to network sleep.

## 30.2 RPM steps

When stationary and safe:

```text
idle
1500 rpm
2000 rpm
2500 rpm
idle
```

Repeat and use OBD RPM as reference.

## 30.3 Negative controls

For a candidate brake bit, do not only repeat braking. Also perform unrelated controls while not touching the brake.

The goal is to distinguish causal/context-specific relations from coincidental correlation.

## 30.4 Driving tests later

Driving-related collection such as wheel speed and steering relationships should come after stationary tooling is reliable and should be performed only under controlled, safe conditions. The project should not require interactive laptop operation by a driver while the vehicle is moving.

---

# 31. Automatic state segmentation

Later analysis should detect likely macro-state transitions without relying on manual tags:

- bus wake;
- ignition transition;
- engine start;
- engine running;
- vehicle moving;
- vehicle stopped;
- engine stop;
- network sleep.

Use:

- traffic volume;
- active ID set;
- known signals;
- OBD references;
- transition clustering.

Automatic state names remain hypotheses until linked to known references.

---

# 32. Logging and diagnostics

## 32.1 Application logs

Use structured/log-friendly messages with source and severity.

Do not print every CAN frame to the terminal during capture.

## 32.2 System events

Persist important operational events:

- USB connected/disconnected;
- parser resync;
- device-clock reset/wrap;
- `0xAF` loss marker;
- queue high-water warning;
- SQLite write failure;
- OBD connect/disconnect;
- OBD timeout;
- voice subsystem failure;
- advisor unavailable.

This allows later analysis to distinguish vehicle behavior from acquisition problems.

---

# 33. Capture startup / preflight

`baccable capture` should run a preflight and show something like:

```text
Preflight

[OK] C1 connected
[OK] C2 connected
[OK] BH connected
[OK] C1 USB mode appears to provide binary CAN capture
[OK] C2 USB mode appears to provide binary CAN capture
[OK] BH USB mode appears to provide binary CAN capture
[OK] session directory writable
[OK] SQLite writable
[OK] free disk space: 118 GB
[--] vLinker disabled for this session
[--] voice disabled

Ready. Press ENTER to record.
```

For Milestone 1, OBD and voice are optional and must not block capture.

Later, allow flags:

```bash
baccable capture --no-obd --no-voice
baccable capture --obd
baccable capture --voice
```

---

# 34. Capture shutdown

On Ctrl-C / Stop:

1. stop starting new OBD requests;
2. stop non-critical analysis/advisor work;
3. stop serial readers;
4. flush queues;
5. flush SQLite batches;
6. flush raw files;
7. close ports;
8. store end metadata;
9. checkpoint SQLite as appropriate;
10. compute basic session statistics;
11. show the final summary.

Do not kill the process immediately on the first Ctrl-C unless data integrity cannot otherwise be preserved.

A second forced interrupt may abort cleanup but must be recorded on next recovery when possible.

---

# 35. Export formats

SQLite is the live source of truth.

Provide exports for analysis/interoperability:

## 35.1 Parquet

Preferred bulk analysis format:

```text
can.parquet
obd.parquet
events.parquet
timeline.parquet
```

## 35.2 Human-readable CAN export

Provide a candump-like/text representation for tools and quick inspection, while documenting that BACCAble's integrated binary format lacks an explicit standard/extended flag.

## 35.3 CSV/JSON

Use for events, summaries, and compatibility, not as the primary storage for multi-million-frame sessions.

---

# 36. Testing strategy

## 36.1 Unit tests

Must cover:

- 16-byte BACCAble record parsing;
- DLC 0..8;
- `0xAF` loss parsing;
- arbitrary chunk boundaries;
- parser resynchronization;
- device timestamp wrap;
- reconnect/reset epoch;
- event timestamping;
- SQLite batching;
- session manifest;
- raw file preservation;
- config validation.

## 36.2 Integration tests

Use pseudo-terminal/fake serial sources to simulate three BACCAble devices simultaneously.

Test:

- different frame rates;
- one bus disconnecting;
- delayed disk writes;
- queue pressure;
- graceful shutdown;
- replay equivalence.

## 36.3 OBD tests

Use a fake ELM/vLinker transport.

Test:

- prompt parsing;
- echo/no echo;
- multiline replies;
- `NO DATA`;
- timeout;
- reconnect;
- supported PID discovery;
- allowlist rejection of unsafe commands.

## 36.4 Hardware acceptance tests

Maintain an explicit hardware checklist separate from unit-test success.

Minimum capture acceptance:

- all three USB ports record simultaneously;
- session longer than a short smoke test;
- no unexpected parser errors;
- loss markers correctly displayed if induced/observed;
- port mapping survives a restart or is recoverable through guided remapping;
- UI tagging does not increase frame loss.

---

# 37. Development milestones

## Milestone 0 — reconnaissance, one short coding pass — COMPLETE

Before implementation:

- inspect `eujot/BACCAble` current checkout/online source;
- read `AGENTS.md`;
- read `docs/ACTION_PLAN.md` for firmware context;
- read `docs/architecture/USB_DIAGNOSTICS.md`;
- confirm current binary capture implementation in source;
- identify device USB VID/PID/descriptors from source/hardware or macOS enumeration;
- create the Python project skeleton;
- write `README.md` with a one-command development setup.

Do not modify BACCAble firmware during this milestone.

## Milestone 1 — three-bus CAN recorder + manual tags — SOFTWARE READY; ACCEPTANCE PENDING

**This is the first real product. Ship it as soon as possible.**

Deliver:

- device discovery/mapping;
- 3x binary capture;
- raw preservation;
- SQLite;
- loss marker support;
- host timestamps;
- basic TUI/status;
- keyboard/event palette;
- clean stop;
- session summary;
- parser/replay tests.

Stop and perform real hardware acceptance before moving on.

## Milestone 1.5 — reliability and replay

Deliver:

- replay;
- reconnect handling;
- recovery from incomplete session;
- performance instrumentation;
- queue pressure diagnostics;
- export basics.

## Milestone 2 — vLinker MS OBD

Deliver:

- BLE discovery;
- vLinker transport;
- conservative ELM/STN client;
- supported standard PID discovery;
- read-only polling scheduler;
- raw OBD transactions;
- OBD values in same SQLite timeline;
- UI OBD status.

## Milestone 3 — voice markers

Deliver:

- local STT backend interface;
- wake prefix;
- aliases;
- transcript + canonical tag;
- uncertainty/confidence;
- correction UI/CLI.

## Milestone 4 — video/manual external timeline

Deliver:

- attach video;
- video-local markers;
- anchors;
- offset/drift fit;
- mapping to session timeline;
- audit trail.

## Milestone 5 — offline analysis

Deliver:

- survey;
- event correlation;
- OBD correlation;
- bit-field candidates;
- counters/checksum hints;
- cross-bus relationships;
- persistent hypotheses.

## Milestone 6 — live deterministic advisor

Before adding an LLM, create deterministic live summaries and simple rules:

- insufficient repetitions;
- event contaminated by multiple simultaneous markers;
- new ID/state near event;
- strong candidate needs negative control;
- OBD reference unavailable;
- capture quality warning.

This validates the advisor data model without model dependency.

## Milestone 7 — Codex/AI creative advisor

Deliver the model-agnostic advisor bridge and tools.

The AI may:

- inspect summaries;
- request deeper read-only queries;
- generate hypotheses;
- propose new experiments;
- detect unexpected relationships;
- request repetitions;
- propose negative controls;
- prioritize pending experiments;
- update hypothesis evidence after user confirmation/analysis.

It may not transmit arbitrary CAN frames or initiate unsafe diagnostics.

## Milestone 8 — verified DBC workflow

Deliver:

- DBC candidate generation;
- verification workflow;
- session-to-session evidence;
- verified-only export policy;
- regression tests for known signals.

---

# 38. CLI target

Long-term CLI:

```text
baccable doctor
baccable capture
baccable sessions
baccable session info SESSION
baccable replay SESSION

baccable event add SESSION --at ... LABEL
baccable event list SESSION
baccable event edit ...
baccable event undo ...

baccable obd scan
baccable obd test
baccable obd supported

baccable video attach SESSION FILE
baccable video anchor SESSION VIDEO ...
baccable video mark SESSION VIDEO ...

baccable export SESSION

baccable analyze survey SESSION
baccable analyze event SESSION LABEL
baccable analyze obd SESSION PARAMETER
baccable analyze cross-bus SESSION
baccable analyze all SESSION

baccable hypotheses list
baccable hypotheses show ID
baccable hypotheses verify ID
baccable hypotheses reject ID

baccable experiments list
baccable experiments next
baccable experiments start ID
baccable experiments complete ID

baccable advisor status SESSION
baccable advisor context SESSION
baccable advisor watch SESSION
```

Milestone 1 should implement only the commands required for immediate capture.

---

# 39. Configuration examples

## 39.1 `config/vehicle.yaml`

```yaml
vehicle:
  make: Alfa Romeo
  model: Giulia
  year: null
  engine: null
  notes: null

buses:
  C1:
    obd_pins: [6, 14]
    nominal_bitrate: 500000
  C2:
    obd_pins: [12, 13]
    nominal_bitrate: 500000
  BH:
    obd_pins: [3, 11]
    nominal_bitrate: 125000

source:
  repo: https://github.com/eujot/BACCAble
  note: verify these values against current firmware and actual hardware
```

## 39.2 `config/devices.yaml`

```yaml
baccable:
  C1:
    port_hint: null
    usb_serial: null
    vid: null
    pid: null
  C2:
    port_hint: null
    usb_serial: null
    vid: null
    pid: null
  BH:
    port_hint: null
    usb_serial: null
    vid: null
    pid: null

vlinker:
  transport: auto
  ble_identifier: null
```

## 39.3 `config/analysis.yaml`

```yaml
event_windows:
  default_before_s: 2.0
  default_after_s: 3.0

hypotheses:
  min_repetitions_for_probable: 5
  require_independent_session_for_verified: true

advisor:
  enabled: false
  context_interval_s: 5
  mode: exploratory
```

Thresholds are defaults, not scientific truth. Store actual evidence.

---

# 40. Performance and reliability expectations

The agent should benchmark the system on the actual MacBook before optimization.

Requirements:

- no frame-by-frame terminal rendering;
- batch SQLite inserts;
- bounded queues with visible health metrics;
- raw file append path independent of expensive analysis;
- no pandas DataFrame creation in the live ingest path;
- no AI calls in the live ingest path;
- no synchronous BLE wait that can stop CAN readers;
- ability to disable all optional consumers.

Measure:

- frames/s per bus;
- raw write throughput;
- DB batch latency;
- queue occupancy/high-water mark;
- process CPU and memory;
- capture-loss markers;
- parser errors.

---

# 41. Data integrity

For completed sessions, consider recording SHA-256 hashes for immutable raw files in `manifest.json`.

Example:

```json
{
  "raw_files": {
    "C1.bin": {"sha256": "...", "bytes": 1234567},
    "C2.bin": {"sha256": "...", "bytes": 1234567},
    "BH.bin": {"sha256": "...", "bytes": 1234567}
  }
}
```

Derived exports do not need to be immutable; they can be regenerated.

---

# 42. Session metadata

Record enough information to reproduce interpretation later:

- project software commit/version;
- macOS version;
- Python version;
- BACCAble repo commit if known;
- firmware role/version for each controller if identifiable;
- USB identity and mapped role;
- firmware settings relevant to transmission/capture;
- vehicle profile;
- OBD splitter notes;
- vLinker model/firmware if retrievable;
- enabled OBD PID set;
- capture start/end UTC;
- session label/notes;
- capture-loss total;
- abnormal shutdown flag.

---

# 43. Knowledge persistence across sessions

Do not rely on AI chat history as the vehicle database.

Persist:

- known signals;
- hypotheses;
- evidence;
- rejected ideas;
- observations;
- experiments performed;
- pending experiments;
- cross-bus relationships;
- DBC revisions.

At the beginning of a new session the advisor should be able to report:

```text
Unresolved work from prior sessions:
- brake candidate needs engine-off validation
- steering-angle candidate needs left/right symmetric sweep
- BH 0x4A2 interaction not reproduced

Highest-value stationary experiment:
repeat left indicator with driver door closed/open.
```

---

# 44. Human/AI interaction philosophy

The advisor is not there merely to confirm the user's guesses.

It should actively search for:

- contradictions;
- unexplained changes;
- alternative interpretations;
- interactions between functions;
- delayed effects;
- retransmitted signals;
- state-dependent meanings;
- cases where a candidate appears only because two actions were performed together.

Useful advisor language is concrete:

```text
Observed:
C2 0x18F bit 22 changed on 8/8 brake presses and 8/8 releases.

Missing evidence:
No test excludes headlight/stop-lamp interaction.

Suggested test:
Keep brake released. Toggle low beams 5 times. Then repeat brake 5 times
with low beams continuously ON.
```

Avoid vague statements such as "collect more data" when a specific discriminating test can be proposed.

---

# 45. Coding-agent instructions

The coding agent implementing this project should follow these rules.

## 45.1 Before coding

1. Inspect the current `baccable_lab` Git state.
2. Inspect the current `eujot/BACCAble` source used as protocol reference.
3. Read BACCAble `AGENTS.md`.
4. Read `docs/architecture/USB_DIAGNOSTICS.md`.
5. Confirm the 16-byte binary capture implementation in source, not only docs.
6. Do not change BACCAble firmware unless a concrete blocker is proven.
7. Start with Milestone 1.

## 45.2 Implementation discipline

- Make small vertical slices that run end-to-end.
- Add tests for parsers before using them on long sessions.
- Prefer working capture over architectural perfection.
- Do not generate dozens of empty modules prematurely.
- Add complexity only when the preceding milestone is hardware-accepted.
- Keep acquisition code simple and observable.
- Never silently discard malformed/lost data; record a system event.
- Avoid hidden automatic behavior that can transmit to the vehicle.

## 45.3 First implementation sequence

The fastest useful sequence should be approximately:

1. `pyproject.toml` and CLI entry point;
2. binary parser + tests;
3. one-port capture to raw file;
4. one-port parsed SQLite capture;
5. three-port concurrent capture;
6. role mapping / `doctor`;
7. live counters;
8. keyboard markers;
9. clean shutdown/summary;
10. real hardware smoke test;
11. 20–30 minute capture acceptance;
12. replay;
13. only then start vLinker integration.

## 45.4 Do not block on unknown optional details

If BLE details, voice engine selection, or advisor integration are not yet known, do not delay Milestone 1. Define interfaces and continue with capture.

---

# 46. Acceptance scenario for the first real product

On the actual MacBook:

1. connect BACCAble's three USB cables through the hub;
2. configure C1/C2/BH firmware USB mode as CAN capture;
3. run `baccable doctor` and verify mapping;
4. run `baccable capture --no-obd --no-voice`;
5. wake the vehicle and perform a stationary sequence;
6. use keyboard markers for:
   - unlock;
   - driver door open/close;
   - engine start;
   - brake ON/OFF repetitions;
   - left/right indicator repetitions;
   - engine stop;
   - lock;
7. stop capture;
8. inspect session summary;
9. query the SQLite database and confirm events and CAN records share a host monotonic timeline;
10. verify raw `C1.bin`, `C2.bin`, `BH.bin` exist and have non-zero size;
11. replay them through the parser;
12. confirm reported loss count is zero or, if not, that loss is explicit and attributable.

That is a successful first version.

---

# 47. Acceptance scenario for OBD integration

After Milestone 1 is accepted:

1. connect vLinker MS to the second OBD splitter branch;
2. ensure Car Scanner is disconnected;
3. `baccable obd scan` finds the adapter;
4. safe adapter identification succeeds;
5. supported standard PIDs are discovered;
6. capture starts with all three BACCAble buses plus OBD polling;
7. engine RPM and other values appear in the TUI;
8. raw request/response and decoded values are stored;
9. CAN capture remains healthy while BLE experiences a deliberate temporary disconnect;
10. OBD reconnect does not stop CAN acquisition.

---

# 48. Acceptance scenario for live advisor

Only after data collection and basic analysis are stable:

1. run a controlled repeated `brake_on/brake_off` experiment;
2. deterministic live summary finds candidate changing bits;
3. advisor receives compact context;
4. advisor suggests a meaningful follow-up, e.g. more repetitions or a negative control;
5. user accepts the test;
6. experiment is stored;
7. result updates hypothesis evidence;
8. CAN capture continues correctly even if the advisor process is killed.

---

# 49. Reference sources

The coding agent should verify these sources rather than relying on copied assumptions.

## BACCAble fork

- Repository: https://github.com/eujot/BACCAble
- Agent instructions: https://github.com/eujot/BACCAble/blob/master/AGENTS.md
- Action plan: https://github.com/eujot/BACCAble/blob/master/docs/ACTION_PLAN.md
- USB diagnostics/capture: https://github.com/eujot/BACCAble/blob/master/docs/architecture/USB_DIAGNOSTICS.md

## Upstream/reference project

- https://github.com/gaucho1978/BACCAble

## Alfa Romeo Giulia/Stelvio PID reference

- https://github.com/danardi78/Alfaromeo-Giulia-Stelvio-PIDs

Treat community PID definitions as references requiring verification on the actual vehicle.

## vLinker

- Vgate download center: https://www.vgatemall.com/downloadcenter/
- The user's vLinker MS is already confirmed operational with Car Scanner on this MacBook, which is more useful for initial compatibility than a generic product claim.

---

# 50. Final priority statement

When implementation decisions conflict, use this priority order:

1. **Capture the CAN data without loss.**
2. **Preserve raw evidence.**
3. **Make loss/failure visible.**
4. **Put sources on one reliable timeline.**
5. **Make event tagging fast and easy.**
6. **Add OBD reference values.**
7. **Add replay and offline analysis.**
8. **Add voice and video enrichment.**
9. **Add live AI/Codex guidance.**
10. **Generate and verify a DBC.**

Do not sacrifice items 1–4 to make items 5–10 more sophisticated.

The first release should already be useful after a few commands and should produce a session that remains valuable even if every later AI feature is rewritten.

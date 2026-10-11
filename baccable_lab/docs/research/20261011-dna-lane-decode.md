# DNA drive mode and lane-assist decode (October 2026)

## Why this note exists — and a correction

An earlier pass over the 2026-10-11 bundles concluded that the drive mode could
only be *observed*, not *set*, because it found **no separate request frame** for
the monostable DNA knob. That conclusion was wrong, and the mistake was
methodological:

- It looked for an **input/request** frame and ignored the frame the **body
  actually broadcasts**.
- It treated `0x384` as a passive *report*. In the firmware it is a body→ECU
  **command broadcast** whose consumers — TCM, IPC, ECM, DTCM, DCTM, DASM, CDCM,
  BCM on C1; HAL, ORC, EPS, BSM on C2 — act on its drive-mode field. BACCAble already **retransmits a modified copy** of this frame for the ESC/TC feature
  (`can_forward` in `vehicle/drive_mode.c`).

**A body broadcast that consumers obey is actionable.** The absence of a request
frame does **not** mean a function cannot be controlled — the actionable path is
to rewrite the broadcast the body emits. This lesson is now recorded in
[RESEARCH_PLAYBOOK.md](../RESEARCH_PLAYBOOK.md). It is the correction the owner
had to point out; the firmware `Default mode` option below reuses the existing
rewrite path.

## Sessions used

| Session | Bundle | Scenarios relevant here | Outcome |
| --- | --- | --- | --- |
| `20261010-104325Z` | `10paz1.json` | access, lighting | done (no DNA/LKA) |
| `20261010-111025Z` | `10paz2.json` | `dna_modes`, `lane_assist`, `wipers_speeds`, `rain_sensing`, … | **`dna_modes` and `lane_assist` were skipped**; wipers/rain done |
| `20261011-190746Z` | `dnskla.json` | `dna_all_modes`, `lane_assist_toggle` | done |
| `20261011-191050Z` | `dna.json` | `dna_all_modes` | done |
| `20261011-191202Z` | `kla.json` | `lane_assist_toggle` | done |

The October-10 campaign deliberately **skipped** `dna_modes` and `lane_assist`
(they were not exercised), so it carries **no** DNA or lane-assist ground truth;
that is exactly why the dedicated October-11 recordings were needed. Markers are
written **after** the action, so the settled state lies *after* each marker.

## Drive mode (DNA) — confirmed

The per-mode code is reported identically on three body frames and matches the
firmware enum already recorded for `dna_mode` / `drive_style_display`:

| Frame | Field | Natural | Dynamic | AllWeather | Race | Consumers |
| --- | --- | --- | --- | --- | --- | --- |
| C1/C2 `0x384` | byte 1 `[6:2]` | `0` | `2` | `4` | `12` | TCM, IPC, ECM, DTCM, DCTM, DASM, CDCM, BCM (C2: HAL, ORC, EPS, BSM) |
| BH `0x46C` | byte 7 `[4:0]` | `0` | `2` | `4` | `12` | IPC, ETM, ESEM |
| BH `0x25A` | byte 0 `[6:2]` | `16` | `8` | `4` | `1` | DriveStyle module (dashboard presentation) |

Example raw bytes `0x384` byte 1: N=`01`, D=`09`, A=`11`, R=`31`. All four modes
appear in both October-11 sessions (three runs each), reversible, against a known
Natural baseline. One run in `191050Z` did not actually reach Race (`0x384` stayed
at Dynamic) — a recording gap, not a decode issue.

**No input/request frame exists** for the mode: the monostable knob is read
locally by the body. The **actionable** frame is the body broadcast `0x384`
(and its display companions `0x46C` / `0x25A`); rewriting the field is how the
firmware `Default mode` option sets the mode, and how the existing ESC/TC feature
forces Race.

## Lane-keeping assist (LKA)

- **State:** BH `0x4F6` byte 6 bit 1 and C2 `0x547` byte 6 bit 1 (identical
  payloads, gateway-duplicated): `0x02` = enabled, `0x00` = disabled. It tracked
  **12/12** double-click toggles across the two lane sessions and also cleared
  whenever Race was engaged. The frames carry **no checksum** (byte 7 is constant
  `0x00`).
- **Control (momentary):** C2 `0x384` byte 3 bit 6 pulses briefly per click, so a
  double-click shows two pulses about 0.2 s before the state changes. On BH the
  same button is `0x5A0` byte 4 bit 0. This corrects an earlier claim that the
  double-click "was not captured" — it is captured on C2 `0x384`.

The state frame is described in [CAN_DICTIONARY.md](../CAN_DICTIONARY.md) as
`lane_assist_enabled`; the physical meaning still needs a dashboard-lamp check on
the car, and the firmware `Disable LKA` option clears this bit.

## Wiper-driven lights — still open, and why

The owner's third feature (dipped beam follows wiper activity) is **not yet
decodable** from the existing data:

- `10paz2` ran `wipers_speeds` (intermittent / slow / fast / off) and
  `rain_sensing` (soak / dry), but **no state bit survives** counter/confounder
  filtering with bounded windows (occupancy check at `≥0.85` separation returns
  nothing). The BH comfort frame `0x2F0` (byte 4 bit 4) remains the best wiper
  lead, and the rain-sensor frame is still unidentified.
- The dedicated `wipers_lights_auto` scenario (headlights in AUTO, daylight,
  wipers off → continuous → off, then auto mode with a few cycles per minute,
  marking the dipped-beam reaction) has **not been recorded** yet.

## Method corrections recorded

- **Separate the local stimulus from the CAN broadcast.** A momentary stalk
  button or monostable knob may be read locally, yet the body still *broadcasts*
  the resulting state — and that broadcast is what other ECUs obey and what the
  firmware can rewrite.
- **Bound every state window to the scenario span.** A final interval that runs
  to the end of the session poisons occupancy with unrelated later activity; an
  earlier run of this analysis produced a spurious "wipers/rain" bit this way.
- **Classify counters/CRC before scoring.** `0x4AE`, `0x15A`, `0x5A8`
  (counter + CRC), `0x341` (accumulator), `0x4B4`, `0x59E` (periodic heartbeats)
  and `0x749` (slow rotator) again dominate the raw ranking and must be excluded.

## Firmware review boundary, 2026-10-12

The mode bit encodings above match the implementation. `can_forward()` queues a
modified copy on the board's single CAN peripheral; it does not suppress the
original frame. The earlier inline-gateway wording was inaccurate. Captures of
received traffic alone do not establish how every consumer resolves competing
reports. The original DNA control evidence and existing ESC/TC behavior remain;
this review adds no vehicle acceptance.

The lane-state handler is now restricted to BH `0x4F6` / C2 `0x547`, standard data
frames with DLC 8. Clearing a correlated state bit is still an experimental
control method: actual assist deactivation remains unverified. No button pulse
sequence or new actuator command is inferred from the status decode.

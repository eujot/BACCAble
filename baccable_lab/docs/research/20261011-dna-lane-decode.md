# DNA drive mode and lane-assist decode (October 2026)

## Why this note exists — and a correction

An earlier pass over the 2026-10-11 bundles concluded that the drive mode could
only be *observed* because it found no separate request frame for the DNA knob.
That inference was too broad:

- It looked for an **input/request** frame and ignored the frame the **body
  actually broadcasts**.
- It treated `0x384` as a passive *report*. The firmware describes it as a
  body→ECU broadcast to TCM, IPC, ECM, DTCM, DCTM, DASM, CDCM and BCM on C1;
  HAL, ORC, EPS and BSM on C2. BACCAble retransmits a modified copy for the
  ESC/TC feature (`can_forward` in `vehicle/drive_mode.c`).

The broadcast may influence its consumers, but retransmitting a copy does not
prove that the body changes its own selected mode. The beta-22 vehicle test showed
a Dynamic lamp with a faint Natural lamp during repeated rewriting. The one-shot
`Default mode` behavior still needs a vehicle test; see
[RESEARCH_PLAYBOOK.md](../RESEARCH_PLAYBOOK.md) for the distinction between a
local input and a broadcast report.

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

No separate knob request frame was identified on the recorded C1/C2/BH buses.
The recordings do not show the physical path from the selector to the body. The
body broadcast `0x384` (and its companions `0x46C` / `0x25A`) repeats the selected
mode; a rewritten copy can influence listeners, but the beta-22 vehicle test
did not establish that one injected copy changes the body's stored mode.

## Lane-keeping assist (LKA)

- **State:** BH `0x4F6` byte 6 bit 1 and C2 `0x547` byte 6 bit 1 (identical
  payloads, gateway-duplicated): `0x02` = enabled, `0x00` = disabled. It tracked
  **12/12** double-click toggles across the two lane sessions and also cleared
  whenever Race was engaged. The frames carry **no checksum** (byte 7 is constant
  `0x00`).
- **Button (momentary):** C2 `0x384` byte 3 bit 6 pulses briefly per click. On BH
  the same button is visible in `0x5A0` byte 4 bit 0. For example, in
  `20261011-191202Z` the C2 button report changes from `08 09 8A 80 00 00 00 00`
  to `08 09 8A C0 00 00 00 00` at 24.207 s and returns to released at 24.316 s.
  The enabled-state bit changes to OFF at 24.528 s and continues in later
  reports. This corrects an earlier claim that the button was not captured.
  The state sometimes changes between the two observed presses; the capture
  therefore does not prove which press the controller acts on.

The state frame is described in [CAN_DICTIONARY.md](../CAN_DICTIONARY.md) as
`lane_assist_enabled`; the physical meaning still needs a dashboard-lamp check on
the car. The experimental `Disable LKA` start-up action now sends one C2 `0x384`
pressed report and a released report 100 ms later, only when C2 `0x547` reports
enabled. It no longer rewrites the recurring state frames. Whether this pulse
reaches the controller and turns assistance off needs a vehicle test.

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

The earlier lane-state rewrite was restricted to BH `0x4F6` / C2 `0x547`, standard
data frames with DLC 8. Beta-22 testing found that its continuous rewrite blocked
later driver re-enabling. The subsequent one-shot status rewrite was replaced by
the C2 button pulse described above. Actual deactivation by that pulse remains
unverified; the received captures alone do not identify the controller's input.

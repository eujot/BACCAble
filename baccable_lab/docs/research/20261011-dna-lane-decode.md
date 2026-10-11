# DNA drive-mode and lane-assist decode: recordings from October 11, 2026

Three guided-scenario sessions were recorded on the same day and analysed from
their `baccable bundle` artifacts and the raw `session.sqlite3` frames:

| Session | Scenarios | Confirmations | Bundle |
| --- | --- | --- | --- |
| `20261011-190746Z` | `dna_all_modes`, `lane_assist_toggle` | 18 | `dnskla.json` |
| `20261011-191050Z` | `dna_all_modes` | 12 | `dna.json` |
| `20261011-191202Z` | `lane_assist_toggle` | 6 | `kla.json` |

Markers are written **after** the action, so the first half of every interval
between two markers is the confirmed state. Frame-structure classification (drop
counter/CRC frames before scoring) followed [RESEARCH_PLAYBOOK.md](../RESEARCH_PLAYBOOK.md).

## DNA drive mode — confirmed

The per-mode code is reported identically on three body frames and matches the
firmware enum already recorded for `dna_mode` / `drive_style_display`:

| Frame | Field | Natural | Dynamic | AllWeather | Race |
| --- | --- | --- | --- | --- | --- |
| C1/C2 `0x384` | byte 1 `[6:2]` | `0` | `2` | `4` | `12` |
| BH `0x46C` | byte 7 `[4:0]` | `0` | `2` | `4` | `12` |
| BH `0x25A` | byte 0 `[6:2]` | `16` | `8` | `4` | `1` |

Example raw bytes `0x384` byte 1: N=`01`, D=`09`, A=`11`, R=`31`. All four modes
appear in two sessions (three runs each), reversible, with a N baseline; one run
in `191050Z` did not actually reach Race (`0x384` stayed at Dynamic), a recording
gap rather than a decode issue.

`0x384` is a **body broadcast to other ECUs** (TCM, IPC, ECM, DTCM, DCTM, DASM,
CDCM, BCM on C1; HAL, ORC, EPS, BSM on C2), and the firmware already rewrites and
forwards it for the ESC/TC feature. There is **no separate DNA request frame** —
the monostable knob is read locally by the body.

## Lane-keeping assist — candidate state and control

- **State:** BH `0x4F6` byte 6 bit 1 and C2 `0x547` byte 6 bit 1 (identical
  payloads, gateway-duplicated): `0x02` = enabled, `0x00` = disabled. It tracked
  **12/12** toggles across the two lane sessions and also cleared with Race.
- **Control (momentary):** C2 `0x384` byte 3 bit 6 pulses briefly per click, so a
  double-click shows two pulses about 0.2 s before the state changes. On BH the
  same button is `0x5A0` byte 4 bit 0.
- These frames carry **no checksum** (byte 7 is constant `0x00`).

The state frame is now described in the dictionary
([CAN_DICTIONARY.md](../CAN_DICTIONARY.md)) as `lane_assist_enabled`; the physical
meaning still needs a dashboard-lamp confirmation on the car.

## Tooling notes

- `0x4AE`, `0x15A`, `0x5A8` (counter + CRC), `0x341` (accumulator), `0x4B4`,
  `0x59E` (periodic heartbeats) and `0x749` (slow rotator) again dominate the raw
  candidate ranking and must be excluded before scoring.
- This decode drove two firmware options: **`Default mode`** (OFF / A / D / Race)
  and **`Disable LKA`**; see `firmware/baccable/vehicle/drive_mode.c` and the
  action plan.

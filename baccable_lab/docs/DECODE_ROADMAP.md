# CAN decode roadmap

Status: drafted 2026-10-24 from the dictionary `2026-10-24.8`, the hypothesis
ledger and the session inventory. This is the **operational checklist** for
turning the remaining undecoded traffic into decoded, evidenced signals.

Work top-down. Each campaign is self-contained: it names the scenarios to run
(most already exist), the hypothesis it closes, the analysis, and the exact
definition of done. Keep the evidence-first rules in
[SESSION_ANALYSIS.md](SESSION_ANALYSIS.md) and
[RESEARCH_PLAYBOOK.md](RESEARCH_PLAYBOOK.md); do not re-derive them here.

## How to run a campaign

1. **Record first.** Run the named scenario(s) in the car, one control at a
   time, and confirm every step when the effect is actually visible. Never add a
   time target that the operator cannot honour.
2. **Analyse with the negative-claim protocol.** Match by `(bus, ID, DLC)`,
   search every bus, bound windows away from the event onset, and reject
   counters/blinkers by confirming the field is quiet on a no-action baseline
   (the `0x41B` trap: a byte that *looks* like a window flag was a blinking
   counter).
3. **Record evidence.** Update `hypotheses.json` (status + contradictions +
   evidence) and `giulia.json` (signal `origin`, `note`, `marker_rules`,
   `manual_marker_evidence`), then regenerate the catalog and update
   `docs/ACTION_PLAN.md`.
4. **Verify.** `cd baccable_lab && python3 -m unittest discover -s tests` must
   stay green (81 tests). If a decode adds a user-visible reading, update both
   user guides in the same change.

Every campaign ends in one of three statuses per hypothesis: **supported**
(with a recorded method), **refuted** (with its method and contradictions), or
still **candidate/unresolved** (with the missing evidence named). A negative
claim is written "not demonstrated with this method in these recordings", never
"does not exist".

---

## Phase 1 — Parse the Racelogic REF channels (done — no result)

**Status: completed 2026-10-24 — no new channels.** The downloaded REF
(SHA-256 `85248788…`) decompresses to **9 unique signals** plus 4 mph
duplicates, and every one is already handled:

| Channel | ID | Result |
| --- | --- | --- |
| Steering_Angle | `0x0DE` | already supported |
| Accelerator_Pedal_Position, Engine_Speed | `0x0FC` | already supported |
| Brake_Position | `0x101` | already supported |
| Wheel_Speed RR/RL/FR/FL | `0x0EE` | already supported |
| Seat_Belt_-_Driver | `0x257` | rejected (fails the marker check) |

The "nine broadcast channels" named in the PDF are exactly these nine, not
nine IDs with some left unread. The high-volume undecoded block (`0x0F0`,
`0x0F4`, `0x0F9`, `0x0FE`, `0x0FF`, `0x100`, `0x103`, `0x107`, `0x118`, `0x11A`,
`0x120`, `0x122` and the `0x1Fx` ESP cluster) is **not** in the Racelogic source
and must come from the vehicle campaigns below.

---

## Phase 2 — Body campaign (windows, doors, closures, locks, one lamp at a time)

**Goal.** Promote the body hypotheses that are already candidate, and finally
find the window field.

**Scenarios (already exist):** `window_driver`, `window_passenger`,
`door_driver`, `door_passenger`, `boot_lid`, `bonnet`, `fuel_door`,
`lock_unlock`, `mirror_fold`, `seat_belt_driver`; lighting:
`side_lights`, `dipped_beam`, `high_beam`, `fog_lights`, `interior_lights`.

**Targets and method**

| Hypothesis | Frame / field | Run and check |
| --- | --- | --- |
| BODY-CLOSURE-001 | C1 `0x116` byte 2 bit 2 (boot, active-low), byte 3 bit 1 (bonnet, active-low) | `boot_lid` and `bonnet` separately, with the other closed; confirm the active-low sense on both edges |
| BODY-CLOSURE-002 | BH `0x3DE` companion | same sessions, confirm it mirrors `0x116` |
| BODY-DOORLOCK-001 | C2 `0x0F1` byte 3 (lock/door) | `lock_unlock` then `door_driver`/`door_passenger`; separate lock state from per-door ajar |
| BODY-LIGHTFLAG-001 | C1 `0x0FA` byte 0 (one-hot lamp) | run `side_lights`, `dipped_beam`, `high_beam`, `fog_lights` **one at a time** and read byte 0 raw; map each bit to its lamp |
| BODY-WINDOW-001 | (unresolved — no field yet) | `window_driver` and `window_passenger` separately, 3 runs each, no other control touched; look for a field that **latches** during open and clears during close and is **silent** on the no-action baseline |

**Analysis notes.** The window field is the hard one: previous recordings mixed
driver/passenger markers and the generic byte-flip scan returns counters
(`0x41B`, `0x0FB`, `0x341`, `0x2E6`, …). Before accepting any bit, prove it is
constant on a quiet idle baseline, not a ~1 s blinker. If still not found after
two disciplined runs, record that as a refutation-with-method and move on.

**Done when.** BODY-CLOSURE-001/002, BODY-DOORLOCK-001 and BODY-LIGHTFLAG-001
are `supported` (or refuted with method), and BODY-WINDOW-001 has either a
supported field or a written refutation. `marker_index` reflects the new
decoders.

---

## Phase 3 — Left direction indicator (close the symmetric gap)

**Goal.** Promote LIGHT-INDICATOR-C2-42E-001.

**Context.** C2 `0x42D` (right) is already supported; `0x42E` byte 1 bit 2 is
the symmetric left candidate, but only one independent left recording exists.

**Steps**

1. Run `indicator_left` twice in a separate stationary session, observing the
   physical lamp.
2. Compare `0x42E` byte 1 bit 2 against BH `0x46C` and C2 `0x73E`; verify it
   stays clear on `indicator_right` and rises with `hazard_lights`.

**Done when.** `0x42E` is `supported` with a note that it is an active-state
decoder, not a blink-phase decoder, and the residual "high right after OFF"
frames are explained as release latency or marker timing.

---

## Phase 4 — Brake separation: pedal vs parking brake

**Goal.** Promote BODY-BRAKE-001 and BODY-BRAKE-002 and stop the generic
brake/EPB conflation.

**Scenarios:** `brake_pedal`, `parking_brake`, `brake_lights_pedal`.

**Steps**

1. Pedal-only cycles with EPB unchanged: `brake_pedal` → C1 `0x104` byte 2 bit 6
   and `0x100` byte 0.
2. EPB-only cycles with the pedal held: `parking_brake` → C1 `0x384` byte 3 bit 3.
3. Inspect C2 separately; do not reuse the C1 layout on C2.

**Done when.** Pedal lead and park-brake report are separated and each is
`supported` (or one is refuted with a written method).

---

## Phase 5 — One clean drive (ESP cluster and scale verification)

**Goal.** Open the untouched `0x1Fx` ESP/chassis cluster and verify the public
source scales.

**Context.** The only recorded drive (`20260928-203436Z`) is truncated. `0x1F1`,
`0x1F2`, `0x1F5`, `0x1F6`, `0x1F7`, `0x1FA` (C1) and `0x1F6` (C2) have no
hypothesis at all. `0x7D0` (C1, DLC 1) is unexplained.

**Steps**

1. Run the driving scenarios (`drive_steer`, `drive_accelerate`, `drive_brake`,
   `drive_speed_change`) to produce a clean, non-truncated drive capture. Add an
   independent speed reference if one is available.
2. Correlate each `0x1Fx` byte against speed, brake and steering; the cluster is
   adjacent to `0x1F5` (brake pressure), so longitudinal/lateral acceleration,
   yaw and ESP state are the prime suspects.
3. Re-check the known scales: `0x0EE` wheel vs `0x101` speed (~5.7 %), and the
   `0x0DE` steering sign against a labelled physical left/right turn.
4. Sample `0x7D0` over idle and motion to see whether it is a keepalive, a wake
   flag or a status.

**Done when.** Each `0x1Fx` ID has at least a candidate field + `next_test`
documented, the steering sign is resolved, and `0x7D0` has a written hypothesis
or refutation.

---

## Phase 6 — Close the remaining single-control hypotheses

Smaller, independent items that each need one focused recording or a clean
separation of two controls.

| Hypothesis | Frame | What to do |
| --- | --- | --- |
| WASHER-2EC-001 | C2 `0x2EC` byte 4 bit 4 | run `washer` and distinguish a spray-only pull from a single manual wipe before using it to suppress washer wipes |
| IPC-LIGHT-001/002/003 | BH `0x46C` b5.1, C1 `0x545` b4.7, C2 `0x73E` b3.6 | keep lamps steady while changing IPC brightness (or vice versa) to separate illumination state from a brightness value |
| DNA-RELATED-4AE-001 | C1 `0x4AE` byte 3 | run `dna_all_modes` and `lane_assist_toggle` separately to split the Race code from the LKA state |
| DNA-RELATED-73C-001 | C1 `0x73C` byte 1 | correlate `0x07`/`0x9F` with ACC availability from `adaptive_cruise`/`cruise_control` |
| COUNTER-SUSPECT-004 | C1 `0x341` | sample over a long idle baseline; confirm the two 16-bit fields are time counters before accepting any state bit |

**Done when.** Each is `supported`, `refuted`, or explicitly left `candidate`
with the missing evidence named.

---

## Phase 7 — Diagnostics DIDs (gated)

**Goal.** Optionally expand the `Readings` catalog from the 90 external
`(ECU, DID)` pairs in the owner CSV (TCM DNA, ECM selected-gear `0x192D`, BCM
light-switch `0x0133`/outside-temp `0x013C`, IPC brightness `0x0104`).

**Gate.** This needs ISO-TP multi-frame handling, which was deliberately removed
with the Experiments menu. Treat it as a separate decision, not a default step.
If pursued: test one read-only `0x22` request per ECU/DID, record the full
positive/negative ISO-TP reply and gateway behaviour, then add firmware readings
one at a time.

---

## Tracking

| # | Campaign | Status | Result |
| --- | --- | --- | --- |
| 1 | Racelogic REF parse | ✅ done — no result | REF defines only 9 signals, all already mapped |
| 2 | Body campaign | ▶️ next | |
| 3 | Left indicator `0x42E` | ☐ todo | |
| 4 | Brake pedal vs EPB | ☐ todo | |
| 5 | Clean drive / ESP | ☐ todo | |
| 6 | Single-control closures | ☐ todo | |
| 7 | Diagnostics DIDs | ☐ gated | |

Update this table and `docs/ACTION_PLAN.md` after each campaign. Keep the two
user guides in sync whenever a decode surfaces a reading.

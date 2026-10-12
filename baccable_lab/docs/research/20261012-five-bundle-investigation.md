# Five-bundle CAN investigation (October 12, 2026)

This review reuses the owner's five JSON bundles and checks their ranked claims
against the original SQLite frames in read-only mode. Byte indexes start at zero
and bit 0 is the least-significant bit. The bundles are input artifacts, not
proof that a ranked bit has a vehicle meaning. No CAN transmission was made.
Firmware CAN comparison uses integrated baseline `7015b40`; the Readings
changes were merged subsequently in PR #55. The cumulative
[review ledger](20261012-five-bundle-review.json) covers 20 sessions and
21,354,624 frames, including original input fingerprints. Its dictionary
snapshot is `2026-10-12.1`; the resulting curated dictionary is `2026-10-12.4`.

| Bundle | Raw session | Frames | Scope and quality |
| --- | --- | ---: | --- |
| `10paz1.json` | `20261010-104325Z` | 3,861,290 | 19 access/lighting scenarios, 114 done; 8,186 BH frames lost at startup |
| `10paz2.json` | `20261010-111025Z` | 6,760,480 | 59 scenarios, 227 done/124 skipped; DNA/LKA skipped; 437 BH frames lost at startup |
| `dnskla.json` | `20261011-190746Z` | 950,504 | DNA/LKA, 18 done; C1 startup loss saturated at 65,535 (lower bound), BH 4,869 |
| `dna.json` | `20261011-191050Z` | 368,781 | DNA, 12 done; C2 startup loss 10,051, 11 resyncs/16 malformed records, BH 806 |
| `kla.json` | `20261011-191202Z` | 293,926 | LKA, 6 done; BH startup loss 373 |

All five bundles have empty `context`: vehicle model year, engine, firmware
binary, conditions and audio source remain unknown. They embed an older firmware
source hash (`7dff7fe`) and dictionary versions `2026-10-02.2` or
`2026-10-11.1`; percentages of mapped traffic (14.3–16.0%) describe those
snapshots, not the current checkout. Raw session inventory contains 276, 276,
232, 231 and 231 `(bus, ID, DLC)` shapes respectively. Every one is already
present in the current dictionary's **observed inventory**. This review finds a
new meaning in previously unmapped frames, rather than a previously unseen ID.

Bundle SHA-256 provenance, in table order: `b4bb84d16f1e005c2aed4e74be137da9636e86685a5b8c690d77011f70259cb3`,
`80bb459de8892dafc7b9dae54d22c119844f4cb2e0612bc29353f40b73a93f6a`,
`d3aa6be270bfb909956cd93afbfe42a2a4541d2c2f0ab7aaf0b3655c0ff87786`,
`3f4c5d1e0bed0b608af742de132793736e955d1d8d7c393d189bd22306686086`,
`b7e6d3709cd3772102dfcfa38468fd14ba41b0b32d0e6c80430c74dcb02a60ea`.

## Strong new finding: paired C2 direction-indicator frames

`C2 0x42D` and `C2 0x42E` are separate DLC-5 messages at about 100 ms while
present. In both, byte 1 bit 2 follows the active side. October event IDs 85–90
are three hazard ON/OFF cycles; 97–102 are three left cycles; 103–108 are three
right cycles. In settled 0.3–2.0 s windows, `0x42D` is active in **17/17**
samples after each hazard ON and **17/17** after each right ON; it is clear in
all three hazard OFF windows and all three right OFF windows. It stays clear
through the left ON windows. `0x42E` is active in 17/17 samples after each
hazard ON and each left ON, and clear throughout all right ON windows. A few
left OFF windows retain a short active tail (5/16 samples after event 100),
so timing around release needs care.

The independent `20260928-204100Z` right-indicator session (events 2–5) uses
different byte-0 codes but confirms the stable bit: `0x42D` has byte 1 bit 2
in 13/17 and 17/17 samples in the two right ON windows. Clip the first OFF
window at the next marker minus 0.3 s: the next ON arrives only 1.318 s later,
so a fixed two-second window would include it. This bounded OFF window still
has 3/8 active samples; the second has 4/17. A separately inspected 3–4 s
window after the final OFF is clear in 11/11 samples. `0x42E` is clear in all
four bounded windows. Thus the right association has
independent support, with a recorded timing caveat. The left association has
three clean October cycles but lacks an independent left-labelled session.

Raw examples show why a whole-byte comparison would miss cross-session support:

| Session / state | `0x42D` | `0x42E` |
| --- | --- | --- |
| October, right ON | `05 04 07 06 00` | `25 00 07 1B 00` |
| October, left ON | `25 00 01 55 00` | `05 04 01 48 00` |
| October, hazard ON | `05 04 0E F3 00` | `05 04 0E F3 00` |
| October, both OFF | `25 00 0C D4 00` | `25 00 0C D4 00` |
| September, right ON | `04 04 ...` | `44 00 ...` |

The low nibble of byte 2 increments modulo 16; byte 3 varies with it. Do not
score these as indicator bits. Byte 0 also differs by session/configuration
(`0x05/0x25` versus `0x04/0x44`). The robust observation is byte 1 bit 2 on
each **bus-specific ID**. The recordings do not distinguish a switch request,
a requested lamp state, and physical lamp feedback. `0x42D` is now a supported
capture field; `0x42E` remains a candidate pending independent left/physical
confirmation.

The firmware already handles indicator information elsewhere: BH `0x46C`
byte 6 bits 2:1 are read by `vehicle_handle_body_drive_mode`; `0x354` and
`0x73E` carry lighting reports/animation fields in `lighting_frames.c`.
`standard_frames.c` dispatches none of `0x42D`, `0x42E`, `0x0FA` or `0x0F1`.
The new pair therefore adds a potentially steadier **C2 indication source**;
it is not a reason to change firmware output or control behavior yet.

## Rechecked existing decodes and rejected shortcuts

The DNA/LKA bundles independently reproduce the existing mappings: C1/C2
`0x384` byte 1 bits 6:2, BH `0x46C` byte 7 bits 4:0 and BH `0x25A` byte 0
bits 6:2 report Natural/Dynamic/AllWeather/Race with the codes already in
`drive_mode.c`. LKA state is BH `0x4F6` / C2 `0x547` byte 6 bit 1, with the
momentary stalk button on C2 `0x384` byte 3 bit 6 / BH `0x5A0` byte 4 bit 0.
The prior [DNA/LKA study](20261011-dna-lane-decode.md) documents the twelve
LKA toggles and physical-control limit. The October-10 bundles **skipped** DNA
and LKA, so they provide no extra controlled repetition for these features.

Several high-ranked bundle bits fail a complete state-interval comparison:

| Claim | Raw check | Decision |
| --- | --- | --- |
| BH `0x2F0` byte 4 bit 4 = wipers | Across three intermittent/slow/fast/off runs, occupancy is `0.70/0/0.75/1.00`, `0/0.09/1.00/0`, `0/0.22/1.00/1.00`. Raw bytes drift `...28 7E 38 52` → `...28 4A 38 08` → `...28 34 37 EA` even across changes in wiper state. | Historical lead refuted. The frame contains slow-changing data, but its physical meaning is unknown. |
| BH `0x2F0` byte 4 bit 5 = rain | Two rain/dry pairs show 0.85/0 and 0.88/0, but the same bit is high during both wiper OFF and active intervals and low during others. | Coincidence with slow drift/order; no rain decoder. |
| C1/C2 `0x257` byte 2 bit 5 = belt fastened | Fastened occupancy on C1 is 0.52/0.60/0.75; unfastened is 0.79/0.72/1.00. C2 is similar. | Contradicts the proposed polarity; do not promote. |
| C1 `0x419` byte 2 bit 5 = driver door | Driver open/close occupancy in the first cycle is 0.60/0.56 and in the second 0.53/0.36; passenger intervals give a useful negative control but do not repair the driver inconsistency. | Candidate frame only; do not name this bit. |
| C1 `0x5E0` byte 2 bit 3 = brake hold | Three active intervals give 0.67/0.77/0.83; following boundaries give 0.50/0.33/0.00. | Interesting but not a clean state and potentially confounded by brake pedal use. |
| C2 `0x0F1` byte 3 bit 6 = lock | Three lock intervals give 0.81/0.70/0.92; unlock 0.19/0/0. | Strengthens the previous candidate, but movement/settling and only one session prevent promotion. |

`C1 0x0FA` byte 1 bits 0/1 also move with right/left markers, but occupancy
about 0.6–0.8 in ON intervals makes them less clear than `0x42D/E`. Existing
`C1/C2 0x116` boot/bonnet, `C1 0x104` brake, `C1/C2 0x1FC` engine-running
and climate candidates remain at their earlier evidence level. The ranked
`0x4AE`, `0x15A`, `0x5A8` counter/check bytes and `0x341` drifting fields are
still false-positive traps. These do not justify new firmware decoders.

## Next smallest discriminating recording

For the left frame, repeat only `indicator_left` in a new stationary session,
marking ON and fully cancelled OFF after the lamps settle, and note the visible
lamp state. To distinguish `0x42D/E` request from feedback, capture switch
activation, visible blink phase, and the BH `0x46C` / C2 `0x73E` timing in the
same recording. For wipers/rain, use the existing `wipers_lights_auto` scenario
with long equal OFF/intermittent/continuous intervals and physically observe
both wiper and dipped-beam behavior. No broad repeat of all five bundles is
needed. The updated catalog, report provenance and new decoder's real-payload,
bus-isolation and exact-DLC checks pass the 70-test Lab suite.

# Rain, wipers and automatic lighting: cross-session review

The recordings support a **wiper-cycle phase on C2 `0x2EC`** and separate
**AUTO mode** from **dipped-beam indication**. This is sufficient to design the
input condition for wiper-triggered lighting. Raw rain intensity, exact
one-edge-per-physical-sweep semantics and injected lamp control remain unverified.

## Evidence and capture health

The target `20261024-105643Z` has 36/36 confirmations of `rain_lights_auto`
over three runs, lasts 409.903 s and contains 2,157,769 frames across 232
bus/ID/DLC identities: C1 1,104,128; C2 971,006; BH 82,635. Replaying the raw
streams gives exactly the SQLite counts. C2 recorded 26,085 dropped frames and
11 resynchronizations/16 malformed bytes at startup; BH recorded 2,167 startup
drops. Recorded later loss counts are zero, which does not prove an error-free
capture. Vehicle details and firmware identity are absent from the empty context.

The targeted inventory covers all 24 databases through the target session ID:
23,535,295 frames, including failed/empty recordings. Newer sessions are excluded.
Decisive controls are `20261010-111025Z` (three wiper speed/OFF series, events
12–23), `20261010-104325Z` (AUTO pairs 61–66, manual dipped pairs 73–78 and
fog/high-beam controls), and `20260928-204607Z` (automatic-lighting pair 10–11).

[Targeted evidence](20261024-rain-lights-evidence.json) preserves original events,
notes, frame IDs, both clocks, inventories and SHA-256 hashes. All 24 inputs
passed the before/after fingerprint check. SQLite was read-only with WAL content
included; no capture or marker was edited. The [cumulative ledger](20261024-rain-lights-ledger.json)
retains the prior 20-session review by immutable path/hash and adds the fresh
target review. Its inherited snapshot is explicit; historical detailed reports
remain unchanged. That ledger has 21 sessions; the targeted inventory additionally
covers the three intervening short/failed recordings.

Bytes are zero-based; bit 0 is the LSB. Host times below are relative to the
first target frame. Timing within one board uses its device clock; no device
clock subtraction across buses or definitive physical ECU attribution is made.

## Wiper phase: C2 0x2EC, DLC 8, byte 2 bit 5

The message has a median 50 ms period and maximum 54 ms gap in the target.
Its cycle field alternates between these observed payloads:

```text
00 00 00 00 00 00 00 00   low phase
00 00 20 00 00 00 00 00   high phase
```

The new session contains 28 complete pulses. The three manual wiping bouts
contain 6, 6 and 7 rises, spaced approximately 1,500 ms apart. Confirmation after
three sweeps does not stop the blades; the entire recorded bout therefore cannot
be equated to the instruction's three sweeps.

| Target manual bout, host seconds | First high frame ID | Typical rising-edge interval |
| --- | ---: | ---: |
| 167.887–176.096 | 884114 | 1,500 ms |
| 286.940–295.473 | 1510975 | 1,500 ms |
| 385.358–395.403 | 2029346 | 1,500 ms |

The independent old speed series repeat slow intervals of 1,499–1,501 ms and
fast intervals of 949–951 ms. The phase returns low after OFF. That session has
44 rises: 43 in the rain/washer/wiper portion and one unlabelled excursion at
403.85 s, retained without assigning a physical action. The old light-only
controls and September automatic-lighting pair have no high cycle samples.

At all three new `rain_detected` confirmations (events 7, 19, 31), the nearest
frame is already low: IDs 809065, 1373331 and 1958141. Fixed post-marker
occupancy can thus miss the signal. Firmware would need edge counting and a
last-activity timeout. This is a phase associated with wiping, not a continuously
asserted enable/rain flag. A motor request, park-position phase and another
BCM wipe phase cannot yet be distinguished without physical sweep observation.

Byte 4 bit 4 is only a washer/stalk candidate. It pulses with the new spraying,
but stays clear during old `washer_used` steps 6–11 and has an unexplained
excursion during the old high-beam procedure. It cannot safely exclude washer
wipes. Selecting AUTO on a dry screen also produces a single cycle in the new
trials, so one rise alone is unsuitable as a sustained-rain trigger.

## AUTO selection and dipped-beam indication

| Bus, identity | Field | Supported meaning |
| --- | --- | --- |
| BH `0x354`, DLC 4 | byte 3 bit 6 | AUTO-lighting mode indication |
| C2 `0x73E`, DLC 4 | byte 3 bit 6 | Corresponding AUTO indication |
| BH `0x354`, DLC 4 | byte 1 bit 6 | Dipped-beam indication |
| C2 `0x73E`, DLC 4 | byte 1 bit 6 | Corresponding dipped-beam indication |
| C1/C2 `0x384`, DLC 8 | byte 3 bit 7 | Additional dipped-associated report |

New AUTO remains set while lamps switch ON/OFF three times. Older daytime AUTO
selection switches byte 3 bit 6 with the dipped field clear. Older manual dipped
selection switches byte 1 bit 6 with AUTO clear. The September pair changes both
at once and could not resolve their meanings alone.

```text
BH 0x354: 00 00 00 40  AUTO, dipped off
BH 0x354: 02 40 80 40  AUTO, dipped on, associated position-lamp reports
C2 0x73E: 00 00 00 40  AUTO, dipped off
C2 0x73E: 00 40 00 40  AUTO, dipped on
```

BH dipped changes occur at 74.194/112.861, 208.513/239.737 and
316.261/347.057 s. Mirrored reports are corroboration, not independent physical
lamp measurements. C1 `0x73E` uses `02 00 80 00` for associated position-lamp
fields; the same numeric ID does not permit copying the C2 layout onto C1.
The dictionary supersedes `auto_low_beam_report` with `auto_lights_mode_report`
and retains the old interpretation and marker history.

## Raw light-response candidates: BH 0x738 bytes 1 and 2

These separate raw bytes usually read `6/6` in target daylight and `0/0` while
the covered sensor causes darkness. The period is about 1,000 ms. They recover
before the dipped indication goes off:

| Run | Both first zero → dipped ON, BH clock | Both return to 6 → dipped OFF, BH clock |
| --- | ---: | ---: |
| 1 | 1,137 ms | 19,904 ms |
| 2 | 538 ms | 14,803 ms |
| 3 | 298 ms | 14,144 ms |

Sampling is one second, so these are frame intervals rather than precise
physical sensor latency. Run 3 includes intermediate values; a short channel-2
dip also occurs during one spray phase. Older recordings have baselines such as
`3/3`, `4/5` and `0/0`. Units, scale, physical source and universal thresholds
remain unknown. The older manual-lamp trials leave these channels unchanged.

The earlier scenario-branch handoff dismissed the whole BH `0x738` as a
monotonic counter. That was too broad: its light-response candidates must be
examined separately. C1 `0x4B1` already contains body/door fields and `0x4B2`
oil-temperature fields in the dictionary. Their whole-frame variation does not
establish a wiper signal either. C1 `0x41A` byte 2 mask `60` follows manual
wiping but also all three fog/high-beam trials while the C2 phase stays clear;
its electrical/battery relationship remains unresolved. The older BH `0x2F0`
rain/wiper hypotheses remain refuted.

## Firmware readiness and the smallest remaining check

A trigger can use fresh C2 `0x2EC` edges and the AUTO indication on C2 `0x73E`.
An example policy is three rises in ten seconds followed by a hold period after
the last wipe. These are proposed thresholds, not observed factory behavior.
Manual lighting selection needs priority; stale-data and release behavior must
avoid lamp cycling. Raw rain intensity is unnecessary for a feature based on
actual wiper activity. Washer exclusion remains unresolved.

The output path needs a small stationary vehicle check. `lighting_frames.c`
contains a light-animation sequence transmitting a low-beam-shaped `0x73E`
through `can_forward`; that function transmits on the current board's own CAN
interface. Source comments and RX-only recordings do not establish that sustained
injection operates dipped beams, rear position lamps and all relevant modules.
They do not identify a separate BCM request or physical transmitting ECU.

The useful next check is visible front/rear lamp observation with recorded
feedback during a narrowly scoped output test, including factory darkness and
manual override. Another full sensor/wiper recording is unnecessary merely to
find an input trigger. No actuator firmware or CAN injection was added here.

## Reproduction and validation

From the repository root:

```sh
python3 baccable_lab/scripts/analyze_rain_lights.py \
  --sessions /path/to/original/baccable_lab/sessions \
  --output /tmp/new-rain-lights-evidence.json
```

The script refuses an existing output or a path inside the captures, preserves
both clocks, checks fingerprints, and replays the target raw streams. The
cumulative ledger inherits old detailed results and intersects their stable-bit
templates with the fresh target profile. Dictionary `2026-10-24.2` adds the
wiper phase and reviewed light fields; optical and washer interpretations stay
hypotheses. All 75 Lab tests pass, including real captured payloads, independent
AUTO/lamp states, exact lengths, bus isolation, source hashes and generated
catalog consistency. No firmware or hardware test is claimed.

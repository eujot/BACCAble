# Follow-up: sensor isolation and delayed dipped-beam release

Session `20261024-111510Z` adds 28/28 `sensor_wiper_lights` confirmations over
two runs and 1,644,370 frames (C1 841,214; C2 740,094; BH 63,062). Context now
identifies owner-reported Giulia 2.0, v5-beta-29, daylight and outdoors.
[Original events, field traces and fingerprints](20261024-111510-rain-lights-evidence.json)
retain the source evidence. Raw replay matches SQLite on all buses and hashes
are unchanged. C1/C2 startup drop counts of 65,535 are saturated lower bounds;
BH reports 20,897 startup drops. Later recorded loss is zero. Some initial
C1/BH frames contain an old device-clock segment followed by a large jump;
all relevant action timing below is after startup and within one board clock.

## New discriminating result: sensor covered with lamps manually OFF

BH `0x738` bytes 1 and 2 go from `6/6` to `0/0` while AUTO is off and the dipped
report remains off. This happens twice:

| Run | Both channels zero, host seconds | Both return to 6 | Relevant events |
| --- | ---: | ---: | --- |
| 1 | 23.888 | 48.881 | 1–3: bright, covered, lamps still off |
| 2 | 188.879 | 202.981 | 15–17: bright, covered, lamps still off |

This independently supports light-responsive raw channels distinct from lamp
status. The hypothesis is promoted from candidate to supported **raw response**;
physical sensor source, units, scale and universal thresholds remain unknown.
Neither channel is a raw rain-intensity decoder.

## Delayed lamp OFF is expected, not a failed correlation

For the specifically marked AUTO/covered/on/uncovered/off cycles:

| Run | Both channels back to 6, BH device ms | Dipped OFF, BH device ms | Delay |
| --- | ---: | ---: | ---: |
| 1 | 1,482,315 | 1,496,726 | 14,411 ms |
| 2 | 1,614,354 | 1,628,828 | 14,474 ms |

The first run has channel 2 at intermediate value 4 one sample before it reaches
6. Measuring from the first nonzero sample instead gives 15,412 ms. All intervals
have about one-second sensor sampling resolution; they are not exact physical
response delays. The marked lamp-off confirmations arrive after the frame
changes. A short window after uncovering would incorrectly call both pairs a
mismatch. Compare the full bounded interval through settled lamp OFF.

This agrees with the preceding recording's approximately 14–20-second recovery
intervals. AUTO (BH `0x354` / C2 `0x73E` byte 3 bit 6) and dipped indication
(byte 1 bit 6) remain separate. There are extra transitions around AUTO selection
and an additional unmarked cover/uncover episode in run 1. They are preserved
in the evidence rather than forced into the two labelled cycles. In particular,
run 2 has a brief lamp ON/OFF around switch selection before the next labelled
cover. Its cause is not independently identified.

## Independent wiper confirmation

C2 `0x2EC` DLC 8 byte 2 bit 5 gives 27 complete pulses. The two slow/fast/OFF
runs confirm the earlier field: slow rise intervals are 1,499–1,502 ms, settled
fast intervals approximately 949–952 ms, and phase returns low after OFF. A
transition interval can be about 999 ms while speed changes. The message median
period is 50 ms and maximum gap 54 ms. Exact physical sweep counting remains
unverified, but another full input-identification recording is unnecessary.

Spray-associated byte 4 bit 4 pulses repeat in this session, strengthening the
candidate. The contradictory older washer-labelled trials still prevent naming
it a proven washer exclusion. During both rain phases the dipped indication
stays off, corroborating the factory negative control. No independent raw rain
signal or new lamp-control command has been established by this focused review.

## Consequence for firmware work

The input gate can use AUTO plus repeated wiper-phase edges. Sensor recovery
must be separated from the delayed factory lamp state; an automatic controller
must preserve factory darkness and manual-switch priority. This recording
contains no injected lamp-output acceptance test, so that remains the smallest
useful next hardware check. The passive dictionary layouts are unchanged;
version `2026-10-24.3` adds evidence and promotes only the raw-light response
hypothesis. No firmware or device operation changes were made.

Reproduce the targeted traces by importing `scripts/analyze_rain_lights.py` and
calling `analyze(Path('/path/to/sessions/20261024-111510Z'))`; it reads a finished
SQLite/WAL snapshot and verifies original fingerprints. The accompanying
cumulative ledger appends this targeted review to the immutable prior ledger;
its empty generic marker-correlation list means not evaluated by the generic
fixed-window heuristic, not a failed match. Earlier stable templates retain their
original session coverage. Raw replay was separately checked with the existing
`BinaryCaptureParser`.

## Subsequent three-session audit

The [consolidated October-24 audit](20261024-three-session-audit.md) verifies
the original evidence, records the labelled Race/Mid result, and supersedes
older uncertainty or generic marker interpretations where explicitly stated.

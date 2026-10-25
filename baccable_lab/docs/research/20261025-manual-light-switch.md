# Manual dipped-beam switch: focused raw-capture follow-up

## Result

Session `20261010-104325Z` contains three manual dipped-beam ON/OFF pairs,
original keyboard/scenario events 73–78. Raw frame changes occur before the
operator completion markers. Re-examining them does not establish a new CAN
actuator command. It does expose a brief AUTO indication that must not be
mistaken for the command: two of three ON trials pass through it; all three
OFF trials pass through it and then settle with AUTO clear.

This is a refinement of [the timing audit](20261025-light-origin-timing.md),
not a claim that an unobserved command cannot exist. The original captures,
markers and user checkout are unchanged. No firmware or operating instructions
change. The owner reports facelift LED lamps, with exact model year unknown.

## Exact observed state sequences

Zero-based bytes; these payloads describe the isolated trial, not universal
full-frame templates to replay (other lamp functions can alter other bits).

| State in this trial | BH 0x354 / DLC4 | C2 0x73E / DLC4 | C1 0x73E / DLC4 |
| --- | --- | --- | --- |
| Settled OFF, AUTO clear | `00 00 00 00` | `00 00 00 00` | `00 00 00 00` |
| Brief AUTO, lamps still OFF | `00 00 00 40` | `00 00 00 40` | `00 00 00 00` |
| Manual dipped ON, AUTO clear | `02 40 80 00` | `00 40 00 00` | `02 00 80 00` |

A transition through an AUTO position is consistent with these sequences, but
physical knob travel is inferred, not directly sampled. The third ON trial
has no captured AUTO precursor. A shorter-than-report-period AUTO state may
have occurred, so its absence does not prove the knob skipped that position.

| Manual pair | C2 ON host time (s, navigation only) | C2 AUTO precursor to ON | C2 OFF host time (s) | AUTO remains set after OFF |
| --- | ---: | ---: | ---: | ---: |
| 1 | 842.847206 | 208 ms | 846.195934 | 525 ms |
| 2 | 849.437236 | 203 ms | 853.325124 | 345 ms |
| 3 | 856.673298 | Not observed | 859.698595 | 505 ms |

Intervals use C2 device time, not differences of host timestamps. BH records
precursors 211/210 ms before ON in pairs 1/2; after OFF it retains AUTO for
519/340/500 ms. This is not a stable return to AUTO: it subsequently clears.
The earlier summary that OFF returns to AUTO needed this qualification.

The first ON pair on BH is:

- frame 2778235, 840.282608 s: `00 00 00 00` (baseline sample);
- frame 2790757, 842.702275 s: `00 00 00 40`;
- frame 2792080, 842.902790 s: `02 40 80 00`;
- frame 2809128, 846.128782 s: `00 00 00 40`;
- frame 2812041, 846.732323 s: `00 00 00 00`.

The 211 ms precursor interval is measured from BH device values 1069002 and
1069213, not the host-time difference of about 201 ms.

## Negative controls and delayed responses

The immediately earlier AUTO-only trials, events 61–66, produce AUTO state
changes around 790.3–814.3 s without setting the dipped bit. This independently
shows that selecting AUTO is not itself a general lamp-ON command in daylight.
During all three manual dipped pairs, BH 0x738 light bytes remain **3/3**.
Other bytes vary, but there is no change of the two light channels to trigger
these manual lamp transitions.

On the same C2 device clock, the three ON transitions are followed by:

| Pair | 0x42D b0 reaches 0x25 | 0x42E b0 reaches 0x25 | 0x384 b3.7 rises |
| --- | ---: | ---: | ---: |
| 1 | +105 ms | +106 ms | +242 ms |
| 2 | +109 ms | +109 ms | +247 ms |
| 3 | +116 ms | +117 ms | +204 ms |

These are reactions/publications after the earliest identified dipped report
on C2. They do not identify a sender or prove that the earlier message is
executed by a receiver. C1/BH/C2 ordering remains subject to the clock/USB
limitations quantified in the prior audit.

## All-ID precursor scan and its limits

The new script reads every bus/ID/DLC shape in the 780–865 s neighborhood,
then compares each trial's same-board lamp edge against a stable baseline
[-1800,-800] ms and changes/excursions in [-750,+100] ms. It permits even a
single baseline sample to avoid discarding slow frames, retaining sample counts
as weaker evidence. Shapes without a pre-sample are retained separately.
It scans every bit, including bits that return before the settled lamp state.
The prior audit also retains whole-byte state traces and a wider excursion scan.

For all three ON trials, the only matching bits quiet in the 815–840 s negative
control are the already-known BH 0x354 and C1/C2 0x73E lamp bits. The AUTO bit
matches two ON trials and is not a necessary observed precursor. Other repeated
matches include C1 0x341 b1.2 (20 control changes) and BH/C2 0x0AB b2.6
(24 control changes); these are not specific evidence of the light switch.
No newly appearing ID repeats across all three ON neighborhoods. Sparse
0x7D0/0x749/0x7C0/0x4AC occurrences are retained rather than silently excluded.

This method does not demonstrate a separate repeatable CAN switch pulse.
It does not rule out a request encoded in a variable multi-bit field, another
bus, unchanged periodic data, LIN, or a hardwired switch input. Naming an ECU
or declaring that no command exists would exceed the evidence.

## Integration and vehicle feedback

The owner's later beta-34 report was `Wiper 26 A=1 ARM=1 Q109 F=off`,
`BH738 1/1 TX=125`, with Rain lights enabled and actual lamps remaining OFF.
This is operator feedback, not a new CAN recording. It supports an active
request and historical BH queue admissions; it does not show successful bus
transmission, a currently increasing TX count, or receiver acceptance. At a
1/1 sample, the >7 output guard is not the blocker. Preserve this negative
vehicle outcome separately from the older passive captures.

For the next decision, first establish whether modified sensor frames were
actually transmitted and consumed, and verify the facelift-specific BCM input
and lamp wiring. The manual recording is available and has now been inspected;
repeating that same scenario alone is not the missing evidence.

## Reproduction

```sh
python3 baccable_lab/scripts/analyze_manual_lights.py \
  --sessions /path/to/sessions \
  --evidence baccable_lab/docs/research/20261025-light-origin-timing.json \
  --output /tmp/manual-lights-NEW.json
```

[Evidence ledger](20261025-manual-light-switch.json) retains original events,
input fingerprints, exact frames/sequences/payloads, relative device times,
negative-control counts and sparse shapes. All 167 retained raw records match
the original binary input. SQLite is opened read-only, WAL is retained and
capture fingerprints are unchanged before/after. Recording quality limitations
remain in the original summary and previous ledger; the firmware capture does
not identify IDE/RTR/TX or the physical CAN producer.

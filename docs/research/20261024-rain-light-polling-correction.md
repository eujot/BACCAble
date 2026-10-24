# Rain-light polling correction and scope of lamp confirmation

## Confirmed software defect and correction

The merged baseline is `241b993` (PR #77). Rain lights queued C2 requests every
1,000 ms, and every C2 transmission reset `last_c2_poll_time`. Ordinary status
polls require 1,010 ms, so rain traffic repeatedly deferred them. Alternating
USB/version replies exceeded their five-second freshness limit. A production
UART/USB/rain-controller regression actually disables a C2-only USB capture
on the old sources, then passes with this correction.

The UART scheduler now timestamps only successfully queued periodic polls;
other transmissions cannot move their deadlines. USB/version alternation also
advances only after queue admission, so a full queue does not skip a poll type.
Normal addressed reply windows and the existing rain leases remain unchanged.

The regression simulates 60 seconds of continuous 20 ms screen requests,
actual UART serialization/completion and parsed auxiliary replies. C1 has no
USB cable, C2 is connected, BH is disconnected, and one rain reply is lost.
It asserts continuous position-lamp additions, continued USB capture, all four
peer/version/USB freshness limits, and immediate local disable. Hardware/peer
responses are simulated; production scheduling, USB state and C1 rain logic
are executed. The independent C2 suite covers wiper edges, AUTO and hold times.

## Physical observations retained

The owner explicitly clarified on 2026-10-24 that lamps turned on after covering
the light sensor or operating the physical switch. This agrees with the original
scenario markers: `20261024-111510Z` events 5/6 and 19/20 pair the covered
sensor with `dipped_beam_on`; events 7/21 record the delayed OFF. The older
`20261010-104325Z` manual dipped pairs 73–78 supply independent switch trials.
These are physical observations, not merely inferred lamp states.

The captured patterns used by Rain lights are:

| Bus / ID / DLC | AUTO daylight | Lamps active |
| --- | --- | --- |
| C2 / 0x73E / 4 | `00 00 00 40` | `00 40 00 40` (dipped beams) |
| C1 / 0x73E / 4 | position bits clear | `02 00 80 00` (associated position-lamp bits) |

The feature preserves unrelated bits rather than replacing the full payload.
The host tests explicitly compare generated output with these captured patterns.
The [rain correlation](../../baccable_lab/docs/research/20261024-rain-lights-correlation.md)
and [follow-up](../../baccable_lab/docs/research/20261024-111510-rain-lights-followup.md)
retain the original timestamps, hashes and marker provenance; no recording or
historical evidence has been changed.

The exact remaining distinction is how the receiver handles an additional
BACCAble transmission while the factory sender continues. The owner's clarified
trials did not exercise that injection. Their successful lamp activation proves
the ON/OFF association and factory operation, but cannot establish steady replay
acceptance or all front/rear outputs. No alternative BCM request is invented,
and no new vehicle acceptance is claimed by this software correction.

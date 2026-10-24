# Startup DNA and suspension audit, 2026-10-24

## Capture evidence and its limits

Read the original `20261010-111025Z/session.sqlite3` with SQLite `mode=ro`,
including its WAL. No capture, marker, index or checkpoint was changed.
The previous dictionary and hypotheses identify BH `0x256`/DLC 4 as selector
input and `0x25A`/DLC 4 as mode presentation. C1 `0x256` is unrelated.

The session contains a suspension-button pulse (`00 80 00 00`) in N with no
calibration transition and 18 pulses in D alternating `20 10 00 00` and
`20 00 00 00` reports. Representative original frame IDs:

| BH button pulse | Earlier 0x25A frame/payload | Later 0x25A frame/payload |
| --- | --- | --- |
| 3051548 | 3049430 / `20 10 00 00` | 3053685 / `20 00 00 00` |
| 3068574 | 3067504 / `20 00 00 00` | 3070463 / `20 10 00 00` |
| 3366229 | 3364363 / `20 10 00 00` | 3368365 / `20 00 00 00` |
| 3550467 | 3549691 / `20 10 00 00` | 3552877 / `20 00 00 00` |

Events 109–120 supply suspension-test context, not exact press timestamps.
Event 109 does not align with the nearest pulse in the same way as the later
markers; do not equate nearest-marker matching with a causal label.
The N-to-D entry settles at `20 10 00 00`, consistent with the owner's statement
that selecting D starts firm and pressing the button selects Soft. Therefore
D `byte1[5:4]=1` is Stiff and `0` is Soft, the reverse of the initial implementation.

The earlier all-mode captures show Race settling at `04 20 00 00`. The owner
reports that Race starts Stiff and its button selects Mid. Race Stiff=2 is
supported by this combined evidence; Race Mid=1 is an inference from the
calibration field and has no directly labelled Race-button capture here.
The firmware uses that candidate code but must not be called vehicle-validated.
No absence-of-input claim is made about other recordings.

## Implementation findings

- Slot 36 was already the V6 preference. Suspension now owns slot 41, with
  import of 80-byte settings records into the 82-byte format.
- Removing the separate diagnostic menu left two calls to a deleted helper.
  Ordinary reading positions now directly use the retained preference array.
- C1 starts its response deadline only after UART accepts the request. BH
  independently checks engine running, with a bounded wait so an old request
  cannot unexpectedly apply after a later engine start.
- Selector and suspension use one press/release state machine. Manual input
  relinquishes ownership immediately, without a synthetic release over the driver.
- Both suspension choices are enforced from fresh, settled feedback, even when
  already in D/Race. One accepted toggle is never repeated on missing feedback.
  A mode change, unknown code, engine stop or stale data cannot report success.
- The root menu has five entries, ordinary page buffers match the 72-slot
  visibility capacity, and C1 returns to 96 KiB code plus 20 KiB optional disk.

Software verification and integration status are maintained in
[the action plan](../../../docs/ACTION_PLAN.md).

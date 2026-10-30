# Steering input and display audit — 2026-10-30

Baseline: fetched origin/master af1fd72 plus committed current UX a5e89cd.
Branch: fix/steering-input-recovery. Owner reports missing steps during rapid
clicks, isolated clicks after idle, held buttons and direction reversals.

## Path and confirmed defects

C1 dispatches standard CAN 0x2FA/DLC3 to vehicle_handle_steering_controls.
Both CC and ACC must be disabled. menu_input_update recognizes edges, then
menu_event changes selection before scheduling display output. Normal CAN RX
drains up to eight frames per main-loop pass; there is no input-specific RX
queue. Active ELM/diagnostic-link mode bypasses this ordinary dispatch path.

| Finding | Evidence and disposition |
| --- | --- |
| A press after an idle report gap is discarded even if the last report was neutral | The unconditional 300 ms disarm forgets a known release. Fixed: neutral-to-direction remains a fresh edge; stale held input and RES still require release. |
| Opposite direction without an intermediate neutral is discarded | Only previous == neutral was accepted for gentle directions. Fixed: opposite directional states also constitute a fresh edge, including strong directions. |
| Relaxing a strong press can look like another click | No change needed: same-direction strong-to-gentle transition remains non-clicking. Repeat restarts after the normal delay. |
| Rapid screen updates omit intermediate positions | Intentional latest-screen scheduling; selected index changes before the render throttle. No evidence of dropped recognized steps here. |
| Single missing release between identical press codes | Cannot distinguish two physical presses from one hold using identical reports. No synthetic extra click added. Requires capture evidence. |
| CAN load or electrical loss | Possible upstream loss is not demonstrated by code inspection or host tests. No timing/queue-size change justified without measurements. |

## Timing and output

UP/DOWN has no RES click minimum or double-click delay. Gentle hold repeats after
500 ms, then every 180 ms while reports remain fresh. Strong directions jump
groups without repeat. The freshness threshold remains 300 ms. Permission loss
still resets the recognizer, so a new release is required. RES hold remains
900 ms and the double-click window 280 ms; idle-direction recovery does not
permit RES activation from a stale stream.

Navigation events within 350 ms form a display burst. C1 defers intermediate
screens until one transfer interval or 120 ms of quiet. MY23 Quick 20 ms uses
13 fragments plus margin, giving a 280 ms scheduling interval. This is not a
measurement of end-to-end response time. Board UART keeps the newest waiting
screen; BH finishes an immutable active image and then sends the latest target.
Queue rejection retains pending work. Existing BH regressions check final-image
delivery and CAN retry rather than claiming every intermediate screen is shown.

Reports/Gaps/Max gap/Input age describe the recognized report stream, not every
physical button edge. Gaps only counts intervals over 300 ms, and permission loss
resets these counters. Zero Gaps cannot rule out one missing release. Acceptance
in the vehicle should correlate a raw 0x2FA trace, CC/ACC permission and observed
selection/output during each symptom. No capture was interpreted in this task.

## Validation and boundaries

Production recognizer tests cover 100 quick released presses, all four direction
codes after idle, stale-held disarming/rearm, reversals, pressure relaxation,
repeat boundaries, permission loss, RES suppression after gaps and tick wrap.
Four menu build variants run these regressions. Existing dedicated repeat tests
exercise all seven boundary/rearm cases; the direct-reversal expectation is
updated. Two BH final-snapshot/retry cases run on both widths. All pass with
native Apple Clang ASan/UBSan (15 selected case executions).

Four standard ARM builds and four-flavor cppcheck use full Arm GNU 15.2.Rel1.
No flash or physical acceptance is claimed. Existing UX changes are retained.
No receive buffer, CAN filter, actuation or IPC pacing configuration changed.

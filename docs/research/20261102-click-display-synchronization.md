# Click/display synchronization feasibility — 2026-11-02

Fresh fetch: origin/master remains af1fd72. Reviewed implementation is 5c1f6b3,
including current UX and directional input corrections. This is a source audit
and implementation proposal, not an additional firmware change or hardware test.

## Confirmed current behavior

menu_event changes the selected item before navigation render pacing. A burst
within 350 ms can defer rendering until a transfer interval or 120 ms quiet.
Non-direction events clear navigation_pending before dispatch; SELECT/HOLD has
no screen-revision fence. It operates on the logical selection, whether or not
that selection has reached BH/IPC. RES click recognition adds its own delay,
but this is not a guarantee that the display has caught up.

board_uart.c has one replaceable latest pending screen and an immutable active
UART transmission. body.c/display_stream.c has an immutable active Full image
and a replaceable latest target. This bounds backlog and prevents fragment
splicing, but several layers can be at different revisions. can_tx success
means queue acceptance; display_stream_accept currently tracks that acceptance,
not confirmed CAN transmission or visible IPC output. C1 has no per-screen
completion feedback from BH.

For MY23 there are 13 fragments. Nominal pacing occupancy is approximately
260 ms at 20 ms/fragment (plus queueing/arbitration); C1's scheduling interval is
280 ms. Repeat at 180 ms therefore generates steps faster than distinct Full
screens can be completed at the default pace. Sending every intermediate screen
would create increasing lag. These figures are scheduling arithmetic, not
measured first-glyph or full-screen latency.

An additional timing-model mismatch exists: C1 navigation_interval uses compiled
DISPLAY_LEGACY_FRAGMENT_COUNT, whereas BH uses the effective runtime legacy
width. An 18-character build switched to Legacy 24 underestimates the nominal
13-fragment transfer (11 fragments assumed). Fixing this improves the estimate,
but cannot substitute for completion feedback. IPC pace/method settings are also
queued requests, not acknowledged peer state; leases/reset can temporarily differ.

## Recommended implementation sequence

1. Give each complete C1 view snapshot a revision. Carry revision and protocol
   capability through C1/BH transport and return received / CAN-completed revision
   separately. Extend CAN TX bookkeeping to track the last fragment's actual
   completion; do not relabel queue acceptance as transmission. Validate reset,
   wrap, stale replies and older-peer negotiation. Physical visibility remains
   unknown because no IPC presentation acknowledgement is established.
2. Use this progress for display scheduling instead of a second independent
   nominal timer. Keep one complete active image plus newest pending image.
   Apply every recognized navigation event to the logical target; coalesce screen
   snapshots, never silently discard SELECT/BACK/HOLD or reorder them across views.
3. Add a presentation barrier for SELECT/HOLD tied to the target view revision
   at the event. Define and test cancel/new-navigation behavior and bounded retry;
   never run a queued action later against a different selection or replay a hold
   after reconnect. The user-facing contract must distinguish delayed execution
   from a request to press again. An enqueue-time or elapsed-time-only fence cannot
   guarantee presentation. Display failure must not deadlock input indefinitely.
4. Add counters for accepted directional events, stream rejection reasons,
   logical revision, BH received/completed revision and navigation-to-transmission
   age. Keep physical input acquisition, recognized events and display latency
   distinct. This makes the remaining in-car symptoms diagnosable.
5. Only then evaluate 10 ms pacing or Delta on the actual IPC. Faster pacing
   lowers nominal transfer time, but cannot repair missing input reports. Delta
   can lower CAN traffic but must be checked for partial/mixed visual updates.

A full FIFO of all intermediate screens is not recommended: sustained input
above display throughput accumulates delay. A bounded ordered event queue may
be useful around presentation barriers, with explicit overflow handling and
run-length compression only where event order/meaning is preserved. Netting
opposite directions across group/view boundaries is not generally valid.

## Validation needed

Replay rapid taps, hold/reverse, RES during a pending screen, UART rejection,
CAN mailbox delay, radio takeover, peer restart and both runtime widths. Assert
final logical target, ordering of semantic events, no action on a different
view revision, bounded memory/backlog and no duplicate hold. Measure first
visible response and completed screen separately in the vehicle.

No new executable tests were run for this source-only assessment. Existing
snapshot/retry regressions from the prior task do not establish cross-board
revision synchronization. No firmware, manuals, flash or user files changed.

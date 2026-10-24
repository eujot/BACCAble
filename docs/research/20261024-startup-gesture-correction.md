# Beta-29 startup DNA and suspension correction

## Evidence and diagnosis

The owner reports that beta-29 default D changes the mode, default Race does not,
and D/Soft once changed the mode without changing suspension. That establishes
limited vehicle acceptance of the injected selector input, with unresolved Race
and suspension reliability. No failed automatic-start recording or BACCAble TX
trace is available, so the exact cause of those attempts is not proven.

The firmware source in the preceding research branch is identical to the
v5-beta-29 release source `c2b6ba7`. The labelled manual matrix
`20261024-113104Z` and older evidence establish:

- D-to-Race mode responses after 1,196/1,210 ms in manual holds lasting
  2,187/1,886 ms. Beta-29's nominal 1,800 ms exceeds those response times;
  hold duration alone is not demonstrated as the cause of failure.
- During a physical long hold, the periodic BH `0x256` reports remain pressed.
  Normal released reports repeat about every 250 ms. Beta-29 adds virtual held
  reports on its own 250 ms clock but does not react to intervening original
  neutral reports. Those received neutral frames can interrupt the input seen
  by a listener. Source code and the trace waveform establish this mismatch;
  the listener's exact neutral-debounce behavior remains unverified.
- D/Race mode and calibration reports have brief transitional combinations.
  Beta-29 can start its next action after only a few hundred milliseconds of
  observed target mode. Manual actions in the capture are spaced much farther
  apart. The recording does not establish the earliest acceptable second
  operation at engine startup.
- A suspension pulse in the capture changes feedback after 25–45 ms, and its
  observed width is 92–262 ms. Beta-29's 150 ms is within that range. However,
  it may begin anywhere in the factory-neutral heartbeat phase, and the code
  never tries again after a lost or ignored accepted pulse.
- `can_tx()` returning HAL_OK confirms software-queue admission, not wire
  completion or ECU acceptance. Its previous immediate-success test stub and
  frequent neutral observations did not exercise the real 250/500 ms report
  cadences or a first pulse ignored by the receiver.

These are concrete code/waveform weaknesses and plausible failure paths, not
proof of a single hardware cause. The correction retains actual-mode feedback
rather than rewriting status reports.

## Corrected behavior

The engine gate remains fresh RPM above 400 for two seconds on C1 and BH.
Before a new action, BH now requires actual mode stable for one second and a
factory-neutral report no older than 20 ms. It also requires an empty software
TX queue and all three hardware mailboxes free before starting or refreshing a
press. A release remains queueable while the transmitter is busy.

During a press, receiving a new factory-neutral frame schedules immediate
reassertion of the owned input; unrelated bits are copied from that newest
frame. This sends an additional frame on the existing bus, not interception of
the original. A long hold also retains the 250 ms fallback refresh. Race holds
for 2,200 ms, providing a margin over the newly recorded manual hold widths.
Short directional presses remain 200 ms and the suspension pulse remains 150 ms.
Any non-neutral physical selector/button input still cancels synthetic control.

Suspension still needs fresh 300 ms settled feedback in the actual requested
mode. There is at most one conditional retry: after release, the opposite
calibration must remain stable and freshly reported for at least one second
(a report gap over 400 ms restarts stability, and the newest sample must be
no older than 300 ms), the first press/release must have drained from the transmitter, and no desired
calibration response may have been seen. A brief desired reply latches evidence
and forbids another toggle even if later reports revert. Missing, stale or
unknown feedback cannot justify a blind retry. The second attempt is checked
again immediately before sending; a manual input, mode change or engine loss
cancels it. Retry eligibility does not extend the first 1.5-second feedback
window; it expires rather than issuing a late second pulse. Two ignored pulses
end in failure. A partial success can therefore
leave D selected with `No confirm` because suspension was not corroborated.

The overall BH operation limit grows from 10 to 12 seconds, in addition to its
maximum three-second independent engine corroboration. C1 waits 20 rather than
15 seconds for BH, leaving room for paced gestures and the guarded retry; its
subsequent five-second DNA/transmission corroboration is unchanged. Duplicate
requests and transport resync cannot replay a completed operation. Settings
format, UART packet shape and menu labels are unchanged; update C1/BH together.

## Verification and limits

The selector fixture now uses recorded neutral (250 ms), mode (500 ms) and
suspension (250 ms) cadences. Regression cases cover neutral arrival during a
hold, one-second mode pacing, press alignment, hardware/software TX drain,
one ignored first pulse, late feedback, missing feedback, a brief desired state,
persistent rejection, physical override, engine loss, queue/release failures,
initial/target combinations, duplicate requests and timer wrap. The transport
fixture distinguishes an empty software queue from busy hardware mailboxes.
These are software regression models, not a claim that a particular receiver
uses the modeled timing or accepts the new waveform.

Local build/test results and integration status are recorded in
[the action plan](../ACTION_PLAN.md). Vehicle acceptance must verify repeated
fresh-start Race and D/Soft, final reported state, failure indication and later
physical selector use. Original captures and markers were not changed.

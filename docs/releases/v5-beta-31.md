# BACCAble v5-beta-31

This beta adds a Rain test action for diagnosing the rain-triggered lighting
path and makes its forced-output test safer and easier to interpret.

## Included changes

### Rain test diagnostics

- `Actions > Rain test` displays the C2 wiper phase counter, fresh AUTO state,
  active rain request and C2 output queue-admission counter.
- The wiper counter works while the saved `Rain lights` feature is OFF. The
  first sample after a long gap establishes the phase and is not counted.
- `C2: no reply` marks missing or expired diagnostic data. `A=?` marks an old
  AUTO sample. `F` is the local C1 force request; `Q` means a frame entered the
  C2 queue. Neither indicates that a vehicle controller accepted the request
  or that a lamp physically turned on.
- SELECT forces the existing bus-specific `0x73E` lamp bits for up to 30 s.
  C2 also expires the force after 3 s without C1 commands, and C1 retries a
  STOP that the UART queue rejects.

### Rain lights scope

The existing automatic rain threshold, 60 s request hold, lamp masks and factory
AUTO handover are unchanged. The test action isolates wiper detection and the
software output path; it does not establish vehicle acceptance of the injected
CAN frames. Test with matching C1 and C2 firmware. No vehicle trial of this
beta is claimed.

## Build and verification

The local Apple Clang ASan/UBSan host suite passed. All four firmware flavors
passed lint and built with Arm GNU 15.2.Rel1. C1 uses 97,884/98,304 Flash bytes
and 15,420/16,384 static RAM bytes. Release CI uses the repository-pinned Arm
GNU 15.3.Rel1 and remains the publication gate. No device was flashed.

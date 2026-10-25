# BACCAble v5-beta-34

This beta moves Rain lights output to the BH board. BH now injects a dark
light-sensor sample on CAN while the automatic rain-light request is active.

## Included changes

### Rain lights through the BH light sensor

- After three detected wiper phases within 15 seconds in AUTO, C2 requests
  darkness directly from BH. The request is held for 60 seconds after the last
  detected phase and expires if board communication stops.
- BH waits for a fresh AUTO report and a factory BH `0x738`/DLC 8 frame. It
  copies that frame, changes bytes 1 and 2 to zero, and queues the copy on the
  BH CAN bus. C1 and C2 no longer inject lamp-state frames.
- `Rain test` can temporarily request the same BH output. It expires when
  renewals stop and has a 30-second maximum duration. The sensor page's `TX`
  value counts dark `0x738` frames accepted into the BH CAN queue; it does not
  confirm that vehicle lamps turned on.

### CAN evidence and limits

- The reviewed October 24 sessions contain 32 groups of `0x738` frames whose
  light bytes differ while the remaining bytes, including byte 5, stay the
  same. The firmware copies the remaining bytes from the most recent factory
  sample.
- BH transmits the copy after receiving the factory frame. It does not rely on
  conflicting same-ID payload bits winning CAN arbitration.
- Whether the vehicle's lighting controller accepts the injected frame and
  turns on the expected front and rear lamps still requires an in-car test.
  Use matching C1/C2/BH firmware and test while stationary.

### `Rain test` UART diagnostics

- C2's active Rain lights request uses the former C2-to-C1 UART reply slot,
  keeping the C1 shared-link traffic budget unchanged.
- The C2 diagnostic reply is also delivered to BH so forced testing can renew
  the BH request lease. The BH sensor reply reports the queued dark-frame count.

## Build and verification

The host test suite and 82 Lab tests passed. Focused Rain lights, Rain test and
shared-UART regressions passed, including a 105-second run across both
diagnostic pages with Rain lights enabled and no `no reply` display or UART
queue rejection. C1, C2, BH and CAN passed local lint and ARM GNU 15.2.Rel1
builds. CI and release packaging results will be recorded after publication.

No board was flashed for this change. Bench and in-vehicle acceptance of the
injected BH `0x738` frame and physical lamp behavior remain open.

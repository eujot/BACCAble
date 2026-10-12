# BACCAble v5-beta-22

This beta includes the Readings preview prefetch, start-up drive-mode and
lane-assist preferences, updated default Favorites, and the October CAN research
catalog. It follows `v5-beta-21`.

## Device changes

- On a two-line MY23 Readings screen, the device queries diagnostic values for
  the next visible page while showing its label on line 2. After navigation,
  cached replies can appear immediately. The request rate remains one per
  500 ms; rapid scrolling, missing replies or stale values can still show `--`.
- `Default mode` selects the drive mode requested after start-up (OFF, A, D or
  Race). A manual mode change stops the override for that start-up cycle.
- `Disable LKA` clears the observed lane-assist state bit in retransmitted CAN
  frames when enabled. Its effect on physical lane assistance has not yet been
  verified in the vehicle.
- Factory Favorites for one- and two-line displays are organized by theme.
  Existing user-edited sets keep their saved choices.

## Capture and research

- Guided CAN scenarios, skip support and session bundles make recordings easier
  to repeat and review. The scenario and dictionary documentation includes DNA
  and lane-assist mappings.
- Analysis of five October bundles adds C2 `0x42D` byte 1 bit 2 as a supported
  right-indicator state field. C2 `0x42E` is the corresponding left-side
  candidate. The earlier BH `0x2F0` byte 4 bit 4 wiper-state claim is refuted.
  These catalog findings do not add a firmware control feature.

## Verification and limits

The release workflow builds matching C1, C2, BH and CAN firmware images and
checks them with the native host suite, Lab tests, cppcheck and ARM builds.
`BUILD_INFO.json` identifies the source and compiler; `SHA256SUMS` covers the
downloaded assets. Install matching board images together.

The released images have not been installed or tested in a vehicle. Readings
prefetch appearance, the physical effect of `Disable LKA`, drive-mode override
behavior and the candidate indicator fields still need vehicle confirmation.

# BACCAble v5-beta-30

This beta adds automatic rain-triggered lighting and improves the reliability of startup drive-mode and suspension requests. It also prevents rain-light traffic from delaying C2 status and USB polling.

## Included changes

### Rain lights

- `Settings > Features > Rain lights` is in the `Comfort` group, defaults to OFF and must be saved ON. Use matching C1 and C2 firmware.
- When the lighting switch is in AUTO, three detected wiper-cycle rises within 15 seconds request dipped beams and associated position lamps. Each further cycle renews a 60-second hold. Wiping intensity is not measured; several washer sweeps can also meet the threshold.
- After the hold expires, or AUTO is left, BACCAble stops adding lamp bits to fresh factory lighting frames. It does not send a lights-off request, leaving the factory AUTO system in control.
- Earlier vehicle trials confirmed that the lamps turn on when the light sensor is covered or the physical switch is operated. The corresponding lighting values are known and are used by this feature. Acceptance of BACCAble's added CAN transmission while factory frames continue, including complete front/rear lamp behavior and manual priority, still needs a vehicle trial.

### Communication and startup actions

- Rain-light requests no longer postpone C2 or BH status and USB polls. Poll deadlines move only after their own request enters the UART queue; a full queue retries the same USB/version poll type. This prevents stale C2 status from terminating capture when only C2 has a USB connection.
- Race startup selection uses a 2.2-second held selector input, aligns with fresh neutral reports, waits for stable observed mode and drains prior CAN transmissions before a new press.
- Startup suspension keeps the 150 ms pulse and may retry once only when fresh, stable feedback shows the first pulse was not accepted. A missing or stale report does not trigger a blind retry. These software changes have not yet received vehicle acceptance.

### CAN research and saved settings

- Adds the October 24 rain/wiper follow-up and three-session audit, preserving raw captures, markers, hashes and contradictions. The three reviewed sessions contain 90/90 confirmations.
- Updates the machine-readable Giulia CAN dictionary and generated catalog with the reviewed evidence and firmware source provenance.
- Rain lights adds settings slot 42. Existing 80-byte and 82-byte settings records are imported with Rain lights OFF; saving writes the expanded 84-byte record. Older firmware may not read the longer record after downgrade.
- Both user guides and the menu guide document the setting, timing and outstanding vehicle checks.

## Build and verification

Local validation on the merged source passed the Apple Clang ASan/UBSan host suite, all 77 Lab tests, all 16 Arm GNU 15.2.Rel1 board/display builds, four-flavor cppcheck and the four standard firmware size/identity gates. The CI release workflow for this tag also runs the repository's pinned CI builds and tests before publishing artifacts.

No firmware change in this beta has been flashed or accepted in a vehicle. After release, install matching C1/C2/BH images and test rain activation, timeout, factory darkness handover, manual override and the startup Race/suspension behavior in the car.

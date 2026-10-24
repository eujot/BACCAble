# BACCAble v5-beta-29

This beta adds engine-aware startup drive-mode selection and a saved suspension
preference, and removes the separate Pelican `Experiments` DID menu. Ordinary
OBD `Readings` and `Favorites` remain available.

## Included changes

### Startup drive mode and suspension

- `Settings > Features > Default mode` retains `A`, `N`, `D` and `Race`. For
  active choices, C1 and BH wait for fresh engine-speed reports above 400 RPM
  for two seconds before selecting a mode. `N` remains passive.
- Race selection waits for the engine because the vehicle accepts Race only
  while it is running. The selector sequence follows fresh vehicle mode reports,
  releases each simulated input, and yields to the physical selector.
- The startup status checks received mode and transmission reports before
  showing `Confirmed`. This confirms reported state, not every vehicle system.
- `Settings > Features > Suspension` selects `Stiff`, `Soft` for D, or `Mid`
  for Race. The softer choice is shown as `Softer` for A/N and has no effect.
  After the mode is confirmed, BH checks fresh suspension feedback and sends at
  most one 150 ms button pulse when a change is needed. It waits for feedback
  after releasing the button and does not repeat an unconfirmed pulse.
- Install matching C1 and BH firmware. D suspension values are supported by the
  recorded captures; Race `Mid` feedback is an inference and still needs
  confirmation in a vehicle. No hardware acceptance is claimed for this beta.

### Menu and settings cleanup

- Removes the separate `Experiments` menu, its Pelican/OBDb DID polling and
  decoder, and its dedicated tests. Ordinary OBD support and readings are kept.
- The suspension option uses flash slot 41; existing 80-byte settings records
  load with `Stiff` as the new default and are written as 82 bytes on save. Older
  firmware may not read the longer record after downgrading.
- Restores the C1 application allocation to 96 KiB and the optional USB disk to
  20 KiB. Standard C1 firmware does not expose the disk. Back up files before
  upgrading a custom 16 KiB C1 mass-storage build because its disk start address
  changes.

## Build and verification

The release workflow builds matching C1, C2, BH and CAN images, runs the host and
Lab checks, and packages `BUILD_INFO.json` and `SHA256SUMS`. Use matching board
images together and verify the build identity and checksums before flashing.
The workflow run records the source commit and embedded version. No firmware in
this beta has been accepted in a vehicle.

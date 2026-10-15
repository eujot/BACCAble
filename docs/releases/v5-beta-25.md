# BACCAble v5-beta-25

This beta is built from the integrated master revision after v5-beta-24. It
includes the ignition-cycle startup and menu-save corrections from PR #63,
along with the v5-beta-24 features and readings.

## Included changes

- C1 recognizes an ignition OFF/RUN cycle while the board remains powered and
  waits for RUN before its one-time startup action. Cranking and unknown states
  keep the active session. Transport resynchronization retains a pending event.
- C2 waits up to five seconds after auxiliary configuration for LKA-enabled and
  released button reports, then sends one recorded left-stalk double-click.
  Driver input cancels a pending gesture; an already started gesture completes
  its release steps. This remains an experimental behavior pending vehicle
  acceptance.
- `Disable LKA` is now labeled `LKA`. OFF requests the startup double-click;
  ON preserves factory behavior and does not force physical activation.
- Hold saves committed menu edits directly in the applicable editors. Dirty
  Back offers save or discard, and failed saves retain the draft for retry.
  Toggle, enum, sort and mirror editors no longer change values just by opening.
- Includes the `Experiments` readings and other features documented in
  [v5-beta-24](v5-beta-24.md).

## Build and verification

The release workflow builds matching C1, C2, BH and CAN images and packages
`BUILD_INFO.json` and `SHA256SUMS`. Install matching board images together and
check their build identity and checksums before flashing.

The firmware changes have passed host regression checks, static analysis and
the four role builds in CI. No v5-beta-25 image has been installed in a vehicle.
Ignition cycling, the startup LKA gesture, driver re-enable and menu button
gestures still need vehicle acceptance.

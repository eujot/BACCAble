# BACCAble v5-beta-26

This beta is built from the integrated master after PR #67. It adds oil-level
readings selected by engine and drivetrain, with a stepwise animated bar for the
existing IPC display.

## Included changes

- `Oil level` shows the ECU reading in mm with `LOW`, `OK` or `HIGH` status.
  `Oil level bar` is a separate full-width reading and can be selected in either
  Favorites line. The bar marker moves one character per completed display step
  toward the latest reading, in both directions. New measurements update the
  target without skipping positions.
- `Settings > Features > Drivetrain` offers `Not set`, `RWD` and `Q4`, alongside
  the existing `Engine` selection. Set both to choose the applicable inclusive
  reference range:

  | Engine | RWD | Q4 |
  | --- | --- | --- |
  | 2.0 I4 | 72–82 mm | 67–77 mm |
  | 2.2 D | 61–65 mm | 79–84 mm |

  These ranges were supplied by the owner and were not independently validated
  in this release. No range is defined for 2.9 V6. An unset drivetrain or V6
  does not show an `OK` judgment.
- The existing ECU oil-height measurements and cache feed both pages. Missing,
  expired or invalid measurements show `Oil -- mm`. Each reading can be hidden
  independently, while saved favorites and existing page identities are kept.
- Animation is performed by BH. At the default `Quick 20ms` CAN fragment pace,
  a full MY23 screen step uses 13 fragments and takes about 260 ms without bus or
  radio delay. `Write Delta` sends changed fragments only. The numeric value and
  status remain the latest ECU sample; intermediate bar positions are animation.

## Build and verification

The beta release workflow builds matching C1, C2, BH and CAN images and packages
`BUILD_INFO.json` and `SHA256SUMS`. Install matching board images together and
check their build identity and checksums before flashing.

PR #67 passed GitHub CI, including host tests, lint and the C1, C2, BH and CAN
builds. Local tests covered the four oil ranges, settings, Favorites, expiry,
bar movement in both directions, changed targets, CAN retries and radio
interruption. All four display profiles built for each firmware flavor with the
full local Arm GNU toolchain. No v5-beta-26 image has been installed in a vehicle.
Verify the reference ranges, sensor availability, displayed glyphs and bar
animation in the vehicle before relying on the result.

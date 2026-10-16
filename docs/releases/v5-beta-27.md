# BACCAble v5-beta-27

This beta adds a dedicated IPC display test pattern for comparing glyphs that
could improve the oil level bar on the vehicle's instrument display.

## Included changes

- `Information > IPC display test > Oil bar glyphs` sends candidate line,
  fractional tick, fill-density and point characters on the selected display
  source. The two test lines show `─ ━ ═` and `▏▎▍▌▋▊▉█`, then `░▒▓█` and
  `·•●◉■`.
- The pattern is a diagnostic comparison only. It does not change the normal
  oil level bar or guarantee that a particular IPC/source can draw every glyph.
- Existing test patterns retain their order and identifiers. The English and
  Polish user guides explain how to run the new pattern.

## Build and verification

The beta workflow builds matching C1, C2, BH and CAN images and packages
`BUILD_INFO.json` and `SHA256SUMS`. Install matching board images together and
check their build identity and checksums before flashing.

The source commit passed GitHub CI tests, lint and all four flavor builds. The
release workflow runs those checks again with its pinned toolchain before
publishing. The glyphs have not been compared on vehicle hardware, and no
v5-beta-27 image has been installed in a vehicle.

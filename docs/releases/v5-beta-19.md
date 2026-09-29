# BACCAble v5-beta-19

This beta includes the MY23 dual-line menu and shared navigation updates merged
in PR #47. Build identity is recorded in the attached `BUILD_INFO.json` and
`SHA256SUMS` files.

## Changes

- MY23 uses a two-line menu with separate current-item and preview/value areas;
  menu output retains the active audio source and reasserts menu text through
  the existing display arbitration.
- Favorites support six ordered sets per fuel profile, with up to five atomic
  measurements per set. Existing measurement IDs and preference migration are
  retained.
- Settings and Favorite editors use explicit save/discard steps. Successful
  saves remain in the editor; failed saves keep the draft available for retry.
- Lists show the current and next item, and the menu uses distinct short and
  tiny parameter labels to fit the display.
- CI exercises legacy/MY23 and normal/large-display builds for C1, C2, BH and
  CAN. Firmware source is commit `d071cfc` (master after PR #47 and this notes update).

## Installation and limits

Install the matching C1, C2 and BH images from this release together. The CAN
image is for its separate role. Use the included checksums to verify downloads.

The MY23 menu, actual glyph appearance, radio takeover/reassertion, button
timing and USB CAN recovery still need vehicle acceptance. CI and host tests do
not verify behavior on the vehicle display. See the English and Polish user
guides for menu operation and the MY23 acceptance procedure.

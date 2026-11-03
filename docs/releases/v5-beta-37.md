# BACCAble v5-beta-37

This beta builds on v5-beta-36 with a live two-second menu demonstration and improved factory audio-source tracking. It includes PR #100 and its Demo review corrections.

## Included changes

### Live Demo

- `Information → Demo` presents supported readings, saved Favorite pages, Vehicle and Features menus, Favorite editing, Actions, IPC options and Information using the active display profile.
- Readings use live vehicle data. Feature and Vehicle changes are previews; Demo does not save settings, run Actions or store mirror calibration.
- UDS polling waits for the screen to settle, observes a 500 ms minimum interval and pauses during DTC clearing. Replies refresh values without triggering another request.
- Feature previews follow the real choices and limits, respect unavailable options and skip hidden Actions.
- Double-click RES exits to Information. The tour ends automatically.

### Factory audio source display

- FM (`0x0A`) and DAB (`0x02`) text transfers preserve their source while BACCAble redraws the display, including complete single-frame DAB text.
- Unknown contexts and incomplete transfers do not replace the last complete source. USB and CarPlay source handling remains supported.

## Build and verification

PR #100 host tests, BACCAble Lab tests, lint and C1/C2/BH/CAN firmware builds passed. After the Demo review corrections, the full Apple Clang ASan/UBSan host suite passed (230 reported cases); all four firmware flavors passed ARM builds and cppcheck using Arm GNU 15.2.Rel1.

The release workflow rebuilds and packages firmware with the pinned CI toolchain. No board was flashed. Demo readability, vehicle polling behavior and visible FM/DAB source icons still need acceptance with matching firmware installed in the vehicle.

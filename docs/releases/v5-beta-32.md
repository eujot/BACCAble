# BACCAble v5-beta-32

This beta adds a screen-only menu tour, removes the external LED-strip feature
to recover C1 Flash, and adds raw light-sensor diagnostics to `Rain test`.

## Included changes

### Menu tour and UX

- `Information > Demo` walks through ordinary menu screens, sample readings and
  example setting changes. It does not write settings or send vehicle actions;
  a RES double click exits the tour.
- Menu navigation now coalesces quick repeated direction clicks and displays
  the latest selection at a steadier pace. Save feedback and MY23 multi-value
  reading spacing are improved.
- Removes the obsolete UTF-8, display-width and special-character test screens.

### LED-strip removal and Flash space

- Removes external WS281x LED-strip support and its timer/DMA setup. The
  separate IPC `Shift light` feature remains available.
- Keeps the old Flash setting slot reserved and clears it when settings load,
  preserving the position of later settings.

### Rain test light-sensor page

- Adds a second `Actions > Rain test` page showing BH `0x738` bytes 1 and 2,
  the two raw channels that responded to light changes in reviewed captures.
- The display distinguishes a missing BH reply, a missing or stale CAN sample,
  and fresh raw values. Zero is a valid sample. Values have no established lux
  scale or universal darkness threshold.
- This page observes the signal only. It does not emulate darkness or establish
  that transmitting `0x738` values controls factory AUTO lighting. The existing
  rain-light trigger and output behavior are unchanged.

## Build and verification

The merged PR #86 passed the full host test suite, CAN knowledge tests, lint,
and firmware builds for C1, C2, BH and CAN in CI. This release workflow rebuilds
and packages the firmware with the repository-pinned toolchain. No board was
flashed; in-vehicle acceptance of the demo, live sensor display and Rain lights
behavior is not claimed.

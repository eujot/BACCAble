# BACCAble v5-beta-28

This beta adds a configurable startup DNA mode and replaces the experimental
reading list with grouped Giulia OBD signals. It also includes OBD catalog and
ISO-TP reader corrections merged since v5-beta-27.

## Included changes

### Default drive mode

- `Setup > Default mode` offers `A`, `N`, `D` and `Race`. `N` is passive: it
  leaves the factory-selected mode alone and sends no selector gesture.
- At startup, the BH board can reproduce the short selector input observed on
  BH CAN frame `0x256` to move the factory selector to `A` or `D`. To reach
  `Race`, firmware first selects `D`, then performs the captured longer hold.
- Synthetic input is bounded, preserves unrelated frame fields, releases the
  selector, and yields to physical knob input. C1 waits for the matching BH
  result and checks subsequent factory mode reports before showing `Confirmed`.
  A rejected or unconfirmed change shows `No confirm` and leaves the actual
  mode in place; selecting `Race` can therefore leave the car in `D` if the
  second step is not accepted.
- Install matching C1 and BH firmware. The physical sender of `0x256` and
  vehicle acceptance of reproduced selector input remain unverified in a car.

### Experiments and OBD readings

- Replaces the old experimental gasoline pages with 64 read-only Giulia signals
  from the Pelican/OBDb source, arranged in nine source-based groups. Values
  display with the source units and conversions; the ordinary catalog and
  Favorites remain separate.
- Hardens UDS `0x22` and ISO-TP handling: validates service, DID and response
  address; handles multi-frame replies, pending responses and bounded timeouts;
  retries failed queue/read operations; and follows each signal's refresh
  interval. Shift gear uses its own source state mapping.
- Fixes shared catalog bounds so parameter request, cache and display lookups
  use the actual number of definitions instead of a stale fixed limit. Invalid
  or unsupported replies remain unavailable rather than appearing as values.
- Shortens selected labels so values and units fit the 18-character display.

### Storage and display

- The C1 application allocation grows from 96 KiB to 100 KiB. The optional C1
  mass-storage area shrinks from 20 KiB to 16 KiB; standard C1 builds do not
  enable mass storage. Back up files from a custom C1 mass-storage build before
  flashing. C2/BH small-volume formatting and mounting are also corrected.
- Existing oil-level bar behavior is unchanged. The `Oil bar glyphs` display
  test from v5-beta-27 remains a diagnostic comparison only.

### CAN research and maintenance

- Adds the all-session evidence and analysis tools for the BH `0x256` selector
  input to the Lab knowledge base and generated CAN catalog. Original captures
  and markers were preserved; the sender identity and injected-input acceptance
  are still unverified.
- Updates the C1 firmware size reporting budget to match its 100 KiB allocation.

## Build and verification

The release workflow builds matching C1, C2, BH and CAN images and packages
`BUILD_INFO.json` and `SHA256SUMS`. Install matching board images together and
check their build identity and checksums before flashing. The PR's GitHub CI
passed host tests, Lab tests, lint and firmware builds. The release workflow
must pass its pinned toolchain checks before publication.

The local validation used Arm GNU 15.2.Rel1; CI and release builds use the
pinned Arm GNU 15.3.Rel1 toolchain and may produce different sizes. The largest
reported local C1 image used 102,236 of 102,400 bytes of flash. No firmware with
the new BH selector replay has been accepted in a vehicle; verify startup mode,
release behavior, failure status and later physical knob operation on hardware.

# BACCAble v5-beta-24

This beta rebuilds the firmware from master commit `95806ba`, the merge of the
v5-beta-24 release-notes PR. That revision includes PR #59 (`42e3098`), which
integrated the `Experiments` menu and public-source readings. The firmware
source tree is the same as v5-beta-23; this release records the integrated
master revision and its build identity. It adds no firmware behavior beyond
that beta.

## Included features

- Adds the separate `Experiments` menu with `Electrical`, `Oil`, `Engine` and
  `Vehicle` groups. It contains additional manufacturer diagnostic readings
  collected from public Giulia/Stelvio sources. These requests and their scaling
  have not been verified on this vehicle; unavailable or rejected replies show
  `--`. The section remains separate from ordinary `Readings` and Favorites.
- Includes the Readings next-page prefetch and the start-up drive-mode and
  lane-assist changes from the preceding integration. `Disable LKA` sends the
  recorded left-stalk double-click on C2 `0x384` once at start-up when LKA is
  reported enabled. Its physical effect and later driver re-enable still need
  vehicle verification.
- Includes the passive CAN catalog updates for steering angle, four wheel
  speeds, brake pedal position and accelerator pedal position. These are Lab
  decoders; they do not add firmware controls. Wheel-speed scale still needs
  comparison with an independent vehicle-speed source.

## Build and verification

The release workflow runs the shared CI checks, builds matching C1, C2, BH and
CAN images, and packages `BUILD_INFO.json` and `SHA256SUMS`. Install matching
board images together. Check the build identity and checksums before flashing.

The added diagnostic readings and driver-assistance behavior have not been
verified with this release installed in a vehicle. Public-source readings can
be unsupported on other model years, engines or ECU variants.

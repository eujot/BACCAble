# BACCAble v5-beta-36

This beta builds on v5-beta-35 with menu and input improvements, clearer vehicle
configuration, and restored reading semantics. It includes the changes merged
in PR #99.

## Included changes

### Menu operation and input

- Save and discard prompts are consistent across Vehicle, Features and Favorites.
  Holding the control saves; the documented double-click action discards. Save
  failures keep the edit available for retry.
- Favorites retains the selected reading format while editing, protects dirty
  drafts during idle, filters readings by vehicle support and gives clearer
  validation feedback. Restore defaults is identified as a Favorites action.
- Menu navigation recovers from neutral button reports and direct direction
  changes. Physical direction clicks update the display promptly; generated
  hold repeats are paced to the display transfer rate.
- Action confirmations retain the target operation and allow six seconds to
  confirm. Information labels explain steering input report counts and gaps.

### Vehicle and settings

- Vehicle labels clarify Adaptive susp., AdBlue / SCR and Startup LKA. Vehicle
  settings explain when stored options are unavailable for the selected profile.
- Features are grouped more clearly; default DNA mode and suspension settings are
  adjacent, and unsupported Race selection is skipped.
- Root menu order places Vehicle before Information; Settings calls the editor
  `Edit favorites`.

### Readings and Favorites

- Reading names distinguish oil quality, charge sources, turbo requests,
  regeneration values and over-rev time/count. Short labels are unique and the
  redundant percent sign was removed from OilQ.
- Acceleration timers show `RUN` during an attempt, `MISS` when no valid result
  exists, and seconds for a completed result. Empty Best values no longer show
  internal timer placeholders. Stale readings show `--` in Readings and Favorites.
- The first speed report establishes a baseline. Startup while moving cannot
  create a launch result, and a resumed speed report after more than one second
  cancels an active attempt before a stale crossing can be saved.
- Gear, DNA, pedal map, seatbelt alarm and DPF regeneration values use their
  native meanings. `Seatbelt alarm` indicates acoustic-warning configuration,
  not belt fastening.

## Build and verification

PR #99 host tests, BACCAble Lab tests, lint and C1/C2/BH/CAN firmware builds
passed on the GitHub CI workflow. The release workflow will rebuild the images
with the pinned Arm GNU toolchain and publish checksummed assets.

No board was flashed for these changes. Input behavior, IPC presentation and
acceleration timing still need acceptance with the matching firmware installed
in the vehicle.

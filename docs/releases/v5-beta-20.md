# BACCAble v5-beta-20

This beta includes the MY23 IPC refresh controls from PR #48 and the 16-column
Readings and Favorites corrections from PR #49. Firmware artifacts record the
exact build identity in `BUILD_INFO.json`; `SHA256SUMS` verifies downloads.

## Changes

- MY23 Readings keep their complete catalog pages on one line and fit the
  measured 16-character upper display field. Compact templates remove excess
  spaces while retaining clear names and values.
- Favorites select two complete catalog pages, one per display line. Diagnostic
  values across both pages are deduplicated and receive enough polling time to
  avoid unnecessary `--` gaps.
- Invalid gear and other fractional or out-of-range enum values render as
  unavailable instead of misleading numbers or labels.
- Settings → BACCAble IPC provides temporary Safe / Quick / Fast refresh pacing
  and Full / Delta write modes. These choices reset to Safe + Full on power
  cycle; Full output uses complete padded screen passes.
- The menu retains the active audio source and periodically reasserts its screen
  content. Radio takeover and vehicle appearance still require hardware checks.

## Installation and limits

Install the matching C1, C2 and BH images from this release together. The CAN
image is for its separate role. Verify downloaded files with the included
checksums. The release workflow runs the shared host, static-analysis and
firmware-build checks; these do not verify display appearance, button timing or
CAN capture in a vehicle.

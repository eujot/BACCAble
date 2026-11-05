# BACCAble v5-beta-38

This beta adds persistent battery history to the C1 firmware. It includes PR #102 and its audit corrections.

## Battery history

- `Readings → Battery → History` lets you browse samples from the latest ten engine-start sessions on Legacy and MY23 displays.
- The logger records SOC IBS and the same battery-voltage source used by Readings, every ten minutes. It keeps up to 73 samples (twelve hours) per session and does not require the menu to remain open.
- Samples are marked unavailable when current data cannot be obtained. Voltage uses a bounded background diagnostic request; DTC operations pause requests. Logging does not wake the vehicle.
- Records use CRC and a commit marker so an interrupted sample is ignored. Old sessions are reclaimed only when safe Flash space is prepared.
- C1 now allocates 104 KiB to application code and 12 KiB to history. Existing settings retain their address. C1 USB mass storage is unavailable; serial USB diagnostics remain available. Back up any files from a custom C1 mass-storage build before upgrading because that former disk area is reused.

## Build and verification

PR #102 host tests, BACCAble Lab tests, lint and C1/C2/BH/CAN firmware builds passed in CI. Host coverage includes session retention, interrupted and corrupted records, query handling, time limits and menu navigation. No board was flashed.

History operation, Flash erase/program latency against CAN and steering, and real vehicle session detection still need hardware acceptance. Physical 128 KiB Flash is required.

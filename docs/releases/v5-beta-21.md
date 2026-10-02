# BACCAble v5-beta-21

This beta sets the owner-tested 20 ms IPC menu refresh to the startup and reset
default. The owner reported stable display output without visible artifacts.
Full-screen transmission remains the default; Safe 50ms and Fast 10ms remain
available as temporary test choices.

## Changes

- C1 and BH now use Quick 20ms + Write Full after power-up, Reset default,
  or expiry of a temporary setting. A selected Safe 50ms + Full configuration
  is renewed correctly while C1 is active.
- Button-driven renders restart the 100 ms periodic refresh interval, avoiding
  an immediate duplicate render in the same menu loop.
- Duplicate screen packets are filtered before assembling the UART packet.
- The display stream skips content comparisons for fragments already forced
  dirty while retaining complete-screen order and periodic reassertion.
- English and Polish user guides and the IPC refresh architecture notes describe
  the 20 ms default.

## Validation and limits

Host tests with Apple Clang and ASan/UBSan, 17 CAN dictionary tests, cppcheck,
and full C1/C2/BH/CAN builds passed locally with Arm GNU Toolchain 15.2.Rel1.
The release workflow runs the pinned shared CI and produces matching firmware
artifacts, BUILD_INFO.json, and SHA256SUMS.

The 20 ms behavior was accepted by the owner on the previously tested firmware.
The new binaries have not been installed or tested in a vehicle. Local linked
sizes changed by C1 -20 bytes, BH +8 bytes, and no change for C2/CAN; static RAM
usage is unchanged. These numbers use Arm GNU 15.2.Rel1 and are not the release
workflow's pinned compiler measurements.

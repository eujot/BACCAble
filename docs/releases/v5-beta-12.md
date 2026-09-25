# Beta 12 — USB CAN capture and BACCAble Lab

## Included

- Integrate BACCAble Lab Milestone 1: USB device discovery, binary CAN recording,
  raw files, SQLite sessions, event markers, session inspection and CSV export.
- Record any selected subset of C1/C2/BH, including a single CAN bus.
- Identify firmware roles in USB product names and serial prefixes; device paths
  may change after this update. Capture still requires explicit role-to-port mapping.
- Restore USB CAN activation when reselecting a saved mode after session expiry.
- Configure HSI48 synchronization to USB SOF for crystal-less USB operation.
- Drain the bounded capture ring while CDC has room, retaining records on busy.
- Stop C2/BH USB presence reports from granting unsolicited transmit windows on
  the shared UART, which could disrupt inter-board communication and display updates.
- Add firmware regressions and run the Lab tests in CI.

## Use

Install matching C1/C2/BH images. Connect the required USB cables before applying
USB mode CAN on C1. Run `baccable doctor`, then for one bus, for example:

```sh
baccable capture --port C1=/dev/cu.usbmodemXXXX --no-obd --no-voice
```

Add C2/BH mappings to record additional buses. Lab setup instructions are in
`baccable_lab/README.md` in the source tree; the firmware archive is not a Python
application installer.

## Validation and known limits

Host regressions and the four firmware variants are checked by the release CI.
Three-board enumeration, menu responsiveness and sustained capture still require
vehicle acceptance; this beta does not claim lossless or hardware-verified capture.

- An unconfigured USB capture session expires after ten seconds independently on
  each board. To rearm after late connection, apply OFF and then CAN with cables connected.
- Lab shutdown can omit queued tail chunks; firmware software-ring loss markers
  do not include all hardware CAN FIFO overruns.
- Streams contain received frames, not local diagnostic transmissions. Board
  clocks are independent, and the record format lacks an explicit extended-ID flag.
- SOC source mappings and decoding are unchanged. Missing SOC remains under
  investigation; the old IBS source is available on the Advanced Battery sources page.

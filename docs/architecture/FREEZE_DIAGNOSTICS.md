# Freeze investigation build

The normal firmware now recovers USART2 if an accepted interrupt-driven transmit
has not completed within one second. It aborts the old transfer and resumes RX;
subsequent screens/commands can proceed. An uncertain old command is not replayed
because it may already have been executed. This handles a stuck transfer flag,
not a stopped CPU, disabled clock, or every possible CAN/UART fault.

## Build and collect logs

Build **all three boards** with `EXTRA_CPPFLAGS=-DFREEZE_DIAGNOSTICS` and matching
display widths. The supplied local test images are in `build/freeze-C1`,
`build/freeze-C2` and `build/freeze-BH` under `firmware/baccable`.

This diagnostic variant exposes a read-only USB disk on C1 as well as C2/BH.
**C1 ELM is disabled to make room for disk support. C1 USB also takes the LED-strip
pins.** USB capture is not needed; leave it disabled during this investigation.
Use the ordinary build to test behavior specifically involving ELM sessions.
Physical 128 KiB Flash remains required. The usual flashing guide applies.

1. Flash the matching BIN to each controller and run normally without USB.
2. After ten seconds, a disk checkpoint is attempted. Subsequent checkpoints
   occur every five minutes, or after UART recovery with at least one minute
   between attempts. Storage failures are throttled too.
3. If the screen freezes, note the time, active reading, last action and LEDs.
   When practical, wait at least one minute before removing power. A stopped
   processor cannot produce another checkpoint.
4. After restarting, connect each board's USB to a computer in normal runtime
   mode, **not DFU**, and copy `FREEZE.LOG`. Rename the copies `C1-FREEZE.LOG`,
   `C2-FREEZE.LOG` and `BH-FREEZE.LOG` and provide all three with the observations.
   Do not repeatedly power-cycle before collecting them.

The file holds 24 fixed 256-byte records (6 KiB). It wraps instead of growing;
records are not necessarily in chronological file order. Uptime restarts from
zero after each boot. Each successful checkpoint is synchronized to Flash.
Existing disk contents are not cleared by the logger; the existing filesystem
initializer may format an unformatted disk. A full or damaged disk can prevent
logging. Copy existing files before installing diagnostic firmware.

## Fields

| Field | Meaning |
| --- | --- |
| Firmware prefix | Build identity; retain the exact ELF with the logs |
| `ms`, `loop` | Uptime and main-loop progress |
| `reset` | Raw STM32 RCC CSR reset flags captured before initialization |
| `uart_to` | USART2 transmit timeout recoveries since boot |
| `gap`, `min_gap` | Current and minimum sampled stack-to-heap gap, bytes |
| `oom` | Rejected heap-growth requests; not every possible memory corruption |
| `can_age` | Milliseconds since normal processing last received a CAN frame |
| `esr` | Raw CAN error status, including bus-off/error counters |
| `sleep`, `diag` | Low-power and exclusive board diagnostic-link state |
| `temp=NA` | MCU/PCB temperature is **not measured** |

The stack gap is sampled at the logger, not a full stack high-water measurement.
The logger avoids writing when its sampled gap is below 768 bytes. It uses no
heap allocation of its own. Tiny FatFs mode reduces local file-object stack use;
USB transfers use one 512-byte sector in diagnostic builds.

No firmware disk writes occur while USB is configured or a runtime USB mode is
active. USB disk reads are rejected while a checkpoint is being written. Flash
writes can briefly delay CAN/UART processing; logging is deliberately sparse and
is intended for investigation, not permanent high-rate telemetry.

## Limits and next evidence

A checkpoint survives power removal after its successful save, but the last
unsaved interval is lost. Power interruption during a Flash/FAT update may damage
the last record or filesystem. There is no claim of power-fail-atomic logging.
A HardFault or dead loop still requires SWD inspection for the exact program
counter and stack: this patch does not write Flash from a fault handler or add
a watchdog reset that could hide the failure.

A solid green LED does not identify a fault by itself. For suspected overheating,
record enclosure/board temperature with an external probe and compare cold/warm
operation. Oil/coolant readings are not electronics-temperature measurements.
The MCU ADC and temperature sensor are not initialized in this firmware; adding
an unvalidated reading would be misleading.

Tests cover missing UART completion and the production logger with substituted
clock/filesystem interfaces (bounds, cadence, failed mounts, USB exclusion and
low-memory guard). Physical flash endurance, power-cut behavior, USB enumeration
and the freeze itself still require device testing.

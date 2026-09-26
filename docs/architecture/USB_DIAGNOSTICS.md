# USB diagnostics and new menu actions

**Documentation maintenance:** changes to USB modes, capture/diagnostic operation,
commands, timeouts or related menu actions must also update the affected sections
of both user guides: [English](../../manuals/BACCAble_USER_GUIDE_EN.md) and
[Polish](../../manuals/BACCAble_USER_GUIDE_PL.md). Follow the
[AGENTS.md maintenance rule](../../AGENTS.md#keep-both-user-guides-current)
and keep the instructions synchronized in the same change.

Use a matching C1/C2/BH firmware set from this integration. Verify the C1 Flash
requirement in [the integration report](UPSTREAM_SYNC.md#flash-and-ram) before
flashing. The commands below describe runtime USB, not the STM32 DFU bootloader.

## Reading pages and actions

- `Settings` → `Features` → `Auto rotate`: enable to advance through the
  current list every five seconds. Use favorites to create a short rotating set.
- `Actions` → `Peak hold`: toggle between live values and numerical maxima.
  Changing the selected page starts a new interval. An unavailable or stale signal
  displays `--` rather than an old maximum.
- `Settings` → `Features` → `BCM fault reader`: enable, then leave Features. Open
  `Actions` → `Read BCM faults`; browse results with previous/next, press RES to
  retry, or go Back to leave. A `+` after the result count means the BCM returned
  more than the displayed limit of 20 codes. Reading does not clear faults.
- `Auto PDC mute` and `Mute audio in R` are independent options in Features.
  They require valid current vehicle messages before acting.
- `Actions` → `IBS SOC override` is experimental and requires action confirmation
  and a running engine. It substitutes 75% in a short burst of otherwise copied
  IBS messages when the observed SOC is within the upstream range. It is not a
  verified way to improve battery charging. It is off after startup and stops at
  engine stop or diagnostic-mode entry.

Leave Features to persist committed changes automatically. New settings are initially off;
existing saved settings take priority over compiled defaults. Despite its current
`Clear DTCs` label, clearing still requests the existing multi-controller sweep;
only the reader is restricted to BCM. The historical FREEZE.LOG branch is obsolete while
the current software remains stable.

## ELM-compatible USB diagnostics

1. Connect the **C1 USB port** with a data-capable cable. On the dedicated board
   described in the original manual, C1 is the right-hand port, C2 the middle and
   BH the left in the manual's orientation. Prefer PCB labels; viewing the board
   from the opposite side reverses left and right.
2. Select `Settings` → `Features` → `USB mode: ELM327`, then leave Features. This replaces
   CAN capture. USB reconnects as a serial device; select its new port in
   the host application. Connect/open it promptly: an unconfigured USB session
   expires after ten seconds.
3. Use a terminal or diagnostic application supporting an ELM-style CAN adapter.
   Commands end with carriage return. For example, `ATI` identifies the
   interpreter, `ATZ` resets command preferences, `ATE0` disables echo and `ATH1`
   enables response addresses. A response finishes with `>`.
4. The interpreter tries the vehicle buses and remembers where each requested
   controller answered. Physical bus rates remain C1/C2 **500 kbit/s** and BH
   **125 kbit/s**. Protocol/divisor commands influence adapter preferences and
   search order; they do not reconfigure the vehicle's physical bus speeds.
5. Close the client when finished. A USB disconnect expires after ten seconds;
   120 seconds without a complete ELM command also ends the session. Auxiliary
   boards restore normal operation after their own lease expires if C1 vanishes.

While ELM diagnostics owns the buses, normal feature processing is suspended.
Do not expect the display menu to remain navigable during a diagnostic session.
The USB port shares a C1 LED-strip pin, so strip output pauses while USB uses it.

This is a bounded ELM-compatible subset, not a complete ELM327 implementation.
It supports CAN request formatting, filters, raw frames and automatic ISO-TP
assembly up to 255 response bytes, with client requests up to 32 bytes. It does
not add K-line/J1850 hardware or ECU security access. Unsupported AT commands
return `?`; unavailable replies return `NO DATA`. `ATRV` uses a cached measured
voltage if available and otherwise returns `NO DATA`. Compatibility with each
third-party diagnostic application needs testing. Remote ISO-TP flow control
requires at least 10 ms between consecutive frames to fit the inter-board link.

On macOS, look for the newly appearing `/dev/cu.usbmodem*`; on Linux, `/dev/ttyACM*`;
on Windows, use the newly enumerated COM port. Port numbers are host-assigned.

## Binary CAN capture

1. Connect the USB port of the bus to record: C1 for powertrain, C2 for chassis,
   BH for body. Multiple host connections can record the buses separately.
   Eject an existing USB disk before changing its role.
2. On C1 select `USB mode: CAN` in Features and leave that submenu. ELM mode is
   switched off. C1 sends the same capture setting to C2/BH over the board link.
   An unconfigured session expires after ten seconds, so connect the cables first.
3. Open the serial device as a binary stream. This mode does not produce SLCAN
   text or automatically create a CAN log on the USB disk. Normal vehicle feature
   processing continues during capture.
4. Save complete 16-byte records and decode the fields below. Each stream uses
   that board's local clock, so timestamps from different boards need alignment.
5. Disconnect to expire capture, or disable it and leave Features. A connected
   auxiliary capture port keeps the board set awake. Timeout changes the current
   session; select OFF and leave Features if capture should also stay off after reboot.

| Bytes | Meaning |
| --- | --- |
| 0 | `0xa0` through `0xa8`: low nibble is CAN DLC |
| 1–3 | Millisecond timestamp, little-endian, wrapping at 24 bits |
| 4–7 | CAN identifier, little-endian |
| 8–15 | CAN data, unused bytes zeroed |

A record beginning with `0xaf` reports capture loss: bytes 4–5 contain the dropped
frame count, capped at 65535; a later incoming frame emits the marker once buffer
space is available. This extension distinguishes overflow from ordinary data.
The upstream record format has no explicit standard/extended-ID flag and does
not record RTR frames. Capture is bounded and may drop data when the host cannot
keep up; the loss marker should be retained in exported logs.

## Runtime USB audit, 2026-09-25

Follow-up after the user observed runtime serial enumeration, but only two ports
with three cables and intermittent UI unresponsiveness:

- Auxiliary USB presence reporting incorrectly updated
  `we_can_send_a_message_reply` every second and on connection changes. C2/BH
  thereby granted themselves transmit windows on the shared half-duplex UART,
  bypassing C1 arbitration. Simultaneous replies can corrupt board traffic,
  including display updates. Presence is now queued without granting a window;
  normal addressed commands/status polls from C1 permit transmission. A regression
  test rejects the old behavior on C2; both auxiliary flavors pass with the fix.
  This is a confirmed code defect, not yet a confirmed explanation of the user's
  particular hardware stall.
- USB product names now include C1/C2/BH (CA for CANable), and serial numbers
  prefix the existing UID-derived value with the role. `baccable doctor` can show
  the role even when UID contents match across boards (the user previously
  reported identical DFU serials). This cannot restore a device that fails to
  enumerate. macOS device paths may change after installing this firmware.
- Each auxiliary capture session still expires independently after ten seconds
  without USB configuration. Connecting a third cable later does not rearm that
  board automatically. Connect all cables, select OFF and leave Features, then
  select CAN and leave Features to resend activation. The mode broadcast has no
  per-board acknowledgement; accepted UART enqueueing alone does not prove all
  boards received it. Missing ports require role identification before diagnosis.
- The capture drain is bounded by the 16-record ring, only filled in the main
  loop; CDC busy returns immediately and retains the record. USB transmit-full
  reporting only sets an error bit. No blocking wait for a host reader was found.
  Mode switching does include 22 ms of USB reset delays (and C1 UART resume),
  which can temporarily delay processing; this is not an infinite capture loop.
  USB low-power entry is disabled. Physical UART collision rates and USB/CPU load
  remain hardware acceptance items.

Follow-up acceptance: flash the matching three-role set, connect all three USB
cables before enabling CAN, run `doctor` and record each role/serial/path. Check
menu navigation with all ports unopened, while recording all three, and after
stopping the recorder. Then test disconnect/reconnect both within and beyond the
ten-second expiry. Record which role disappears and whether the menu recovers
after CAN is disabled; do not infer a hardware lockup from a stale display alone.

The user observed all three STM32 DFU devices (`0483:df11`) through the same
cables and hub, but no runtime serial ports after selecting CAN. This establishes
working DFU USB communication, not successful application enumeration. The local
candidate `usb-fix-5d8a8f8` on `milestone-1-recorder` addresses these findings:

- **Rearming a saved mode:** expiry clears the runtime request and the RAM menu
  flags but leaves Flash unchanged. Selecting CAN again could reproduce the
  saved record exactly; the no-write path then skipped both USB activation and
  auxiliary-board synchronization. That path now reapplies a changed runtime
  request and resends board settings, without an unnecessary Flash write.
- **USB clock:** the default HSI48 configuration had only a comment about CRS,
  without enabling it. Automatic synchronization to USB SOF is now configured,
  following the [ST F072 CDC clock example](https://github.com/STMicroelectronics/STM32CubeF0/blob/master/Projects/STM32072B_EVAL/Applications/USB_Device/CDC_Standalone/Src/main.c).
  The HSE/PLL build retains its existing clock source. Missing CRS is a clock
  reliability defect; its role in the reported enumeration failure still needs
  hardware verification.
- **Capture throughput:** each main-loop pass can receive eight CAN frames but
  previously moved only four into the CDC queue. The bounded capture ring now
  drains while CDC has room, retaining records when CDC returns busy. Simulated
  sustained eight-frame batches no longer overflow merely because of that
  scheduling mismatch. This does not establish maximum real bus throughput.

The earlier claim that missing `GPIO_AF2_USB` configuration conclusively explains
the failure was incorrect. The [ST F072 CDC MSP example](https://github.com/STMicroelectronics/STM32CubeF0/blob/master/Projects/STM32072B_EVAL/Applications/USB_Device/CDC_Standalone/Src/usbd_conf.c)
explicitly describes that GPIO configuration as optional. No speculative GPIO
change is included. Inspection also covered USB descriptors/class switching,
endpoint PMA placement, IRQ dispatch, CDC buffer ownership, board command routing,
power/LED ownership, CAN receive filters and ELM filter restoration. No additional
confirmed enumeration blocker was found in those paths.

Host regressions cover expiry/rearm, no-write persistence, CDC busy retry, record
format/loss markers and C1/C2/BH throughput. The persistence and auxiliary
throughput regressions fail against the original production sources. Four-role
ARM builds and cppcheck pass. These are software checks, not Mac USB acceptance.

Hardware acceptance after installing the matching C1/C2/BH candidate:

1. Exit DFU, connect all three USB cables, select CAN on C1 and leave Features.
   Confirm three runtime serial devices (VID:PID `0483:5740`), then map their
   roles explicitly using PCB labels and one cable at a time. Reapply CAN if an
   unplugged board's ten-second session expires.
2. Exercise expiry and rearm: disconnect all cables for over ten seconds,
   reconnect them, select CAN and leave Features. Confirm all three ports return
   without changing an unrelated setting. Repeat after reboot with CAN saved.
3. Record a stationary session with `baccable capture --port C1=... --port C2=...
   --port BH=... --no-obd --no-voice`. Require plausible nonzero frame counts on
   each active bus, parseable 16-byte records and inspection of reported losses.

Known capture limits remain: hardware CAN FIFO overruns are not counted by the
`0xaf` software-ring marker; zero reported drops is not proof of lossless capture.
Frames are timestamped when processed, and board clocks are independent. RTR and
the explicit standard/extended flag are absent. Normal vehicle features continue.
The beta-12 Mac recorder does not drain its pending input queue on shutdown.
The local post-beta-12 fix drains it while readers finish their final read, and
aborts blocked producers on storage failure. This protects bytes already read
by Lab, not data still buffered by the OS/device when capture stops. Session
failure is reported when storage or cleanup fails. See ACTION_PLAN for validation.

The local post-beta-12 firmware also avoids resetting an awake C1 UART when
entering USB mode. Only a UART paused by low-power entry is resumed; pending
normal transmissions retain their HAL state. Test USB entry during menu traffic
and wake from low power on hardware before claiming the stall is resolved.

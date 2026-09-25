# Hardware setup

1. Use a matching C1/C2/BH firmware release.
2. Connect the boards whose CAN buses you want to record to the Mac (one board
   is sufficient; use a powered hub for multiple USB connections). Keep the Vgate
   adapter separate; Milestone 1 does not use OBD.
3. In the C1 menu select `Settings -> Features -> USB mode: CAN` and leave
   Features. C1 applies its mode and sends the capture setting to C2/BH over
   the board link; there is no separate menu setting on each auxiliary board.
   Connect USB first because an unconfigured session expires after ten seconds.
4. Run `baccable doctor` and map the selected `/dev/cu.usbmodem*` paths explicitly.
5. For C1 alone use `baccable capture --port C1=/dev/cu.usbmodemXXXX --no-obd --no-voice`.
   Add `--port C2=...` and/or `--port BH=...` only when recording those buses.

Selecting ports in Lab only controls what the Mac records. The C1 menu still
propagates the firmware USB mode to the other boards.

The USB responsiveness candidate adds C1/C2/BH to USB product names and serial
prefixes; `doctor` displays these when that firmware is installed. Older builds
still require cable-by-cable identification. Device paths may change after an
update. If a board was connected after its ten-second CAN session expired,
connect all cables, apply OFF in Features, then apply CAN again.

This firmware mode is binary capture, not SLCAN, and normal BACCAble feature
processing continues. It is not a guarantee that the boards are electrically
silent. Do not call a session passive without separate hardware evidence.

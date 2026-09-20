# Hardware setup

1. Use a matching C1/C2/BH firmware release.
2. On each board select `Settings -> Features -> USB mode: CAN` and leave the
   submenu so the setting is applied.
3. Connect C1, C2 and BH to the Mac through a powered hub. Keep the Vgate
   adapter separate; Milestone 1 does not use OBD.
4. Run `baccable doctor` and map the three `/dev/cu.usbmodem*` paths explicitly.
5. Use `baccable capture --no-obd --no-voice` for the first capture.

This firmware mode is binary capture, not SLCAN, and normal BACCAble feature
processing continues. It is not a guarantee that the boards are electrically
silent. Do not call a session passive without separate hardware evidence.

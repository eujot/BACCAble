# BACCAble hardware acceptance record

This document is the worksheet for P1-06 in [ACTION_PLAN.md](ACTION_PLAN.md).
It records physical evidence for one complete C1/C2/BH installation. A firmware
build, release page or host test does not fill in an observed hardware result.
Leave unknown fields as `UNKNOWN`; do not infer installation from an available
binary.

## Candidate identity

| Field | C1 | C2 | BH |
| --- | --- | --- | --- |
| Firmware version / commit |  |  |  |
| Exact ELF or release asset |  |  |  |
| Board revision / MCU marking |  |  |  |
| Reported and physically confirmed Flash |  |  |  |
| IPC / vehicle revision |  |  |  |
| Display width (18/24) |  |  |  |
| Engine profile / feature flags |  |  |  |
| Flash date and operator |  |  |  |

Record the output of `dfu-util --list`, the source commit, build version and
flags. C1 requires physically confirmed 128 KiB Flash. Follow
[FLASHING.md](FLASHING.md); keep one DFU target connected at a time.

## Test conditions

Record vehicle model/year, battery voltage, engine state, gear, ambient
conditions, display type, IPC revision, connected USB devices and whether all
three boards run the same candidate. Attach captures or photographs by filename
and state where they are stored.

## Acceptance checks

For every row write `PASS`, `FAIL` or `UNKNOWN` and include an observation or
capture reference. `Request sent` or UART queue acceptance is not physical ACK.

| Area | Procedure and evidence to record | Result / evidence |
| --- | --- | --- |
| Menu/input | Short and held RES are exclusive; rapid NEXT/PREV and repeat work; no stale input after a report gap; 18/24-character text and glyphs are readable. |  |
| Idle/save | Idle returns to visible Favorites; unaccepted edits/capture cancel; committed values survive restart; failed save exit retries without losing destination. |  |
| Display | Measure first visible response and full completion during rapid pages, factory text, radio/RDS handover and Race mask. Record IPC capture and timings. |  |
| Dyno/brake | Enable Dyno, stop engine, restart, and verify C1/C2 agree. Exercise brake guard and record observed vehicle behavior. |  |
| PDC/audio | Exercise BACCAble-owned PDC mute, Reverse restore, manual PDC choice and BH reverse audio. Record sensor LED and audio result. |  |
| QV GPIO | Start an exhaust pulse, disable the preference and cross engine-stop/sleep boundaries. Confirm Q10/Q11 release physically. |  |
| Other vehicle features | Test launch, ESC/TC, AWD, mirror, ACC, windows, pedal modes and relevant preferences on compatible equipment. |  |
| Diagnostics | Verify BCM read/clear exclusion, multi-controller DTC clear wording, ELM discovery/disconnect, CAN capture overflow and USB ownership. |  |
| Reliability | Exercise startup, sleep/wake, disconnect and sustained operation. The old freeze is currently absent; record a new incident only if it returns. |  |
| Resources | Record runtime stack/memory evidence for the exact flags. Linker totals alone do not prove worst-case stack safety. |  |

## Result

Candidate:
Overall result: `PASS` / `FAIL` / `INCOMPLETE`
Open findings:
Next action:

An incomplete worksheet keeps P1-06 open. Update the action plan with the exact
candidate, evidence paths and observed results after the vehicle session.

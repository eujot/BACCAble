# Capture markers

Markers add a host timestamp to a human-observed action while BACCAble Lab
records raw CAN traffic. They are not generated from CAN and do not prove that
a named ECU, feature or signal exists. Feature availability, wording and network
layout vary by vehicle model, year, market and equipment. The Alfa Romeo Giulia
handbook, for example, describes optional driver assistance, rain sensing,
different DNA modes and active chassis features; it does not define their CAN
IDs or payload encoding.

## Fast entry

The capture screen keeps the quick-key meanings visible. Keys `1`–`0` retain the
established actions:

| Key | Marker | Key | Marker |
| --- | --- | --- | --- |
| `1` | Unlock | `6` | Brake off |
| `2` | Driver door open | `7` | Left indicator on |
| `3` | Driver door close | `8` | Left indicator off |
| `4` | Engine start | `9` | Right indicator on |
| `5` | Brake on | `0` | Right indicator off |

Press `f` and type an action or keyword in English or Polish. A single best match is recorded with
its stable catalog label. When several labels match, choose a numbered result;
press Enter to save the original phrase as a custom marker. When nothing
matches, the phrase is saved directly as a custom marker. Search ignores case
and accents, and recognizes common terms such as ABS, ESC/ESP, ASR/TCS, ACC,
FCW, AEB, BSM, LDW/LKA, TPMS, PDC, DNA, Q4, ADC, SOC, and Polish terms such as
`deszcz` or `temperatura intercoolera`. `c` always opens a custom
marker prompt. `g` browses the full catalog; the group key and item number are
shown on screen. Press `n` to annotate the latest marker, `u` to undo it, and
`q` to finish.

## Catalog

Use the group selector (`g`) when you want a known label rather than search.
Every action below is a manually placed timestamp:

| Group | Available markers |
| --- | --- |
| Vehicle | Unlock, lock, driver/passenger door open or close, engine start/stop, ignition on/off |
| Driving | Accelerator pressed/released, cruise control set/resumed/cancelled, speed changed, hill start, wheel slip, rough road, road event |
| Powertrain / transmission | Reverse/neutral/park/drive selected, gear up/down, manual mode, DNA mode, Start/Stop active/restarted |
| Engine / live readings | RPM/load/boost changed, coolant/oil/intercooler temperature high, oil pressure warning, battery SOC/fuel level changed, cooling fan active |
| Brakes / stability | Brake and parking brake on/off, ABS/ESC/traction control intervention, hill descent, brake warning/hold |
| Driver assistance / safety | ACC set/following/resumed/cancelled, forward collision warning, emergency braking, blind spot warning, lane warning/steering intervention, driver attention warning |
| Body / access | Trunk, hood and fuel door, windows, mirrors, seat belt |
| IPC / media | Menu opened/overwritten/restored, screen/source/track changes, radio button, phone call, navigation prompt |
| Suspension / chassis | Suspension active/inactive/mode, ride height, leveling, AWD, steering mode/input |
| Weather / wipers | Rain start/stop, wiper speeds/off, washer, rear wiper, ambient light, outside temperature |
| Lights | Left/right indicators, dipped beam, headlights, high beam, fog and hazard lights |
| Lighting / visibility | Side/automatic headlights, high-beam/fog/hazard off, interior and brake lights |
| Climate / comfort | A/C, temperature, fan, front/rear defrost, recirculation, climate mode, seat/steering heat |
| Parking / manoeuvring | Parking sensors, warnings, reverse camera, park assist start/finish, parking brake, door warning, vehicle approach |
| Electrical / diagnostics | Module wake/sleep, warning/fault/clear messages, TPMS/battery warnings, charging change, OBD connection, diagnostic session |

Use custom text for an event not represented here, including vehicle-specific
conditions such as coolant or intercooler temperature, engine load, fan
operation, a particular dashboard message, or a precise test step. Matching
search terms include `SOC`, `RPM`, `boost`, `coolant temp` and `intercooler`.
Add a short
note (`n`) when the marker alone is ambiguous (for example, which door, which
DNA mode, or what triggered an intervention).

## What a raw dump can tell you

The live pane shows bus, timestamp, CAN ID and payload bytes. CAN is a broadcast
network: an ID participates in bus arbitration and may conventionally map to a
transmitting ECU, but it is not a human-readable feature name. A raw dump needs
message definitions and signal layouts (often a DBC or manufacturer-specific
documentation), plus validation against the exact vehicle configuration, to
decode meanings. ISO-TP diagnostic traffic is a higher-level transport over
CAN and must be interpreted separately. BACCAble Lab currently records raw
frames and manual markers; it does not identify ECUs, decode vehicle signals,
or infer these catalog events.

The catalog is intended as a practical checklist for safe, ordinary
observations, not a request to trigger emergency braking, stability control,
collision warnings or other risky behavior. Only mark such an event if it
occurs naturally or during a controlled test in a safe place.

## References

- [Alfa Romeo Giulia owner handbook, 2020](https://aftersales.fiat.com/eLumData/EN/83/620_GIULIA/83_620_GIULIA_603.93.670_EN_01_03.20_L_LG/83_620_GIULIA_603.93.670_EN_01_03.20_L_LG.pdf)
- [Alfa Romeo Giulia owner handbook, 2018](https://aftersales.fiat.com/eLumData/EN/83/620_GIULIA/83_620_GIULIA_603.93.175_EN_01_09.18_L_LG%23AP_603.93.175_01_09.18/83_620_GIULIA_603.93.175_EN_01_09.18_L_LG%23AP_603.93.175_01_09.18.pdf)
- [Alfa Romeo Stelvio owner handbook, 2018](https://aftersales.fiat.com/eLumData/EN/83/630_STELVIO/83_630_STELVIO_603.93.295_EN_01_11.18_L_LG/83_630_STELVIO_603.93.295_EN_01_11.18_L_LG.pdf)
- [Fiat 500X owner handbook, 2015](https://aftersales.fiat.com/eLumData/EN/00/334_500X/00_334_500X_603.91.203_EN_01_01.17_L_LG%23AP_603.91.203_02_01.17/00_334_500X_603.91.203_EN_01_01.17_L_LG%23AP_603.91.203_02_01.17.pdf)
- [Official Stellantis eLUM handbook portal](https://aftersales.fiat.com/elum/ssoLandingPage.aspx?languageID=2) (select model, market or VIN for the matching manual)
- [Linux kernel SocketCAN documentation](https://docs.kernel.org/networking/can.html)
- [Linux kernel ISO-TP documentation](https://docs.kernel.org/networking/iso15765-2.html)

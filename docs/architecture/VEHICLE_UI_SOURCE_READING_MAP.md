# Vehicle profile / Readings source-to-reading map

Step 1 contract: every new stable `ReadingId` references an existing UDS
`ParameterDefinition` source (0..97) per engine class. Sources are unchanged;
this map is the new UI's canonical meaning table, independent of the old
`$...` page templates.

Eligibility legend: P20 = Petrol 2.0 (4-cyl), P29 = Petrol 2.9 (V6),
D22 = Diesel 2.2, All = every engine class. `—` = no source for that engine.

## Oil

| Reading | Label | Unit/type | Gas | Diesel | Eligible |
| --- | --- | --- | --- | --- | --- |
| `READING_OIL_TEMPERATURE` | Oil temp | C | 5 | 5 | All |
| `READING_OIL_PRESSURE` | Oil pressure | bar | 0 | 0 | All |
| `READING_OIL_LEVEL_MM` | Oil level | mm | 89 | 64 | All |
| `READING_OIL_LEVEL_STATUS` | Oil status | LOW/OK/HIGH | 89 | 64 | All |
| `READING_OIL_LEVEL_BAR` | Oil bar | status bar | 89 | 64 | All |
| `READING_OIL_VOLUME` | Oil volume | L | 28 | 28 | All |
| `READING_OIL_QUALITY` | Oil quality | % | 31 | 63 | All |

## Temperatures

| Reading | Label | Unit | Gas | Diesel | Eligible |
| --- | --- | --- | --- | --- | --- |
| `READING_COOLANT_TEMP` | Coolant | C | 42 | 68 | All |
| `READING_GEARBOX_TEMP` | Gearbox | C | 33 | 33 | All |
| `READING_INTERCOOLER_IN` | IC inlet | C | 23 | — | P20/P29 |
| `READING_INTERCOOLER_OUT` | IC outlet | C | 22 | — | P20/P29 |
| `READING_INTAKE_AIR_TEMP` | Intake air | C | — | 88 | D22 |
| `READING_EXHAUST_TEMP` | Exhaust | C | 40 | 67 | All |
| `READING_FUEL_TEMP` | Fuel temp | C | — | 84 | D22 |
| `READING_CATALYST_TEMP` | Catalyst | C | 41 | — | P20/P29 |

## Engine

| Reading | Label | Unit | Gas | Diesel | Eligible |
| --- | --- | --- | --- | --- | --- |
| `READING_POWER` | Power | PS | 1 | 1 | All |
| `READING_TORQUE` | Torque | Nm | 2 | 2 | All |
| `READING_BOOST` | Boost | bar | 25 | 79 | All |
| `READING_INTAKE_PRESSURE` | Intake | bar | 24 | — | P20/P29 |
| `READING_IGNITION_CYL_1..4` | Ign cyl 1..4 | deg | 45..48 | 45..48 | All |
| `READING_IGNITION_CYL_5..6` | Ign cyl 5..6 | deg | 19/20 | — | P29 |
| `READING_MISFIRES_TOTAL` | Misfires | count | 90 | 90 | All |
| `READING_EGR_TARGET` | EGR target | % | — | 73 | D22 |
| `READING_EGR_ACTUAL` | EGR actual | % | — | 74 | D22 |
| `READING_TURBO_TEMP` | Turbo temp | C | — | 78 | D22 |

## Fuel

| Reading | Label | Unit | Gas | Diesel | Eligible |
| --- | --- | --- | --- | --- | --- |
| `READING_FUEL_RATE` | Fuel rate | L/h | — | 87 | D22 |
| `READING_FUEL_PRESSURE` | Fuel rail | bar | — | 83 | D22 |

## Battery

| Reading | Label | Unit | Gas | Diesel | Eligible |
| --- | --- | --- | --- | --- | --- |
| `READING_BATTERY_VOLTAGE` | Battery | V | 35 | 62 | All |
| `READING_BATTERY_CURRENT` | Battery A | A | 4 | 4 | All |
| `READING_BATTERY_SOC_IBS` | Batt IBS | % | 3 | 3 | All |
| `READING_BATTERY_SOC_ECU` | Batt ECU | % | 21 | 21 | All |
| `READING_BATTERY_SOC_BCM` | Batt BCM | % | 34 | 34 | All |
| `READING_IBS_RAW0` | IBS raw 0 | raw | 95 | 95 | All |
| `READING_IBS_RAW1` | IBS raw 1 | raw | 96 | 96 | All |

## Diesel / DPF

| Reading | Label | Unit | Gas | Diesel | Eligible |
| --- | --- | --- | --- | --- | --- |
| `READING_DPF_LOAD` | DPF load | % | — | 55 | D22 |
| `READING_DPF_TEMP` | DPF temp | C | — | 56 | D22 |
| `READING_DPF_REGEN_PROGRESS` | Regen | % | — | 57 | D22 |
| `READING_DPF_REGEN_MODE` | Regen mode | enum | — | 8 | D22 |
| `READING_DPF_SINCE_REGEN` | Regen ago | km | — | 58 | D22 |
| `READING_DPF_REGEN_COUNT` | Regen count | count | — | 59 | D22 |
| `READING_DPF_AVG_INTERVAL` | Regen avg km | km | — | 60 | D22 |
| `READING_DPF_AVG_DURATION` | Regen avg min | min | — | 61 | D22 |
| `READING_ADBLUE_VOLUME` | AdBlue | L | — | 65 | D22 + SCR |
| `READING_ADBLUE_LEVEL` | AdBlue % | % | — | 66 | D22 + SCR |

## Driving

| Reading | Label | Unit | Gas | Diesel | Eligible |
| --- | --- | --- | --- | --- | --- |
| `READING_SPEED` | Speed | km/h | 7 | 7 | All |
| `READING_GEAR` | Gear | enum | 6 | 6 | All |
| `READING_DNA_MODE` | DNA mode | enum | 15 | 15 | All |
| `READING_PEDAL_MAP` | Pedal map | enum | 17 | 17 | All |
| `READING_SEATBELT` | Seatbelt | enum | 13 | 13 | All |
| `READING_DISTANCE` | Distance | km | 27 | 85 | All |

## Performance

| Reading | Label | Unit | Gas | Diesel | Eligible |
| --- | --- | --- | --- | --- | --- |
| `READING_TIME_0_100` | 0-100 | s | 9 | 9 | All |
| `READING_TIME_100_200` | 100-200 | s | 10 | 10 | All |
| `READING_BEST_0_100` | Best 0-100 | s | 11 | 11 | All |
| `READING_BEST_100_200` | Best 100-200 | s | 12 | 12 | All |

## Diagnostics

| Reading | Label | Unit | Gas | Diesel | Eligible |
| --- | --- | --- | --- | --- | --- |
| `READING_OIL_TEMP_ECU` | Oil ECU temp | C | 30 | 30 | All |
| `READING_OIL_PRESSURE_ECU` | Oil ECU press | bar | 29 | 29 | All |
| `READING_MULTIAIR_TEMP` | MultiAir | C | 32 | — | P20 |
| `READING_KNOCK_SENSOR` | Knock | mV | 43 | — | P20/P29 |
| `READING_KEY_ID` | Key ID | raw | 44 | — | P20/P29 |
| `READING_ENGINE_RUN_TIME` | Run time | min | 37 | 37 | All |
| `READING_OVER_REV_TIME` | Over-rev s | s | 38 | 38 | All |
| `READING_OVER_REV_COUNT` | Over-rev n | count | 39 | 39 | All |
| `READING_AC_PRESSURE` | A/C pressure | bar | 36 | 86 | All |
| `READING_TURBO_SENSOR_V` | Turbo V | V | 26 | — | P20/P29 |
| `READING_INTAKE_SENSOR_V` | Intake V | V | — | 82 | D22 |
| `READING_TURBO_TARGET_BAR` | Turbo req bar | bar | — | 76 | D22 |
| `READING_TURBO_TARGET_PCT` | Turbo req % | % | — | 77 | D22 |
| `READING_TURBO_ACTUAL_PCT` | Turbo % | % | — | 80 | D22 |
| `READING_BOOST_TARGET` | Boost req | bar | — | 81 | D22 |

## Default-set mapping and open items

- Warm-up: Oil temp + Coolant / Gearbox.
- Oil: Oil level mm + status / Oil level bar (exclusive full row).
- Engine: Power (RPM substitute; RPM is native telemetry, no UDS source) + Boost / Power + Torque.
- Cooling: Coolant / Intercooler in+out (gasoline) or Intake air (diesel).
- Battery: Voltage + current / IBS state of charge.
- Diesel DPF: DPF load + temp / regen progress.
- Gasoline Performance: last 0-100 / best 0-100.

Exact native enum label strings (gear, DNA, pedal map, seatbelt, DPF regen mode)
are pending decoder confirmation at the P8 UI cutover; the catalog already
returns bounded placeholders rather than guessing values that would be shown on
the display.

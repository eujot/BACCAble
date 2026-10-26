# CAN feature capabilities and limits

Evidence review: 2026-10-26. Firmware inspected on `research/light-origin-timing`,
based on `origin/master` 71b7ded; evidence includes dictionary 2026-10-25.6 and
its hypotheses. This is a design boundary, not a compatibility certification.
No vehicle transmissions, firmware changes or new hardware tests were made.

## The distinction that determines the design

A device can have a wired/LIN actuator and still accept a CAN request upstream.
Conversely, a CAN report of its state does not establish a CAN control input.
For every proposed feature distinguish:

1. Physical input: switch, sensor, remote receiver or another ECU.
2. Decision owner: the controller which evaluates conditions and chooses an action.
3. Request accepted by that controller, if one exists on an accessible bus.
4. Output: discrete power/control, LIN command or CAN command to another ECU.
5. Published state: information for the cluster and other consumers.

The older Stelvio training identifies BCM as the acquisition point for DNA,
suspension and Start/Stop buttons, as well as a CAN/LIN gateway [S2]. This
explains why a wired input may also have an actionable CAN representation.
It does not identify the physical transmitter of a recorded frame. In our car,
BH selector replay works; light-sensor-report replay has not lit the lamps.

## Capability matrix

“Implemented” describes source code, not proof of acceptance on every vehicle.
“Confirmed” below means the specifically recorded owner result, not a platform
certification. See the [hardware acceptance record](../HARDWARE_ACCEPTANCE.md).

| Function | Evidence and current mechanism | Boundary / appropriate direction |
| --- | --- | --- |
| Readings, alerts, graphs | Passive CAN decoding and separate addressed diagnostic reads exist. | Good extension target. Confirm engine, units, scaling, validity and stale-data handling. A displayed value is not a writable setting. Unsupported DID must remain unavailable, not zero. |
| IPC menu and radio text | `features/body.c`, display stream and MY23 UI implement text transport and factory-source handling. | Reuse that transport; preserve radio/source ownership. Older IPC and MY23 digital layouts need separate acceptance. A writable text field does not imply arbitrary graphics or new native cluster applications. |
| Default DNA and softer suspension | `features/dna_selector.c`: BH 0x256/DLC 4 input gestures, distinct from C1 0x256/DLC 8. Dictionary records owner acceptance on 2026-10-24 for Race and softer suspension with beta-30 source. | Best model for a bounded input simulation: fresh template, hold/release, actual mode feedback, driver priority. Retry path was not independently observed. Race and adjustable damping require supported controllers/equipment; D soft and Race mid are not universal damper modes. |
| Start/Stop preference | `features/start_stop.c` schedules a simulated button; `vehicle/controls_frames.c` modifies C1 0x4B1 button field. | Existing implementation, not a universal permanent ECU recoding. Verify acceptance, toggling and startup conditions per variant. A wired physical button does not exclude this CAN path. |
| Lane-assist preference | `vehicle/drive_mode.c` observes state and sequences virtual button activity; `controls_frames.c` already documents bus differences. | Validate LDW/LKA generation and feedback. A preference cannot add absent camera/steering functionality. Do not use a matching button bit as proof that all generations implement the same action. |
| Window comfort actions | `vehicle/body_commands.c` modifies C1 0x1EF remote/comfort requests and checksum. OEM older Stelvio diagram places window actuation behind BCM on LIN [S2]. | This is an upstream request approach, not direct window-motor CAN control. Existing code does not prove correct travel or final position in every car. Preserve factory anti-pinch/manual control; timed movement is not position confirmation. |
| Parking mirrors | `features/parking_mirrors.c` and `vehicle/mirrors_frames.c` use movement/position state. Older Giulia has basic, folding and memory/BSM mirror variants, with LIN and local motor control [S3]. | Verify position feedback, calibration and actual installed motors. No CAN message can create folding hardware or memory feedback missing from the assembly. Do not generalize the owner's result to basic mirrors. |
| ESC/TC, Dyno and brake actions | `vehicle/drive_mode.c` includes modified mode publications; `features/chassis.c` includes diagnostic sequences/tester presence. | These are different mechanisms from selector replay. A Race display is not proof of consistent ECM/TCM/ABS behavior. Diagnostic maintenance actions are not a general-purpose driving control API; require dedicated acceptance and verified return of control. |
| Exhaust valves | `features/exhaust.c` contains diagnostic control and a separate GPIO interface to an added remote. | Specify engine/controller and installation. Some supported behavior requires external wiring; an OBD-only installation cannot reproduce the GPIO path. |
| Seatbelt settings / other diagnostic actions | `features/seatbelt.c` starts an addressed diagnostic session; BCM fault reading and other routines have separate protocol flows. | Read support does not imply write/actuator permission. ECU session, security, software revision and SGW matter [S5]. Never infer that a temporary actuator test is a supported persistent road feature. |
| IBS / battery-value override | `features/ibs_override.c` republishes modified 0x41A data. | Treat as competing state injection, not a verified charging-policy setting. A spoofed reported measurement does not change the physical battery or establish correct charging decisions. |
| Rain-triggered dipped beams | `features/rain_lights.c` / `rain_test.c`; BH 0x738 darkness copies in beta-34. Owner reports lamps stayed OFF. | Not a validated actuation path. 0x738 is light-responsive telemetry with unknown consumer/producer, not a confirmed command. Do not label this solved by increasing TX rate. |
| Wipers, washers, horn, locks, boot and interior lighting | BCM ownership appears in older OEM training; wiper/washer and headlamp paths are covered by the linked lighting research. | Ownership is a place to look, not evidence of an exposed CAN request. Identify each input and output separately. Do not invent CAN IDs from a dashboard indicator, shared BCM ownership or a different model's protocol. |
| New rain/dusk automation or BCM coding | No verified Giulia/Stelvio configuration field for the requested rain-to-dipped behavior was found. | First establish an existing supported setting/request. PROXI/configuration changes can select existing behavior, not add a missing algorithm to BCM firmware. Custom BCM firmware is a separate project with recovery, integrity and variant requirements; BACCAble cannot simply upload a new feature through ordinary frames. |

## Lighting: apply the correct hardware generation

The detailed sources, pinout limitations, capture evidence and download hash
are retained in the [lighting report](../../baccable_lab/docs/research/20261025-light-origin-timing.md)
and [manual-switch audit](../../baccable_lab/docs/research/20261025-manual-light-switch.md).

| Documented equipment | Established path | What cannot be inferred |
| --- | --- | --- |
| Older basic halogen | 2018 OEM diagrams show discrete BCM lamp outputs. H7 dipped/H15 high/DRL, not H4. | CAN publication of dipped state is not a command accepted by the bulb. |
| Older D5S 25 W xenon | BCM control is drawn separately from adjustment. | No universal xenon or LED connector mapping. |
| Older D3S 35 W adaptive xenon | BCM lamp/HDU and shutter paths are distinct from AFLM CAN-C2 and LIN adjustment. | CAN-connected adaptive electronics do not establish CAN dipped ON/OFF. Positioning and illumination are different functions. |
| Facelift full LED / adaptive matrix | Manufacturer confirms new lighting equipment [S4]; owner identifies facelift LEDs. | Exact vehicle MY, factory/retrofit status and matching schematic remain unknown. Older lamp output pins must not be transplanted. |

Older panel documentation gives a wired resistor-multiplexed headlamp selector
input, not a CAN knob. An older LRSM connector identifies LIN. BCM can decide
locally from these inputs and publish its decision on CAN without needing an
independent CAN ON command. That is a supported explanation, not proof that
all possible BCM CAN requests are absent. Exact resistance values and a
validated electrical emulation interface are missing.

Capture conclusions must retain their scope:

- BH 0x738 responds to covering/uncovering the sensor; observed channels reach
  0..9, not a proven protocol limit of 6 or 7. Earlier counter/checksum claims
  are not established. The beta-34 >7 guard excludes observed data; that is
  separate from the owner's unsuccessful 1/1 test.
- C2 0x73E b1.6 tracks dipped state, with downstream correlated publications.
  Manual switching does not require a 0x738 light-level change.
- AUTO release can lag brightness recovery by roughly 14–20 seconds in the
  recorded trials. Account for this before declaring an OFF test failed.
- No repeatable additional command was established by the existing all-ID
  search. This is not proof that a differently encoded request cannot exist.

## Hard limits of the current installation

- A parallel CAN node cannot remove, replace or retract the factory message.
  `can_forward()` transmits another frame; it does not make BACCAble an inline
  filtering gateway. An inline design requires a different physical topology.
- Dominant zeros do not safely overwrite another transmitter's data. Arbitration
  is not payload merging; simultaneous conflicting payloads can cause errors.
  Sequential modified/original frames remain an application-level conflict [S6].
- Queue acceptance, a TX counter and even bus ACK do not prove that the intended
  ECU accepted an action or that an actuator moved.
- CAN wiring cannot directly drive a separate power output, resistor input or
  LIN slave. A verified upstream CAN request can bridge that gap through BCM;
  without one, added electrical/LIN hardware is a different implementation.
- Commands cannot add absent motors, sensors, adaptive dampers, matrix segments
  or the controller software needed to operate them.
- Access to the three BACCAble buses does not imply access to every private
  vehicle network or permission to run diagnostics. SGW and ECU access checks
  must be established for the actual connection [S5].
- An identifier is not a universal source address. Match bus, IDE, ID and DLC;
  identical IDs on separate buses need not mean the same thing. RX-only traces
  cannot establish physical ECU origin by identifier alone.

## Compatibility contract for older cars

Record model, build date, market, engine, drive, BCM/IPC/target ECU part and
software numbers, lamp type, installed options, retrofits and SGW/connection
path. A year label or the availability of a menu item is insufficient.

For each feature keep three independent results: **observed signal**,
**accepted request**, and **verified vehicle outcome**. Unknown remains unknown.
A validated newer-car packet must not automatically become an older-car default.
Check fresh frame layout, timing, checksum/counter evidence and equipment
before enabling any replay. If detection is ambiguous, leave actuation disabled
and continue passive observation. Do not learn a protocol from a single startup
value or assume missing traffic means an OFF state.

Older-versus-newer acceptance must include manual override, release after a
bounded request, restart/sleep, stale/missing input, a rejected request and
coexistence with factory traffic. Use the existing hardware worksheet rather
than claiming compatibility from compilation or host tests.

## Gate for the next experiment

1. **Name the target and owner.** What physical behavior should change? Obtain
   the exact-variant input/owner/output diagram. Separate manufacturer evidence,
   OEM material on a mirror, code assumptions and capture observations.
2. **Reuse evidence.** Read the dictionary, hypotheses and existing annotated
   sessions. Do not repeat the already recorded manual-light trials without a
   specific unanswered question. Capture correlation ranks candidates; it does
   not identify commands.
3. **State a falsifiable mechanism.** Identify why the receiver should consume
   this field as a request, its release and prerequisites. “The value changes
   when it turns on” is insufficient. Diagnostic and configuration paths must
   be evaluated separately from normal operating CAN traffic.
4. **Prove delivery and outcome separately.** For a bounded stationary test,
   observe actual transmitted frames independently, bus error/drop state,
   receiver feedback and physical output. Stop on interference or missing
   release. Do not compensate for a failed mechanism by flooding the bus.
5. **Use valid timing.** Same-board timestamps support local ordering. Separate
   clocks and USB batching require calibrated uncertainty. The existing audit
   found BH residuals of about ±14 ms: a smaller apparent cross-bus lead cannot
   prove source direction. A fixed USB-offset assumption is insufficient.
6. **Choose the next branch from the result.** No TX: fix transport. TX but no
   acceptance: inspect routing, access and semantics. Acceptance but no output:
   inspect prerequisites/equipment. Correct physical result: validate release,
   override and other variants before making it a normal feature.

For Rain lights, the next useful evidence is exact-variant BCM input/output
wiring and verified TX/receiver behavior, not another search for a “stronger”
0x738/0x73E publication. If no CAN request is established, evaluating an isolated
physical-input interface is a hardware project, with exact electrical data and
factory/manual behavior preserved. It is not yet a recommended wiring change.

## Source ledger and limitations

- **S1 — OEM Giulia exterior-lighting techCONNECT, 2018-04-04:**
  [17-page document, third-party mirror](https://giuliatech.com/uploads/short-url/2BHNuuQa75NknmcalseEgW0Dmkj.pdf).
  Diagrams and connector pages visually inspected in the preceding research;
  hash and page-specific findings are in the linked lighting report. Older
  equipment only; it does not establish the facelift harness.
- **S2 — Fiat-hosted Stelvio electrical training, REV 0.0:**
  [949001497.pdf](https://lab-aftersales.fiat.com/elearn_testreda/949001497.pdf).
  Indexed page 28 lists BCM functions, page 37 describes window LIN control,
  pages 61/76 cover basic lamps/panel. Live download returned HTTP 503; only
  indexed excerpts of this source were available. No whole-document inspection
  is claimed. Do not import unrelated Fiat model search hits.
- **S3 — OEM 2018 Giulia Ti mirror service description, third-party mirror:**
  [Exterior mirrors](https://lemon-manuals.la/Alfa%20Romeo/2018/Giulia%20Ti%2C%20RWD/Repair%20and%20Diagnosis%20%28Single%20Page%29/Body%20%26%20Frame/Power%20Mirrors%20%28Service%20Information%29/Description%20And%20Operation/Exterior/).
  Establishes equipment tiers and DMCM/PMCM/BCM LIN paths, not our injected
  request's acceptance across all tiers.
- **S4 — Stellantis facelift announcement, 2022-10-27:**
  [Giulia and Stelvio: timeless design](https://www.media.stellantis.com/em-en/alfa-romeo/press/the-alfa-romeo-giulia-and-stelvio-timeless-design).
  Establishes full-LED adaptive matrix equipment; no wiring or CAN command IDs.
- **S5 — Diagnostic tool manufacturer's support list:**
  [Multiecuscan supported vehicles](https://www.multiecuscan.com/SupportedVehiclesList.aspx),
  checked 2026-10-26. Documents SGW diagnostic-command restrictions on some
  2018+ and late-2017 vehicles including Giulia/Stelvio. This is evidence about
  diagnostic access, not proof that SGW causes every failed ordinary CAN replay.
- **S6 — CAN interface manufacturer's protocol reference:**
  [Kvaser CAN frame types](https://kvaser.com/canlib-webhelp/page_user_guide_can_frame_types_types.htm)
  and [CAN protocol tutorial](https://kvaser.com/can-protocol-tutorial/).
  Protocol evidence, not Alfa-specific application semantics.

Sources accessed/reviewed across 2026-10-25–26. Additional exact connector and
service-operation links are retained in the lighting report rather than copied
as a second pinout catalog. No dictionary meaning or raw recording was changed
by this architectural review. Firmware existence and capture correlation remain
explicitly weaker evidence than a confirmed physical outcome.

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
suspension and Start/Stop buttons, as well as a CAN/LIN gateway [S2]. This is evidence for that documented architecture, not a wiring identification
for the owner's monostable selector. It does not identify the physical
transmitter of a recorded frame. In our car,
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

## BCM physical connections: verified topology, 2026-10-26

These are vehicle-scoped service-document findings, not a VIN-matched universal
pinout. Connector names C6/F and C8/H are connector designations, not CAN buses.
The sources below are OEM service material reproduced on a third-party host;
search-index text was accessible where direct page retrieval failed.

### Discrete signals: examples with connector evidence

2019 Giulia Sport RWD, LHD [BCM C8 D2762H][T1]:

| Connection | C8 cavity | Direction/function |
| --- | --- | --- |
| Exterior-light selector | 42 | Input |
| Panel dimmer | 13 | Multiplexed input |
| High-beam stalk | 22 | Input |
| Horn switch | 11 | Input; not horn power |
| DNA selector in this documented variant | 21 | Input; applicability to the owner's monostable selector is unverified |
| ESS disable/enable button | 28 | Input; not engine START button |
| Washer / front wiper controls | 55 / 58 | Input / multiplexed input |
| Hazard switch | 59 | Input |
| Fuel level | 5 | Sensor input |
| Boot/liftgate switch / release | 39 / 29 | Input / driver output |
| Interior lighting | 4 / 47 | Dome / ambient outputs |
| Front fog relay | 10 | Relay control |

2018 Giulia Ti Lusso RWD, LHD [BCM C6 D2762F][T2] additionally identifies door
ajar inputs (7, 20, 24), washer-fluid level (22), second fuel level (25), ambient
air temperature (45), and discrete rear lamp outputs including reverse (31/33)
and number plate (56). These examples establish physical endpoints, not output
ratings or installation instructions. LIN and discrete inputs coexist even
inside a single switch assembly; do not classify the entire assembly from one pin.

### LIN slaves and their master

The [2019 Giulia Ti AWD network description][T3] explicitly separates:

| Master / network | Connected equipment |
| --- | --- |
| BCM LIN1 | IBS, rain/light sensor, alarm equipment, optional child detection |
| BCM LIN2 | Front wiper smart motor, steering switch bank, rear camera control |
| BCM LIN3 | Four window smart motors and driver/passenger mirror modules |
| BCM LIN4 | Cruise-control steering-wheel commands |
| PCM LIN | Intelligent alternator and auxiliary water pump |
| AFLS LIN | Left/right headlamp modules |
| HVAC LIN | Humidity sensor and climate-control panel |
| EMCM LIN | Volume knob |

This lists control communication, not every power or video connection. Equipment
and nomenclature differ by variant; it is not a universal facelift connector map.

IBS is independently corroborated by the [2017 Giulia diagnostic procedure][T4],
which traces D401 LIN1 to BCM C6/F cavity 2, and a [2024 Giulia procedure][T5]
which again checks a LIN circuit between IBS and BCM. Neither describes IBS as
a native CAN node. The [2018 ESS description][T6] says IBS measures voltage,
current and temperature and that BCM communicates an Autostop inhibition on
CAN-C; PCM primarily manages ESS. Thus local sensor information and subsequent
CAN decisions/reports are distinct. BACCAble's 0x41A override does not replace
the physical LIN sample or establish a commanded alternator target.

The [2018 Giulia LRSM connector D2683A][T7] has pin 1 RUN supply, 2 ground,
3 D401 LIN1. The [2024 wiper/washer operating description][T8] explicitly gives
BCM -> LIN2 -> wiper motor; the washer pump instead receives a switched voltage
through a relay inside BCM. Headlamp washers use a separate relay-control path.
Do not call all these devices CAN actuators or all of them simple wired loads.

### Confirmed CAN participants, not guaranteed command APIs

Older Stelvio OEM training separates these networks (optional nodes depend on
engine/equipment). Lists below are representative, not exhaustive:

| Network | Rate / diagnostic connector pins | Explicitly listed participants |
| --- | --- | --- |
| C1 | 500 kbit/s; 6/14 | BCM, ECM, TCM, AGSM, ABS, ORC, RFH, IPC, ACC, CDCM, AWD DTCM |
| C2 | 500 kbit/s; 12/13 | BCM, ABS, ORC, EPS, ESL, PAM, HALF, AFLS, CDCM, TVM |
| BH | 125 kbit/s; 3/11 | BCM, IPC, HVAC, ETM, EMCM, amplifier, seat/steering comfort module, blind-spot modules, trailer modules, powered liftgate |

Sources: [Italian OEM training page 11][T9] for C1; [English pages 13–14][S2]
for C2/BH. Indexed excerpts only; live PDFs still return HTTP 503. The English
BH paragraph accidentally says “CAN-C2” before 125 kbit/s; its heading, diagram
and connector pins identify BH. Some ECUs legitimately connect to multiple
networks. Physical access also depends on SGW/installation. This is not a claim
that all their actuator requests are exposed or accepted from an added node.

### What this changes in the research strategy

- For documented variants, sensor -> LIN -> BCM is the physical rain/light
  input. BH 0x738 cannot be a direct native CAN transmission from that LIN-only
  sensor. Its exact publisher remains unidentified. An outgoing BCM report is
  not automatically an input to the same BCM.
- Rain-sensor input and wiper control can form a BCM-local LIN chain; no CAN
  start-wiping command is required by that topology. CAN activity may only
  report the resulting state.
- DNA in the owner's car has an empirically accepted CAN input mechanism.
  The older wired-selector pinout must not be used to assign its physical
  wiring. See the variant correction and evidence chain below.
- For IBS, identify which controller consumes the republished measurement and
  verify its physical response. Do not infer success/failure solely from LIN
  topology or a changed reading. Alternator control belongs to the PCM branch.
- A real CAN member is a stronger target for protocol research, but still needs
  an identified request, prerequisites and feedback. A relay/LIN endpoint needs
  an upstream request to its owner or different interface hardware.

[T1]: https://lemon-manuals.la/Alfa%20Romeo/2019/Giulia%20Sport%2C%20RWD/Repair%20and%20Diagnosis%20%28Single%20Page%29/Quick%20Lookups/Electrical%20Component%20Locations/Connector%20End%20Views%20%26%20Electrical%20Component%20Locations%20%285%20Of%2011%29/Module-Body%20Control%20C8%20%28Standard%29%20Connector%20End%20View%20%28D2762H%29/
[T2]: https://lemon-manuals.la/Alfa%20Romeo/2018/Giulia%20Ti%20Lusso%2C%20RWD/Repair%20and%20Diagnosis%20%28Single%20Page%29/Electrical/Body%20Electrical/Connector%20End%20Views%20%26%20Electrical%20Component%20Locations%20%285%20Of%2011%29/Module-Body%20Control%20C6%20%28Standard%29%20Connector%20End%20View%20%28D2762F%29/
[T3]: https://lemon-manuals.la/Alfa%20Romeo/2019/Giulia%20Ti%2C%20AWD/Repair%20and%20Diagnosis%20%28Single%20Page%29/Accessories%20%26%20Equipment/Communication%20Devices/Electronic%20Control%20Modules%20%28Service%20Information%29/Communication/Description%20And%20Operation/Communication%20And%20Networks/Communication/
[T4]: https://lemon-manuals.la/Alfa%20Romeo/2017/Giulia%20Base%2C%20RWD/Repair%20and%20Diagnosis%20%28Single%20Page%29/Accessories%20%26%20Equipment/Communication%20Devices/Body%20Control%20Module%20%28BCM%29%20-%20DTCS%20B12AE-15%20To%20U1702-87/DTC%20Troubleshooting/U1411-29-IBS%20%28Battery%20Sensor%20Node%29-Signal%20Invalid/Diagnostic%20Test/
[T5]: https://lemon-manuals.la/Alfa%20Romeo/2024/Giulia%20Competizione%2C%20RWD/Repair%20and%20Diagnosis%20%28Single%20Page%29/Engine%20Performance/System/2.0L%20-%20DTCS%20U04B1-00%20To%20U3041-00/DTC%20Troubleshooting/U04B1-00%20Invalid%20Data%20Received%20From%20Battery%20Monitor%20Module/Diagnostic%20Test/
[T6]: https://lemon-manuals.la/Alfa%20Romeo/2018/Giulia%20Ti%20Lusso%2C%20RWD/Repair%20and%20Diagnosis%20%28Single%20Page%29/Engine%20Mechanical/Starter/Starting%20System%20%28Service%20Information%29/Description%20And%20Operation/Engine%20Stop%20Start/
[T7]: https://lemon-manuals.la/Alfa%20Romeo/2018/Giulia%20Quadrifoglio/Repair%20and%20Diagnosis/Electrical/Wiring%20Diagrams/Connector%20End%20Views%20%26%20Electrical%20Component%20Locations%20%286%20Of%2011%29/Module-Light%20Rain%20Sensor%20%28Standard%29%20Connector%20End%20View%20%28D2683A%29/
[T8]: https://lemon-manuals.la/Alfa%20Romeo/2024/Giulia%20Competizione%2C%20RWD/Repair%20and%20Diagnosis%20%28Single%20Page%29/Accessories%20%26%20Equipment/Communication%20Devices/Body%20Control%20Module%20%28BCM%29%20-%20DTCS%20B1042-24%20To%20B1054-11/DTC%20Troubleshooting/B1049-14%20Head%20Lamp%20Washer-Circuit%20Short%20To%20Ground%20Or%20Open/Theory%20Of%20Operation/
[T9]: https://lab-aftersales.fiat.com/elearn_testreda/949001496.pdf
[S2]: https://lab-aftersales.fiat.com/elearn_testreda/949001497.pdf

## DNA variant correction and reusable evidence chain

Owner clarification, 2026-10-26: the tested vehicle has a **monostable DNA
selector**. The earlier explanation assigning its input directly to BCM from
an older wiring diagram was unsupported and is withdrawn for this vehicle.
Retain the older C8/21 entry only with its documented model/variant scope.
Neither model year alone nor the label "DNA" identifies the selector topology.

| Claim | Evidence | Development consequence |
| --- | --- | --- |
| The owner's selector is monostable | Owner clarification on 2026-10-26; recorded momentary direction pulses are consistent with it | Treat input as press/hold/release, not a latched target-mode value |
| BH 0x256/DLC 4 carries selector-associated directional input | [All-session discovery](../../baccable_lab/docs/research/20261023-dna-all-sessions.md) and [pulse evidence](../../baccable_lab/docs/research/20261023-dna-selector-evidence.json) | Use bus and DLC as well as ID; C1 0x256/DLC 8 is unrelated |
| Short direction pulses and sustained holds have different outcomes | Discovery report records 36 direction pulses; five long holds precede Race by about 1.2 seconds | Preserve gesture timing and prerequisites; do not substitute a single target-mode report |
| Replaying the gestures can actually select the mode | [Maintained hypotheses](../../baccable_lab/baccable_lab/knowledge/hypotheses.json), entry for BH 0x256, records owner acceptance on 2026-10-24 with beta-30 source: Race and softer suspension applied | This is an accepted CAN actuation mechanism on the tested car, not merely passive correlation. Acceptance is owner feedback without an accompanying TX trace; retry behavior is not independently verified |
| Physical source of 0x256 | RX recordings do not distinguish selector electronics from another publisher; no matching selector connector schematic established | Keep source ECU unresolved. Do not assert either direct selector CAN wiring or BCM-wired input as a proven fact for this vehicle |

Implementation reference: [BH selector state machine](../../firmware/baccable/features/dna_selector.c).
It distinguishes direction, hold and release, waits for engine state, monitors
mode feedback and yields to physical input. The
[three-session audit](../../baccable_lab/docs/research/20261024-three-session-audit.md)
provides the later mode/suspension observations. Check current code before
reusing timing constants; historical measured delays are not firmware settings.

For future feature research, keep three independent questions:

1. **Physical connection:** which device and bus/wire, on which exact variant?
2. **Protocol role:** input request, sensor report, selected state or diagnostic?
3. **Acceptance:** does replay cause the physical outcome, and under which conditions?

An unknown answer to (1) must not erase positive evidence for (3). Conversely,
success with DNA does not establish that light-sensor or lamp-state reports
are accepted commands. An older schematic cannot override the tested newer
variant without checking connector, part number and topology compatibility.

## CAN requests to LIN equipment: mirror correlation

Review 2026-10-26. LIN topology does **not** exclude upstream CAN actuation.
There is also no generic CAN-to-LIN command tunnel implied by BCM gateway status.
A controller must implement the particular request and translate its meaning
into local action; this is not a byte-for-byte forwarding assumption.

For mirrors, distinguish three evidence layers:

- [BACCAble request definition](../../firmware/baccable/state/mirrors.c):
  BH 0x5A8/DLC 8, four target-position bytes followed by `80 00 00 00`.
- [Parking controller](../../firmware/baccable/features/parking_mirrors.c) and
  [feedback handler](../../firmware/baccable/vehicle/mirrors_frames.c): send
  position targets and consume BH 0x5A6 position/movement reports. C1 0x5A8 is
  unrelated. These are code-proven roles, not independently identified factory
  transmitters/receivers or a new vehicle acceptance test.
- [2018 Giulia mirror service description][S3M]: DMCM/PMCM communicate with BCM
  over LIN and locally drive mirror motors. Thus "mirrors on LIN" more precisely
  means their control modules are LIN participants; motors have local wiring.

Combining them supports the architecture **CAN request -> factory controller(s)
-> LIN mirror module -> motor**, with feedback returned to CAN. The exact
receiver of the BACCAble 0x5A8 packet is not identified by source code alone.
Do not label BCM as experimentally proven receiver or CSWM as proven original
sender. Factory seat/mirror memory recall is a useful *candidate context* for
finding the original request, not a proven attribution of this ID.

| LIN subsystem | Upstream CAN evidence | Research conclusion |
| --- | --- | --- |
| Mirror positioning | Existing BH target/feedback implementation | Reuse the request/feedback pattern; verify equipped position sensors and actual behavior per variant |
| Window comfort | [body_commands.c](../../firmware/baccable/vehicle/body_commands.c) edits C1 0x1EF comfort fields and checksum; OEM window topology is LIN | Closest implemented analogue. This is a comfort request, not arbitrary motor or individual-position control; source code is not a completed hardware acceptance record |
| Front wipers | Documented BCM LIN2 motor control; stalk/sensor inputs exist locally | CAN request remains unknown. Do not confuse the observed cycle/status report with a motor request |
| Rain/dusk sensor | LIN1 measurement input to BCM; light-responsive BH report; beta-34 negative outcome | Sensor emulation over CAN is unproven. Search for a BCM-consumed request separately from its outgoing report |
| IBS / alternator | IBS on BCM LIN1; alternator on PCM LIN; BACCAble changes C1 0x41A SOC report | Potential influence on a downstream consumer differs from commanding IBS or an alternator target. Measure response; do not infer charging control from displayed SOC |
| Alarm, rear camera, HVAC panel, AFLS submodules | Network membership documented, no accepted request established here | Candidate only after a factory action shows the relevant request. Camera LIN control is not its video path; AFLS positioning is not dipped ON |
| Steering/cruise buttons | LIN input devices in documented variants | They report user input; finding their CAN representation does not mean the physical button module can be commanded or every consumer accepts replay |

A better next capture, if these mechanisms need investigation, is a *factory
remote action*: mirror memory recall versus manual joystick movement, or factory
window comfort versus a local window switch. A remote action creates a reason
for an upstream request to exist. Identify transient requests separately from
continuous state reports, then establish feedback and release. Use already
recorded scenarios first. This review does not assert that new captures were
analysed or that additional actuator commands were discovered.

For Rain lights the useful question is now: **does BCM accept a normal operating
CAN request for illumination, and which ECU would normally send it?** A local
sensor/stalk-to-BCM-to-output path need not use one. Diagnostic actuator testing
is a separate interface with sessions, access restrictions and return-of-control
requirements; its availability would not establish a normal driving request.

[S3M]: https://lemon-manuals.la/Alfa%20Romeo/2018/Giulia%20Ti%2C%20RWD/Repair%20and%20Diagnosis%20%28Single%20Page%29/Body%20%26%20Frame/Power%20Mirrors%20%28Service%20Information%29/Description%20And%20Operation/Exterior/

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

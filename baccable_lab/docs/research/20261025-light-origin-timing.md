# Light-report origin and timing audit — 2026-10-25

## Outcome and scope

BH `0x738`/DLC8 carries periodic light-responsive measurements. The captures
support this much more strongly than an explicit dipped-beam ON command.
They do **not** identify its transmitting ECU or prove that BCM consumes an
injected copy. BCM relaying its own sensor input remains a plausible alternative
in which CAN injection would not operate the lamps.

The owner confirmed **facelift LED headlights** during this investigation.
Exact model year, VIN and lamp variant remain unknown. Earlier xenon diagrams
must not be treated as the wiring of this vehicle. BH is a CAN network/capture
role, not an ECU that originates every message on that network.

Inventory: all 27 local sessions, through `20261024-113644Z`; focused chronology
on seven sessions and an all-ID transition scan across 20 light edges in three
sessions. October 24 `105643Z` and `111510Z` provide sensor isolation; `113104Z`
and `113644Z` provide later daylight/DNA/audio controls. October 10 `104325Z`
provides three manual ON/OFF pairs around 843–860 s. September 28 `204607Z`
and October 10 `110821Z` are older references. There is no beta-34 capture in
these inputs; later detailed manifests identify beta-29, and some earlier
metadata is incomplete. This audit does not claim a beta-34 vehicle test.

## What the sensor frame means

- BH `0x738` has DLC8 and a median period of 1,000 ms. C1 has a different
  `0x738`/DLC5, mostly zero; C2 has no `0x738` in this inventory. A matching
  numeric identifier does not prove a forwarded copy or common producer.
- Bytes 1 and 2 react independently to sensor covering. Across these captures
  the observed range is **0–9**, not 0–6 or 0–7; no lux scale is established.
- In `111510Z`, both channels become dark while lamps remain manually OFF in
  both isolation runs. Therefore these bytes are not merely lamp ON status.
- In the older manual-switch pairs, lamps turn ON/OFF while the light channels
  remain bright (3/3). Thus a channel change is not required for manual actuation.
- Byte 4 high three bits can remain unchanged for **508.471 s** in `104325Z`.
  A per-frame rolling counter interpretation is unsupported. Byte 5 has no
  established checksum algorithm. Earlier identical-other-byte observations
  do not prove every possible injected payload valid.

Five isolated covered-sensor ON edges, measured entirely on BH's device clock:

| Session | Lamp ON host time (s, navigation only) | Latest 0/0 sample to BH lamp report (ms) |
| --- | ---: | ---: |
| 20261024-105643Z | 74.296 | 1,137 |
| 20261024-105643Z | 208.614 | 538 |
| 20261024-105643Z | 316.363 | 298 |
| 20261024-111510Z | 98.140 | 1,168 |
| 20261024-111510Z | 227.649 | 588 |

These are observed report-to-report delays, not physical sensor response times:
`0x738` samples only once per second. The isolated bright 6/6 recovery to lamp
OFF delays are 19.904, 14.803, 14.144 s in `105643Z`, and 14.411/14.474 s in
the two labelled `111510Z` AUTO runs. This confirms the long release delay.
Other `111510Z` ON edges at 54.621 and 214.152 s occur on selection of AUTO,
with the channels already bright; do not count them as fresh darkness triggers.

## Lamp reports and ordering

| Role / ID / DLC | Observed field | Median period | Evidence / interpretation |
| --- | --- | ---: | --- |
| BH / 0x738 / 8 | bytes 1, 2 | 1,000 ms | Light-responsive measurement report; source unknown |
| BH / 0x354 / 4 | b1 bit 6; b3 bit 6 | 250 ms | Dipped state; AUTO indication |
| C2 / 0x73E / 4 | b1 bit 6; b3 bit 6 | 250 ms | Dipped state; AUTO indication |
| C1 / 0x73E / 4 | b0 bit 1 | 250 ms | Position-light indication used as a correlated edge, not an independent dipped sensor |
| C1,C2 / 0x384 / 8 | b3 bit 7 | 250 ms | Lamp-state correlation |
| C2 / 0x42D,0x42E / 5 | b0, stable 0x44 OFF / 0x25 ON | 100 ms | Delayed lamp-state correlation, ECU/function not assigned |

For the 20 selected edges, the **same C2 clock** places stable `0x42D/0x42E`
codes 105–218 ms after the `0x73E` dipped change. C2 `0x384` follows by
45–247 ms. `0x42E` can pass through intermediate byte values: a stable whole-byte
code can lag the first changing bit by another 100 ms. The evidence retains
both whole-byte traces and bit-level scan results rather than hiding that case.
Neither order nor repetition identifies which ECU sends a frame or proves a
receiver executes it. The first report may itself be BCM's published decision.

The manual pairs also show AUTO falling before dipped ON (about one report
interval in some pairs). That is useful evidence of switch-state publication,
not proof that replaying the AUTO bit requests dipped beams.

The all-ID scan retains repeated stable bit changes, short excursions and
rare shapes near every selected edge. Only the already identified lamp-report
fields consistently match every edge across all three sessions. Additional
`0x41A/0x422` correlations in the manual session do not reproduce as universal
precursors in the AUTO sessions. Pulse candidates include varying telemetry
and counter-like activity; no new repeatable, independently supported actuator
command was established. This bounded scan does not rule out commands encoded
as multi-bit enums, unchanged periodic requests, another bus, or LIN/hardwire
control. Complete candidates, including weak/contradictory ones, are retained.

## Why cross-bus host order cannot identify a sender

`baccable_lab/capture.py` reads up to 4,096 USB bytes with a 200 ms timeout,
then timestamps the entire read. Up to 256 CAN records share one `host_ns`.
In `105643Z`, median within-read device spans are C1 95 ms, C2 108 ms and BH
194 ms. The boards have independent device clocks; raw device milliseconds
cannot be subtracted across boards either. Device timestamps are millisecond
firmware receive times, not synchronized physical start-of-frame timestamps.

For the first AUTO ON in that session, raw host timestamps suggest C1 precedes
BH by 162.694 ms and C2 by 80.331 ms. Subtracting each record's age within its
USB batch gives estimates of 16.694 and 17.331 ms instead. The correction is
**illustrative**, not clock synchronization: endpoint transport delay remains
unknown. In the old manual trial around 856.8 s, raw host order even places
C2 `0x42D` before BH's report, although same-C2 data puts it 116 ms after C2
`0x73E`. Do not infer direction, gateway residence time or producer from those
host offsets. The ledger retains raw and batch-adjusted values separately.

## Additional clock-fit check requested by the owner

A constant transport delay **can** be calibrated if measured independently.
The existing captures do not contain such a calibration. They do contain enough
USB endpoints to estimate clock drift and reduce batching noise. The script
fits `host_ms = slope * device_ms + intercept` separately for every board using
the latest CAN record in each read. This avoids using the lamp transition itself
as a synchronization anchor (which would assume the desired result).

For `105643Z`, fitted apparent drift is C1 -331 ppm, C2 -170 ppm and BH -364 ppm.
After that fit, the 5th–95th percentile endpoint residuals are:

| Board | Residual p05 to p95 (ms) |
| --- | ---: |
| C1 | -3.3 to +3.3 |
| C2 | -2.6 to +2.7 |
| BH | -12.5 to +14.5 |

The six C2 `0x73E` changes then precede corresponding BH `0x354` changes by
8.2–10.3 ms in fitted time; C1's correlated changes precede them by 8.1–12.2 ms.
These gaps are smaller than the BH endpoint scatter. `111510Z` likewise has BH
residuals -11.9 to +14.1 ms and fitted C2 lead 2.0–13.8 ms. The fit is useful:
it removes a misleading ~0.1 s raw-host difference. It does **not** establish
physical precedence at the remaining ~0.01 s scale. The percentiles describe
scatter, not hard accuracy limits, and constant per-board transport latency
is mathematically inseparable from clock offset in this recording alone.

A synchronized common trigger/clock or independently calibrated read latency
would improve a future capture. Matching frames across buses can help estimate
drift, but assuming identical transmission time would fold actual gateway delay
into the clock correction. Even proven precedence establishes temporal order,
not ECU identity or that a receiver executes a command: a report of an internal
BCM decision may precede every other report without being its cause.

## Hardware topology: reference, not a VIN-specific identification

A reproduced OEM service description for
[2024 Giulia Competizione](https://lemon-manuals.la/Alfa%20Romeo/2024/Giulia%20Competizione%2C%20RWD/Repair%20and%20Diagnosis%20%28Single%20Page%29/Accessories%20%26%20Equipment/Communication%20Devices/Body%20Control%20Module%20%28BCM%29%20-%20DTCS%20B1042-24%20To%20B1054-11/DTC%20Troubleshooting/B1049-14%20Head%20Lamp%20Washer-Circuit%20Short%20To%20Ground%20Or%20Open/Theory%20Of%20Operation/)
states that LRSM connects to BCM and BCM sends wiper control on LIN2. This
page does not establish the facelift LED lamp command wiring or LRSM's exact
input protocol. The older reproduced
[2018 LRSM connector pinout](https://lemon-manuals.la/Alfa%20Romeo/2018/Giulia%20Quadrifoglio/Repair%20and%20Diagnosis/Electrical/Wiring%20Diagrams/Connector%20End%20Views%20%26%20Electrical%20Component%20Locations%20%286%20Of%2011%29/Module-Light%20Rain%20Sensor%20%28Standard%29%20Connector%20End%20View%20%28D2683A%29/)
lists power, ground and LIN1. It supports investigating the LIN-to-BCM
alternative, but does not prove the owner's facelift wiring. Both are third-party
mirrors of service material, not a verified vehicle-specific OEM schematic.

Working hypothesis: LRSM input -> BCM decision -> published CAN lamp states,
with `0x738` possibly BCM's relayed measurement. Physical source remains
unassigned in the dictionary. The assumption that a separate BH CAN command
must exist is not justified. Likewise, the CAN identifier is not a source
address; multiple observers and earlier receipt alone cannot label an ECU.

## Concrete beta-34 issue found during this audit

`firmware/baccable/features/rain_lights.c:202` rejects a factory sensor frame
if either light byte is greater than 7. Real daylight samples exceed that bound:

| Session | Samples after startup | Rejected by beta-34 data guard |
| --- | ---: | ---: |
| 20261010-111025Z | 1,291 | 130 |
| 20261024-113104Z | 136 | 65 |
| 20261024-113644Z | 305 | 271 |

This is a counterfactual evaluation of the code against recorded inputs, **not**
evidence that Rain lights was requested in those sessions. When darkness output
is requested on such a frame, no dark copy is queued. The two main sensor tests
never exceed 6, which explains why focusing only on them missed this restriction.
The observed range must inform the guard before an actuator acceptance test;
merely fixing it still does not prove BCM listens to `0x738`. No firmware was
changed or flashed by this research task.

## Corrections and next discriminating observation

The separate, pre-existing uncommitted local note
`20261025-light-command-search.md` was preserved, not overwritten. Its claims
that the earliest report is necessarily a command, that payload dominant bits
replace factory data, and that b4/b5 are a confirmed counter/checksum are not
accepted by this audit. CAN arbitration does not extend to choosing a winning
payload: conflicting same-ID data can cause bit errors.

Next acceptance should distinguish **no transmission**, **valid transmission
ignored**, and **lamps really operated**. First remove the observed-range guard
blocker in a separate firmware change. On a stationary car in daytime AUTO,
record factory 0x738, accepted TX count, errors, lamp reports and actual front
and rear lamps during a bounded darkness request. Include a bright/dark cover
control and release long enough for the 14–20 s delay. TX queue acceptance is
not proof of bus delivery or ECU consumption; these recordings do not retain
BACCAble's own TX. An independent bus observer is needed to verify it.

For physical attribution, obtain the facelift LED/VIN-matched LRSM/BCM/lamp
wiring and, if it is LIN, observe the sensor link alongside CAN. Synchronized
hardware timestamps can improve ordering but still do not identify a CAN
producer without topology or an isolated controlled source observation.

## Reproduction and integrity

The scripts open SQLite read-only, include WAL, preserve original markers and
fingerprint metadata, DB/WAL and raw streams before/after. Startup loss and
parser issues are retained in each summary; saturated 65,535 loss counts are
lower bounds, not precise counts. No claim of perfect recording follows from
zero later reported loss. All 832 retained decoded change records are cross-checked against
its original binary record near the stored USB-read offset.

Run from repository root, choosing fresh output paths outside `sessions`:

```sh
python3 baccable_lab/scripts/analyze_light_origin.py \
  --sessions /path/to/sessions --output /tmp/light-origin-NEW.json
python3 baccable_lab/scripts/scan_light_candidates.py \
  --sessions /path/to/sessions --evidence /tmp/light-origin-NEW.json \
  --output /tmp/light-candidates-NEW.json
```

Committed evidence: [timing ledger](20261025-light-origin-timing.json) and
[all-ID candidate scan](20261025-light-origin-candidates.json). They retain input
hashes, original marker provenance, frame IDs/sequences, payloads, source hashes,
raw timestamps and stated limitations. Only passive research/catalog documents
and their reproduction scripts change in this branch; both user guides and
firmware behavior remain unchanged.

## AUTO actuation deduction and external-document review

Follow-up question: a physical command must reach the lamps when AUTO decides
that it is dark. That premise is correct. It does not require the light sensor
input or final lamp-control connection to be one of the three recorded CAN
networks. Conversely, this audit has **not** ruled out CAN control of facelift
LED lamps; calling every identified lamp field "only status" would also go
beyond the passive evidence.

Additional comparison of retained payloads:

| Operation | C2 0x73E / DLC4 | BH 0x354 / DLC4 | C1 0x73E / DLC4 |
| --- | --- | --- | --- |
| Manual ON, three October 10 trials | `00400000` | `02408000` | `02008000` |
| AUTO ON, three isolated October 24 cover trials | `00400040` | `02408040` | `02008000` |

The common C2 b1.6 field is downstream of the choice between manual and AUTO.
Its role can be a common lamp request, published logical state, or both for
different receivers. The different b3.6 records the AUTO mode. C2 0x42D/E follow
by 105–218 ms in the selected manual/AUTO edges. Their side-specific indicator
fields make left/right lighting-controller reports worth investigating, but
neither their source nor physical-feedback semantics are established. The
same ordering is possible if BCM schedules several reports at different rates.

The best supported functional model is sensor/switch input -> BCM lighting
decision -> lamp actuation, with CAN publications of light measurement/mode/state.
BCM as decision maker is supported by the service descriptions below; exact
0x738 producer and facelift lamp transport remain hypotheses. There are two
compatible output models:

1. BCM sends a request over CAN (C2 0x73E b1.6 is the strongest identified
   candidate common to manual and AUTO), followed by downstream reports.
2. BCM operates a dedicated output or another link and publishes 0x73E/0x354
   alongside it; changing their CAN copies does not change the original output.

The negative beta-34 darkness test is consistent with BCM ignoring a relayed
0x738 measurement, but does not discriminate these output models. It also does
not exclude application filtering or non-delivery of queued transmissions.
No retest/injection was performed by this documentation task.

### External sources and applicability, checked 2026-10-25

- [Stellantis, facelift announcement, 2022-10-27](https://www.media.stellantis.com/em-en/alfa-romeo/press/the-alfa-romeo-giulia-and-stelvio-timeless-design):
  confirms Full-LED Adaptive Matrix introduction and adaptive dipped/high-beam
  functions. It provides no electrical pinout or CAN identifier.
- [2024 Giulia Competizione service description, reproduced OEM material](https://lemon-manuals.la/Alfa%20Romeo/2024/Giulia%20Competizione%2C%20RWD/Repair%20and%20Diagnosis%20%28Single%20Page%29/Accessories%20%26%20Equipment/Communication%20Devices/Body%20Control%20Module%20%28BCM%29%20-%20DTCS%20B1042-24%20To%20B1054-11/DTC%20Troubleshooting/B1049-14%20Head%20Lamp%20Washer-Circuit%20Short%20To%20Ground%20Or%20Open/Theory%20Of%20Operation/):
  LRSM connected to BCM; does not specify the facelift lamp output bus. Its
  explicit LIN2 reference concerns wipers, not headlights.
- [2018 LRSM connector D2683A, reproduced OEM pinout](https://lemon-manuals.la/Alfa%20Romeo/2018/Giulia%20Quadrifoglio/Repair%20and%20Diagnosis/Electrical/Wiring%20Diagrams/Connector%20End%20Views%20%26%20Electrical%20Component%20Locations%20%286%20Of%2011%29/Module-Light%20Rain%20Sensor%20%28Standard%29%20Connector%20End%20View%20%28D2683A%29/):
  power, ground and LIN1; historical support for the sensor-input alternative,
  not a confirmed facelift pinout.
- [2018 Giulia lighting operation, reproduced OEM material](https://lemon-manuals.la/Alfa%20Romeo/2018/Giulia%20Ti%2C%20RWD/Repair%20and%20Diagnosis%20%28Single%20Page%29/Accessories%20%26%20Equipment/Interior%2FIllumination%20Lights/Lamps%2FLighting%20-%20Interior%20%28Service%20Information%29/Description%20And%20Operation/):
  exterior knob resistor-multiplex input to BCM and separate PWM/CAN illumination
  outputs. Historical evidence that a physical switch need not be a CAN node;
  not proof of the facelift switch wiring.
- [2018 techCONNECT headlamp document, mirrored OEM material](https://giuliatech.com/uploads/short-url/2BHNuuQa75NknmcalseEgW0Dmkj.pdf):
  search-indexed connector descriptions include direct positive low-beam control
  and xenon/AFS topology. The full PDF exceeded the browser fetch size limit;
  its diagrams were not independently inspected here. This is an older-system
  counterexample, never evidence that facelift LEDs use the same pinout.
- [ROHM, BCM design overview](https://www.rohm.com/solution/automotive/body/body-control-module):
  distinguishes CAN/LIN networking and smart-switch outputs. This is a component
  manufacturer's general design reference, not a Giulia wiring diagram.

Searches for facelift-specific lamp CAN/LIN pinouts, OEM fault descriptions
and public manufacturer bulletins did not yield a verified exact-variant
schematic. Retail listings, retrofit compatibility, old xenon diagrams and a
service index labelled "2024" are insufficient to fill that gap. Exact model
year and factory-versus-retrofit status were requested; none is assumed here.
The decisive missing evidence is the physical input/output topology and actual
CAN TX delivery, not another repetition of the already recorded manual trial.

## Halogen/xenon topology and competing publications

The owner requested broader applicability research, including pre-facelift
Giulia/Stelvio. The 17-page OEM techCONNECT document dated 2018-04-04 was now
successfully downloaded from the mirror linked above, text-extracted with
macOS PDFKit, and diagrams on pages 8–12 visually inspected. This supersedes
the earlier browser-size limitation. SHA256 of the downloaded original:
`c4fa1676d40f9ded73e0cf9febe149913eb5e225c8f90b6d707de68753ee853c`.
The copyrighted document is not copied into the repository.

Findings from that document: basic halogen lamps have dedicated BCM wires
(pages 8–10); D5S 25 W control is directly from BCM (page 11). The D3S 35 W
diagram separates BCM lamp/shutter wiring from the AFLM's CAN-C2 and LIN
positioning path (page 12). CAN/LIN involvement in adaptation therefore does
not establish CAN actuation of dipped beams. These are pre-facelift layouts,
not a verified LED retrofit or factory facelift pinout.

Independent manufacturer references:

- [Stelvio owner manual, printed page 288](https://vehicleinfo.mopar.com/assets/publications/en-us/Alfa_Romeo/2024/Stelvio/5538902-24_GU_OM_EN_USC_DIGITAL_E1.pdf#page=290)
  lists H7 dipped and H15 high-beam/DRL bulbs for basic halogen equipment, not
  H4. Its introduction explicitly allows descriptions of equipment no longer
  available; a 2024 cover date does not make every illustrated lamp a facelift
  part. This source establishes bulb families, not wiring.
- [Fiat-hosted Stelvio training document, page 61](https://lab-aftersales.fiat.com/elearn_testreda/949001497.pdf#page=61)
  has a basic-headlamp BCM wiring diagram and positive-control connector
  functions; page 13 identifies AFLS on CAN-C2. Retrieved as indexed excerpts;
  the live PDF request returned HTTP 503, so the complete file was not inspected.

### Why another 0x73E publisher is not a stable control strategy

In the user's captures, factory C2 0x73E repeats approximately every 250 ms.
Appending a modified frame does not remove the factory frame. If a receiver
uses the indicated bit as a command, it can see alternating OFF/ON values;
its filtering or state machine determines whether it ignores, retains or
changes output. A last-value-wins implementation would be reversed by the next
factory publication. If the bit is merely published state, injection may change
only another ECU's interpretation without affecting BCM's physical output.
Neither behavior is established for these lamps.

This is an application-level conflict even when every frame is delivered
correctly in sequence. It differs from simultaneous same-ID/different-payload
transmission, which can cause CAN errors. Dominant/recessive bit levels do not
grant a modified frame higher semantic priority. Sending faster does not prove
control and is not the recommended feature design. The same conflict concern
applies to interleaved factory and injected 0x738 values.

A dependable design would need either a verified request input that changes
BCM's own decision, or an appropriate physical input interface with factory
behavior preserved. A documented, latched request could coexist with ordinary
state publications; none has been identified for this function. An inline
filter/gateway could replace publications only after hardware/network redesign;
a parallel CAN node cannot retract factory messages. No suppression, gateway
modification, diagnostic actuation or new transmission was attempted here.

This strengthens the decision to stop treating status-copy injection as a
validated Rain lights solution. It does not prove every older layout applies
to the owner's LEDs or that all possible CAN request mechanisms are absent.

### Verified connector detail and interpretation boundary

Reinspection of the 2018 figures: the basic-lamp connector (page 10) assigns
pin 1 to positive dipped control, pin 5 to positive high-beam control, pins 2/3
to DRL/side-light control/signal, pins 9/12 to adjustment power/signal and pins
13/14 to ground. Page 8 draws discrete wires from BCM to the lamps. This pinout
is scoped to that basic assembly, not the xenon or facelift connector.

Page 11 explicitly describes direct BCM control for D5S 25 W. Page 12 shows
BCM-to-HDU/lamp and shutter paths for D3S 35 W, separately from AFLM's CAN-C2
connection and LIN adjustment branches. Lamp electronics perform discharge
lamp power conversion; "direct BCM control" does not mean the BCM directly
produces the discharge lamp ignition voltage. Exact semiconductor switching,
PWM waveform and variant-specific electrical ratings are not established by
these functional drawings.

Consequently, for the illustrated older variants the last control/power path
to dipped illumination is discrete wiring. It is incorrect to infer a CAN
command accepted by the lamp from a correlated CAN indicator alone. This does
not exclude BCM receiving upstream CAN inputs, publishing its output state,
or an adaptive ECU using that state as a prerequisite for adjustment. It also
does not assign the source/function of 0x73E in a particular newer vehicle.

No BCM cavity number or xenon/LED connector pin is prescribed for installation:
those require the exact-variant harness schematic. The page-10 basic connector
is evidence of topology, not an instruction to connect to the owner's LED lamp.

## Headlamp rotary switch to BCM, follow-up 2026-10-26

The 2018 techCONNECT document, pages 16–17, explicitly connects the headlamp
control panel to BCM. Its 12-pin panel connector identifies pin 4 as exterior
light signal, pin 1 as fused ignition-on supply and pin 12 as ground. The
panel pinout contains dedicated signals, not CAN-H/CAN-L or a LIN connection.
The equivalent [Fiat Stelvio training page 76](https://lab-aftersales.fiat.com/elearn_testreda/949001497.pdf#page=76)
also describes the panel-to-BCM connection and those functions (indexed excerpt;
full live download of that source was previously unavailable).

A reproduced [2018 Giulia Ti service description](https://lemon-manuals.la/Alfa%20Romeo/2018/Giulia%20Ti%2C%20RWD/Repair%20and%20Diagnosis%20%28Single%20Page%29/Accessories%20%26%20Equipment/Interior%2FIllumination%20Lights/Lamps%2FLighting%20-%20Interior%20%28Service%20Information%29/Description%20And%20Operation/)
explicitly identifies a hardwired resistor-multiplexed input from the exterior
rotary knob. BCM interprets the encoded selection; the knob does not carry the
lamp operating current. No resistor values, voltage thresholds or validated
position-to-resistance table were obtained. Do not invent them or infer that
shorting the input produces dipped ON.

The reproduced [2019 Giulia Sport RWD, LHD BCM C8 pinout D2762H](https://lemon-manuals.la/Alfa%20Romeo/2019/Giulia%20Sport%2C%20RWD/Repair%20and%20Diagnosis%20%28Single%20Page%29/Quick%20Lookups/Electrical%20Component%20Locations/Connector%20End%20Views%20%26%20Electrical%20Component%20Locations%20%285%20Of%2011%29/Module-Body%20Control%20C8%20%28Standard%29%20Connector%20End%20View%20%28D2762H%29/)
identifies cavity 42 as headlamp switch signal, circuit M9, white/yellow,
0.35 mm2. Cavity 13 is the separate panel-dimmer MUX input. These documents
identify corresponding endpoints but are not a single VIN-matched end-to-end
harness drawing; pin 4 -> BCM C8/42 must not be prescribed for the owner's
facelift without matching its variant. Connector C8 is not the name of a CAN bus.

Functional conclusion for documented older versions: knob selection -> wired
resistive input -> BCM decision -> dedicated lamp outputs, with CAN reports
available to other controllers. This explains why no independent switch CAN
pulse need appear before the observed 0x73E/0x354 changes. It does not prove
that no alternative BCM request exists. Emulating this input would require an
appropriate electrical interface and exact variant measurements; sending a
copy of a CAN state publication is not equivalent. No hardware connection,
firmware modification or actuation was made.

# BACCAble — in-car user guide

This guide describes the current dashboard-menu firmware source, including the
startup drive-mode/suspension corrections and Rain lights after v5-beta-29.
Startup Race and the softer suspension setting were accepted in the car on
2026-10-24; Rain lights output acceptance is still open. It starts after
BACCAble has been installed and
connected to the vehicle. Installation, wiring and firmware flashing are outside
its scope. Use a matching C1/C2/BH firmware set.

The older [English manual](Baccable_manual_EN.pdf) supplied the original function
descriptions. Menu paths, controls, labels and operating details below have been
checked against the current firmware. When the old manual differs, follow this
guide. Vehicle equipment and ECU software determine which readings and actions
are supported; a menu entry alone does not establish compatibility.

## Contents

- [First use](#first-use)
- [Controls and navigation](#controls-and-navigation)
- [Menu structure](#menu-structure)
- [Display symbols and messages](#display-symbols-and-messages)
- [Temporary IPC refresh options](#temporary-ipc-refresh-options)
- [Favorites and reading layout](#favorites-and-reading-layout)
- [Settings: Features](#settings-features)
- [Function workflows](#function-workflows)
- [Actions](#actions)
- [Information](#information)
- [Reading the measurements](#reading-the-measurements)
- [Reading catalog](#reading-catalog)
- [USB CAN capture and diagnostics](#usb-can-capture-and-diagnostics)
- [Troubleshooting](#troubleshooting)
- [Changes from the old manual](#changes-from-the-old-manual)
- [Sources and scope of verification](#sources-and-scope-of-verification)

## First use

1. Switch on the ignition with the car stationary. Start the engine for functions
   that need it. Some readings are available only with the engine running.
2. Switch **both CC and ACC off**, then release the steering-wheel buttons.
3. Hold **RES for about 0.9 seconds**, then release. The menu opens in Favorites.
4. **Double-click RES** to return to the main menu. Use up/down to find `Settings`,
   click RES, then enter `Features`.
5. Select `Engine`. Choose `2.0 I4`, `2.9 V6` or `2.2 D` with clicks or up/down.
   Hold RES to save directly, or double-click to show the save/discard prompt. The
   screen remains on that setting. Double-click again to return to Features.
6. Check `MY23 display` against the installed instrument panel. MY23 uses two lines
   regardless of the legacy 18/24-character build option. Build and flash a
   matching C1/C2/BH set; this candidate introduces a longer screen packet.

C1 handles powertrain CAN and menu input, C2 chassis CAN, and BH body CAN and
IPC transmission. Check `Information` for board versions. Configure the menu
while stationary; follow the project's [use restrictions](../README.md#disclaimer).

## Controls and navigation

RES and the ACC distance button have the same menu role when CC and ACC are off.

| Gesture | Result |
| --- | --- |
| Hold RES for about 0.9 s with the menu closed | Open Favorites. |
| Click RES and release | Enter/use an item or edit its draft. A reading does not react. |
| Double-click RES | Request Back. Start the second press within 0.28 s of the first release. |
| Hold RES for about 0.9 s in an editor | Save the displayed draft directly, including after a save/discard prompt. |
| Hold RES in a displayed APPLY/STORE prompt | Confirm that action or mirror capture. |
| Gentle down/up | Next/previous item or change the draft. Lists wrap. |
| Hold gentle down/up | Repeat after 0.5 s, then every 0.18 s; no repeats in Actions or while reordering a favorite item. |
| Stronger down/up | Jump between functional groups; in a draft, adjust its value. |

A click is resolved after the double-click window, so a single click has a short
intentional delay. Presses shorter than 30 ms are rejected. A third rapid click
following a double-click is consumed; a deliberate hold immediately after it still works. Scrolling cancels a pending click pair.
After a gap over 0.3 s in button reports, release the button and try again.
Releasing a hold never also clicks. A stronger direction can first register a
normal item step before its group jump.

### Returning, inactivity and saving

Entering an editor shows the saved value without changing it. Hold RES in an
editor to save directly; no preceding double-click is required. Double-click
returns one level. With an unchanged setting it returns immediately.
With a changed draft, the first double-click shows `HOLD SAVE•2X DISCARD` on MY23
(`Hold save/2x drop` on legacy). Hold RES to persist and apply; the setting stays
visible with `SAVED` feedback for about three seconds. A separate double-click
returns. Double-click in
the confirmation instead to discard and return, keeping the original setting.
A failed save keeps the draft and confirmation available for another hold.
A restart loads committed values only. USB mode changes only after a successful
explicit save; selecting CAN or merely leaving an unsaved draft does not activate it.

The same draft workflow applies to Features, Favorite slots, mirror Enabled and
Sort order. There is no Save & Exit item. Live readings stay visible. Other menu
views return to Favorites after 30 s without input, or 60 s in editors; unfinished
drafts are discarded. Active fault work postpones this timeout.

The Favorites editor uses the same draft, confirmation, hold-to-save and discard
rules. Saving stays in the editor; a separate double-click returns to Settings.
Timeout discards unfinished changes. The selected reading and favorite are
persisted when leaving ordinary views.
If `× Save failed: RES` appears during
an exit, click or hold to retry that exit or double-click to cancel it and remain in the
current view. Waiting does not automatically retry.

## Menu structure

```text
Main menu
├─ Favorites                 6 sets, 2 rows × up to 4 items
├─ Readings
│  ├─ Oil
│  ├─ Temperatures
│  ├─ Engine
│  ├─ Fuel
│  ├─ Battery
│  ├─ Diesel / DPF
│  ├─ Driving
│  ├─ Performance
│  └─ Diagnostics
├─ Actions                   Run a function or diagnostic operation
├─ Settings
│  ├─ Features               Device and vehicle-function settings
│  │  └─ Park mirror
│  │     ├─ Enabled
│  │     ├─ Store position
│  │     └─ < Back
│  ├─ Favorites              Favorite 1–6 → Row A/B → item → reading + format
│  └─ BACCAble IPC           Temporary refresh experiments
└─ Information               Versions, input status and Demo
```

The main menu order is fixed. Readings shows individual measurements organized
into the groups above; groups with no reading eligible for the configured vehicle
profile are omitted. Reading availability follows the vehicle profile (engine
class and installed equipment), not the old `Advanced pages`/`Shown pages` toggles.
Favorites compose individual readings into two logical rows. On first start the
engine-aware defaults are loaded and saved once (the 0x106 UI record).

## Display symbols and messages

MY23 has **16 visible glyphs in line 1 and 22 in line 2**. A list shows the
current item with `›` above the next item. Editors show a setting and its draft
value, with `HOLD SAVE`. Confirmation and saved feedback use the second line.
Live Favorites show one row of a Favorite set on each line (MY23), or one row
subpage on legacy. Readings show one selected measurement.
`SAVED` confirms a completed settings save; `×` means a local failure.
The legacy profile retains its 18/24-character first line and `BACCAble beta`
footer. It uses the same button and draft rules, with single-line prompts.

BH uses the last complete media-text source it observed, such as USB or CarPlay,
rather than forcing Bluetooth. It resends the full overlay about once per second
and restarts after radio text settles. Closing restores the last complete radio
text. Both MY23 fields are cleared and padded independently, so shorter titles
cannot shift the second line or leave old characters. Physical readability,
alignment and radio takeover still require the vehicle acceptance test.

The first up/down click updates the menu immediately. During rapid clicks,
every click changes the selection, but the display may skip intermediate
screens and show the latest choice after a short pause. The delay follows the
selected IPC pace (10, 20 or 50 ms); radio text can add more delay.

| Mark or message | Meaning and response |
| --- | --- |
| `O` / `Ø` before an option | Disabled / enabled, or not selected / selected in a membership list. Short RES changes it. |
| `ON` / `OFF` in a status | Reported or locally tracked state. This is not necessarily an editable checkbox. |
| `>` | Enter a submenu or start an action/workflow with RES. |
| `*` | A number is being edited, or a favorite is selected for moving. |
| `< Back` | Return from the submenu. Double-click RES also goes back. |
| `!` | A condition needs attention, for example `! Start engine` or `! Stop the car`. |
| `?` | The state is unknown or confirmation is missing, for example `? BH no reply`. |
| `×` | A local operation failed, for example saving settings or sending a request. |
| `--` within a reading | No usable value: missing, stale, unsupported or invalid data, or a value too wide for its field. It does not mean zero. |
| `WAIT` / `Request queued` | A request is pending. This does not confirm that the car performed it. |
| `Request sent` | The sending sequence finished. Check the vehicle's actual state. |
| `No confirmation` / timeout | No confirmation arrived within the UI's waiting period. A timeout does not cancel an already sent command. |
| `RUN` / `MISS` | An acceleration measurement is running / has no valid result within its measurement window. |

`°C` means temperature in degrees Celsius; dense combined screens may use `C`.
Ignition correction uses `°` for an angle. Negative readings retain their minus
sign. Optional `«`/`»` beside a numeric editor indicate adjustment, when space
permits. Lists do not show item counters.

## Temporary IPC refresh options

Open `Settings → BACCAble IPC`. Up/down selects a row; a short RES click cycles
its value and applies it immediately. Double-click returns to Settings. These
experimental controls do not use HOLD SAVE and never write to Flash.

| Row / choice | Effect |
| --- | --- |
| `Safe 50ms` | Conservative: 50 ms between menu CAN fragments. |
| `Quick 20ms` | Default: 20 ms between fragments. |
| `Fast 10ms` | 10 ms between fragments. |
| `Write Full` | Default: send a complete, padded screen from the first to the last fragment. Its content stays fixed during that transmission; newer screens wait in one replaceable slot. |
| `Write Delta` | Send changed fragments; newer values may replace content during transmission. This is a comparison mode and can show mixed old/new text. |
| `Reset default` | Restore `Quick 20ms` and `Write Full` immediately. |

Remember a test as a pair, for example **Quick + Full**. MY23 needs 13 frames:
a full pass takes roughly 650 / 260 / 130 ms at Safe / Quick / Fast, plus any
CAN or radio wait. A new screen can also wait for the active pass to finish.
The IPC decides when received fragments become visible; Full prevents firmware
from mixing screens, but does not guarantee an instantaneous visual swap.
The periodic full reassertion remains one second in both modes. Radio source
tracking, settling and bounded arbitration remain active. Restoring radio text
keeps its original 50 ms pace.

Use matching C1/C2/BH firmware. These choices exist only in RAM, even if other
Features are saved. A complete power cycle restores Quick + Full. Disconnect
**both USB power and OBD** to power down the entire device. BH also falls back
to Quick + Full after five seconds without renewal from C1. While C1 is running,
it renews the chosen experimental pair once per second, including after menu
exit; a BH-only restart may therefore receive that pair again. `UART busy: retry`
means the click was not accepted; retry it. Queue acceptance is not a BH or IPC
acknowledgment.

For comparison, use the same audio source and repeat: long/short menu labels,
current/next scrolling, live Favorites, radio track change, then return to the
menu. Record the pair, visible overlap, response delay and source. Start with
Quick + Full, then optionally Safe + Full or Fast + Full, and optionally compare Delta.
If behavior worsens, select Reset default or completely power down the device.

## Favorites and reading layout

Favorites are six sets, each with two rows (**A** and **B**) of up to four items.
Each item stores one individual reading and a display format: **Auto**, **Full**,
**Medium** or **Short**. **Auto** chooses the most readable fitting label once
when the layout is resolved. A Favorite row joins its items with at least one
space and never clips digits; `--` means missing or stale. MY23 shows both rows;
legacy navigates the non-empty rows (1/A, 1/B, 2/A, …). The same reading may
appear in several rows and is polled only once.

### Default favorites

A first start loads engine-aware defaults and saves them once. A later vehicle
profile change never overwrites a saved layout; an unsupported saved item simply
shows `--` until it applies again.

| Set | Row A | Row B |
| --- | --- | --- |
| Warm-up | Oil temp + coolant | Gearbox temp |
| Oil | Oil level mm + LOW/OK/HIGH | Oil level bar |
| Engine | Power + boost | Power + torque |
| Cooling | Coolant | Intercooler inlet + outlet |
| Battery | Voltage + current | IBS state of charge |
| Diesel DPF / Gasoline Performance | DPF load + temp (diesel) / 0–100 (gasoline) | Regen progress (diesel) / Best 0–100 (gasoline) |

### Edit a favorite

1. Open `Settings → Favorites` and choose `Favorite 1` through `Favorite 6`.
2. Choose `Row A` or `Row B`, then an item slot.
3. Choose a reading (or `Empty`) and a format (Auto/Full/Medium/Short).
4. Hold to save. A first BACK confirms a dirty draft; a second BACK discards it.

### Oil level and bar

`Oil level` (mm + LOW/OK/HIGH) and `Oil level bar` are two separate Readings;
the bar occupies its row alone. The owner-supplied OK ranges are unchanged:
2.0 RWD 72–82 mm, 2.0 Q4 67–77 mm, 2.2 RWD 61–65 mm, 2.2 Q4 79–84 mm. They are
not newly verified here. Unknown drivetrain shows `?` and `Set drivetrain`.

## Settings: Features

Open `Settings → Features`. The tables follow the actual browsing order.
The group headings below explain stronger up/down jumps; they are not extra
submenus. Short RES opens the editor at the saved value. Once inside, clicks
or up/down change on/off and named choices. `LKA` displays `OFF` or `ON`
explicitly; the other switches use their checkbox in the list.

For **Launch Nm**, **Shift RPM** and **Pedal trim**, click to edit and use
up/down to adjust. Hold to save directly. Double-click instead to show
confirmation; hold there to save or double-click again to discard. Switches and named choices use the same explicit
workflow; their draft does not control the car before successful persistence.

### Display group

| Option | Values | Purpose |
| --- | --- | --- |
| `Shift light` | Off / on | Request the instrument-panel shift warning above `Shift RPM`, with compatible Race-mode display support. This does not shift the gearbox. |
| `Shift RPM` | 1500–6000 RPM, step 250 | Engine-speed threshold for the shift warning. |
| `MY23 display` | Off / on | Use the MY23 menu layout: 16 glyphs on line 1 and 22 on line 2. OFF retains the legacy single-line layout with the compiled 18/24-character width. |
| `DPF regen alert` | Off / on | Enable diesel regeneration notifications. See [DPF monitoring](#dpf-monitoring). |
| `Auto rotate` | Off / on | Rotate live pages every five seconds. |

### Comfort group

| Option | Values | Purpose |
| --- | --- | --- |
| `Seatbelt alarm` | Off / on | Request the seatbelt acoustic-warning configuration. This is distinct from the read-only `Seatbelt alarm` page. Check the resulting vehicle setting; do not assume removing BACCAble reverses ECU configuration. |
| `Stop odo blink` | Off / on | Suppress the blinking odometer display. It does not repair the configuration mismatch that caused the blink. |
| `Park mirror` | Submenu | Enable reverse mirror positioning and explicitly store the target positions. See the [mirror procedure](#parking-mirrors). |
| `Close windows` | OFF / 1 lock / 2 locks | Choose the key-fob lock gesture that starts window closing. Narrow screens use `Close win`. |
| `Open windows` | OFF / 1 unlock / 2 unlocks | Choose the key-fob unlock gesture that starts window opening. Narrow screens use `Open win`. |
| `QV exhaust` | Off / on | Permit the exhaust-valve request action and the gear-lever release-button gesture on compatible hardware. |
| `Rain lights` | OFF / ON; default OFF | In AUTO, ask BH to report darkness after three detected wiper cycles within 15 seconds. Keep the request until 60 seconds after the last cycle. Requires matching C1/C2/BH firmware. Vehicle lamp operation remains unverified. |
| `Auto PDC mute` | Off / on | Temporarily mute active parking-sensor beeps while braking outside Reverse, then restore the change BACCAble made when braking is released or Reverse is selected. Requires current brake, gear and PDC reports. |
| `Mute audio in R` | Off / on | Mute audio in Reverse and restore audio afterwards if BACCAble muted it. Previously muted audio and a manual unmute are respected. This is independent of PDC mute. |


`Settings` → `Features` → `Comfort` → `Rain lights`: select ON and hold to save. Times are fixed.
Each further detected cycle renews the 60-second hold. One sweep alone does not
trigger the function; several washer sweeps can trigger it. This uses wiper
activity, not a measured rain intensity. Very sparse wiping may not reach the threshold.

Leaving AUTO, disabling the option, or stale input/link data ends the request.
C2 sends the request directly to BH; BH also checks its own fresh AUTO report
and expires the request after three seconds without renewal. For each fresh
BH `0x738` light-sensor report, BH copies the frame and sends its two raw light
bytes as zero while the request is active. It stops sending dark copies when
the request ends; it does not send a lights-off command. Factory AUTO can take
about 14–20 seconds to turn the lamps off after light returns in the recorded
trials. Whether the vehicle accepts the added dark report and turns on front
and rear lamps is still unverified. Check the actual lamps in a stationary
daylight trial. The board-link timing keeps C2 status and USB capture responsive.
When Rain lights is enabled, unchanged menu text is refreshed less often to
reserve the shared board link for lighting and status commands; a changed menu
selection still appears immediately.

### Assistance group

| Option | Values | Purpose |
| --- | --- | --- |
| `Virtual ACC` | Off / on | Use a compatible standard cruise-control pad for ACC functions. It requires an ACC-capable, correctly configured vehicle; it does not add radar or ACC capability. |
| `ACC resume` | OFF / RES / + | Select whether BACCAble sends automatic RES or gentle-up requests while ACC is engaged and holding a stationary car. This can allow the car to resume movement; see [assistance controls](#acc-and-has-controls). |
| `Virtual HAS` | Off / on | Permit simulated Highway Assist button presses through the action or a double press of the lane button. Requires compatible, configured vehicle assistance equipment. |
| `LKA` | OFF / ON | **OFF** requests LKA off once at each ignition start. C1 recognizes OFF → RUN on `0x1EF`, including starts without device sleep; cranking does not start a second session. After configuration reaches C2, it waits up to 5 s for LKA ON on `0x547` and a released stalk report, then sends one C2 `0x384` double-click: press, release, press, release, about 100 ms apart. Initial OFF reports are allowed while waiting; no click is sent while LKA is off. A driver press cancels the pending action. **ON** keeps factory behavior and sends no automatic click; it does not forcibly enable LKA. Saving applies this preference at the next ignition start. No later periodic override is sent. Existing `Disable LKA: ON` becomes `LKA: OFF`, preserving the saved intent. Beta 24 was reported to work once after connection, but failed on later ignition cycles; this correction still needs repeated vehicle testing. |

### Drivetrain group

| Option | Values | Purpose |
| --- | --- | --- |
| `Launch Nm` | 25–600 Nm, step 25 | Reported engine torque at which launch assist requests release of the forced front brakes. It is not a torque limit or a tuning setting. |
| `Default mode` | A / N / D / Race | At vehicle start-up, BH simulates the recorded DNA selector input on `0x256` (four bytes). For A/D/Race, C1 and BH wait for fresh RPM above 400 for two seconds; the menu shows `Start engine` while C1 waits. `N` leaves factory selection untouched; old saved OFF becomes N without changing stored settings. A/D use 200 ms deflections; Race first reaches D, then holds the D direction for 2.2 s. Held input is refreshed after factory-neutral reports, with a 250 ms fallback refresh for the long hold. Each new press follows a fresh neutral report and waits for an idle CAN transmitter. The actual mode must remain stable for one second before the next gesture. At most three mode gestures are allowed within twelve seconds. The physical selector takes priority, and any pending press is released on cancellation, engine stop or stale data. C1 waits up to 20 s for BH after the engine gate, then checks DNA and CRC-valid transmission reports for five seconds. `Confirmed` requires sustained fresh matching reports; it corroborates reported state, not every subsystem. The actual mode, including an intermediate mode before Race, remains selected. Settings save or ignition OFF cancels; transport resync does not re-arm. Update C1 and BH together. Owner-reported testing on 2026-10-24 accepted the `Race` preset in the car; `A` was not reported separately. |
| `Suspension` | Stiff / Soft / Mid | Sets the next-start suspension preference: D offers `Stiff` or `Soft`; Race offers `Stiff` or `Mid`. For A/N the softer choice is labelled `Softer` and has no effect. After the actual mode is stable for one second and suspension feedback for 300 ms, BACCAble presses the button for 150 ms only if a change is needed, including when already in D/Race. It waits for fresh feedback after release. One retry is permitted only after at least one second with fresh, continuously unchanged opposite-state feedback and completed earlier CAN transmission. Any observed desired-state reply or missing/unknown feedback prevents that retry. There are at most two presses. `No confirm` can mean suspension was not confirmed even though the DNA mode changed. Manual selector or suspension input cancels the automatic operation. Requires matching C1/BH firmware. Manual D and Race/Mid feedback codes are supported by recordings, including two complete labelled Race cycles on 2026-10-24. Automatic startup replay was accepted in the car on 2026-10-24: the configured `Soft` setting is applied. The feedback-gated retry path was not separately observed. |
| `ESC/TC control` | Off / on | Permit the ESC/traction-control toggle and supported lane-button gesture. Vehicle configuration must support the function. |
| `Dyno action` | Off / on | Permit `Toggle Dyno mode`; enabling this preference alone does not enter Dyno. |
| `Brake action` | Off / on | Permit `Brake override` and `Disable launch`. Enabling this preference alone does not apply the brakes. |
| `AWD off action` | Off / on | Permit `AWD off request` on an AWD vehicle. Enabling it alone does not send the request. |
| `Pedal mode` | OFF / Auto / Bypass / A / N / D / R / Hybrid / Kids | Choose the response mode of an installed compatible pedal controller. See [pedal modes](#pedal-controller). |
| `Pedal trim` | −10 to +10, step 2 | Adjust pedal-map response. This is a map trim, not a percentage increase in engine power. Kids uses its own fixed low trim. |

### Device group

| Option | Values | Purpose |
| --- | --- | --- |
| `Block Start/Stop` | Off / on | Automatically request disabling the vehicle's automatic engine Start/Stop function. “On” means blocking Start/Stop. |
| `CAN routing` | Off / on | Expose selected native CAN reports through the firmware's diagnostic routing mechanism for compatible diagnostic clients. It is not the USB capture switch. |
| `DTC clear action` | Off / on | Permit the multi-controller `Clear DTCs` action. |
| `BCM fault reader` | Off / on | Permit `Read BCM faults`, which reads the Body Control Module only. |
| `Engine` | 2.0 I4 / 2.9 V6 / 2.2 D | Select the engine-specific reading catalog. Switching discards cached measurements and temporarily filters incompatible favorites. |
| `Drivetrain` | Not set / RWD / Q4 | Select the drivetrain used with `Engine` for oil-level limits; default `Not set`. Save the choice by holding RES. This is not a gearbox setting. |
| `USB mode` | OFF / CAN / ELM327 | Select normal operation, binary CAN capture, or ELM-compatible diagnostics. ELM327 is offered in builds with that support. Modes are exclusive. Hold RES in the editor to save and apply, or double-click for confirmation first. |

Feature permissions and vehicle states are different: enabling `Dyno action`,
for example, merely makes its action available. Some actions are omitted when
their permission is off. A pending request can prevent changing its associated
permission until that request is resolved.

## Function workflows

### Parking mirrors

Compatible mirrors must report their position; ordinary powered mirrors without
position feedback may not support this function.

1. Park the car with the mirror controls available. Open
   `Settings → Features → Park mirror`.
2. Select `Enabled`, set its draft to ON, double-click for confirmation and
   hold to save. Enabling does **not** store the current positions.
3. Enter `Store position`. The prompt is `> Adjust;HOLD=save`.
4. Use the car's mirror controls to place both mirrors at the desired reversing
   positions. Let them stop moving, then hold RES at the STORE prompt to request storing them.
5. `Store: queued` means that the request was queued to BH. There is no storage
   acknowledgement from BH on this screen. Short RES dismisses the result;
   double-click RES goes back. If sending was busy, retry while the desired positions
   are still set.
6. Return the mirrors to their normal driving positions and leave configuration.
   Verify the stored targets during a stationary reversing test.

With the engine running, select Reverse and indicate left or right to request
the stored position for that side. BACCAble captures the normal positions for
restoration. A movement request can take a few seconds to begin. Selecting the
other indicator can also select the other mirror during the maneuver.

Restoration is requested immediately in Park, with the engine stopped, or when
the function is disabled; Neutral must persist for about 0.5 s. Other exits from
Reverse allow a delay of up to 10 s. Brief gear transitions are deliberately
ignored. Missing or stale position/gear reports can prevent movement: check the
mirrors rather than treating a queued request as proof of movement.

### Window gestures

Choose one or two key-fob presses independently for opening and closing. For a
multi-press gesture, make the presses within about three seconds of each other.
Closing starts after approximately four seconds from the last lock request;
allow the movement sequence to finish.

An **extra lock press** requests a brief reopening for ventilation after closing:
two presses with `1 lock`, or three with `2 locks`. The opening is time-based,
so its actual size depends on the vehicle. An unlock request cancels an ongoing
close request; a lock request cancels an ongoing open request. Observe the windows
and confirm the final position before leaving the car.

### Pedal controller

These settings require the optional compatible pedal controller. They do not
remap the engine ECU. `Pedal map` shows the controller's reported map, which can
differ from the selected mode if communication is absent or a request is pending.

| Mode | Behavior |
| --- | --- |
| OFF | Request Bypass when switching off, then stop automatic map management. |
| Auto | Follow the vehicle's DNA selection: A, N, D or R. |
| Bypass | Keep the controller's bypass response. |
| A / N / D / R | Request a fixed All Weather / Natural / Dynamic / Race map. |
| Hybrid | Use N normally and R when the vehicle reports Race. |
| Kids | Use A with fixed −10 trim. Above 4000 RPM on gasoline, 3000 RPM on diesel, or 100 km/h, request minimum throttle. This depends on the external controller and is not a guaranteed speed limiter. |

### ACC and HAS controls

With `Virtual ACC` enabled, the ordinary CC on-button also requests ACC on.
While ACC is already engaged, RES acts as the ACC distance button. With CC and
ACC off, RES remains the menu control. The function requires the vehicle's radar,
other required equipment and compatible configuration; the menu cannot supply
missing equipment.

`ACC resume: RES` or `ACC resume: +` sends the selected button request while ACC
is engaged, the car has remained stationary and ACC is applying the brake.
Use the setting appropriate to the vehicle's supported resume behavior. Select
OFF to stop these automatic requests. The driver must still supervise stopping
and restarting.

For HAS, enable `Virtual HAS`, then use `Actions → Press HAS button`, or double
press the lane-assist button within about one second. This simulates a button
press; it does not establish that Highway Assist engaged. Check the vehicle's
own assistance indication.

### ESC/TC, Dyno and brake/launch controls

These are vehicle-control functions for controlled testing. Dyno can disable
ABS, ESC and traction-control functions. Brake override can hold the front
brakes. The display reports requests and board replies, not measured brake
pressure or guaranteed drivetrain state.

- **ESC/TC:** enable `ESC/TC control`, then use `Toggle ESC/TC`. A supported lane
  button held for roughly 2–3 s provides the legacy gesture. Release after the
  response. The action is blocked while Dyno is active or its request is pending.
- **Dyno:** enable `Dyno action`, stop the car and wait for stationary status.
  Release brake override and restore ESC/TC's normal setting first. Use
  `Toggle Dyno mode` and confirm. The legacy gesture is a hold of the **parking
  assist button**, about six seconds; this is not the gear-lever P button.
  Use the toggle again to request exit. Engine shutdown also clears the Dyno
  state. Vehicle warning indications may need an ignition cycle to clear.
- **Brake/launch:** enable `Brake action`, set the desired `Launch Nm`, and enter
  Dyno while stationary. `Brake override` requests front-brake forcing and arms
  launch assist when C2 reports the forced state. The confirmation says
  `Brake+launch? HOLD`. Launch assist requests release when reported torque
  reaches the threshold.
- **Manual release:** if launch assist is armed, first execute `Disable launch`,
  then `Brake override` to request release. **Disable launch only disarms the
  torque-triggered release; it does not release the forced brakes.** Check the
  actual vehicle response before proceeding or leaving the test setup.

### Exhaust and AWD requests

Enable `QV exhaust` for compatible valve hardware. `QV exhaust req` alternates
between requesting open and returning to factory control. Double pressing the
gear-lever **release button** within about one second is the legacy alternative.
`AUTO` means factory control, not a guaranteed closed valve position.

Enable `AWD off action` for an AWD vehicle, remain stationary and execute
`AWD off request`. BACCAble repeats the disabling request until it is cancelled.
Select the action again and confirm `Stop AWD req?HOLD` to stop requesting it.
`AWD req: OFF WAIT` is a request to switch AWD off; it is not a measured
confirmation that only two wheels are being driven.

### DPF monitoring

On diesel, `DPF regen alert` enables notification of a detected high-regeneration
phase. Depending on the compiled visual/sound options, this uses the cluster's
DPF indication and/or a chime. The indication can persist for about ten reports
after the regeneration mode returns to NONE. It does not force regeneration.

Use `DPF load`, `DPF temp`, `DPF regen %` and `DPF regen mode` together:
load estimates filter loading, while regen percentage reports regeneration
progress. They are different measurements. `DPF regen mode` can show:

| Screen value | Meaning |
| --- | --- |
| `NONE` / `NONE.` | No named active regeneration phase in the corresponding decoded state. |
| `DPF LO` / `DPF HI` | Low / high DPF regeneration mode. |
| `NSC De-NOx` | NOx-storage-catalyst NOx regeneration. |
| `NSC De-SOx` | NOx-storage-catalyst desulfurization. |
| `SCR HeatUp` | SCR heating phase. |
| `?` | Unrecognized mode. |

The catalog does not determine whether an individual car has SCR/AdBlue hardware.
Hide unsupported AdBlue pages on a vehicle without it.

### Immobilizer status and owner control

`Information → Immobilizer` shows the stored ON/OFF state of BACCAble's protection
against diagnostic key-programming activity. This is separate from the factory
immobilizer and does not mean that the engine is currently blocked. Panic-alarm
behavior depends on the vehicle's configuration.

In builds where owner control is enabled, the existing toggle gesture is:
close the BACCAble menu, switch CC and ACC off, run the engine in Neutral, release
the steering-wheel controls, then hold **gentle up for about 30 seconds**.
Three instrument-panel brightness flashes indicate enabled; six indicate
disabled. Check Information afterwards. The gesture saves its setting separately;
a direction held inside the menu does not activate it.

## Actions

Open the main-menu `Actions`. Enable the corresponding permission in Features
first where required. The table follows the action order; unavailable permissions
can remove entries from the list.

For actions other than `Read BCM faults` and `Peak hold`, short RES shows a
confirmation; hold RES **within three seconds** to execute. Moving away
or returning cancels the confirmation. Conditions are checked again when the
action executes.

| Action | Permission / conditions | Effect |
| --- | --- | --- |
| `QV exhaust req` | QV exhaust; compatible hardware | Request valve open, or release to factory control. WAIT is not a valve-position acknowledgement. |
| `Press HAS button` | Virtual HAS | Send the simulated HAS button press. `HAS: Request sent` does not mean HAS engaged. |
| `Toggle ESC/TC` | ESC/TC control; Dyno inactive | Toggle the custom stability/traction-control request. Confirmation can remain unavailable. |
| `Toggle Dyno mode` | Dyno action; car stationary long enough; no brake override or inverted ESC state | Request Dyno on/off. Enabling asks `Dyno+ESC? HOLD`. A C2 reply ends the pending board request; verify vehicle state. |
| `Brake override` | Brake action; launch assist disarmed before changing an existing override; stationary and Dyno active to enable | Request forced front brakes plus launch assist, or request normal braking. See the release procedure above. |
| `Disable launch` | Brake action; launch assist currently armed | Stop torque-triggered brake release. Does not release brake override. |
| `AWD off request` | AWD off action; stationary to begin | Start repeated AWD-off requests, or explicitly cancel the sequence. No measured drivetrain confirmation. |
| `Read BCM faults` | BCM fault reader; no DTC clear running | Open the BCM fault-code browser immediately. Reading does not erase codes. |
| `Clear DTCs` | DTC clear action; no conflicting BCM read/clear | Run a clear request across the configured controllers on the vehicle buses. It is broader than the BCM reader and does not repair the fault. |
| `Reset best times` | No feature permission required | Clear the saved best acceleration records. Confirmation is required; a completed save reports `Records cleared`. |
| `Peak hold` | No feature permission required | Immediately toggle numerical maximum hold for the current reading. |
| `IBS SOC override` | Running engine; explicit confirmation | Toggle a temporary experimental SOC-message override. See the limitations below. |
| `Rain test` | No permission required; matching C1/C2/BH firmware | Open two rain-lights diagnostic pages; up/down switches between wiper status and the BH light-responsive sample. In AUTO, SELECT requests dark BH `0x738` copies for at most 30 s from either page; BH stops within 3 s if renewals cease. BACK ends the test. |

The first `Rain test` page is unchanged: `Wiper` counts rising wiper phases even
with `Rain lights` OFF; the first sample after a gap is only a baseline. `A`
shows fresh AUTO state (`?` means unknown), `ARM` the active rain request, `Q`
the count of active C2-to-BH requests admitted to UART modulo 256, and `F` the
C1 force request. `C2: no reply` means the C2 data expired. `Q` does not
confirm that BH transmitted a CAN frame or that lamps came on.
The second page shows `BH738 6/6` for example: the two **raw**, separate values
from BH `0x738` bytes 1 and 2. Daylight was usually `6/6` and a covered light
sensor `0/0` in recorded tests; these are not lux or calibrated dark/bright
thresholds. The second line's `TX` counts dark `0x738` frames accepted into
BH's CAN queue modulo 256; it does not confirm vehicle acceptance. `BH738 no
reply` means no fresh reply from BH; `BH738 no frame` means BH replied but has
no recent `0x738` frame. The visible page refreshes
about every two seconds. During a forced test, C2 is contacted every second to
keep the temporary request alive; the BH sample updates about every five
seconds. `BH738 no reply` with `UART/BH timeout` means a fresh reply has not
arrived, not that BH is necessarily disconnected.

Dyno/brake requests wait up to ten seconds for their board confirmation. A timeout
does not replay or cancel a sent command, and a later response may still arrive.
Brake acknowledgement describes the override sequence, not measured pressure.
AWD and exhaust requests have no measured-state acknowledgement.

### Reading and clearing faults

1. Enable `BCM fault reader` in Features and return.
2. Select `Actions → Read BCM faults`. Wait for `Reading faults...` to finish.
3. Browse codes with up/down. Codes appear in forms such as `Uxxxx-xx`; the screen
   does not supply a repair description. Up to **20 codes** are displayed. A `+`
   prefix indicates that the response contained more codes than fit in the list.
4. Short RES starts another read; double-click RES leaves and cancels an unfinished read.

`No faults reported` refers only to this successful BCM read, not the entire
vehicle. `× CAN send failed`, `× Read timeout`, `× ECU rejected` and
`× Invalid reply` describe distinct failures; they are not a clean fault report.

Record needed codes before clearing. Enable `DTC clear action`, select
`Clear DTCs` and confirm. `DTC: Request sent` means the multi-controller request
sequence finished, not that every controller accepted it or that the cause was
removed. Read relevant controllers again with suitable diagnostics to verify.

### Peak hold and acceleration results

`Peak hold` keeps each numeric field's maximum since the current page interval
started. Changing pages or toggling hold starts a new interval. It is the largest
signed value, not the largest absolute magnitude. Status fields remain live;
stale or unavailable signals still show `--` instead of an old maximum. Switch
hold off for normal live readings. Rotation changes pages and therefore resets
the interval.

Acceleration timing uses reported vehicle speed. `0-100 km/h` starts when moving
from rest and completes when the upper threshold is reached; `100-200 km/h` uses
the corresponding higher thresholds. `RUN` is an active attempt; `MISS` indicates
no valid result in the measurement window (20 s and 40 s respectively). Completed
results use seconds. `Best 0-100` and `Best 100-200` retain the best saved results
across restarts until reset. These are vehicle-signal timings, not a certified
external timing measurement.

### IBS SOC override

This experimental action **transmits modified battery-sensor reports**. For an
observed IBS SOC from 75% up to but excluding 98%, it briefly repeats reports
with SOC replaced by 75%. It is not a verified charging improvement and does not
restore a missing SOC reading.

It is off after startup, stops with the engine, and is cancelled by diagnostic
mode entry. Browsing SOC pages never enables it. Before changing USB mode,
disable the override; otherwise the menu displays `! Stop IBS first`.

## Information

The status pages are read-only. Gentle up/down moves through them; short RES does
not toggle their state. The final `Demo` entry starts the menu tour with RES.

| Page, in order | Meaning |
| --- | --- |
| C1 version | Firmware version of the controller managing the menu. |
| C2 version | Chassis-controller version. `? C2 no reply` means no recent version reply. |
| BH version | Body-controller version. `? BH no reply` means no recent version reply. |
| `MY23:ON/OFF …ch` | Legacy: MY23 preference and compiled 18/24 width. MY23 instead shows `IPC MY23` and `16 / 22 glyphs`. |
| `Immobilizer: ON/OFF` | Stored BACCAble immobilizer preference, not an editable menu switch. |
| `Reports:` | Number of steering-wheel input reports observed by the menu. Useful when diagnosing unresponsive controls. |
| `Gaps:` | Count of interruptions detected in the input report stream. |
| `Max gap:…ms` | Longest observed input-report interval in milliseconds. |
| `Input age:…ms` | Time since the last input report; `--` if none is available. |
| `> Demo` | Start a timed tour of the displayed menu and example values. |

C2/BH version replies older than about five seconds are treated as stale. Input
counters describe menu controls, **not CAN capture frame counts**.

Select `Information → Demo` to start the tour. It displays Favorites, Readings,
Actions and Features in a fixed sequence, including example oil and temperature
readings and apparent changes to Rain lights, Default mode and Suspension. These
are presentation examples: Demo does not change settings, write Flash or send
vehicle actions. A double-click of RES ends the tour and returns to Information;
other clicks do not operate the displayed items. The tour also returns to
Information automatically after its final screen. Values such as `77 mm` and
`2500 rpm` are examples, not current vehicle measurements.


## Reading the measurements

Select `Readings`, a group, and a reading. The live screen may use a shorter
caption than its label. All readings below are included in the catalog, but
unsupported ECU requests can remain `--`.

- **Power** is calculated from reported torque and RPM and expressed in metric
  horsepower (`PS`, called CV in the old manual). It is not a dyno measurement.
- **Oil volume, level, quality, pressure and temperature are different values.**
  Quality is an ECU-reported estimate, not a laboratory assessment. Observe the
  vehicle manufacturer's conditions for a valid oil-level measurement.
- **Ignition cyl 1–6** reports cylinder ignition correction in degrees. It is not
  a misfire count. V6 misfire coverage is limited to cylinders 1–4 because the
  catalog has no verified individual cylinder 5/6 misfire definitions.
- **Distance (ECU)** is the diagnostic distance value described in the old manual
  as distance since ECU odometer reset; do not equate it with the cluster's total
  lifetime odometer without validating that ECU's interpretation.
- **Gear** uses N, R or a gear number; DNA uses A, N, D or R. Missing states can
  show `--`/`?`. `Pedal map` uses the external controller's map response.

### Battery charge: which SOC is being shown?

SOC means state of charge. The firmware exposes several independent sources:

| Page or field | Source and interpretation |
| --- | --- |
| `Batt charge BCM` → `Batt BCM SOC` (gasoline) | Body Control Module diagnostic SOC. |
| `Batt charge / A` (both fuel catalogs) | Engine ECU diagnostic SOC, paired with native battery current. |
| `Battery charge` (diesel) | Native IBS battery-sensor SOC. |
| `Battery sources` (both catalogs, advanced) | IBS SOC followed by engine ECU SOC, each in percent. |
| `Battery IBS raw` | Raw IBS byte 0 and byte 1 as numbers. These are not two percentages. |
| `IBS raw0/SOC/V`, `IBS raw1/SOC/V` | One raw byte, BCM SOC and BCM voltage. These service pages use the BCM definitions even in the diesel catalog and may be unsupported. |

`Batt charge / A` can therefore show `--%` while current is present, and
`Batt charge BCM` can be unavailable while an IBS source is available. Sources
are not substituted for one another. A missing SOC does not establish an empty
battery. Use `Battery sources` to compare availability and `Battery V / A` to
observe voltage/current separately.

The inherited current convention is positive for charging and negative for
discharging. Voltage alone does not prove charging. Raw IBS pages are for source
diagnosis; do not reinterpret an entire raw byte as SOC without accounting for
its flags and decoding.

## Reading catalog

Readings are individual measurements grouped as below; groups with no reading
eligible for the configured vehicle profile are omitted. The live screen may use a shorter caption.
A value the ECU does not answer shows `--`.

### Oil

| Reading | Unit / meaning | Eligibility |
| --- | --- | --- |
| Oil temp | C | both |
| Oil pressure | bar | both |
| Oil level | mm | both |
| Oil status | LOW / OK / HIGH | both |
| Oil bar | full-width bar | both |
| Oil volume | L | both |
| Oil quality | % | both |

### Temperatures

| Reading | Unit / meaning | Eligibility |
| --- | --- | --- |
| Coolant | C | both |
| Gearbox | C | both |
| IC inlet | C | gasoline |
| IC outlet | C | gasoline |
| Intake air | C | diesel |
| Exhaust | C | both |
| Fuel temp | C | diesel |
| Catalyst | C | gasoline |

### Engine

| Reading | Unit / meaning | Eligibility |
| --- | --- | --- |
| Power | PS | both |
| Torque | Nm | both |
| Boost | bar | both |
| Intake | bar | gasoline |
| Ign cyl 1 | deg | both |
| Ign cyl 2 | deg | both |
| Ign cyl 3 | deg | both |
| Ign cyl 4 | deg | both |
| Ign cyl 5 | deg | 2.9 petrol |
| Ign cyl 6 | deg | 2.9 petrol |
| Misfires | count | both |
| EGR target | % | diesel |
| EGR actual | % | diesel |
| Turbo temp | C | diesel |

### Fuel

| Reading | Unit / meaning | Eligibility |
| --- | --- | --- |
| Fuel rate | L/h | diesel |
| Fuel rail | bar | diesel |

### Battery

| Reading | Unit / meaning | Eligibility |
| --- | --- | --- |
| Battery | V | both |
| Battery A | A | both |
| Batt IBS | % | both |
| Batt ECU | % | both |
| Batt BCM | % | both |
| IBS raw 0 | raw byte | both |
| IBS raw 1 | raw byte | both |

### Diesel / DPF

| Reading | Unit / meaning | Eligibility |
| --- | --- | --- |
| DPF load | % | diesel |
| DPF temp | C | diesel |
| Regen | % | diesel |
| Regen mode | phase | diesel |
| Regen ago | km | diesel |
| Regen count | count | diesel |
| Regen avg km | km | diesel |
| Regen avg min | min | diesel |
| AdBlue | L | diesel + SCR |
| AdBlue % | % | diesel + SCR |

### Driving

| Reading | Unit / meaning | Eligibility |
| --- | --- | --- |
| Speed | km/h | both |
| Gear | N / R / gear | both |
| DNA mode | A / N / D / R | both |
| Pedal map | map letter | both |
| Seatbelt | ON / OFF | both |
| Distance | km | both |

### Performance

| Reading | Unit / meaning | Eligibility |
| --- | --- | --- |
| 0-100 | s | both |
| 100-200 | s | both |
| Best 0-100 | s | both |
| Best100-200 | s | both |

### Diagnostics

| Reading | Unit / meaning | Eligibility |
| --- | --- | --- |
| Oil ECU temp | C | both |
| Oil ECU press | bar | both |
| MultiAir | C | 2.0 petrol |
| Knock | mV | gasoline |
| Key ID | number | gasoline |
| Run time | min | both |
| Over-rev s | s | both |
| Over-rev n | count | both |
| A/C pressure | bar | both |
| Turbo V | V | gasoline |
| Intake V | V | diesel |
| Turbo req bar | bar | diesel |
| Turbo req % | % | diesel |
| Turbo % | % | diesel |
| Boost req | bar | diesel |

## USB CAN capture and diagnostics

C1 reserves 96 KiB for the application and 20 KiB for optional USB file storage.
Standard C1 firmware uses serial USB and **does not expose this disk**. When
upgrading a custom mass-storage build from the 16 KiB layout, copy its files
before flashing: the disk start address changes and the disk may need formatting.
Saved settings keep their existing storage address. CAN recordings are stored
on the computer, so their capacity is unaffected.

USB is optional for normal menu use. Runtime USB modes are different from the
STM32 **DFU bootloader** used for flashing: seeing a device in `dfu-util --list`
does not mean a serial capture port should exist in DFU.

### CAN capture

1. Connect the USB cable(s) for the buses to record to the computer. A powered
   vehicle supplies the CAN traffic. A working USB data hub can be used. Connect
   the cables before activating the mode; eject any mounted BACCAble USB disk
   before changing its role.
2. Select `Settings → Features → USB mode`, select the CAN draft, double-click
   for confirmation and hold RES to save. C1 sends
   the capture setting to C2 and BH; you do not select it independently in three
   dashboard menus.
3. Identify the enumerated ports. On macOS they appear as `/dev/cu.usbmodem…`.
   C1 = powertrain, C2 = chassis, BH = body. Use USB product/serial identity and
   the Lab doctor output where available; host-assigned path numbering alone
   does not prove the board role.
4. Use BACCAble Lab with explicit role-to-port assignments. Capture can use one,
   two or all three buses. With Lab installed and its environment active:

   ```sh
   baccable doctor
   baccable capture --port C1=/dev/cu.usbmodemXXXX --no-obd --no-voice
   ```

   Replace the placeholder with the actual path. Add
   `--port C2=/dev/cu.usbmodemYYYY` and `--port BH=/dev/cu.usbmodemZZZZ` for more
   buses. Doctor diagnoses the host setup; it does not activate USB mode in the
   car or start recording.
5. Check that every selected bus receives frames and that loss/error counters
   remain acceptable. Describe or mark each vehicle action in the capture
   workflow so it can later be correlated with changing CAN data. Stop with
   Lab's normal quit/Ctrl-C procedure and inspect the session summary.
6. Capture is driven by **guided scenarios**: the screen shows one controlled
   procedure at a time (in Polish, using the car's own terms) and you confirm
   each step with `ENTER`/`SPACJA` (zrobione), `x` (nieudane) or `k` (pomiń ten
   krok). A step you do not want to perform — for example provoking ABS, ESC or
   traction control — is skipped with `k` and writes no marker, so it never looks
   like evidence that the action happened. `o` skips the whole procedure and `e`
   skips it and stops scheduling it permanently (also `baccable scenarios
   --exclude ID` / `--include ID`, both kept in the same state file). A stopped
   scenario resumes with `r`, `u` undoes the last
   confirmation, `s` jumps to another procedure and `q` (or `Ctrl-C`) ends the
   session. Keys always work during a scenario: `p` stops, `n` adds a note, `u`
   undoes and `q` exits. Colour, a progress bar and a short bell make the screen readable and
   usable from inside the car (`--caps`, `--no-color`, `--no-bell`). Procedures
   run in a functional order (stationary, then parking-lot driving, then on-road
   assistance) and advance automatically; progress is stored in
   `config/scenario_state.json`, so the next launch continues where you left off.
   List the procedures and their progress with `baccable scenarios`; follow
   [the scenario guide](../baccable_lab/docs/SCENARIOS.md).

The initial unconfigured USB session expires after about ten seconds. Each
auxiliary board needs its own host connection; an unconnected board can leave
capture while the connected boards continue. C1 also tracks connected peers so
a one-bus C2/BH capture can keep the capture coordination awake. If a desired
board's port has already expired, reconnect cables and reapply CAN mode. You do
not need all three ports for a one-bus capture.

CAN mode emits a **binary stream**, not SLCAN text, and does not automatically
record to an onboard USB disk. Normal vehicle features continue during capture,
so the log may also include diagnostic traffic generated by BACCAble or another
tool. Lab preserves raw streams and decoded records in a separate session with
`session.sqlite3`; a new capture does not overwrite the previous session. It
records identifiers, payloads and timing, not automatic human names for unknown
signals. Loss markers or port errors mean the recording needs review.

For installation, session controls and validation, use the
[Lab guide](../baccable_lab/README.md),
[hardware setup](../baccable_lab/docs/HARDWARE_SETUP.md) and
[capture guide](../baccable_lab/docs/CAPTURE.md).

If only C1 appears although all USB cables were connected before enabling CAN,
check the matching firmware set and the C2/BH status reports. The board link also
carries menu text; its failure can hide the menu while C1 USB still works.

### USB recovery candidate diagnostics

In the USB recovery candidate, C1 sends addressed mode requests and waits for
C2/BH acknowledgements. Allow a few seconds for all three ports. USB initialization
has at most three attempts; a failure leaves normal menu processing running.
Explicitly save OFF, then explicitly save CAN to try again. Install
all three matching candidate images; old auxiliary firmware lacks this protocol.

With the updated Lab and its optional `usb-status` dependencies installed, run:

```sh
baccable doctor --usb-status --samples 3
```

This reads status without opening a capture port. `pending_ack` lists boards that
have not acknowledged the mode; `fresh: false` marks old or missing status.
`usb_stage: ready` means USB started; `configured: true` means the host configured
it. `last_usb_error` remains recorded after recovery until reboot. The directly
read `main_loop_ms` should advance between samples. A stationary screen alone
cannot establish whether C1 stopped or BH lost screen updates.

Eject mounted disks before changing mode. A macOS disk-removal warning does not
confirm CDC failure; check the new ports and status. See the
[USB audit and vehicle test](../docs/architecture/USB_DIAGNOSTICS.md#msc-to-cdc-failure-and-recovery-candidate-2026-09-27)
for installation, diagnostics and the repeatability test. Candidate vehicle
acceptance is still pending.

### ELM-compatible diagnostics

Select the ELM327 draft in `USB mode`, double-click for confirmation, hold to
save, then connect a compatible host
diagnostic client to C1's new serial port. This is a bounded ELM-style CAN
implementation, not a full ELM327 with every protocol. Unsupported commands can
return `?`; unavailable diagnostic data returns `NO DATA`.

Normal vehicle-feature processing is suspended during ELM diagnostics, so the
menu may not remain navigable. Disconnect USB to let the session expire after
about ten seconds; 120 seconds without a complete ELM command also ends it.
See [USB diagnostics](../docs/architecture/USB_DIAGNOSTICS.md) for supported
commands and bus behavior. Use explicitly save USB mode OFF for ordinary operation when finished.

## Troubleshooting

| Symptom | Check / next step |
| --- | --- |
| RES does not open the menu | Ignition/panel awake; CC and ACC both off; release controls, then hold RES for 0.9 s. Check C1 connection/firmware. ELM diagnostics can temporarily own processing. |
| Menu skips or stops reacting | Distinguish gentle from strong presses. Release after a stream interruption. Inspect Information's Reports/Gaps/Input age and C2/BH status. |
| Radio text remains on the display | BH should resend the complete two-line BACCAble message about once per second and restart it after a radio title/artist transfer. It follows the last observed media-text source. If the menu remains absent, check the BH link, matching firmware and the actual BH CAN output; host capture records received frames, not BH's own transmissions. |
| A reading or action is missing | Correct vehicle profile; check the action permission in Features. An unsupported saved favorite shows `--` until its profile applies again. |
| `No favorites` / `No pages` | Add compatible favorites or restore page visibility. Double-click RES still returns. |
| A value shows `--` | Wait for fresh reports, check ignition/engine conditions and ECU compatibility. USB diagnostics can suspend normal queries. A displayed catalog entry is not a guarantee that the ECU implements it. |
| `Batt BCM SOC --` or `Batt charge --%` | Compare the independent sources using Battery sources. Do not treat missing SOC as 0%, and do not use IBS override as a repair. |
| Setting reverts after a restart | Hold RES in the editor to save; a preceding double-click is optional. Resolve `× Save failed: RES`; unsaved RAM changes are not persistent. |
| `! Disabled in setup` | Enable the associated permission under Features, then return. |
| `! Stop the car`, `! Enable Dyno`, `! Release brake`, `! Reset ESC`, `! Disable launch` | Satisfy the specific action condition. For brakes, use the documented disarm/release sequence. |
| `! Request pending` | Wait for the outstanding request result; do not assume it was executed or cancelled. |
| `? C2 no reply` / `? BH no reply` | Check power, matching firmware and the inter-board connection. A stale version reply is different from proof that the entire CAN bus has stopped. |
| Mirror does not dip | Position-feedback hardware, stored target, Enabled, running engine, Reverse and the side's indicator; also check BH. `Store: queued` alone is not storage confirmation. |
| Pedal mode changes but response does not | Compatible pedal hardware and communication; compare the reported Pedal map with the requested setting. |
| CAN USB port absent | Leave DFU, boot matching application firmware, connect a data cable, apply CAN by leaving Features, and check for session expiry. DFU success only verifies the bootloader's USB path. |
| Only two of three capture ports appear | Identify which board is missing by product/serial or connecting individually. Check that board's cable, firmware and capture timeout, then reapply mode with all intended cables connected. |
| Capture has frames but unknown signals | Raw capture does not automatically name signals. Correlate logged actions and repeated tests with frame changes; preserve the original session. |

## Changes from the old manual

Readings and Favorites are now individual measurements. The old compound pages,
`Page favorites`, `Shown pages`, `Favorite order` and `Sort order` are gone;
Favorites are six two-row layouts and Readings are grouped individual readings
(see [Menu structure](#menu-structure)). Old page IDs are intentionally not
retained; a first start loads and saves the new engine-aware defaults once.

| Older wording or behavior | Current equivalent |
| --- | --- |
| Short RES returns to the main menu | Short RES selects; **double-click RES goes back**. |
| Strong press skips ten parameters | Strong press jumps functional groups; Favorites moves one page. |
| Save & Exit | Draft confirmation and hold to save; explicit retry on failure. |
| Flat parameter/function menu | Favorites, Readings, Actions, Settings and Information. |
| Enabling Park Mirror stores positions | `Park mirror → Enabled` and `Store position` are separate operations. |
| Generic Battery % / BATTERY | Distinct BCM, engine ECU and native IBS SOC sources; see the battery table. |
| OIL UN. AIR | `MultiAir temp`, I4 only. |
| SPARKL.1–4 | `Ignition cyl 1`–`Ignition cyl 4`; the values are ignition corrections. |
| T-ON / OVER RPM | `Engine run time`, `Over-rev time`, `Over-rev count`. |
| ODOMETER LAST | `Distance (ECU)`. |
| EXAUST GAS / CATAL. / WATER | `Exhaust temp`, `Catalyst temp`, `Coolant temp`. |
| AIR COND. / CUR. GEAR / R-DNA | `A/C pressure`, `Gear`, `DNA mode`. |
| GO during acceleration | `RUN`; completed values are seconds, invalid attempts show MISS. |
| Brake Normal/Assist/Forced cycle | Separate `Brake override` and `Disable launch` actions with conditions and confirmations. |
| Read faults described as unfinished | A bounded BCM reader is implemented; it is not an all-ECU scanner. |
| Immobilizer among menu functions | Read-only state in Information; the separate owner gesture remains. |
| Remote start | No exposed operational menu option in this release; older descriptions do not establish current support. |
| Flash CANable firmware to sniff | Runtime `USB mode: CAN` provides binary capture in matching C1/C2/BH firmware. The separate CAN/SLCAN firmware remains a different mode of use. |

## Sources and scope of verification

Descriptions were reconciled with the legacy English DOCX/PDF and the current
source: [menu navigation/actions](../firmware/baccable/features/menu.c),
[input handling](../firmware/baccable/features/menu_input.c),
[feature definitions](../firmware/baccable/settings/setup_entries.c),
[feature ordering/editors](../firmware/baccable/settings/setup_menu.c),
[reading catalog](../firmware/baccable/diagnostics/parameter_catalog.c),
[vehicle handlers](../firmware/baccable/vehicle), and
[runtime USB](../firmware/baccable/features/usb_modes.c).

Further technical references are the [menu guide](../docs/architecture/MENU_UX.md),
[catalog audit](../docs/architecture/CATALOG_AUDIT.md) and
[action plan](../docs/ACTION_PLAN.md). Some older technical-guide examples retain
historical labels; the labels and order in this user guide follow the inspected
beta-13 firmware. Documentation review does not establish physical vehicle
acceptance for every function, ECU version or accessory.

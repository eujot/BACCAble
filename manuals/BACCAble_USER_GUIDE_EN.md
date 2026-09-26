# BACCAble — in-car user guide

This guide describes the dashboard menu and vehicle functions in **v5-beta-13**
(release source `294818a`). It starts after BACCAble has been installed and
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
- [Favorites and reading layout](#favorites-and-reading-layout)
- [Settings: Features](#settings-features)
- [Function workflows](#function-workflows)
- [Actions](#actions)
- [Information](#information)
- [Reading the measurements](#reading-the-measurements)
- [Gasoline reading catalog](#gasoline-reading-catalog)
- [Diesel reading catalog](#diesel-reading-catalog)
- [USB CAN capture and diagnostics](#usb-can-capture-and-diagnostics)
- [Troubleshooting](#troubleshooting)
- [Changes from the old manual](#changes-from-the-old-manual)
- [Sources and scope of verification](#sources-and-scope-of-verification)

## First use

1. With the car stationary, switch on the ignition so the instrument panel and
   steering-wheel controls are awake. Start the engine when a function requires
   it. Some readings are only available with the engine running.
2. Switch **both cruise control (CC) and adaptive cruise control (ACC) off**.
   Release the steering-wheel buttons before operating the menu.
3. Hold **RES for about 1.2 seconds**, then release it. The menu opens on the last
   favorite. An empty list displays `No favorites`.
4. Hold RES again to return to the main menu. Use gentle up/down presses to find
   `Settings`, short RES to enter, then enter `Features`.
5. Find `Engine` and select `2.0 I4`, `2.9 V6` or `2.2 D` to match the engine.
   This chooses the reading catalog; it does not reconfigure the engine ECU.
   Existing gasoline installations may initially use I4, so check this on a V6.
6. Return with a long RES to save accepted settings. Open `Readings` and choose
   a group, or prepare your own [favorites](#favorites-and-reading-layout).

C1 handles the powertrain bus and dashboard menu, C2 the chassis bus, and BH the
body bus. Check their version pages under `Information` if a function involving
another controller does not respond.

Configure the menu while stationary. Functions that alter braking, traction or
drivetrain behavior require a controlled test environment and suitable vehicle
equipment. Follow the project's [use restrictions](../README.md#disclaimer).

## Controls and navigation

The controls are the cruise-control controls on the steering wheel. The ACC
distance button, where present, has the same menu role as RES.

| Control | What it does |
| --- | --- |
| Hold RES for about 1.2 s with the menu closed | Open the last favorite. |
| Short RES, then release | Enter, toggle, select or confirm the displayed item. Ordinary readings and status pages do not change on a short press. |
| Hold RES for about 1.2 s in the menu | Go back one level. From the main menu, close the menu. In a numeric edit, cancel the unaccepted number. |
| Gentle down / up | Next / previous item. Lists wrap around. |
| Hold gentle down / up | Repeat in browsing and settings lists, starting after about 0.5 s. Actions and a favorite being moved do not auto-repeat. |
| Stronger down / up | Jump to the next / previous functional group in reading lists, page editors, Features and Actions. In Favorites, move one page. In a numeric edit, adjust the number. |

A stronger press can first register a gentle step before the group jump. It no
longer means “skip ten pages.” Releasing RES after a long press does not also
select the next screen.

If control reports are interrupted for more than 0.3 s, release the control and
press again. When CC or ACC takes ownership of the buttons, they are unavailable
for menu navigation.

### Returning, inactivity and saving

Long RES returns through the hierarchy, for example:

`oil reading → Temperatures group → main menu → menu closed`.

Ordinary navigation and Information return to Favorites after about **30 s**
without input. Settings and editors return after about **60 s**. Live readings
and Favorites remain displayed. Active fault operations postpone this timeout.
An unfinished numeric draft is cancelled on timeout; already accepted settings
are saved when leaving configuration.

There is **no Save & Exit item**. Return from configuration normally to save
accepted changes. Some functions react as soon as a value is changed; USB mode
is applied when leaving Features. Cancelling a numeric draft does not undo other
settings already accepted.

If `× Save failed: RES` appears, short RES retries the pending save/exit. Long RES
cancels that exit and returns to the current view. The accepted changes remain in
memory, but are not guaranteed to survive a restart. Navigation is otherwise
blocked while this message is displayed; waiting does not retry the save.

## Menu structure

```text
Main menu
├─ Favorites                 Your selected live pages
├─ Readings
│  ├─ All readings
│  ├─ Engine
│  ├─ Temperatures
│  ├─ Battery
│  ├─ DPF / AdBlue
│  ├─ Performance
│  └─ Other
├─ Actions                   Run a function or diagnostic operation
├─ Settings
│  ├─ Features               Device and vehicle-function settings
│  │  └─ Park mirror
│  │     ├─ Enabled
│  │     ├─ Store position
│  │     └─ < Back
│  ├─ Favorites              Add/remove pages from Favorites
│  ├─ Shown pages            Show/hide pages in Readings
│  ├─ Favorite order         Arrange Favorites
│  └─ Sort order             Grouped or alphabetical reading lists
└─ Information               Versions, configuration and input status
```

The main menu order is fixed. Page availability follows the selected engine,
`Advanced pages` and `Shown pages`. A group without eligible pages may show
`No pages`. The complete catalogs below include pages that are hidden by default.

## Display symbols and messages

The instrument panel has either 18 or 24 text positions, depending on the
firmware build. Labels and live screens can differ to make room for values.
For example, `Batt charge BCM` opens a `Batt BCM SOC` reading.

| Mark or message | Meaning and response |
| --- | --- |
| `O` / `Ø` before an option | Disabled / enabled, or not selected / selected in a membership list. Short RES changes it. |
| `ON` / `OFF` in a status | Reported or locally tracked state. This is not necessarily an editable checkbox. |
| `>` | Enter a submenu or start an action/workflow with RES. |
| `*` | A number is being edited, or a favorite is selected for moving. |
| `< Back` | Return from the submenu. Long RES also goes back. |
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

## Favorites and reading layout

### Add useful pages

1. Open `Settings → Favorites`.
2. Browse to a page and press RES. `Ø` means it is in Favorites; press again to
   remove it.
3. Select up to **six pages** for the current fuel catalog. Remove an existing
   favorite before adding another when the list is full.
4. Return with long RES, then open the main-menu `Favorites` to see live values.

Gasoline and diesel have separate saved favorite lists. I4 and V6 share the
gasoline list. An incompatible favorite is temporarily hidden when the engine
profile changes, but still occupies its saved slot. Select the original profile
to remove it if necessary.

### Arrange, hide and sort

| Setting | Operation and effect |
| --- | --- |
| `Favorite order` | Browse to an item, short RES to select it (`*`), move it with up/down, then short RES to finish. Movement stops at the list ends. Long RES leaves the editor and saves the resulting order, including moves already made. |
| `Shown pages` | Short RES shows/hides a page in Readings. Hiding it does not remove it from Favorites. |
| `Sort order` | Short RES switches between functional grouping and A–Z by page label. This affects catalog lists and page editors, not the custom order of Favorites. |
| `Advanced pages` in Features | Reveals technical and alternative layouts in ordinary browsing. Explicit favorites and page editors can include advanced pages even while this is off. Engine compatibility still applies. |
| `Auto rotate` in Features | Advances through the current live list every five seconds. A short favorite list makes a useful rotating dashboard. |

Visibility and remembered reading pages are saved separately for gasoline and
diesel; sort order is shared. An automatically displayed performance result may
temporarily show a hidden page without changing your saved visibility setting.

With no saved preferences, gasoline starts with Oil/coolant temp, Power / torque,
Battery V / A and Speed; diesel starts with Oil/coolant temp, Power / torque,
Battery V / A and DPF load / temp. Existing saved preferences take priority.

## Settings: Features

Open `Settings → Features`. The tables follow the actual browsing order.
The group headings below explain stronger up/down jumps; they are not extra
submenus. For an on/off setting, short RES toggles `O`/`Ø`. For a named choice,
short RES cycles its values.

For **Launch Nm**, **Shift RPM** and **Pedal trim**, short RES starts an edit (`*`),
up/down adjusts the draft, and short RES accepts it. Long RES cancels the draft.

### Display group

| Option | Values | Purpose |
| --- | --- | --- |
| `LED strip` | Off / on | Drive an installed compatible WS281x LED strip using vehicle data such as accelerator position and gear. This requires the optional hardware. Output pauses while USB owns the shared pin. |
| `Shift light` | Off / on | Request the instrument-panel shift warning above `Shift RPM`, with compatible Race-mode display support. This does not shift the gearbox. |
| `Shift RPM` | 1500–6000 RPM, step 250 | Engine-speed threshold for the shift warning. |
| `MY23 display` | Off / on | Select the newer instrument-panel behavior used by supported display features. It does not change the compiled 18/24-character width. |
| `DPF regen alert` | Off / on | Enable diesel regeneration notifications. See [DPF monitoring](#dpf-monitoring). |
| `Auto rotate` | Off / on | Rotate live pages every five seconds. |
| `Advanced pages` | Off / on | Include technical/secondary pages in Readings, subject to profile and manual visibility. |

### Comfort group

| Option | Values | Purpose |
| --- | --- | --- |
| `Seatbelt alarm` | Off / on | Request the seatbelt acoustic-warning configuration. This is distinct from the read-only `Seatbelt alarm` page. Check the resulting vehicle setting; do not assume removing BACCAble reverses ECU configuration. |
| `Stop odo blink` | Off / on | Suppress the blinking odometer display. It does not repair the configuration mismatch that caused the blink. |
| `Park mirror` | Submenu | Enable reverse mirror positioning and explicitly store the target positions. See the [mirror procedure](#parking-mirrors). |
| `Close windows` | OFF / 1 lock / 2 locks | Choose the key-fob lock gesture that starts window closing. Narrow screens use `Close win`. |
| `Open windows` | OFF / 1 unlock / 2 unlocks | Choose the key-fob unlock gesture that starts window opening. Narrow screens use `Open win`. |
| `QV exhaust` | Off / on | Permit the exhaust-valve request action and the gear-lever release-button gesture on compatible hardware. |
| `Auto PDC mute` | Off / on | Temporarily mute active parking-sensor beeps while braking outside Reverse, then restore the change BACCAble made when braking is released or Reverse is selected. Requires current brake, gear and PDC reports. |
| `Mute audio in R` | Off / on | Mute audio in Reverse and restore audio afterwards if BACCAble muted it. Previously muted audio and a manual unmute are respected. This is independent of PDC mute. |

### Assistance group

| Option | Values | Purpose |
| --- | --- | --- |
| `Virtual ACC` | Off / on | Use a compatible standard cruise-control pad for ACC functions. It requires an ACC-capable, correctly configured vehicle; it does not add radar or ACC capability. |
| `ACC resume` | OFF / RES / + | Select whether BACCAble sends automatic RES or gentle-up requests while ACC is engaged and holding a stationary car. This can allow the car to resume movement; see [assistance controls](#acc-and-has-controls). |
| `Virtual HAS` | Off / on | Permit simulated Highway Assist button presses through the action or a double press of the lane button. Requires compatible, configured vehicle assistance equipment. |

### Drivetrain group

| Option | Values | Purpose |
| --- | --- | --- |
| `Launch Nm` | 25–600 Nm, step 25 | Reported engine torque at which launch assist requests release of the forced front brakes. It is not a torque limit or a tuning setting. |
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
| `Engine profile` (shown as `Engine`) | 2.0 I4 / 2.9 V6 / 2.2 D | Select the engine-specific reading catalog. Switching discards cached measurements and temporarily filters incompatible favorites. |
| `USB mode` | OFF / CAN / ELM327 | Select normal operation, binary CAN capture, or ELM-compatible diagnostics. ELM327 is offered in builds with that support. Modes are exclusive. Leave Features to apply. |

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
2. Set `Enabled` to `Ø`. Enabling does **not** store the current positions.
3. Enter `Store position`. The prompt is `> Adjust; RES=save`.
4. Use the car's mirror controls to place both mirrors at the desired reversing
   positions. Let them stop moving, then short RES to request storing them.
5. `Store: queued` means that the request was queued to BH. There is no storage
   acknowledgement from BH on this screen. Short RES dismisses the result;
   long RES goes back. If sending was busy, retry while the desired positions
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
  `Brake+launch? RES`. Launch assist requests release when reported torque
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
Select the action again and confirm `Stop AWD req? RES` to stop requesting it.
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
confirmation; press RES again **within three seconds** to execute. Moving away
or returning cancels the confirmation. Conditions are checked again when the
action executes.

| Action | Permission / conditions | Effect |
| --- | --- | --- |
| `QV exhaust req` | QV exhaust; compatible hardware | Request valve open, or release to factory control. WAIT is not a valve-position acknowledgement. |
| `Press HAS button` | Virtual HAS | Send the simulated HAS button press. `HAS: Request sent` does not mean HAS engaged. |
| `Toggle ESC/TC` | ESC/TC control; Dyno inactive | Toggle the custom stability/traction-control request. Confirmation can remain unavailable. |
| `Toggle Dyno mode` | Dyno action; car stationary long enough; no brake override or inverted ESC state | Request Dyno on/off. Enabling asks `Dyno+ESC? RES`. A C2 reply ends the pending board request; verify vehicle state. |
| `Brake override` | Brake action; launch assist disarmed before changing an existing override; stationary and Dyno active to enable | Request forced front brakes plus launch assist, or request normal braking. See the release procedure above. |
| `Disable launch` | Brake action; launch assist currently armed | Stop torque-triggered brake release. Does not release brake override. |
| `AWD off request` | AWD off action; stationary to begin | Start repeated AWD-off requests, or explicitly cancel the sequence. No measured drivetrain confirmation. |
| `Read BCM faults` | BCM fault reader; no DTC clear running | Open the BCM fault-code browser immediately. Reading does not erase codes. |
| `Clear DTCs` | DTC clear action; no conflicting BCM read/clear | Run a clear request across the configured controllers on the vehicle buses. It is broader than the BCM reader and does not repair the fault. |
| `Reset best times` | No feature permission required | Clear the saved best acceleration records. Confirmation is required; a completed save reports `Records cleared`. |
| `Peak hold` | No feature permission required | Immediately toggle numerical maximum hold for the current reading page. |
| `IBS SOC override` | Running engine; explicit confirmation | Toggle a temporary experimental SOC-message override. See the limitations below. |

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
4. Short RES starts another read; long RES leaves and cancels an unfinished read.

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

These pages are read-only. Gentle up/down moves through them; short RES does not
toggle the displayed state.

| Page, in order | Meaning |
| --- | --- |
| C1 version | Firmware version of the controller managing the menu. |
| C2 version | Chassis-controller version. `? C2 no reply` means no recent version reply. |
| BH version | Body-controller version. `? BH no reply` means no recent version reply. |
| `MY23:ON/OFF …ch` | MY23 preference and compiled display width, 18 or 24 characters. |
| `Immobilizer: ON/OFF` | Stored BACCAble immobilizer preference, not an editable menu switch. |
| `Reports:` | Number of steering-wheel input reports observed by the menu. Useful when diagnosing unresponsive controls. |
| `Gaps:` | Count of interruptions detected in the input report stream. |
| `Max gap:…ms` | Longest observed input-report interval in milliseconds. |
| `Input age:…ms` | Time since the last input report; `--` if none is available. |

C2/BH version replies older than about five seconds are treated as stale. Input
counters describe menu controls, **not CAN capture frame counts**. Special builds
with menu diagnostics may add an `IPC diag` service page; this is not part of the
ordinary release menu.

## Reading the measurements

Select `Readings`, a group, and a page. The live screen may use a shorter caption
than its label in Favorites/Shown pages. All readings below are included in the
catalog, but unsupported ECU requests can remain `--`.

- **Power** is calculated from reported torque and RPM and expressed in metric
  horsepower (`PS`, called CV in the old manual). It is not a dyno measurement.
- **Oil volume, level, quality, pressure and temperature are different values.**
  Quality is an ECU-reported estimate, not a laboratory assessment. Observe the
  vehicle manufacturer's conditions for a valid oil-level measurement.
- **Ignition cyl 1–6** reports cylinder ignition correction in degrees. It is not
  a misfire count. V6 misfire pages cover only cylinders 1–4 because the catalog
  has no verified individual cylinder 5/6 misfire definitions.
- **Distance (ECU)** is the diagnostic distance value described in the old manual
  as distance since ECU odometer reset; do not equate it with the cluster's total
  lifetime odometer without validating that ECU's interpretation.
- **Gear** uses N, R or a gear number; DNA uses A, N, D or R. Missing states can
  show `--`/`?`. `Pedal map` uses the external controller's map response.
- Combined-page values are listed **left to right** in the tables. `O` means oil,
  `W` water/coolant, `I`/`In` intercooler inlet and `X`/`Out` intercooler outlet on
  the dense temperature layout. All temperatures in that layout use Celsius.

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

### Catalog notation

**A** in the last column means advanced: ordinarily hidden until `Advanced pages`
is enabled. **—** means ordinary browsing, subject to manual visibility and
engine compatibility. Group abbreviations are **Eng** = Engine, **Temp** =
Temperatures, **Batt** = Battery, **DPF** = DPF / AdBlue, **Perf** = Performance,
**Other** = Other. Both catalogs are also accessible through All readings.

## Gasoline reading catalog

These are all **64 gasoline pages**. Unless stated otherwise, they apply to both
2.0 I4 and 2.9 V6. The table follows catalog order; grouped or A–Z navigation can
present them in a different order.

| Page label | Group | Unit(s) and meaning | A |
| --- | --- | --- | --- |
| Power / torque | Perf | PS / Nm; calculated power and reported engine torque. | — |
| Oil bar/coolant | Temp | bar / °C; native oil pressure and ECU coolant temperature. | A |
| Oil bar / temp | Temp | bar / °C; native oil pressure and native oil temperature. | A |
| Oil/coolant temp | Temp | °C / °C; native oil temperature and ECU coolant temperature. | — |
| Oil level/qual. | Eng | L / %; ECU oil volume and oil-quality estimate. | A |
| Batt charge / A | Batt | % / A; engine ECU SOC and signed battery current. | A |
| Battery V / A | Batt | V / A; BCM voltage and signed battery current. | — |
| Power | Perf | PS; power calculated from torque and RPM. | — |
| Torque | Perf | Nm; reported engine torque. | — |
| Intercooler out | Temp | °C; intercooler outlet air temperature. | — |
| Intercooler in | Temp | °C; intercooler inlet air temperature. | — |
| Intake abs press | Eng | bar; absolute intake pressure, including atmospheric pressure. | A |
| Boost pressure | Eng | bar; intake absolute pressure minus a fixed 1 bar reference. It can be negative. | — |
| Turbo sensor V | Eng | V; turbo/intake pressure-sensor signal voltage, not boost pressure. | A |
| Distance (ECU) | Perf | km; ECU diagnostic distance counter. | A |
| Oil volume | Eng | L; ECU-reported oil volume. | — |
| Oil pressure | Eng | bar; ECU diagnostic oil pressure; a different source from native combined oil-pressure pages. | — |
| Oil temp (ECU) | Temp | °C; ECU diagnostic oil temperature. | — |
| Oil quality | Eng | %; ECU oil-quality estimate. | A |
| MultiAir temp | Temp | °C; MultiAir module oil temperature. **I4 only.** | — |
| Gearbox temp | Temp | °C; transmission temperature. | — |
| Batt charge BCM | Batt | %; BCM battery SOC; live caption `Batt BCM SOC`. | — |
| Battery current | Batt | A; signed native battery current. | — |
| Battery voltage | Batt | V; BCM diagnostic battery voltage. | — |
| A/C pressure | Other | bar; air-conditioning refrigerant pressure. | A |
| Gear | Eng | N / R / number; reported current gear. | — |
| Engine run time | Eng | min; reported time since engine start. | A |
| Over-rev time | Eng | s; ECU-reported duration in an over-rev condition. | A |
| Over-rev count | Eng | count; ECU-reported over-rev occurrences. | A |
| Exhaust temp | Temp | °C; ECU exhaust-gas temperature. | — |
| Catalyst temp | Temp | °C; catalytic-converter temperature. | — |
| Coolant temp | Temp | °C; engine coolant temperature. | — |
| Knock sensor | Eng | mV; knock-sensor signal, not a count of knocking events. | A |
| Key ID | Other | Numeric diagnostic key identifier; not a complete hexadecimal key identity. | A |
| Ignition cyl 1 | Eng | °; cylinder 1 ignition correction. | — |
| Ignition cyl 2 | Eng | °; cylinder 2 ignition correction. | — |
| Ignition cyl 3 | Eng | °; cylinder 3 ignition correction. | — |
| Ignition cyl 4 | Eng | °; cylinder 4 ignition correction. | — |
| DNA mode | Other | A / N / D / R; reported drive-mode selection. | — |
| Speed | Perf | km/h; reported vehicle speed. | — |
| Seatbelt alarm | Other | ON / OFF; reported acoustic-warning configuration. | A |
| 0-100 km/h | Perf | s, RUN or MISS; current/latest 0–100 attempt. | — |
| 100-200 km/h | Perf | s, RUN or MISS; current/latest 100–200 attempt. | — |
| Best 0-100 | Perf | s; saved best 0–100 time. | — |
| Best 100-200 | Perf | s; saved best 100–200 time. | — |
| Pedal map | Other | B / A / N / D / R; pedal-controller reply, or unavailable marker. | — |
| Ignition cyl 5 | Eng | °; cylinder 5 ignition correction. **V6 only.** | — |
| Ignition cyl 6 | Eng | °; cylinder 6 ignition correction. **V6 only.** | — |
| Oil height | Eng | mm; ECU oil-level height. | A |
| Misfires total | Eng | count; ECU-reported total misfires. | — |
| Misfires cyl 1 | Eng | count; cylinder 1 misfires. | A |
| Misfires cyl 2 | Eng | count; cylinder 2 misfires. | A |
| Misfires cyl 3 | Eng | count; cylinder 3 misfires. | A |
| Misfires cyl 4 | Eng | count; cylinder 4 misfires. | A |
| Misfires cyl 1-4 | Eng | Four counts, for cylinders 1, 2, 3 and 4. V6 coverage is partial. | — |
| Oil L / mm / % | Eng | L / mm / %; oil volume, height and quality. | A |
| Oil/water/IC C | Temp | °C × 4; ECU oil, coolant, intercooler inlet and outlet temperatures. | A |
| Oil/water/gear C | Temp | °C × 3; ECU oil, coolant and gearbox temperatures. | A |
| Battery sources | Batt | % / %; native IBS SOC and engine ECU SOC. | A |
| Battery IBS raw | Batt | Two raw decimal bytes, IBS byte 0 then byte 1; no physical unit. | A |
| IBS raw0/SOC/V | Batt | Raw byte 0 / % / V; IBS raw data, BCM SOC and BCM voltage. | A |
| IBS raw1/SOC/V | Batt | Raw byte 1 / % / V; IBS raw data, BCM SOC and BCM voltage. | A |
| Oil volume/mm | Eng | L / mm; oil volume and oil-level height. | A |
| Misfires/MA temp | Eng | count / °C; total misfires and MultiAir oil temperature. **I4 only.** | A |

## Diesel reading catalog

These are all **60 diesel pages** for the 2.2 D profile. AdBlue readings additionally
require an SCR-equipped vehicle. Some technical diagnostic requests may not be
supported by an individual ECU version.

| Page label | Group | Unit(s) and meaning | A |
| --- | --- | --- | --- |
| Power / torque | Perf | PS / Nm; calculated power and reported engine torque. | — |
| Oil bar/coolant | Temp | bar / °C; native oil pressure and ECU coolant temperature. | A |
| Oil bar / temp | Temp | bar / °C; native oil pressure and native oil temperature. | A |
| Oil/coolant temp | Temp | °C / °C; native oil temperature and ECU coolant temperature. | — |
| Oil level/qual. | Eng | mm / %; oil-level height and quality estimate. Unlike gasoline, the first value is not liters. | A |
| Batt charge / A | Batt | % / A; engine ECU SOC and signed battery current. | A |
| Battery V / A | Batt | V / A; diagnostic voltage and signed battery current. | — |
| DPF load / temp | DPF | % / °C; estimated filter loading and DPF temperature. | — |
| DPF regen / temp | DPF | % / °C; regeneration progress and DPF temperature. | A |
| Power | Perf | PS; power calculated from torque and RPM. | — |
| Torque | Perf | Nm; reported engine torque. | — |
| DPF load | DPF | %; estimated particulate-filter loading/occlusion. | — |
| DPF temp | DPF | °C; particulate-filter temperature. | — |
| DPF regen % | DPF | %; regeneration progress, not filter loading. | — |
| DPF regen mode | DPF | Decoded regeneration/aftertreatment phase; see DPF monitoring. | — |
| Since DPF regen | DPF | km; distance since the last regeneration. | — |
| DPF regen count | DPF | count; number of regenerations reported by the ECU. | A |
| DPF avg interval | DPF | km; ECU average distance between regenerations. | A |
| DPF avg duration | DPF | min; ECU average regeneration duration. | A |
| Battery voltage | Batt | V; diesel diagnostic battery voltage. | — |
| Battery charge | Batt | %; native IBS battery SOC. | — |
| Battery current | Batt | A; signed native battery current. | — |
| Oil quality | Eng | %; ECU oil-quality estimate. | A |
| Oil temp | Temp | °C; native engine oil temperature. | — |
| Oil pressure | Eng | bar; native engine oil pressure. | — |
| Oil level | Eng | mm; ECU oil-level height. | — |
| AdBlue volume | DPF | L; AdBlue quantity. **SCR equipment required.** | — |
| AdBlue level | DPF | %; AdBlue tank level. **SCR equipment required.** | — |
| Gearbox temp | Temp | °C; transmission temperature. | — |
| Exhaust temp | Temp | °C; exhaust-gas temperature. | — |
| Gear | Eng | N / R / number; reported current gear. | — |
| Coolant temp | Temp | °C; engine coolant temperature. | — |
| EGR target | Eng | %; requested exhaust-gas-recirculation value. | A |
| EGR actual | Eng | %; reported actual EGR value. | A |
| Turbo target bar | Eng | bar; ECU target turbo pressure. | A |
| Turbo target % | Eng | %; ECU target turbo-control value. | A |
| Turbo temp | Temp | °C; temperature reported by the turbo-related diagnostic reading. | — |
| Turbo actual bar | Eng | bar; ECU actual turbo pressure. | — |
| Turbo actual % | Eng | %; ECU actual turbo-control value. | A |
| Boost target | Eng | bar; separate ECU boost-pressure target reading. | — |
| Intake sensor V | Eng | V; intake-sensor signal voltage. | A |
| Fuel pressure | Eng | bar; fuel-system pressure. | — |
| Fuel temp | Temp | °C; fuel temperature. | — |
| Distance (ECU) | Perf | km; ECU diagnostic distance counter. | A |
| A/C pressure | Other | bar; air-conditioning refrigerant pressure. | A |
| Fuel rate | Eng | L/h; instantaneous fuel consumption by volume per hour, not L/100 km. | — |
| Intake air temp | Temp | °C; intake-air temperature. | — |
| Speed | Perf | km/h; reported vehicle speed. | — |
| Seatbelt alarm | Other | ON / OFF; reported acoustic-warning configuration. | A |
| 0-100 km/h | Perf | s, RUN or MISS; current/latest 0–100 attempt. | — |
| 100-200 km/h | Perf | s, RUN or MISS; current/latest 100–200 attempt. | — |
| Best 0-100 | Perf | s; saved best 0–100 time. | — |
| Best 100-200 | Perf | s; saved best 100–200 time. | — |
| DNA mode | Other | A / N / D / R; reported drive-mode selection. | — |
| Pedal map | Other | B / A / N / D / R; pedal-controller reply, or unavailable marker. | — |
| Battery sources | Batt | % / %; native IBS SOC and engine ECU SOC. | A |
| Battery IBS raw | Batt | Two raw decimal bytes, IBS byte 0 then byte 1; no physical unit. | A |
| IBS raw0/SOC/V | Batt | Raw byte 0 / % / V; IBS raw data and BCM SOC/voltage definitions, potentially unsupported on diesel. | A |
| IBS raw1/SOC/V | Batt | Raw byte 1 / % / V; IBS raw data and BCM SOC/voltage definitions, potentially unsupported on diesel. | A |
| Oil/water/gear C | Temp | °C × 3; native oil, ECU coolant and gearbox temperatures. | A |

## USB CAN capture and diagnostics

USB is optional for normal menu use. Runtime USB modes are different from the
STM32 **DFU bootloader** used for flashing: seeing a device in `dfu-util --list`
does not mean a serial capture port should exist in DFU.

### CAN capture

1. Connect the USB cable(s) for the buses to record to the computer. A powered
   vehicle supplies the CAN traffic. A working USB data hub can be used. Connect
   the cables before activating the mode; eject any mounted BACCAble USB disk
   before changing its role.
2. Select `Settings → Features → USB mode: CAN`, then leave Features. C1 sends
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

### ELM-compatible diagnostics

Select `USB mode: ELM327`, leave Features, then connect a compatible host
diagnostic client to C1's new serial port. This is a bounded ELM-style CAN
implementation, not a full ELM327 with every protocol. Unsupported commands can
return `?`; unavailable diagnostic data returns `NO DATA`.

Normal vehicle-feature processing is suspended during ELM diagnostics, so the
menu may not remain navigable. Disconnect USB to let the session expire after
about ten seconds; 120 seconds without a complete ELM command also ends it.
See [USB diagnostics](../docs/architecture/USB_DIAGNOSTICS.md) for supported
commands and bus behavior. Use USB mode OFF for ordinary operation when finished.

## Troubleshooting

| Symptom | Check / next step |
| --- | --- |
| RES does not open the menu | Ignition/panel awake; CC and ACC both off; release controls, then hold RES for 1.2 s. Check C1 connection/firmware. ELM diagnostics can temporarily own processing. |
| Menu skips or stops reacting | Distinguish gentle from strong presses. Release after a stream interruption. Inspect Information's Reports/Gaps/Input age and C2/BH status. |
| A page or action is missing | Correct engine profile; Shown pages; Advanced pages; action permission in Features. A hidden favorite may still occupy its saved slot under another profile. |
| `No favorites` / `No pages` | Add compatible favorites or restore page visibility. Long RES still returns. |
| A value shows `--` | Wait for fresh reports, check ignition/engine conditions and ECU compatibility. USB diagnostics can suspend normal queries. A displayed catalog entry is not a guarantee that the ECU implements it. |
| `Batt BCM SOC --` or `Batt charge --%` | Compare the independent sources using Battery sources. Do not treat missing SOC as 0%, and do not use IBS override as a repair. |
| Setting reverts after a restart | Leave configuration to save. Resolve `× Save failed: RES`; unsaved RAM changes are not persistent. |
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

| Older wording or behavior | Current equivalent |
| --- | --- |
| Short RES returns to the main menu | Short RES selects; **long RES goes back**. |
| Strong press skips ten parameters | Strong press jumps functional groups; Favorites moves one page. |
| Save & Exit | Automatic save when leaving configuration; explicit retry on save failure. |
| Flat parameter/function menu | Favorites, Readings, Actions, Settings, Information. |
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

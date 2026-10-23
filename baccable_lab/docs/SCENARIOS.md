# Guided capture scenarios

The scenario is the primary workflow of the capture screen. Lab runs one
controlled in-car procedure at a time, shows the current step large and
prominent, and advances automatically to the next procedure. Progress is
persisted, so starting Lab again continues from the first unfinished step.

The **on-screen wording is Polish**, using the terms a Giulia driver sees in the
car (for example `WYKONAJ:`, `oczekiwany stan`, `postój`, `jazda – plac`,
`jazda – droga`). The stable machine identifiers stay English: scenario `id`,
step `key`, and the canonical `expect` label used by structured markers,
correlation and the CAN dictionary. Polish display text for the canonical labels
lives in `TITLE_BY_LABEL_PL` (`baccable_lab/events/catalog.py`) and is rendered by
`label_text_pl`.

Markers alone never decode CAN. A scenario improves the *quality* of the ground
truth; the evidence rules in [SESSION_ANALYSIS.md](SESSION_ANALYSIS.md) still
apply before a field can be named.

## Screen and controls

The dashboard shows only what matters while running: a boxed `WYKONAJ:` step, the
expected state, the repetition counter, the confounder guard and the controls.
The manual marker palette (quick keys, groups, find, custom) is deliberately
removed from the live screen so it cannot distract from or contaminate the
procedure.

| Key | Action |
| --- | --- |
| `ENTER`, `SPACJA` or `y` | Step performed — mark as **zrobione** |
| `x` | Step could not be performed — mark as **nieudane** |
| `k` | **Pomiń** this step — you deliberately do not perform it (no marker is written) |
| `o` | **Pomiń scenariusz** — skip every remaining step of the current procedure |
| `e` | **Wyklucz** — skip the rest and stop scheduling this procedure permanently |
| `p` | **Zatrzymaj** (pause) the scenario; recording continues |
| `r` | **Wznów** the paused scenario at the same step |
| `u` | **Cofnij** the last confirmation of this run |
| `n` | Add an operator **notatka** to the last confirmed step (e.g. "silnik zgasł") |
| `s` | Choose a scenario to jump to (by number or id) |
| `q` | Finish the session — also works during a scenario; `Ctrl-C` does the same |

A stopped scenario keeps its position. A short bell rings on a confirmation or a
new step, so a confirmation can be heard without looking at the screen. The last
confirmation stays undoable with `u` — including a skipped step.

### Skipping work you will not perform

A step you do not want to perform (for example provoking ABS, ESC or traction
control) is marked **pominięte** with `k`. It advances the procedure but writes
**no timeline marker**: a skipped step must never look like evidence that the
physical action happened. Only its structured `scenario_steps` row (with
`outcome = "skipped"`) is stored, so the analysis sees that the step existed and
was skipped on purpose rather than missed.

`o` skips every remaining confirmation of the current procedure (all steps and
all repetitions) in one keystroke. `e` does the same **and** retires the procedure
permanently: it is stored in the excluded list of `config/scenario_state.json`, is
never scheduled again and is left out of the queue until you re-enable it. This is
the quick way, inside the car, to say "I will not provoke ABS or traction
control". The same decision can be made from a terminal:

```sh
baccable scenarios --exclude abs_intervention_closed_area   # never schedule it
baccable scenarios --exclude esc_intervention_closed_area --exclude traction_control_closed_area
baccable scenarios --include abs_intervention_closed_area   # re-enable it
```

Skipping is not the same as failing: `x` says the action was expected but did not
work (useful evidence), while `k`/`o`/`e` say the action was intentionally not
attempted and carries no signal evidence.

### Colour and readability

The screen uses ANSI colour and a progress bar so it can be read at a glance from
inside the car: the current action on a bright highlighted bar, the expected
state in magenta, the section and step/repetition counter in cyan, the
confirmation message in green (done), red (failed/empty) or yellow (stopped), and
the bus lines in green with no loss or red once loss is reported. A
`postęp: handled/total` bar shows progress within the running procedure, and a
`dalej:` line previews the next step so the driver can prepare.

Options: `--caps` shows the action in upper case, `--no-color` (or the `NO_COLOR`
environment variable) disables colour, and `--no-bell` disables the bell. These
work for both `capture` and `preview`.

## Sequenced, resumable run

- Scenarios run in a **functional order**: all stationary work first (access,
  lighting, wipers, climate, media, chassis, brakes, drivetrain, parking), then
  the **parking-lot driving** procedures (`drive`), then the on-road assistance
  procedures (`adas`). This avoids jumping, for example, from a gear test
  straight to opening the boot.
- When a scenario finishes, Lab **advances** to the next unfinished one
  automatically.
- Each confirmation is stored immediately. Progress lives in
  `config/scenario_state.json` (override with `--state PATH`). Starting Lab
  again resumes at the first unfinished scenario and step; a scenario already
  treated as complete is skipped. `baccable scenarios` marks each procedure:
  `[x]` fully done, `[~]` finished but with skipped steps, `[E]` excluded,
  `[ ]` still pending. The table footer reports how many are done, how many
  finished with skips and how many are excluded.

## Commands

```sh
baccable scenarios                       # ordered list with progress
baccable scenarios --json                # machine-readable list
baccable scenarios --exclude ID --include ID   # retire / re-enable procedures
baccable scenarios --scenarios-dir DIR --state FILE
baccable capture --port C1=/dev/cu.usbmodemXXXX      # resumes from progress
baccable capture --port C1=/dev/cu.usbmodemXXXX --scenario drive_brake  # jump to one
baccable capture --port C1=/dev/cu.usbmodemXXXX --context vehicle=Giulia_MY2020
baccable bundle SESSION_ID --output /tmp/bundle.json # one analysis artifact
baccable preview --scenario indicator_left           # practise with no hardware
```

The bundled library lives in `baccable_lab/scenarios/library/*.toml`. Point
`--scenarios-dir` at your own directory to add or override procedures.

## Scenario file format

Files use TOML (`*.toml`), parsed with the Python standard library, so the core
stays dependency-free. One file may hold several `[[scenario]]` tables.

```toml
[[scenario]]
id = "lock_unlock"               # stable machine id
title = "Centralny zamek: zablokowanie i odblokowanie pilotem"   # on-screen (Polish)
group = "access"                 # used for functional ordering
vehicle = "stationary"           # stationary | driving | any
site = "parking"                 # optional, for driving: parking | road
order = 0                        # optional tie-break within a group
runs = 3                         # repeated cycles
min_separation_s = 3             # guidance shown on screen; 0 = no time target
confounder_guard = ["drzwi", "szyby", "zapłon"]
notes = "Dotykać tylko pilota."

[[scenario.steps]]
action = "Naciśnij UNLOCK na pilocie, poczekaj na mignięcie kierunkowskazów"
expect = "unlock"                # must be a catalog marker label (English key)

[[scenario.steps]]
action = "Naciśnij LOCK na pilocie, poczekaj na mignięcie kierunkowskazów"
expect = "lock"
```

`expect` must match a label from the marker catalog in
`baccable_lab/events/catalog.py`; the loader rejects unknown labels. Leave
`expect` empty for a boundary step such as closing the boot that has no catalog
label — the raw (Polish) action text is then stored instead.

Ordering key: vehicle (stationary before driving), then the group order
`access, lighting, wipers, climate, media, chassis, brakes, drivetrain, parking,
drive, adas`, then `order` and `id`.

## Bundled library

82 procedures (527 confirmations). Groups:

| Group | Procedures | Location | Examples |
| --- | --- | --- | --- |
| access | 10 | postój | zamek, drzwi, bagażnik, maska, klapka paliwa, szyby, lusterka, pas |
| lighting | 10 | postój | kierunkowskazy, awaryjne, mijania/drogowe, przeciwmgielne, pozycyjne, wnętrze, stop |
| wipers | 5 | postój/dowolnie | biegi wycieraczek, spryskiwacz, tylna wycieraczka, czujnik deszczu, czujnik jasności i deszczu + stan świateł (AUTO) |
| climate | 8 | postój | klimatyzacja, temperatura, nawiew, odmrażanie, recyrkulacja, podgrzewanie |
| media | 7 | postój (+1 droga) | menu IPC, ekran, źródło audio, utwór, radio, telefon, nawigacja |
| chassis | 6 | postój (+1 droga) | tryb zawieszenia, prześwit, Q4, tryb kierownicy, ruch kierownicy |
| brakes | 7 | postój/plac/droga | pedał hamulca, EPB, auto-hold, ABS/ESC/ASR (plac), kontrola zjazdu |
| drivetrain | 9 | postój | zapłon, silnik, dźwignia P/R/N/D, wsteczny, manual, DNA (D/N/A/Race), Start/Stop |
| parking | 4 | postój | czujniki parkowania, kamera cofania, asystent, ostrzeżenie drzwi |
| **drive** | **9** | **jazda – plac** | ruszenie (D), przyspieszanie, hamowanie, zmiana prędkości, skręcanie, biegi, manewr na wstecznym, arming tempomatu, pełznięcie |
| adas | 7 | postój/droga | tempomat, ACC, asystent pasa (LKA), FCW/AEB, martwe pole, zmęczenie kierowcy |

The `drive` group is designed for a **parking lot / closed area** at low speed,
so the owner can record while driving without needing a public road. `vehicle =
"driving"` procedures are grouped last; on-road stability and collision tests
must never be provoked on a public road.

## Targeted decode tests (DNA modes, lane assist, rain and light sensors)

Three procedures target specific firmware options, using markers in the catalog
(the DNA mode markers `dna_dynamic` / `dna_natural` / `dna_allweather` /
`dna_race`, the lane toggle `lane_assist_disabled` / `lane_assist_enabled`, and
`wipers_auto` together with the light-sensor pair `light_sensor_dark` /
`light_sensor_bright`):

- `dna_all_modes` (drivetrain, stationary) — visit D, N, A and Race, each with its
  own marker, to confirm the per-mode code on `0x384` / `0x46C` / `0x25A`.
- `lane_assist_toggle` (adas, stationary) — toggle LKA off/on with a double-click on the left stalk button.
- `rain_lights_auto` (wipers, stationary) — records the two sensor signals and
  the current dipped-beam state in three phases, repeated three times: the light
  sensor covered and uncovered with the wipers off, then rain with the wipers in
  AUTO, then three manual wiper cycles.

**The factory car does not switch the dipped beams on because of rain or wiper
operation.** That behaviour is the firmware feature still to be added, so the
rain phases of `rain_lights_auto` confirm that the lamps stay **off**. Those
`dipped_beam_off` confirmations are the negative control: they prove the lamps
were not already on for another reason, which would otherwise look like the
feature working. After the firmware change the same steps become
`dipped_beam_on` and the unchanged procedure is the acceptance test.

### Why `rain_lights_auto` uses this phase order

The phases must not run at the same time, because the light sensor and the rain
sensor share the housing behind the interior mirror:

1. **Light sensor first, wipers OFF.** You force every state explicitly instead
   of relying on the weather: in a dark garage a torch pointed at the sensor
   gives "bright" and switching it off gives "dark"; in daylight an opaque
   cloth gives "dark". With the wipers off an automatic wipe cannot start, so
   the only change is brightness → dipped beam.
2. **Rain sensor second, scene forced bright.** The torch (or daylight) keeps the
   scene bright, so AUTO cannot switch the lamps on because of darkness; the only
   changes are water → wipers and the lamps staying off. This isolates the
   rain/wiper signal.
3. **Manual wipers last, scene still forced bright.** Three manual cycles produce
   the wiper signal without the rain sensor, and the lamps again stay off. This is
   the third future trigger, separated from the rain sensor.

Every phase restores the baseline it started from (wipers off, lamps as the
forced brightness dictates), which is what makes the run repeatable and gives
correlation a clean `before`/`after` window per expected state. The procedure
deliberately omits wiper-speed ramps, gear changes and other actions that would
add markers without adding a distinct CAN transition.

### Daylight and dark-garage variants

The procedure never assumes the ambient light; you always force the state the
step names:

| Step says | Daylight, outdoors | Dark underground garage |
| --- | --- | --- |
| force DAY (bright) | remove the cloth / step out of shade | point a torch at the sensor behind the mirror |
| force NIGHT (dark) | opaque cloth over the sensor | switch the torch off |

The dark-garage variant starts with the lamps already on: the car switches the
dipped beams on at ignition because it is dark. That is the correct night
baseline, not a fault. Keep the torch on the sensor during both rain phases —
otherwise AUTO turns the lamps on for darkness and the `dipped_beam_off` control
proves nothing. Do not cover the sunload sensor on the dashboard; it belongs to
the climate control, not to the lighting.

### No time targets

`rain_lights_auto` sets `min_separation_s = 0`, so the screen shows no "odstęp"
hint and there is no assumed duration for spraying, covering the sensor or
waiting. Each step is confirmed when its effect is actually visible: cover the
sensor → confirm the dark state → confirm the dipped beam; spray → confirm that
the wipers started → confirm that the lamps stayed off. A repetition begins when
the lamps and wipers are back at their baseline, not after a fixed delay. The
same rule should be used when writing new procedures: a step describes a
physical action and its observable result, not a timed script.

Each DNA mode gets a **distinct** marker (not the shared `dna_mode_changed`) so
the per-mode code can be correlated without mixing the four modes.

### How many repetitions make the data reliable

One action needs **≥3 clean ON/OFF cycles in one recording** (what `runs = 3`
already gives) and, for independence, the **same action repeated in a second
session**. Promotion to `validated` also needs a physical confirmation (for
wipers→lights: you watch the lamps).

| Level | Repetitions | What it buys |
| --- | --- | --- |
| one session | 3 cycles (`runs = 3`) | `supported` candidate after counter/confounder filtering |
| two sessions | 3 + 3 cycles | cross-session support and confounder rejection |
| physical check | any | `validated` for wipers→lights |

Do **not** repeat all 82 procedures: run only these three in one session (~10
minutes), then repeat the same three in a second session. 6–9 ON/OFF
observations per action is the practical reliability threshold.

### Controls inside a run

- DNA: end each run on N so the next baseline is known; never touch A or Race.
- Lane: keep indicators, cruise and gear steady; the left-stalk double-click is the only change.
- Wipers: keep the light sensor uncovered during the rain phase so AUTO cannot
  turn the lamps on by itself; the wiper state is the only change. In the light
  phase keep the wipers OFF so the covered sensor cannot start a wipe.

## How the results are used

Each confirmation stores a `scenario_steps` row
(`scenario`, `step_index`, `step_key`, `expect`, `run`, `outcome`) linked to a
timeline marker. `outcome` is `done`, `failed` or `skipped`; only `done`/`failed`
carry a marker, so a skipped step is never used as signal evidence. `baccable
review` includes `scenario_steps` and a per-run `scenario_overview` (done / failed
/ skipped) and treats scenario markers as manual evidence. It also
reports `scenario_candidates`: bits that flipped for the same expected action at
least twice, ranked by repeat count, so a repeated ON/OFF pair becomes a short
list to verify instead of thousands of single-event leads. A field is still only
a candidate until the required repetitions, reversibility and negative controls
indicate a repeatable meaning.

## One artifact for an analysis (bundle)

After a session, `baccable bundle` writes a single self-contained JSON that an
analysis — a person or an AI — can read without re-deriving anything:

```sh
baccable capture --port C1=/dev/cu.usbmodemXXXX \
    --context vehicle=Giulia_MY2020 --context firmware=beta20 \
    --context conditions="stationary, engine cold"           # recorded in the manifest
baccable bundle SESSION_ID --output /tmp/bundle.json
```

The bundle contains: session metadata and input hashes; the **context** you
recorded; capture health (loss, resync, errors); the scenario steps with their
expected states, the **operator note** for each step and the matching **scenario
definition** (`guard`, `runs`, `min_separation_s`, vehicle/site) so the intended
negative control is visible without opening the library; **coverage** (mapped
versus unmapped frames and the highest-volume unmapped identifiers); **ranked
per-action candidates** (bits that flipped for the same expected action at least
twice, with per-run before/after windows); bounded **raw frame excerpts** around
each confirmed step; **structural shape hints** (constant payloads, distinct
counts, variable/stable masks, last-byte CRC-8/SAE-J1850, typical interval); and a
short Markdown `brief`. Options: `--max-frames N`, `--no-excerpts`, `--no-hints`,
`--scenarios-dir DIR`.

This is the recommended single input to hand to the decoding step. Keep the
original recording too: the bundle is derived data, not the source of truth.

## Turning a candidate into a dictionary field

`baccable dictionary` can build and validate a candidate field, and merge a
proposal into a draft dictionary, without editing the curated file by hand:

```sh
baccable dictionary --propose --role C1 --id 0x123 --name brake_state \
    --parts "2,0,2" --unit raw --enum "0=off,1=on"    # prints validated JSON
baccable dictionary --accept proposal.json --output draft.json
```

`--propose` validates the layout against the dictionary schema. `--accept`
merges a proposal into a **draft copy** and never overwrites the curated
`knowledge/giulia.json`; promotion stays a reviewed step, so run
`scripts/attach_dictionary_evidence.py` and `scripts/render_can_dictionary.py`
after copying an accepted entry in.

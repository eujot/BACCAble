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
| `p` | **Zatrzymaj** (pause) the scenario; recording continues |
| `r` | **Wznów** the paused scenario at the same step |
| `u` | **Cofnij** the last confirmation of this run |
| `n` | Add an operator **notatka** to the last confirmed step (e.g. "silnik zgasł") |
| `s` | Choose a scenario to jump to (by number or id) |
| `q` | Finish the session — press `q` twice so it cannot end by accident |

A stopped scenario keeps its position. A short bell rings on a confirmation or a
new step, so a confirmation can be heard without looking at the screen. The last
confirmation stays undoable with `u`.

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
  treated as complete is skipped. `baccable scenarios` marks finished procedures
  with `[x]`.

## Commands

```sh
baccable scenarios                       # ordered list with progress
baccable scenarios --json                # machine-readable list
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
min_separation_s = 3             # guidance, shown on screen
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

79 procedures (473 confirmations). Groups:

| Group | Procedures | Location | Examples |
| --- | --- | --- | --- |
| access | 10 | postój | zamek, drzwi, bagażnik, maska, klapka paliwa, szyby, lusterka, pas |
| lighting | 10 | postój | kierunkowskazy, awaryjne, mijania/drogowe, przeciwmgielne, pozycyjne, wnętrze, stop |
| wipers | 4 | postój/dowolnie | biegi wycieraczek, spryskiwacz, tylna wycieraczka, czujnik deszczu |
| climate | 8 | postój | klimatyzacja, temperatura, nawiew, odmrażanie, recyrkulacja, podgrzewanie |
| media | 7 | postój (+1 droga) | menu IPC, ekran, źródło audio, utwór, radio, telefon, nawigacja |
| chassis | 6 | postój (+1 droga) | tryb zawieszenia, prześwit, Q4, tryb kierownicy, ruch kierownicy |
| brakes | 7 | postój/plac/droga | pedał hamulca, EPB, auto-hold, ABS/ESC/ASR (plac), kontrola zjazdu |
| drivetrain | 8 | postój | zapłon, silnik, dźwignia P/R/N/D, wsteczny, manual, DNA, Start/Stop |
| parking | 4 | postój | czujniki parkowania, kamera cofania, asystent, ostrzeżenie drzwi |
| **drive** | **9** | **jazda – plac** | ruszenie (D), przyspieszanie, hamowanie, zmiana prędkości, skręcanie, biegi, manewr na wstecznym, arming tempomatu, pełznięcie |
| adas | 6 | jazda – droga | tempomat, ACC, asystent pasa, FCW/AEB, martwe pole, zmęczenie kierowcy |

The `drive` group is designed for a **parking lot / closed area** at low speed,
so the owner can record while driving without needing a public road. `vehicle =
"driving"` procedures are grouped last; on-road stability and collision tests
must never be provoked on a public road.

## How the results are used

Each confirmation stores a `scenario_steps` row
(`scenario`, `step_index`, `step_key`, `expect`, `run`, `outcome`) linked to a
timeline marker. `baccable review` includes `scenario_steps` and a per-run
`scenario_overview`, and treats scenario markers as manual evidence. It also
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

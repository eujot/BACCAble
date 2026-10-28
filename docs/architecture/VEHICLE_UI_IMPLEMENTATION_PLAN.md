# Joint implementation specification: vehicle profile, Readings and Favorites

Planning baseline: `origin/master` at `ba9176e`, inspected 2026-10-28.
Status: implementation instructions for later execution; this task changes only
this documentation. Task status remains in [ACTION_PLAN.md](../ACTION_PLAN.md).
This is the single authoritative execution specification. Earlier profile/layout
audits are historical evidence only where they differ from this plan.
The owner requests one integrated delivery with small ordered implementation
steps, and only new or meaningfully modified test cases may be executed.
No implementation or test execution has been performed in this planning task.

## 1. Binding owner decisions and scope

The owner explicitly does not want the old Readings or old Favorites retained.
This supersedes earlier instructions to preserve compound pages, their UI IDs,
old visibility/order and old Favorite layouts. Do not spend time implementing
an old-page-to-new-layout migration or a compatibility catalog of compound pages.

Required result:

- Readings contains individual measurements, organized into useful groups.
- Favorites composes individual measurements into one or two logical rows.
- Fresh defaults are useful and selected automatically; the owner edits from there.
- Existing UI layouts/visibility/order reset once into the new format. Existing
  vehicle/feature settings, statistics and mirror calibration are not reset.
- Existing native/OBD source definitions, DID bytes, scales, offsets and freshness
  behavior remain valid. Resetting the UI does **not** authorize changing these.
- Displays: legacy 18, legacy 24, MY23 16+22; original upstream only proves the
  18/24 software budgets, not physical compatibility of every IPC. Keep this limit
  in the acceptance report. Runtime selection belongs to the vehicle profile.
- No old multi-measurement Readings remain after cutover. Default Favorites are
  new layouts, not old pages hidden behind another name.
- No firmware work, tests modifications, PR or release is performed by this
  planning task. A future agent must read the live Git state before implementing.

## 2. Fixed product contract

### Vehicle profile is part of this delivery

Implement the full vehicle profile in the same task, not merely a temporary
adapter. It owns model, engine, model year, drivetrain, display selection and
installed equipment. Its menu, validation, persistence and eligibility consumers
must all be delivered with the new Readings/Favorites. Fuel is derived from
Engine, not another editable boolean. Existing feature preferences remain
separate. Preserve saved vehicle/feature settings when importing old records;
they are not part of the authorized Favorite reset. The technical contract below
specifies transition rules so existing automatic functions are not accidentally
enabled or silently reclassified as installed equipment.

### Readings and groups

Use this order and English on-screen group names; omit empty/inapplicable groups:

| Group | Contents / boundary |
| --- | --- |
| Oil | Temperature, pressure, level mm/status, level bar, volume, quality where supported |
| Temperatures | Coolant, gearbox, intake/intercooler; oil stays in Oil |
| Engine | RPM, power, torque, boost, ignition/misfires where supported |
| Fuel | Consumption/flow, fuel pressure and supported fuel measurements |
| Battery | Voltage, current, IBS charge and other preferred battery measurements |
| Diesel / DPF | DPF load/temp/state/progress/history; AdBlue only for supported equipment |
| Driving | Speed, gear, DNA and relevant driving states |
| Performance | Last/best acceleration times and run status |
| Diagnostics | Alternative sources and technical/raw values useful for comparison |

One primary group per reading. Do not add duplicated entries merely to populate
multiple groups. Preserve explicitly distinct sources (e.g. native versus ECU
Oil temperature); choose the preferred ordinary entry from existing evidence,
not from an assumed name equivalence. If source equivalence is unknown, retain
explicit source labels in Diagnostics and document which ordinary source is used.
No new UDS measurements are introduced just to fill a group.

MY23 Readings displays one selected measurement: name on line 1, value/unit/status
on line 2. It no longer previews the next reading and must not poll that preview.
Legacy Readings displays the same measurement as a fitted single-line label/value.
Oil level bar is a full-width visual presentation of one measurement, not a
multi-parameter page. Keep its numeric/status counterpart separately selectable.

### Favorites, editing and formatting

- Six sets, two logical rows each, up to four items per row. Empty items are
  explicit; four is a storage ceiling, not a guarantee that four values fit.
- Each item stores a stable new Reading ID and Auto / Full / Medium / Short.
- Each reading has explicit labels, unit, precision, value/enum formatting and
  reserved value width. Do not parse these from old `$...` page strings.
- Shared width-aware renderer uses actual 16/18/22/24 glyph budgets. UTF-8 byte
  length is not glyph count. Define unsupported-symbol fallbacks; preserve units,
  signs, source distinction and precision. Stale/missing is `--`, never zero.
- Join items with at least one space. Fit against reserved numeric/status widths;
  99→100 must not switch styles/positions every update. Auto selects the most
  readable fitting labels once when resolving a layout. Explicit styles do not
  silently change in the current-display editor.
- Reject a draft that cannot fit even with its allowed Auto abbreviations. Offer
  remove/move/shorter format. Do not silently cut values, drop items or scroll.
  Unexpected numeric overflow has a distinct bounded indicator, not clipped digits.
- Oil bar occupies its row exclusively. Same measurement in multiple rows is
  allowed and polled once. Same item twice in a row is rejected with explanation.
- MY23 shows both rows. Legacy navigates 1/A, 1/B, 2/A, etc., skipping empty rows.
  A previously saved wider row becomes deterministic continuation subpages on a
  narrower display if necessary; do not rewrite its stored layout. The editor
  prevents creating an oversized row on the currently selected display.
- Editor permits select measurement/format, add/remove/reorder, move to other row,
  preview and save. Retain HOLD-save, first BACK confirmation, second BACK discard,
  inactivity discard and failed-save rollback. Do not add a conflicting gesture.
- Eligibility comes from vehicle profile. Later profile changes filter temporarily
  unsupported saved items without deleting/replacing the user's layout.
- `Restore defaults` explicitly confirms and replaces Favorite layouts only;
  normal saves and profile changes never reapply defaults.

### Default Favorite sets

Resolve defaults through an explicit engine/equipment eligibility table, not live
RX timing. A cold ECU/cache must not make the defaults disappear or regenerate.

| Set | Row A | Row B |
| --- | --- | --- |
| Warm-up | Oil temperature + coolant temperature | Gearbox temperature |
| Oil | Oil level mm and LOW/OK/HIGH | Oil level bar |
| Engine | RPM + boost | Power + torque |
| Cooling | Coolant temperature | Intercooler inlet + outlet |
| Battery | Voltage + current | IBS state of charge |
| Diesel: DPF | DPF load + temperature | Regen state/progress if fitting |
| Gasoline: Performance | Last 0–100 | Best 0–100 |

Use Auto by default; give oil bar the exclusive full row. Validate every default
against the profile's widths and worst-case formatting. If a combination does
not fit, shorten through Auto, then split it into deterministic legacy subpages;
for MY23 deliberately simplify the preset rather than silently hiding a value.

Before coding the presets, Step 1 must map each cell to existing concrete IDs and
record engine/equipment support and fallback. Known-unsupported fields are omitted;
choose only a documented same-theme supported alternative. Do not invent data or
fill Oil with unrelated readings to keep six nonempty sets. If the profile lacks
enough information, populate proven generic Battery/Driving values and leave
unsupported slots empty until the owner explicitly restores appropriate defaults
or edits them. Show a configuration hint, not an automatic destructive rebuild.
Unknown oil range keeps valid mm with unknown status, never a false OK. Do not
invent V6 oil limits or silently transfer Giulia limits to another model.

## 3. Technical specification and dependency boundaries

### 3.1 Data model: one owner, distinct meanings

C1 owns the authoritative `VehicleProfile`. Suggested new module:
`firmware/baccable/vehicle/vehicle_profile.h/.c`. Use fixed-size C structs,
ordinary enums and functions, no heap or generic reflection/rule engine.

| Field | Allowed values | Rule |
| --- | --- | --- |
| model | Unknown / Giulia / Stelvio | Never infer from engine; model-specific claims need evidence |
| engine | Unknown / Petrol20 / Petrol29 / Diesel22 | Derive gasoline/diesel catalog, fuel and cylinder class |
| model_year | 0=Unknown, otherwise 2000..2099 | Input/serialization bounds, not a claim of supported model years |
| drivetrain | Unknown / RWD / Q4 | Independent of engine and selected driving mode |
| display_selection | Auto / Legacy18 / Legacy24 / MY23 | Explicit selection overrides model year |
| equipment[8] | Unknown / Absent / Present | Adjustable damping, Race, exhaust valves, ACC, HAS, lane assist, parking-mirror support, SCR |
| confirmed | false / true | Distinguishes imported legacy behavior from explicitly accepted new profile |

Unknown is a real state, not numeric zero masquerading as Absent. Validate all
enums/serialized lengths; invalid optional metadata becomes Unknown rather than
a random capability. Do not derive Race from V6 or Q4 from engine output.

Keep separate owners:

- `VehicleProfile`: what the configured car has; only profile commit updates it.
- Feature preferences: what the user wants BACCAble to do; existing settings.
- Live state: RPM, speed, ignition, requests, sample validity and active actions.
- Build capabilities: board role, compiled USB/diagnostic support, fixed capacities.
- UI layouts: Reading IDs, styles, order and Favorite sets, in a separate record.

Do not keep old writable engine/display booleans alongside the canonical profile
indefinitely. During transition, serialization adapters derive legacy slots from
the profile. Runtime accessors read the canonical profile. Remove duplicated
consumer conditions at cleanup; don't maintain two sources of truth manually.

### 3.2 Public policy interfaces

Use these responsibilities; names can be adjusted consistently before coding:

```c
const VehicleProfile *vehicle_profile_current(void);
bool vehicle_profile_validate(const VehicleProfile *draft, ProfileError *error);
VehicleSupport vehicle_feature_support(VehicleFeature feature);
VehicleSupport vehicle_reading_support(ReadingId reading);
VehicleSupport vehicle_parameter_support(uint8_t parameter_id);
DisplayProfile vehicle_display_profile(void);
bool vehicle_oil_range(uint8_t *minimum_mm, uint8_t *maximum_mm);
```

`VehicleSupport` is Supported / Unsupported / Unknown, not a boolean. DisplayProfile
provides resolved mode, simultaneous row count, per-row widths and glyph policy.
The new reading catalog maps Reading ID to source ID; support by source must not
be reconstructed by searching old compound pages. Keep include dependencies
acyclic: shared ID/enums header -> profile/reading metadata -> formatter/layout
-> menu. Profile policy must not depend on the menu renderer or request scheduler.

Actual feature authorization combines profile support, saved enable preference,
compiled support and existing live guards. It is not the same as visibility.
Expose the reason (unsupported, configure vehicle, disabled, stop car, pending)
without duplicating the rule in each UI/worker function. Recheck authorization
at execution, not just when constructing the menu.

### 3.3 Explicit policy matrix

| Consumer | Central rule | Preserve |
| --- | --- | --- |
| Oil numeric/bar | Existing owner-car ranges: 2.0 RWD 72–82, Q4 67–77; 2.2 RWD 61–65, Q4 79–84; unverified model/engine combinations unknown | Existing source/scales and unknown-range status; do not claim verification for another model |
| Display | Explicit mode wins; Auto known year >=2023 -> MY23; older/unknown -> migrated explicit mode or conservative Legacy18 | Year alone cannot distinguish the older screen sizes |
| DPF readings/regen alert | Diesel22 required; unknown engine does not become gasoline/diesel implicitly | Data freshness and active regen status remain telemetry |
| AdBlue | Diesel plus SCR Present; SCR Unknown -> applicability unknown | Don't infer SCR solely from diesel/year |
| Cylinder 5/6 ignition | Petrol29; preserve known source bounds | Other V6 measurements are not automatically proven |
| MultiAir readings | Petrol20 | No gasoline-wide assumption |
| AWD disable | Q4 for confirmed profiles | Existing stationary/pending/stop rules |
| Softer default suspension | Adjustable damping Present | Soft in D / Mid in Race, engine-start timing and feedback logic |
| Default Race | Race Present | Owner's accepted non-V6 mechanism; don't equate QV engine with permission |
| Exhaust action | Exhaust valves Present | Active override must retain AUTO/release route |
| Virtual ACC/HAS, LKA-related actions | Respective equipment Present | Existing live conditions, release/stop and feature enables |
| Park mirrors | Compatible mirror support Present | Calibration/restore and live position validity |

Vehicle selection is configuration, not evidence that unverified DIDs exist on
every ECU. Reading metadata must retain Known/Unverified source applicability;
Unknown ordinary diagnostics can show explicit unavailability, never fabricated
measurements. A model field must not silently change scales without a reviewed
model-specific definition. Oil values above are the existing owner-supplied table;
newly selected unverified models get unknown ranges, not copied certifications.
An imported unconfirmed profile may preserve its existing oil-range interpretation
until explicit configuration; label this compatibility behavior in documentation,
not as newly verified applicability. After confirmation, Unknown model or an
unverified model/engine combination yields unknown range while retaining mm.

### 3.4 Migration and activation policy

Two independent atomic transactions, deliberately **not** an unprotected two-record
transaction:

1. Vehicle/feature settings: import existing SETTINGS_RECORD 0x101 payloads of
   42/41/40 slots; preserve existing feature values. Decode engine slot 16 with
   V6 companion 36 (diesel takes precedence), drivetrain 40 and MY23 6. Migrate
   the old display to an explicit MY23 or compiled Legacy18/Legacy24 selection.
   Model/year/equipment are Unknown, confirmed=false. Never infer equipment from
   an enabled feature. Candidate new settings type is 0x107; check live reservations.
   Serialize fields explicitly with schema/length validation, not raw struct padding.
2. UI layouts: new VISIBILITY_RECORD type (candidate 0x106); ignore all old UI
   layouts and initialize new defaults once, as Step 7 specifies. This must not
   erase settings/statistics/calibration. A UI-save failure does not roll back an
   already committed vehicle profile or mark UI initialization complete.

Imported, unconfirmed installations retain existing actuation permission policy
until the user explicitly saves/confirms Vehicle. Show that equipment is not yet
configured. This compatibility state never claims equipment is Present and never
enables a previously disabled feature. New installations have no grandfathered
actuation permissions. Known engine/drive/display metadata can still render/filter
readings; unknown fields remain unknown.

Before confirming the profile, show the list of enabled preferences that would
become ineligible. Require the existing save confirmation; keep their stored
preferences but suppress their effective operation while incompatible. On a
confirmed profile, Unknown equipment cannot authorize a new equipment-dependent
actuation. Known Absent hides normal options; Unknown shows a configuration reason.
Stop/release/recovery stays available for an operation already owned by BACCAble.

Reject profile commits that would strand active AWD, brake/Dyno, exhaust or mirror
workflows; require completion/release first. Do not auto-release a physical action
merely because the user browsed a profile field. Do not trigger DNA or suspension
startup again while saving. When suppression must be sent to a peer, send an
explicit effective OFF/disabled option; silence can leave old peer state enabled.
Preserve existing timers/leases/acknowledgements and send no full vehicle struct
across UART. BH needs the display contract; peers need relevant effective options.

### 3.5 Commit-time update sequence

Vehicle editing uses one isolated complete profile draft. Follow this order:

1. Validate fields and active-operation constraints; compute consequences in draft.
2. Persist vehicle/profile + preserved feature values atomically. If it fails,
   retain the old effective profile, UI and board configuration; allow retry.
3. Publish the new canonical profile; increment an in-memory profile generation.
4. Cancel pending OBD work and invalidate incompatible cache/peak state; reset oil
   animation and resolve visible lists/layouts. Delayed replies must match the
   active source/session generation, not only the old gasoline/diesel boolean.
5. Switch display contract only when supported by the peer; discard stale render
   fragments without destroying factory source tracking. Apply effective feature
   configuration with existing board synchronization semantics, no startup replay.
6. Keep custom Favorites unchanged. After profile changes, unsupported stored
   items can return later; regenerate defaults only through Restore defaults.

A local profile-generation counter cannot identify an old UDS reply on the wire
if a new request uses the same ECU/DID. On cancellation, retain the old response
deadline and do not reuse that ambiguous request until it expires (using the
existing bounded request timeout). Add a late-same-DID test; do not claim that
comparing only a newly assigned local generation solves this ambiguity.

A selected supported display mode that cannot yet be negotiated must report a
pending/unsupported state and retain the previous working transport. Never send
an unrecognized long packet and assume the screen will recover. On reboot, use
the documented safe transport until the saved mode is negotiated. All shipped
boards must be updated together for the new runtime display contract.

### 3.6 One menu and one ownership model

```text
Settings
  Vehicle
    Model / Model year / Engine / Drivetrain / Display
    Equipment
      Suspension / Race / Exhaust valves / ACC / HAS
      Lane assist / Park mirrors / SCR
    Review changes / Save
  Features
  Favorites
    Favorite 1..6
      Line A / Line B -> item -> reading / format / move / remove
      Preview / Save
    Restore defaults
  Shown readings / Sort order
  BACCAble IPC
```

Remove writable duplicates of Engine/Drivetrain/MY23 under Features. BACCAble IPC
continues to hold transport tuning only. Keep profile editing reachable when
engine/model is Unknown and on legacy displays. A temporary unavailable reading
never disables entry into configuration. Labels/menu paths must be mirrored in
both guides and the display-only Demo when behavior changes.

### 3.7 Dependency graph and deliverable boundaries

```text
P0 source/test inventory + scoped runner
 -> P1 ID/source/default contract
 -> P2 complete profile + settings codec + eligibility policy
 -> P3 individual reading catalog
 -> P4 scalar/row renderer
 -> P5 runtime C1/BH display contract
 -> P6 Favorite layout/default/subpage model
 -> P7 new UI record/reset codec
 -> P8 unified profile/menu/polling/feature cutover
 -> P9 retired path removal + both guides
 -> P10 scoped acceptance tests + builds/resources + hardware report
```

P2's editor is connected at P8; its model/storage is tested earlier. P7 must not
replace boot storage until P8 understands the new format. P3/P4/P6 can be isolated
modules during development, but there is no released legacy-compound mode.
Each P-stage produces a small reviewable commit with its tests; integration is
one coordinated delivery, not a series of independently incompatible releases.
Do not create a second backlog; record progress against these IDs in ACTION_PLAN.

## 4. Current files and tests: inspect these before editing

| Area | Current code | Existing verification |
| --- | --- | --- |
| Catalog and source definitions | `diagnostics/parameter_catalog.c/.h` | `tests/test_catalog.c` |
| Menu groups/filtering/storage | `features/menu_model.c/.h`, `features/menu.c` | `test_menu.c`, included `test_my23_ui.c`, `test_menu_contract.c` |
| Old Favorite helpers | `features/favorite_parameters.c/.h` | Mostly menu-suite fixtures; confirm live callers before reusing |
| Formatting | `features/dashboard_format.c`, `features/my23_ui.c/.h`, `features/dashboard.c` | `test_my23_ui.c`, `test_idle_latin1.c`, menu numeric tests |
| Oil | `features/oil_level.c/.h`, `oil_bar_animation.c/.h` | `test_oil_level.c`, `test_oil_animation.c` |
| Profile/settings | `state/settings.h`, `settings/setup_entries.c`, `setup_menu.c`, `persistence.c` | `test_setup_ui.c`, settings tests in `test_menu.c` |
| Polling/cache | `diagnostics/parameter_request.c`, `parameter_cache.c`, `features/parameter_navigation.c` | Menu polling/reply/freshness tests |
| Display transport | `features/body.c`, `display_stream.c`, `transport/board_uart.c/.h`, `app/build_config.h` | `test_body_display.c`, `test_uart_menu.c`, `test_rain_uart.c` |
| Atomic records | `storage/flash_records.c`, `record_store.c` | `test_core.c`, menu save/load fixtures |
| Build/test wiring | `tests/Makefile`, `.github/workflows/ci.yml`, `.github/scripts/build-ui-matrix.sh` | Existing ASan/UBSan host and four-profile build matrix |

Paths under code are relative to `firmware/baccable/`. Some named tests are
included by `tests/test_menu.c`, not separate executables. When adding a case,
register it in that suite's `HOST_TEST` list. Add included fixtures to
`MENU_FIXTURES` and production files to `MENU_SOURCES` in `tests/Makefile`.
A standalone new test needs both a build dependency and execution in the new
scoped target; wider CI registration must respect Section 7.
Compiling a test without running it does not verify the change.

## 5. Execution order: finish each gate before moving on

### Step 0 — establish a trustworthy baseline

1. Read AGENTS, ACTION_PLAN, MENU_UX, DEVELOPMENT and firmware MAKEFILE.md.
2. Inspect Git status/worktrees and current master. Preserve user edits; create an
   isolated implementation branch. Do not apply the older dirty checkout as base.
3. Identify which profile/rain/menu branches have since merged.
4. Read existing test source and retained CI results; do not execute untouched
   tests to obtain a baseline. Record existing evidence and its SHA/date, or mark
   baseline test status unknown. Firmware compilation/resource measurement is
   allowed: record Flash/RAM for boards and display builds with the full toolchain.
5. Read assertions listed below and classify retired contracts versus invariants.
6. Create the test-impact/allowlist ledger specified in Section 7 and the scoped
   runner before any test execution. This is part of the implementation, not a
   permission to modify tests cosmetically or run them through a broad target.

Gate: exact baseline SHA, tools, available prior test evidence/unknowns and sizes
recorded. The scoped runner executes only new or meaningfully changed cases.
Do not call unavailable baseline evidence a pass or manufacture a baseline by
running suites contrary to the owner's restriction.

### Step 1 — freeze the new catalog contract and test expectations

1. Enumerate all current source definitions and every page's parameter IDs.
2. Produce a source-to-reading table in the implementation documentation: new
   stable Reading ID, original parameter ID, group, label variants, unit, precision,
   numeric/enum width, eligibility, freshness class and default-set usage.
3. Collapse repeated identical IDs used as old two-value padding. Preserve distinct
   source IDs and useful scalar measurements from removed compound pages.
4. Identify Oil bar as a separate presentation referencing the same source.
5. Fill the default table with concrete IDs and eligibility/fallback decisions.
6. Write expected examples/boundaries and a test-change table before coding; don't
   generate expected strings by calling the production formatter under test.

Gate: every new reading references an existing source; every default has a known
mapping; no multi-measurement Reading and no silent source substitution. Do not
change `ParameterDefinition` IDs or 28-byte layout for a display-only redesign.

### Step 2 — implement the full profile model, storage and policy

1. Implement Section 3's canonical types, validation, accessors and read-only API.
   Normalize legacy bits at import; do not leave feature modules reading two
   independent engine representations. Include all listed equipment fields.
2. Implement the settings codec/migration and legacy-confirmation transition.
   Keep vehicle settings and UI reset as separate transactions. Test the codec
   in isolation before changing production boot order.
3. Move oil ranges, engine support and display resolution into shared policy.
   Add explicit feature/reading/source eligibility and unavailable-reason mapping.
4. Inventory and replace repeated profile conditions in menu, setup, dashboard,
   favorite lookup, diagnostics, pedal booster, shift indicator and board_sync.
   Preserve behavior under the imported compatibility state until confirmation.
5. Implement profile-generation/change notification and effective option derivation;
   reject changes that would strand active operations. Keep actual UI activation
   and request/peer wiring in P8, after all consumers have been adapted.
6. Existing settings adapters may temporarily keep old menu code compiling, but
   must route to the canonical profile and cannot maintain another writable copy.

Modified/new tests in this commit: legacy 40/41/42-slot import, diesel/V6 decoding,
all enums, unknowns, year bounds, explicit display/Auto, equipment tri-state,
confirmation consequences, startup behavior unchanged, failed save/power-loss
rollback, no guessed equipment, no forced Race/V6 equivalence, all oil ranges,
model-specific unknown range and active-action commit rejection. Update setup/
settings fixtures meaningfully before selecting their cases for execution.
Gate: profile owns the facts and policies; its codec and transitions work under
scoped tests. No old layouts are reset yet; no physical actions start on edit/save.

### Step 3 — implement the new individual reading model

1. Add a small reading catalog (suggested `features/reading_catalog.c/.h`). Keep
   transport/UDS definitions in their existing location.
2. Encode the Step 1 table with stable IDs, groups and presentation metadata.
3. Expose lookup/list/support operations; no consumers infer eligibility themselves.
4. New tables can coexist temporarily with old UI while being unit-tested. They
   must replace it at Step 8; this is not a retained legacy product mode.

Tests: unique IDs; correct group order; every numeric reading has one source;
bar references one source; source bounds; no duplicated padding; explicit
source distinction; profile filtering; no stale sample converted to zero.
Extend existing request bytes/scales/offset tests meaningfully to cover the new
Reading-to-source mapping, then run those updated cases. Gate: source decoder fixtures
unchanged, new catalog tests registered and executed.

### Step 4 — implement width-aware scalar and row rendering

1. Add one renderer taking reading ID, value/status, format and target glyph width.
2. Add a bounded row resolver/composer; resolve Auto against reserved field widths.
3. Route Oil numeric and bar presentations through shared source/range metadata;
   keep existing proven conversion and boundary logic.
4. Return explicit fit/overflow status; don't truncate UTF-8 or numeric fields.
5. Keep fixed buffers and shared const strings; do not add four full catalogs for
   four widths. Do not change CAN cadence to compensate for formatting.

Tests: 16/18/22/24; empty rows; separators; longest labels; negative values;
99→100; rounding; missing/stale/NaN; longest enums; sign/units preserved; UTF-8
and unsupported glyph fallback; explicit style rejection; stable Auto; bar
exclusive row and exact full width. Gate: tests verify output against independent
expected strings/glyph counts and guard buffer bounds under ASan/UBSan.

### Step 5 — make display width an end-to-end runtime contract

1. Inspect C1 and BH copies of `DASHBOARD_MESSAGE_MAX_LENGTH`, UART sizes, padding,
   footer placement and CAN fragment counts. Inventory before replacing macros.
2. Keep compile-time maximum buffers; distinguish maximum capacity from active
   visible width. Implement Legacy18/Legacy24/MY23 mode identification across UART.
3. Preserve the existing MY23 field boundaries (16+22), factory source ownership,
   complete-frame handling and latest-screen coalescing.
4. Define mismatched-peer behavior explicitly: reject unsupported mode or fall back
   to proven Legacy18 with notice; never reinterpret a longer message silently.
5. Preserve build defines as default selectors, not separate rendering algorithms.

Tests: all runtime modes from one binary; C1/BH mode agreement; malformed/partial
UART frames; fallback; mode switch drops stale fragments; legacy footer intact;
radio interruption/recovery; unchanged CAN frame timing and bounded queues.
Update body/UART/rain-USB coexistence cases to exercise the new mode/traffic
contract first, then run only those modified cases. Gate: inspect actual emitted
packet/fragment bytes, not just the C1 preview string. Build all four boards now.

### Step 6 — implement layout model and deterministic defaults

1. Add a bounded Favorite layout type: two rows × four reading/style pairs;
   six sets, preserving the chosen engine-bank organization explicitly.
2. Add operations for add/remove/reorder/move, validation and draft cloning.
3. Implement defaults from Step 1's table. Fixed support policy selects them;
   temporary CAN/OBD silence does not select different defaults.
4. Implement logical-to-physical layout resolution: MY23 two rows, legacy
   subpages/continuations, skip empty rows, no saved-layout mutation.
5. Return the unique source IDs for the currently visible physical page only.

Tests: every default for supported engine/display combinations; same measurement
in two rows deduplicated; duplicate in a row rejected; bar exclusivity; unknown
range; empty/unsupported rows; previous/wrap navigation; wider saved layouts
on smaller displays; current-profile change preserves stored custom layouts.
Gate: simulated defaults fit, contain no known-incompatible items and need no
more than eight unique sources on a MY23 screen.

### Step 7 — implement a deliberate one-time UI reset, not migration

Implement the new codec/default initializer as a separately tested component.
Do not attach it to the production boot path until the Step 8 UI/polling cutover;
otherwise the old UI could interpret the new payload while its tests still expect
the old format.

1. Introduce a new UI record type/schema in `VISIBILITY_RECORD`; `0x106` is a
   candidate only after checking current reservations. Version/length must differ
   from old 0x103/0x104/0x105 payloads. Never parse old page IDs as new Reading IDs.
2. If a valid new record exists, load it and preserve edits exactly.
3. Otherwise initialize new defaults and reset old UI-only visibility/order/last
   selection. Ignore old layouts; no import/conversion of old Favorite slots.
4. Save the new schema using existing atomic record protection. Treat a failed
   write as failure: defaults may work in RAM, but don't mark reset completed or
   spin writes each loop. Define a bounded retry or explicit retry prompt.
5. A successful new record is the reset marker: next boot must not regenerate
   defaults. Preserve settings/statistics and unrelated Flash regions byte-for-byte.
6. Add confirmed Restore defaults for Favorites; discard/failure leaves prior
   saved layouts intact. Handle a future unsupported schema without silently
   overwriting it. Document that old firmware cannot restore old layouts.

Tests: erased Flash; each old record version; corrupt new record with valid atomic
backup; write failure/power-loss phases; restart after successful reset; custom
save then restart; repeated Restore defaults; cancel; settings/statistics
unchanged. Gate: old Favorites are intentionally gone, exactly as requested;
source definitions, vehicle settings and records outside UI remain untouched.

### Step 8 — switch the real UI and polling paths together

Do this as one integration unit with its updated menu tests; incomplete private
edits are not a releasable intermediate state.

1. Connect the complete Vehicle editor/confirmation and settings load/commit path.
   Remove duplicate old setup entries. Apply Section 3.5 order and effective
   eligibility at menu entry, action execution, periodic workers and board_sync.
   Preserve stop/release paths and old startup timing. Do not silently trigger
   profile confirmation through a Favorite save.
2. Replace Readings group/list selection with new Reading IDs. Remove multi-value
   page browsing and MY23 next-reading preview. Show name/value on MY23 and fitted
   label/value on legacy. Clamp/reset navigation when a profile removes entries.
3. Replace Favorite page selection with row/item/style editing. Include live
   preview plus worst-case fit validation, reorder/move/remove and confirmation.
4. Implement complete drafts: no Flash changes or new-layout polling while merely
   editing. HOLD commits only after successful save; double BACK discards.
5. Display initialized defaults at first entry. Expose Restore defaults clearly.
6. Replace page-based request selection with unique visible-source polling, shared
   cache, bounded one-at-a-time UDS requests and current timeout/freshness rules.
7. Readings polls just its selected source. MY23 Favorites polls both rows; legacy
   polls only its visible subpage. Repeated sources consume one request/cache slot.
8. Cancel/invalidate requests on relevant selection/profile changes. A delayed reply
   from an old source/layout must not overwrite a new value. Do not increase the
   CAN/OBD request rate or tie it to a 20 ms display refresh.
9. Wire profile changes, oil animation reset, peak state and display stream changes.

Tests in this same unit: new menu paths/labels/default screen, group order,
Readings single-source request count, visible Favorite request order/deduplication,
late/wrong replies, freshness, legacy row B reachability, empty sets, draft gestures,
failed save, reset and no repaint/layout jumps at ordinary value transitions.
Gate: all selected new/updated behavior tests pass; deliberately obsolete expectations have been
replaced as specified below, not skipped to make CI green.


### Step 9 — remove retired paths and update documentation

1. Remove temporary profile adapters and duplicated direct engine/year/equipment
   checks outside the policy module. Remove old compound-page UI tables, old
   Favorite migration routines, obsolete
   `$...` template rendering and dead atomic/page helpers after checking all callers.
2. Do not remove `ParameterDefinition` data, decoders, sources still used by native
   telemetry, fault reading or other features. No cleanup outside this change's scope.
3. Update Demo to show new menu labels/layouts without executing or saving anything.
4. Update both EN/PL user guides: groups, individual readings, layouts, formats,
   small/large/MY23 behavior, defaults, authorized reset, Restore defaults and limits.
5. Update architecture CATALOG_AUDIT, MENU_UX and DEVELOPMENT statements that used
   to require old page order/IDs or compound-page preservation. Record source-ID
   preservation separately from the intentionally replaced UI IDs.
6. Update ACTION_PLAN with implementation/check/hardware status in the task branch.

Gate: searches find no executable old layout import, old compound Readings or
unregistered tests. Historical release/research documents may retain historical
terms; don't erase them or remove assertions unrelated to retired behavior.

### Step 10 — integration, resource and vehicle acceptance

Run only the approved scoped commands below; check exit codes and selected-case
execution. Do not use full-suite targets that would run unmodified cases. Summarize
Flash/RAM deltas for every board and display build. Check record size/capacity,
fixed buffers and runtime stack margin, not just the 192-byte layout item estimate.
If it does not fit, deduplicate strings/dead paths first; reduce four-item rows
to three only with an explicit documented scope decision, not a silent change.

Report host/build evidence separately from hardware acceptance. Required vehicle
checks: actual small legacy screen width, legacy 24 where available, MY23 16/22,
both rows or subpages, glyphs, cold/no-data display, positive/negative/extreme
values, oil range/bar, user save/reboot, one-time reset, radio-source coexistence,
no UART starvation and unchanged unrelated startup actions. If hardware is not
available, leave these open; software tests cannot certify the display pixels.

## 6. Mandatory test replacement map

Additional integrated profile cases (new or meaningful extensions): all equipment
Unknown/Absent/Present; unconfirmed imported versus new installation; confirmed
RWD blocks AWD start but never blocks recovery; non-V6 Race Present remains
eligible; enabling a preference does not mark equipment Present; profile save
cannot replay startup DNA; disabled effective peer options are actually sent;
failed profile save keeps current options/display; profile change does not reset
custom Favorites; profile/schema and UI/schema failures stay independent.


A failing old expectation is not automatically a firmware defect, and a changed
requirement is not permission to delete its whole test file. For each failure,
identify: baseline defect / intended behavior change / regression / test fixture
or link dependency error. Change expected behavior only with the contract above.

| Existing assertion/test | Required treatment |
| --- | --- |
| `test_catalog.c`: IDs sequential by page position, old page count and `$...` widths | Replace with new Reading ID uniqueness/group/source/format invariants; keep parameter request/scale/layout checks |
| `test_my23_reading_formats_fit` | Replace old compact-page strings with scalar formatting and width-specific format cases |
| `test_my23_readings_preview_and_unavailable_gear`, `test_readings_preview_prefetch` | Replace next-page preview/prefetch with one selected measurement; keep invalid gear handling |
| `test_whole_page_favorites_and_beta19_migration`, `test_my23_list_and_old_favorite_migration` | Replace layout preservation with old-schema reset/new defaults/second-boot persistence assertions |
| `test_shipped_favorite_pairs` | Assert the new engine-aware default layouts, not old page pairs |
| `test_fifth_favorite_uds_reply` | Replace obsolete hidden-slot semantics with visible-source mapping and delayed-reply isolation |
| `test_atomic_favorite_packing`, `test_favorite_slot_draft_flow` | Rewrite for row/item/style model, fixed capacity and draft contract |
| `test_favorite_poll_cycle` | Keep deduplication, timeout and fairness; adapt to actual visible items/legacy subpage |
| `test_settings_migration` | Extend with complete profile/new settings schema and confirmation flags; KEEP vehicle/feature preservation; UI reset is a different record |
| `test_oil_level`, `test_oil_favorites`, `test_oil_animation.c` | Adapt IDs/entry points; KEEP ranges, LOW/OK/HIGH, unknown, full width and smooth movement |
| `test_menu_contract.c`, `test_setup_ui.c`, `test_unified_ui.c` | Update changed navigation/menu paths; KEEP save/discard/failure/disabled action behavior |
| `test_my23_glyph_budget`, `test_idle_latin1.c`, `test_body_display.c` | Keep bounds/encoding/factory recovery; add runtime 18/24 and proper line separation |
| `test_uart_menu.c`, `test_rain_uart.c`, USB tests | Extend selected cases to exercise new display-profile/configuration traffic and recovery; keep reliability assertions; do not execute untouched cases |
| `test_menu_labels.py` | Extend to new groups/editor messages; allow full reading labels only where width-aware rendering exists, not unlimited fixed-menu labels |
| `test_core.c`/storage and UDS decoding tests | Preserve atomicity, CRC and signed/scaled values; add new record fixtures without weakening old guarantees |

New pure-model tests can be added as `test_reading_catalog.c`,
`test_reading_layout.c`, `test_favorite_layout.c`, `test_ui_layout_store.c` and
`test_vehicle_profile.c` if no equivalent suite exists. Wire production modules,
fixtures and execution into Makefile immediately. Don't write tests that merely
repeat the implementation's table-generation algorithm as the expected result.
Do not use stale binaries, disabled assertions, removed sanitizers, broader
numeric tolerances or skipped test registration to conceal failures.

## 7. Test-execution contract: read, update, select, then run

This section implements the owner's explicit restriction: **do not execute test
cases that were not modified for this work**. It applies to baseline, iteration,
final checks and automatic aggregate targets. A test file being touched, a changed
shared header or updated Makefile is not permission to run every case in it.
No cosmetic rename/comment edit qualifies as a meaningful test modification.

### 7.1 Create a test-impact ledger before changing production behavior

Store one `test-impact.md` alongside implementation evidence (or a section in
this specification), not another backlog. Each row must contain:

- source file and exact registered case name;
- contract/invariant protected and P-stage;
- classification: new / update / replace / untouched;
- specific meaningful assertion/fixture/input change, with commit or diff reference;
- expected behavior after the change and independent oracle;
- executable/build variants needed (18/24, MY23, board roles);
- whether execution is allowed, and final observed result.

Only new/update/replacement cases enter the execution allowlist. Read untouched
cases as reference, but do not run them or count them as verified. For a relevant
invariant (e.g. settings preservation), extend the existing case to exercise the
new profile/layout transaction; retain its still-valid assertions. It then has a
real changed scenario to execute. Do not mechanically modify every file just to
circumvent the restriction.

### 7.2 Implement scoped execution first

The current `tests/test_report.h: host_tests_run()` runs every registered case;
it has no filter. Implement an **opt-in exact case-name allowlist** for the new
workflow, with a dedicated runner/manifest and `test-vehicle-ui` target. Keep
existing runner behavior for other users unless explicitly selected; the new
entry point must require its manifest and never default to all cases.

The implementation should:

1. Build only required executables/variants from the manifest; don't invoke the
   existing broad `test` or `test-menu-fast` recipes as dependencies.
2. Pass selected exact case names to each process (e.g. an environment allowlist).
   `CASE PLAN/RUN/PASS` must include only selected cases.
3. Fail for missing/unknown names, an empty selection, duplicate/conflicting entries,
   a declared case not actually run or a test executable lacking selection support.
4. Fail immediately on compiler, assertion, sanitizer or process errors. Do not
   treat filtered-out cases as PASS and do not launch stale binaries after build failure.
5. Offer a list/dry-run mode printing executables/variants/cases without executing.
6. Include new tests of this selection mechanism (dummy registered cases can count
   invocation); it must prove an unselected sentinel never runs. These are new
   cases and may execute under the restriction.

`test_setup_ui.c`, `test_my23_ui.c` and other fixtures are included by `test_menu.c`.
Select their **case names**, not the whole menu executable. Tests with their own
plain `main` must be meaningfully adapted for this workflow or replaced by a new
scoped executable; do not inadvertently run all their cases. Preserve test-report
semantics so the run summary accurately describes the reduced scope.

### 7.3 Per-stage execution order

For every P-stage:

1. Read affected code and tests; update the ledger and new expected behavior.
2. Modify/add the meaningful test cases and their registration/build dependencies.
3. Change production code and complete matching fixtures/stubs. A first expected
   red run is allowed only for the newly written/modified cases, not stale tests.
4. Inspect the runner's dry-run list; every selected case must be eligible.
5. Build/run only that list; classify failures and fix code or incorrect contract
   expectations. Never delete unchanged assertions merely because they fail.
6. Record pass/failure and limitations; proceed only with that stage's selected
   coverage passing. Commit code and corresponding test changes together.

Before P10, freeze the final manifest: all required profile, schema/reset,
formatting, UI, visible-source polling, transport and resource cases must have
meaningful updated coverage. If coverage is missing, write/update it first.
Do not discover missing tests by launching the entire old suite at the end.

### 7.4 CI and honest limits

Do not run `make test`, `make test-menu-fast`, standalone unchanged Python test
scripts, the unchanged Lab suite, or a full CI dispatch as a shortcut. Extend
and explicitly select Python cases if their contract is affected; unchanged
release-script tests do not need execution for this feature.

Do not disable repository-required CI, weaken assertions or edit workflows to
hide failures. If submission policy requires a full CI suite that includes
untouched cases, report the conflict with the owner's restriction before triggering
that broader run. Continue allowed implementation/checks; obtain revised scope
for broader execution if needed. A passing scoped run is **not** a claim that
untouched regression suites or vehicle hardware passed.

## 8. Commands and reporting for the future implementation

Read the live [MAKEFILE.md](../../firmware/baccable/MAKEFILE.md) first. Run from
the isolated repository root. The scoped runner below is a required **new** tool
from P0, not something available in the inspected baseline. Define its CLI as
shown so a later agent does not invent a different execution flow. Use a manifest
for each stage and one final union; exact executable paths/case names are filled
from the live test inventory.

```sh
# NEW interface to implement in P0; dry run must execute no test code.
python3 tests/run_vehicle_ui_tests.py --manifest tests/vehicle_ui_tests.json --list

# NEW target: build only manifest executables and run only allowed cases.
# Its prerequisites must not include the existing broad test targets.
/usr/bin/make -C tests CC=/usr/bin/clang test-vehicle-ui MANIFEST=vehicle_ui_tests.json

# Compilation/lint/resource checks do not execute untouched host test cases.
export TOOLCHAIN=/Applications/ArmGNUToolchain/15.2.rel1/arm-none-eabi/bin/arm-none-eabi-
export VERSION=vehicle-ui-test
for flavor in C1 C2 BH CAN; do
    /usr/bin/make -C firmware/baccable FLAVOR="$flavor" TOOLCHAIN="$TOOLCHAIN" VERSION="$VERSION" lint || exit 1
    /usr/bin/make -C firmware/baccable -j4 FLAVOR="$flavor" TOOLCHAIN="$TOOLCHAIN" VERSION="$VERSION" all || exit 1
    FLAVOR="$flavor" bash .github/scripts/build-ui-matrix.sh || exit 1
done
git diff --check
```

Example manifest shape (to create during implementation, after the named case
has actually been extended for the new profile schema; not execution permission
for the unchanged baseline case):

```json
{
  "schema": 1,
  "stage": "P2",
  "runs": [{
    "build_target": "build/test_menu",
    "variant": "legacy18",
    "source": "test_menu.c",
    "cases": [{
      "name": "test_settings_migration",
      "change": "updated",
      "evidence": "test-impact.md#p2-settings-profile-import"
    }]
  }]
}
```

Paths in the manifest are relative to `tests/`; the CLI resolves the manifest
path before changing directory. `variant` must agree with the Makefile target;
reject a legacy18 target labeled my23. Use a concrete case-name selection variable
(e.g. `BACCABLE_TEST_CASES`) understood by the adapted runner. This variable is
host-test-only and must not become firmware configuration.

Minimal manifest entry contract: executable build target, build variant, source
file, list of exact case names and test-impact ledger references. Validate paths
against the repo/test build directory; reject arbitrary shell commands. Prefer
structured subprocess arguments. Log selected case count and actual run count
and require equality. Add each new/modified case to standard registration as
appropriate without using that full registration as this task's execution list.

After compiler/host flag changes use a forced rebuild of the selected targets,
not an unfiltered suite. Export TOOLCHAIN so nested UI-matrix make resolves the
full embedded toolchain. Keep legacy/legacy-large/my23/my23-large default-selector
builds even after runtime display selection; CI may use a different compiler.

For each completed P-stage record the commit, changed cases, command exit status,
remaining failures, scope not executed and resource impact in ACTION_PLAN.
If a case is replaced, identify its replacement. Final diff review must establish
that only old UI layouts reset and no source scales, vehicle settings, calibration,
statistics, unrelated automatic behavior or user files were lost. Update both user
guides, retain explicit hardware gaps and keep the implementation branch clean.

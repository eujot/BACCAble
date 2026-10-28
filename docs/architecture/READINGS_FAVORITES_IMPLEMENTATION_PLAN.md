# Implementation plan: individual Readings and composed Favorites

Planning baseline: `origin/master` at `ba9176e`, inspected 2026-10-28.
Status: implementation instructions for later execution; this task changes only
this documentation. Task status remains in [ACTION_PLAN.md](../ACTION_PLAN.md).
Read this document before the earlier profile/layout proposals.

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

### Vehicle dependency

Use the central API proposed in [VEHICLE_PROFILE_AUDIT.md](VEHICLE_PROFILE_AUDIT.md).
If it has not been implemented, first add the minimal profile adapter described
in Step 2. Do not block this work on a broad rewrite of unrelated vehicle actions.
Fuel is derived from Engine, not another editable boolean. Display choices are
Auto / Legacy 18 / Legacy 24 / MY23. Preserve saved MY23 and build-width choices
when importing vehicle settings; do not reset them with the old Favorite record.

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

## 3. Current files and tests: inspect these before editing

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
A standalone new test needs both a build dependency and execution in `make test`.
Compiling a test without running it does not verify the change.

## 4. Execution order: finish each gate before moving on

### Step 0 — establish a trustworthy baseline

1. Read AGENTS, ACTION_PLAN, MENU_UX, DEVELOPMENT and firmware MAKEFILE.md.
2. Inspect Git status/worktrees and current master. Preserve user edits; create an
   isolated implementation branch. Do not apply the older dirty checkout as base.
3. Identify which prerequisite profile/rain/menu branches have since merged.
4. Run the existing host suite and record failures before editing. Build the
   actual starting firmware with the documented full ARM toolchain; record Flash
   text+data and RAM data+bss for all boards and current UI matrix.
5. Read test assertions listed below and mark which enforce intentionally retired
   behavior versus source/transport/save invariants that still apply.

Gate: exact baseline SHA, tools, baseline failures and sizes recorded in ACTION_PLAN.
Do not blame new work for an already failing baseline or hide either kind of failure.

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

### Step 2 — establish the shared profile boundary

1. Reuse implemented `vehicle_profile` if present; otherwise add small accessors
   normalizing current Engine, Drivetrain and IPC settings without duplicating
   writable runtime state. Place oil ranges and catalog eligibility behind it.
2. Define explicit display descriptions: line count, widths and glyph support.
3. Preserve existing vehicle settings when adding year/model/display metadata;
   this is a separate settings migration from the authorized UI reset.
4. Add one profile-change hook for request cancellation/cache invalidation,
   layout resolution and animation/display reset. Preserve startup-action timing.
5. Leave unrelated action/automation policy changes to the vehicle-profile task.

Tests in same commit: engine/fuel normalization, unknown profile, four oil ranges,
V6 unknown range, MY23 override, legacy width default, failed settings save and
no early draft effects. Gate: old UI still works through the adapter; baseline
source/feature behavior unchanged.

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
Keep existing request bytes/scales/offset tests. Gate: source decoder fixtures
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
Run body, UART and rain/USB coexistence regressions. Gate: inspect actual emitted
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

1. Replace Readings group/list selection with new Reading IDs. Remove multi-value
   page browsing and MY23 next-reading preview. Show name/value on MY23 and fitted
   label/value on legacy. Clamp/reset navigation when a profile removes entries.
2. Replace Favorite page selection with row/item/style editing. Include live
   preview plus worst-case fit validation, reorder/move/remove and confirmation.
3. Implement complete drafts: no Flash changes or new-layout polling while merely
   editing. HOLD commits only after successful save; double BACK discards.
4. Display initialized defaults at first entry. Expose Restore defaults clearly.
5. Replace page-based request selection with unique visible-source polling, shared
   cache, bounded one-at-a-time UDS requests and current timeout/freshness rules.
6. Readings polls just its selected source. MY23 Favorites polls both rows; legacy
   polls only its visible subpage. Repeated sources consume one request/cache slot.
7. Cancel/invalidate requests on relevant selection/profile changes. A delayed reply
   from an old source/layout must not overwrite a new value. Do not increase the
   CAN/OBD request rate or tie it to a 20 ms display refresh.
8. Wire profile changes, oil animation reset, peak state and display stream changes.

Tests in this same unit: new menu paths/labels/default screen, group order,
Readings single-source request count, visible Favorite request order/deduplication,
late/wrong replies, freshness, legacy row B reachability, empty sets, draft gestures,
failed save, reset and no repaint/layout jumps at ordinary value transitions.
Gate: all new behavior tests pass; deliberately obsolete expectations have been
replaced as specified below, not skipped to make CI green.

### Step 9 — remove retired paths and update documentation

1. Remove old compound-page UI tables, old Favorite migration routines, obsolete
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

Run the commands below; check exit codes and actual test execution. Summarize
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

## 5. Mandatory test replacement map

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
| `test_settings_migration` | KEEP vehicle/feature setting preservation; UI reset is a different record |
| `test_oil_level`, `test_oil_favorites`, `test_oil_animation.c` | Adapt IDs/entry points; KEEP ranges, LOW/OK/HIGH, unknown, full width and smooth movement |
| `test_menu_contract.c`, `test_setup_ui.c`, `test_unified_ui.c` | Update changed navigation/menu paths; KEEP save/discard/failure/disabled action behavior |
| `test_my23_glyph_budget`, `test_idle_latin1.c`, `test_body_display.c` | Keep bounds/encoding/factory recovery; add runtime 18/24 and proper line separation |
| `test_uart_menu.c`, `test_rain_uart.c`, USB tests | Preserve reliability assertions; adjust stubs/interfaces only where genuinely changed |
| `test_menu_labels.py` | Extend to new groups/editor messages; allow full reading labels only where width-aware rendering exists, not unlimited fixed-menu labels |
| `test_core.c`/storage and UDS decoding tests | Preserve atomicity, CRC and signed/scaled values; add new record fixtures without weakening old guarantees |

New pure-model tests can be added as `test_reading_catalog.c`,
`test_reading_layout.c`, `test_favorite_layout.c`, `test_ui_layout_store.c` and
`test_vehicle_profile.c` if no equivalent suite exists. Wire production modules,
fixtures and execution into Makefile immediately. Don't write tests that merely
repeat the implementation's table-generation algorithm as the expected result.
Do not use stale binaries, disabled assertions, removed sanitizers, broader
numeric tolerances or skipped test registration to conceal failures.

## 6. Commands and reporting

Read the live [MAKEFILE.md](../../firmware/baccable/MAKEFILE.md) first. Run from
the isolated repository root. These are future execution instructions, not
checks already run by this documentation task.

```sh
# Focused UI iteration; run new standalone model suites as well once registered.
/usr/bin/make -C tests -j2 CC=/usr/bin/clang test-menu-fast

# Full integration. This compiles AND executes the registered host suites.
/usr/bin/make -C tests -j2 CC=/usr/bin/clang test
python3 tests/test_menu_labels.py
python3 .github/scripts/test-ci.py

# Explicit complete embedded toolchain; do not use the Homebrew compiler.
export TOOLCHAIN=/Applications/ArmGNUToolchain/15.2.rel1/arm-none-eabi/bin/arm-none-eabi-
export VERSION=readings-layout-test
for flavor in C1 C2 BH CAN; do
    /usr/bin/make -C firmware/baccable FLAVOR="$flavor" TOOLCHAIN="$TOOLCHAIN" VERSION="$VERSION" lint || exit 1
    /usr/bin/make -C firmware/baccable -j4 FLAVOR="$flavor" TOOLCHAIN="$TOOLCHAIN" VERSION="$VERSION" all || exit 1
    FLAVOR="$flavor" bash .github/scripts/build-ui-matrix.sh || exit 1
done
git diff --check
```

After compiler/host flag changes use a forced rebuild (`make -B`) as documented.
Ensure the exported TOOLCHAIN reaches nested make in the UI matrix script. Keep
legacy, legacy-large, my23 and my23-large builds until the full runtime display
contract is verified; default-selector builds still need coverage afterward.
CI's pinned compiler may differ from the local toolchain; record the actual one.

For each completed step record the commit, tests added/replaced, command exit
status, remaining failures and resource impact in the handoff. Do not call a
step complete with unexplained failures. If a test is replaced, name its new
coverage. Final diff review must verify that only UI layouts were reset and no
vehicle settings, source scaling, automatic feature timing or user files were
lost. Preserve unconnected local edits and keep the implementation branch clean.

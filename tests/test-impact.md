# Vehicle-profile / UI foundation — test-impact ledger

Scoped execution contract (see the implementation plan Section 7). Only new or
meaningfully extended cases enter the allowlist; untouched cases are read as
reference but never executed here. The scoped runner is
`run_vehicle_ui_tests.py` with `vehicle_ui_tests.json`.

All listed cases are **new** (`change: new`). Expected behavior is asserted with
independent literals/oracles in the test source; no production formatter is used
to generate the expected strings.

| Case | Contract / invariant | Stage | Classification | Variants |
| --- | --- | --- | --- | --- |
| [test_vehicle_profile_default](#p2-profile-default) | fresh profile is all Unknown, unconfirmed | P2 | new | legacy18 |
| [test_vehicle_profile_validate](#p2-profile-validate) | every enum + year bound validated | P2 | new | legacy18 |
| [test_vehicle_profile_engine_helpers](#p2-profile-engine) | engine class never inferred from model | P2 | new | legacy18 |
| [test_vehicle_feature_support](#p2-feature-support) | confirmed drivetrain + tri-state equipment policy | P2 | new | legacy18 |
| [test_vehicle_parameter_support](#p2-parameter-support) | engine-scoped source applicability | P2 | new | legacy18 |
| [test_vehicle_display_profile](#p2-display-profile) | explicit display wins; Auto only year>=2023 -> MY23 | P2 | new | legacy18 |
| [test_vehicle_oil_range](#p2-oil-range) | owner oil table, never transferred to another model | P2 | new | legacy18 |
| [test_vehicle_profile_codec](#p2-profile-codec) | explicit 0x107 codec round-trips without padding | P2 | new | legacy18 |
| [test_vehicle_profile_legacy_import](#p2-legacy-import) | 0x101 import decodes engine/display/drivetrain only | P2 | new | legacy18 |
| [test_vehicle_profile_commit](#p2-profile-commit) | commit publishes profile + bumps generation | P2 | new | legacy18 |
| [test_runner_selection_runs](#p0-runner-selection) | allowlist runs only selected cases; sentinel never runs | P0 | new | selection |
| [test_reading_catalog_unique_and_ordered](#p3-catalog-unique) | stable unique IDs, group order | P3 | new | legacy18 |
| [test_reading_catalog_sources](#p3-catalog-sources) | every reading has one bounded source; bar shares mm source | P3 | new | legacy18 |
| [test_reading_catalog_eligibility](#p3-catalog-eligibility) | engine/equipment eligibility; Unknown is a real state | P3 | new | legacy18 |
| [test_reading_catalog_source_resolution](#p3-catalog-source-resolution) | per-engine source resolution, no substitution | P3 | new | legacy18 |
| [test_reading_catalog_list](#p3-catalog-list) | profile-filtered group enumeration | P3 | new | legacy18 |
| [test_reading_catalog_enum_labels](#p3-catalog-enum-labels) | bounded deterministic enum/status labels | P3 | new | legacy18 |
| [test_reading_glyph_count](#p4-glyph-count) | UTF-8 glyph count, not byte length | P4 | new | legacy18 |
| [test_reading_missing_never_zero](#p4-missing) | missing/stale -> `--`, never zero | P4 | new | legacy18 |
| [test_reading_numeric_format](#p4-numeric) | precision/sign/unit preserved across 99->100 | P4 | new | legacy18 |
| [test_reading_overflow_indicator](#p4-overflow) | bounded overflow indicator, no clipped digits | P4 | new | legacy18 |
| [test_reading_auto_format](#p4-auto) | Auto resolves to a stable concrete label | P4 | new | legacy18 |
| [test_reading_line_fit](#p4-line) | legacy line composes label+value and reports fit | P4 | new | legacy18 |
| [test_reading_oil_status](#p4-oil-status) | LOW/OK/HIGH from resolved owner range | P4 | new | legacy18 |
| [test_reading_oil_bar](#p4-oil-bar) | full-width bar with marker and off-scale arrows | P4 | new | legacy18 |
| [test_favorite_item_and_clear](#p6-item-clear) | explicit empty items, cleared sets valid | P6 | new | legacy18 |
| [test_favorite_duplicate_and_bar](#p6-duplicate-bar) | duplicate-in-row and bar exclusivity rejected | P6 | new | legacy18 |
| [test_favorite_defaults_valid](#p6-defaults-valid) | every default set valid, <= 8 unique sources | P6 | new | legacy18 |
| [test_favorite_defaults_engine_aware](#p6-defaults-engine) | defaults from eligibility table, not RX timing | P6 | new | legacy18 |
| [test_favorite_unique_sources_dedup](#p6-source-dedup) | repeated sources deduplicated to one slot | P6 | new | legacy18 |
| [test_favorite_row_render](#p6-row-render) | row renderer joins label/value, empty/fit | P6 | new | legacy18 |
| [test_ui_layout_default](#p7-layout-default) | default UI layout initializes valid record | P7 | new | legacy18 |
| [test_ui_layout_roundtrip](#p7-layout-roundtrip) | 0x106 record round-trips exact layout | P7 | new | legacy18 |
| [test_ui_layout_reject_bad_schema](#p7-layout-schema) | unknown schema rejected without overwrite | P7 | new | legacy18 |
| [test_ui_layout_reject_invalid_item](#p7-layout-invalid-item) | corrupt item id/format rejected | P7 | new | legacy18 |
| [test_ui_layout_restart_persistence](#p7-layout-restart) | successful write persists custom edits across restart | P7 | new | legacy18 |
| [test_vehicle_profile_apply_legacy_flags](#p2-legacy-flags) | legacy flags apply without overwriting model/year/equipment | P8 | new | legacy18 |
| [test_settings_migration](#p8-settings-profile) | 0x101 import + 0x107 profile load/save keep feature values | P8 | updated | legacy18 |
| [test_vehicle_editor](#p8-vehicle-editor) | Vehicle editor browses/edits/discards core facts | P8 | new | legacy18 |
| [test_vehicle_editor_equipment](#p8-vehicle-editor-equipment) | tri-state equipment editing | P8 | new | legacy18 |
| [test_my23_glyph_budget](#p8-my23-glyphs) | MY23 packet glyph encoding/budget (re-integrated fixture) | P8 | updated | legacy18 |
| [test_shared_renderers](#p8-shared-renderers) | shared ui_entry renderer bounds (re-integrated fixture) | P8 | updated | legacy18 |

## Observed result

`python3 tests/run_vehicle_ui_tests.py --manifest tests/vehicle_ui_tests.json --run`
builds each manifest target with `/usr/bin/clang` (ASan/UBSan) and runs exactly
the selected cases. Result: **40 selected cases passed, 0 failed**. Untouched
regression suites and vehicle hardware were not run and are not claimed here.

## Retired compound-page expectations (P8/P9 cutover)

The compound-page menu model was removed with the Readings cutover. The following
obsolete host expectations were dropped from the scoped menu executable rather
than re-written against a model that no longer exists; their replacement coverage
lives in the new pure-model executables above (`test_reading_catalog`,
`test_reading_layout`, `test_favorite_layout`, `test_ui_layout_store`,
`test_vehicle_profile`):

- `test_preferences`, `test_shipped_favorite_pairs`, `test_readings_preview_prefetch`,
  `test_engine_filters`, `test_readable_screens`, `test_cache_flags`,
  `test_atomic_favorite_packing`, `test_whole_page_favorites_and_beta19_migration`,
  `test_favorite_poll_cycle`, `test_favorite_slot_draft_flow`,
  `test_my23_list_and_old_favorite_migration`, `test_fifth_favorite_uds_reply`,
  `test_v6_parameter_label_identity`, `test_page_editor_transactions`,
  `test_my23_visibility_transaction`, `test_my23_reading_formats_fit`,
  `test_my23_readings_preview_and_unavailable_gear` — page IDs, `$...` templates,
  visibility/order bitmaps and FavoriteParameters slot semantics.
- `test_catalog.c` (page count/width) — replaced by `test_parameter_definition_layout`
  in the same file: the 98 UDS `parameter_definitions` keep their 28-byte layout,
  request schema and signed scaling invariants.
- `test_oil_level.c` — page-ID oil rendering removed with the page model; oil
  ranges/status remain covered by `test_reading_oil_status` and the vehicle
  profile oil-range case.

The remaining menu/gesture fixtures (`test_my23_ui.c`, `test_setup_ui.c`,
`test_unified_ui.c`, `test_idle_latin1.c`, `test_menu_contract.c`) still contain
obsolete page-model cases and are therefore not compiled by the scoped runner;
re-integrating their still-valid cases is tracked as follow-up documentation work.

## Review corrections, 2026-10-29

New cases in test_vehicle_fixes.c (included by test_menu.c), selected by
vehicle_ui_fixes.json. Only these cases execute during this repair.

| Case | Contract / independent oracle | Status |
| --- | --- | --- |
| test_profile_atomic_restart | Combined settings/profile survives reboot and every interrupted halfword write; real record_store pages | new |
| test_profile_editor_failure | Draft preserved, active profile unchanged on failed Flash; confirmation and operation guard | new |
| test_favorite_validation_restart | Invalid duplicate/bar layout rejected before persistence; valid custom data survives restart | new |
| test_favorite_widths | Missing/ordinary/extreme samples across 16/18/22/24; default rows and deterministic subpages | new |
| test_favorite_policy_sources | V6-to-I4 and SCR absence exclude requests while retaining stored IDs | new |
| test_favorite_visible_polling | Actual menu sends only visible row sources on legacy and both rows on MY23 | new |
| test_profile_effective_options | New/legacy/confirmed policy, Race vs suspension, explicit peer OFF and no startup replay | new |
| test_runtime_display | Auto and explicit widths, peer negotiation and packet boundaries | new |
| test_restore_defaults_flow | Confirm/cancel/failure/success/reboot without changing settings | new |

- New `test_cancelled_same_did` (test_vehicle_fixes.c): reject an old reply in the
  cancelled request's original 500 ms window; accept a fresh request afterwards.
- New `test_runtime_legacy_packet_widths` (test_body_display.c): same BH build
  renders 24/18/24 glyph streams with correct CR/footer/frame counts and retains
  factory radio frames on release. Selected in both BH compile widths.
- Updated `test_settings_migration`: canonical profile save replaces writable
  legacy engine flags; mock supports the combined transaction record.

- Updated `test_rain_with_usb_and_display`: adds periodic display capability
  commands and a lost capability reply to the real UART/USB/rain scenario.
- Updated/re-registered `test_exhaust_profile_and_recovery`: preserves GPIO pulse
  timing, verifies equipment rejection and closing an already owned override.
- Updated `test_vehicle_feature_support`: confirmed RWD is Unsupported and Race
  Present cannot authorize suspension; distinct Absent suspension is asserted.

- New `test_split_profile_migration`: two real legacy pages (0x101 + 0x107),
  every interrupted write in initial 0x108 migration, restart and retry. Both
  original profile metadata and feature preferences must survive each cut.

Final repair validation: 50 selected executions PASS across four menu variants,
two BH widths, exhaust, UART and pure policy. New/updated cases only.
Arm GNU 15.2.Rel1: four lints/builds and 16 UI matrix combinations PASS;
size/identity gates PASS. Hardware and untouched regression suites not executed.

## PR 96 shared UART transition correction

`test_rain_test_with_shared_uart` is updated to validate the page source as well
as freshness after the preceding capability/USB traffic scenario. Run together
with the already modified `test_rain_with_usb_and_display`; do not execute the
untouched retry case or the broader host suite. Reproduced the CI failure at
4000 ms (BH no reply; 1735 queue errors) with the old one-second query period.
The shared five-second period now passes both scenarios with zero queue errors,
including capability traffic throughout Rain test. `test_runtime_display` also
checks the poll boundary and accepts a queued 1500 ms reply: PASS in four menu
variants with ASan/UBSan. Six selected executions total; no untouched cases run.

PR 96 run 40915444154 passed UART, then exposed fixture contamination in BH
display tests: the new width test preceded a sequential suite whose first display
case reads captured frame zero. Keep the original sequence intact and run the
new case last, advancing its clock instead of resetting time backwards. Selected
width case passes in both BH compile widths; CI validates the original sequence.

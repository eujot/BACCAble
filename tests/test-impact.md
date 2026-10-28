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

## Observed result

`python3 tests/run_vehicle_ui_tests.py --manifest tests/vehicle_ui_tests.json --run`
builds each manifest target with `/usr/bin/clang` (ASan/UBSan) and runs exactly
the selected cases. Result: **37 selected cases passed, 0 failed**. Untouched
regression suites and vehicle hardware were not run and are not claimed here.

# Vehicle profile audit and implementation proposal

Status: historical audit, 2026-10-28. No firmware changes.

**Execution instructions are superseded by the unified
[vehicle/UI implementation plan](VEHICLE_UI_IMPLEMENTATION_PLAN.md).**
Use its complete profile scope, migration/confirmation rules and scoped testing
contract. This audit retains source evidence, not a separate delivery sequence.
Baseline: fetched `origin/master` at `ba9176e` (PR #95). The user's main checkout
is the older, dirty `feat/rain-lights-debug`; its edits were preserved. The local
Rain test implementation `3f4f1fe` is a separate branch and does not centralize
vehicle settings. Existing engine-aware filtering is already present in the
reviewed master; this proposal extends it rather than recreating it.

## Later owner decision affecting UI compatibility

The owner subsequently authorized discarding old Readings/Favorite layouts.
The [joint implementation plan](VEHICLE_UI_IMPLEMENTATION_PLAN.md) supersedes
this audit's recommendations to retain old UI page IDs/order and migrate old
Favorite layouts. Vehicle/feature settings and measurement source definitions
remain protected. The audit below describes the inspected old implementation;
its historical facts are not new UI requirements.

## Conclusion

There is a shared settings object, but no shared vehicle profile or capability
policy. Engine, drivetrain and display flags live together with feature-enable
preferences; consumers interpret them independently. Year, model and installed
equipment are not represented. Centralize vehicle identity, derived display
properties, equipment eligibility and oil ranges, while keeping feature choices,
live telemetry and build/board capabilities distinct.

## Current ownership and consumers

All paths below are relative to `firmware/baccable/`.

| Concern | Current implementation | Consumers and limits |
| --- | --- | --- |
| Engine | `state/settings.h`: `is_diesel_enabled`, `gasoline_v6`; `settings/setup_entries.c`: visible `Engine` at Flash slot 16, hidden V6 bit at 36 | `setup_value_label()` / draft handling in `settings/setup_menu.c` expose 2.0 I4, 2.9 V6, 2.2 D. Diesel takes precedence over the companion bit; both bits being set is not necessarily corruption. |
| Drivetrain | `settings_state.drivetrain`, slot 40: Not set / RWD / Q4 | Production reads are in `features/oil_level.c`. It does not gate AWD actions or general readings. |
| Cluster | `ipc_my23_is_installed`, slot 6, visible `MY23 display` | `menu.c`, `setup_menu.c`, `menu_demo.c`, `oil_level.c`, `vehicle/shift_indicator.c`. No model-year lookup. |
| Build defaults | `settings/setup_entries.c`: `IS_DIESEL`, `IPC_MY23_IS_INSTALLED`; initial values also exist in `state/settings.c` | `powertrain_init()` loads Flash before normal use. Build definitions supply defaults, not vehicle detection. |
| Legacy display width | `app/build_config.h`: `LARGE_DISPLAY` selects 18/24 characters | Compile-time buffer/transport constraint, independent of the MY23 setting; not a vehicle year. |
| MY23 layout | `features/my23_ui.h`: 16 visible glyphs on line 1, 22 on line 2 | C1 renders; `features/body.c` on BH recognizes the packet marker and transmits the selected layout. BH does not obtain a full vehicle profile. |
| Reading catalogs | `diagnostics/parameter_catalog.c`, `features/menu_model.c` | Gasoline/diesel catalogs; I4/V6 share gasoline storage. `menu_page_supported()` gates only cylinder 5/6 ignition pages 0x2f/0x30 and MultiAir-related pages 0x14/0x40. |
| Visibility and Favorites | `menu_page_list_filtered()`, `favorite_parameters.c`, `menu.c` | Existing engine eligibility, user-hidden and advanced filters; incompatible saved Favorites are retained. No year/equipment policy. |
| Diagnostic requests | `diagnostics/parameter_request.c`, `features/parameter_navigation.c`, `dashboard.c` | Uses fuel catalog and current page. `parameter_request_begin_id()` validates the definition/request shape, not complete vehicle compatibility. |
| Engine-dependent behavior | `features/pedal_booster.c` | Diesel/gasoline changes RPM thresholds (3000/4000); a profile refactor must not affect only the menu. |
| Feature permissions | `settings/setup_entries.c`, `menu.c: available()` | AWD off, QV exhaust, HAS, etc. are user permissions. They do not establish installed equipment. |
| Execution conditions | `menu.c: action_unavailable()`, `setup_menu.c: setup_unavailable()` | Speed, active requests, brake/Dyno state, etc.; useful guards already exist and must remain. |
| Auxiliary configuration | `features/board_sync.c`, `app/board_commands.c` | Sends specific feature settings and startup requests, not a vehicle description. Do not add a full-profile broadcast just to filter a C1 menu. |
| Flash | `settings/persistence.c`, `settings/setup_menu.c/h` | Settings record 0x101, 42 uint16 slots; loader also accepts 41- and 40-slot payloads. Values load through descriptor validation. |

Useful source entry points: [settings](../../firmware/baccable/state/settings.h),
[entries](../../firmware/baccable/settings/setup_entries.c),
[setup editor](../../firmware/baccable/settings/setup_menu.c),
[menu policy](../../firmware/baccable/features/menu_model.c),
[actions and display](../../firmware/baccable/features/menu.c),
[oil renderer](../../firmware/baccable/features/oil_level.c),
[persistence](../../firmware/baccable/settings/persistence.c),
[board synchronization](../../firmware/baccable/features/board_sync.c).

## Gaps and consequences

1. **Equipment is not a permission.** An RWD selection does not prevent enabling
   `AWD off action`; `available(ACTION_AWD)` and `drivetrain_process()` check the
   permission/sequence, not Q4. QV exhaust and HAS have the same missing class of
   equipment gate. This is a code-policy gap, not evidence of a vehicle failure.
2. **Feature setup visibility is unconditional.** `setup_param_is_visible()`
   checks only that the descriptor has a label. Thus a gasoline profile can still
   browse the `DPF regen alert` setting. Diesel catalogs contain AdBlue pages,
   without a separate SCR equipment selection. Diesel alone is not sufficient
   evidence for all diesel equipment or all ECU DIDs.
3. **No year-to-display relationship.** Selecting an engine cannot select the
   cluster. Many UI sites read the MY23 boolean directly. The requested 2023+
   default should be a central policy, with an override for retrofits or an
   exceptional vehicle, not an unconditional hardware fact for every car.
4. **Profile-change handling is partial.** `menu_engine_changed()` cancels pending
   diagnostics, clears cache and rebuilds pages; the automatic trigger watches
   engine, V6 and Advanced pages. It is not a general vehicle-profile change event.
   Drivetrain is read live for oil limits, but there is no coordinated profile
   update for display transport, equipment, filtering and animation state.
5. **Visibility alone is insufficient.** Hiding a setting does not suppress an
   already enabled periodic function or startup request. Eligibility must also
   guard execution/synchronization. Stop/release/recovery actions must remain
   reachable even if configuration becomes incompatible.
6. **There is no automatic vehicle detection here.** No audited setting path
   reads VIN/model year/equipment into these fields. Diagnostic replies establish
   measurement availability, not the full physical equipment list.

## Oil as the first complete consumer

`oil_level.c: oil_level_range()` currently embeds this owner-supplied table:

| Engine | RWD | Q4 |
| --- | --- | --- |
| 2.0 I4 gasoline | 72–82 mm | 67–77 mm |
| 2.2 diesel | 61–65 mm | 79–84 mm |
| 2.9 V6 | Unknown | Unknown |

With drivetrain unset, the bar says `Set drivetrain`; V6 says `No oil range`.
The numeric reading uses `?` when no range is known. These are preferable to
inventing an OK classification. The ranges were not newly validated in this
audit and must not be automatically generalized to another model or engine.

Gasoline Oil level/bar use page IDs 0x31/0x67 and parameter 89; diesel uses
0x9a/0xbd and parameter 64. Keep the diagnostic definitions/scales and stable
page IDs. Both `dashboard.c` and `menu.c: format_favorite_page()` call the common
oil formatter. The target design must preserve two independent selectable pages
for the two Favorite lines. Numeric validity/freshness, vehicle range and visual
animation are separate responsibilities. An unknown range must not hide a valid
millimetre measurement or imply the sensor is absent.

## Proposed single source of truth

Introduce `vehicle/vehicle_profile.h/.c`, with a small fixed-size `VehicleProfile`
and ordinary C enums/functions. No heap, object framework, generic rule language
or duplicate writable copies of the effective profile.

Persisted facts/preferences in the vehicle profile:

- Model: Not set / Giulia / Stelvio. Use model-specific rules only where evidence
  exists; selecting a model does not validate all signals for it.
- Engine: Not set / 2.0 I4 / 2.2 D / 2.9 V6. Derive fuel and cylinder count;
  do not expose an additional independent Diesel checkbox.
- Model year: Not set or validated four-digit model year, not first-registration year.
- Drivetrain: Not set / RWD / Q4. Do not infer it solely from engine/power.
- Display selection: Auto / Legacy / MY23. In Auto, known model year >=2023
  selects the requested MY23 policy; older known years select Legacy. With year
  unknown, retain the migrated explicit display or the existing build default.
  Legacy width remains the build's 18/24 capacity in the first implementation.
- Equipment: Not set / No / Yes for adjustable suspension, Race support,
  exhaust valves, ACC, HAS, lane assist, compatible parking-mirror equipment,
  and SCR/AdBlue where those flags gate implemented features. Add flags only
  for an actual consumer. **Race must not be inferred solely from V6**: the
  owner-confirmed selector behavior is already recorded for this project.

Separate existing `SettingsState` user choices (enable Rain lights, default
mode, soft suspension, alerts, etc.) from equipment. Speed, ignition, ECU
responses and active requests stay live telemetry; C1/C2/BH/USB build support
stays a firmware capability. None of those becomes a guessed vehicle property.

Suggested narrow interfaces, with rules in the profile module:

- `vehicle_engine()` / `vehicle_catalog()` — normalized engine and existing
  gasoline/diesel catalog selection.
- `vehicle_display()` — resolved display class, visible line count/widths.
- `vehicle_supports(feature)` — Supported / Unsupported / Unknown equipment
  eligibility; does not grant execution permission.
- `vehicle_page_support(page_id)` / `vehicle_parameter_support(parameter_id)` —
  consistent eligibility for listings, Favorites, preview and diagnostic requests.
- `vehicle_oil_range(&minimum, &maximum)` — known range or explicit unavailable.

One small rule table keyed by stable page/parameter IDs can express catalog
exceptions without expanding every parameter definition. All consumers ask this
policy; they must not repeat `year >= 2023`, oil ranges or engine/Q4 combinations.

Visibility combines vehicle eligibility, the existing manual-hidden/advanced
preferences and build support. Execution additionally requires feature permission
and live safety/prerequisite checks. Keep known-unsupported entries out of normal
lists but retain their saved Favorite slots/preferences. Unknown passive readings
remain available with unknown applicability; unknown actuation-specific equipment
needs configuration before a newly confirmed profile can authorize it.

Follow-up: the [reading layout proposal](READING_LAYOUT_PROPOSAL.md) refines
Display into Auto / Legacy 18 / Legacy 24 / MY23 and describes the runtime
buffer/transport work needed to support both legacy widths in one firmware.

## Proposed menu, for review only

```text
Settings
  Vehicle
    Model             Giulia
    Model year        2023
    Engine            2.0 I4
    Drivetrain        Q4
    Display           Auto
    Equipment
      Suspension      Yes
      Race support    Yes
      Exhaust valves  Not set
      ACC             Yes
      HAS             Not set
      Lane assist     Yes
      Park mirrors    Not set
      SCR / AdBlue    No
  Features
  Favorites
  Shown pages
  Sort order
  BACCAble IPC
```

`Vehicle` owns the existing Engine/Drivetrain/display choices; do not leave
second writable copies under Features. BACCAble IPC keeps transport tuning,
not duplicate vehicle identity. Preserve the existing draft/HOLD-save/double-
BACK-discard contract. Treat Vehicle edits as a coherent draft and resolve the
new display after successful save, not halfway through editing its own menu.
Do not alter legacy Favorites navigation just to add Vehicle.

## Delivery sequence and acceptance gates

The task/backlog status belongs only in [ACTION_PLAN.md](../ACTION_PLAN.md).
The following is the proposed implementation sequence, not approved work.

1. **Central policy with no behavior changes.** Add profile accessors initially
   backed by existing slots/fields. Move the current I4/V6 gates, oil table and
   display resolution into that module. Cover the present combinations first;
   keep catalog order, IDs, Favorite records and current byte-level output.
2. **Canonical profile and migration.** Replace vehicle identity booleans with
   one normalized engine enum internally. Keep slots 16/36/40/6 as a serialization
   compatibility adapter, not a second mutable runtime profile. Append explicit
   versioned year/model/display-mode/equipment metadata; use an extended settings
   payload with old 42/41/40-slot import and existing atomic record protection.
   Do not reuse reserved slots or silently reinterpret slot 6. Legacy display
   must migrate to an explicit override so a guessed year cannot blank the menu.
   Preserve gasoline I4/V6 and diesel interpretation, drivetrain and preferences.
   Invalid new metadata stays Unknown. Downgrade behavior must be documented.
3. **Vehicle editor and one commit path.** Add the proposed section. Persist the
   whole draft atomically, roll back on failure, then invoke one profile-change
   handler to cancel old requests, clear affected cached values/peaks, reset oil
   animation, rebuild current lists and restart the display stream if necessary.
   Avoid applying new startup DNA/suspension actions mid-session. Prevent a
   profile change that would strand an active AWD/brake/exhaust operation.
4. **Use eligibility everywhere.** Connect Features, Actions, Readings, both
   Favorite lines/editors, rotation, previews and addressed diagnostic reads.
   Gate periodic work and outgoing board configuration as well as the UI.
   Explicit OFF/release must still reach an already active peer; no full vehicle
   profile broadcast is needed until a peer has a concrete local consumer.
   Existing installations first migrate without silently changing operation;
   activation of the stricter equipment policy follows explicit confirmation of
   the Vehicle profile. New installs with Unknown equipment do not enable
   equipment-dependent actuation. Review this transition policy before coding.
5. **Validate and document.** Host matrix: 2.0/2.2/2.9 × RWD/Q4/Unknown;
   pre-2023/2023+/Unknown × display override; equipment Yes/No/Unknown; each
   migration length; corrupt metadata; failed save and discard; stale diagnostic
   reply across I4/V6 change; incompatible Favorites preserved; both oil pages,
   boundaries, unavailable range and animation reset; no incompatible CAN output.
   Compile/lint C1/C2/BH/CAN and the legacy-width/MY23 matrix, measure Flash/RAM.
   Vehicle acceptance covers older and MY23 clusters, both Favorite lines,
   engine/Q4 oil ranges and startup/recovery behavior. Update both user guides
   in the implementation PR; they are intentionally unchanged for this proposal.

Recommended approval: this compact profile + explicit equipment + overridable
2023 display default, delivered in the sequence above. Do not add ECU/proxy
coding, automatic VIN inference or new diagnostic definitions to this refactor.

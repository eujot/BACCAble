# Width-aware readings and Favorite layouts

Proposal for review, 2026-10-28. No firmware changes or hardware acceptance.
Local baseline: master `ba9176e`, with the preceding
[vehicle-profile proposal](VEHICLE_PROFILE_AUDIT.md). Upstream was independently
cloned/read at `96a9f1f9ae254cbe7298674b85783a6d3ec606dc`.

## What the original code establishes

Upstream [compile_time_defines.h, lines 50–55](https://github.com/gaucho1978/BACCAble/blob/96a9f1f9ae254cbe7298674b85783a6d3ec606dc/firmware/ledsStripController/Core/Inc/compile_time_defines.h#L50-L55)
sets `DASHBOARD_MESSAGE_MAX_LENGTH` to **18** by default and **24** with
`LARGE_DISPLAY`. The [sample configuration, line 19](https://github.com/gaucho1978/BACCAble/blob/96a9f1f9ae254cbe7298674b85783a6d3ec606dc/firmware/ledsStripController/Core/Inc/user_config.h.sample#L19)
says the longer strings require adaptation and marks that option under development.
These are software text budgets, not a proven mapping of diagonal screen sizes.
The inspected headers and searches of EN/IT/PL DOCX manuals did not establish
which physical small IPC variant displays all 18 glyphs, or validate 24 glyphs
on every larger analog IPC. Do not claim universal 3.5/7-inch compatibility.

Our fork independently retains 18/24 in
[build_config.h](../../firmware/baccable/app/build_config.h), and defines MY23
as **16 + 22 visible glyphs** in
[my23_ui.h](../../firmware/baccable/features/my23_ui.h).
Treat legacy 18, legacy 24 and MY23 as three explicit profiles; legacy 18 is the
conservative starting profile for the older/small screen, pending an actual
end-to-end readability check on that hardware. The BACCAble-controlled legacy
reading area is one line even though the factory screen can contain other text.

## Confirmed current implementation gaps

- `menu.c: present_favorite()` passes widths 16 and 22 to
  `format_favorite_page()`. However, that width is consumed by the oil formatter
  only; other MY23 readings call `dashboard_format_my23_page()` without it.
- `features/dashboard_format.c` uses the same compact `my23_template()` for
  either line. Its underlying `dashboard_format_values()` also retains the
  compile-time 18/24 limit. Thus simply asking the final encoder for 22 glyphs
  does not produce a genuinely wider second-line reading. The oil renderer is
  already a useful example of explicitly accepting a target width.
- Favorites currently select **catalog pages**, including compound pages. The
  name `FavoriteParameters` and `atomic_favorites` are historical: version 2
  stores page IDs in its two visible slots. Five slots are serialized, but only
  the first two are currently edited/rendered; the others must not be casually
  reinterpreted as new user choices.
- `favorite_parameters.c` has full/compact/tiny label helpers and
  `favorite_parameters_render()`, but the current Favorites display path goes
  through `present_favorite()` and page formatting instead. Reusing that old
  renderer unmodified would not fix the live path. Its fallback extracts units
  and decimals from page strings; that is unsuitable as the authoritative model
  for new single-measurement composition.
- On legacy displays, `menu_present_view()` transmits only `first`; `second`
  is ignored. Yet `favorite_queries()` gathers diagnostics from both page slots.
  Compatibility must define how a logical second row remains reachable, and must
  avoid polling rows that are not being displayed.
- The existing page-Favorites poller already deduplicates up to eight diagnostic
  IDs, and the cache tracks freshness. Preserve these mechanisms rather than
  issuing an OBD request per render tick or per occurrence of the same reading.
- Readings also uses the next selected page as a MY23 second-line preview.
  The width-aware renderer must serve this path, not just custom Favorites.

Sources: [menu.c](../../firmware/baccable/features/menu.c),
[dashboard_format.c](../../firmware/baccable/features/dashboard_format.c),
[favorite_parameters.c](../../firmware/baccable/features/favorite_parameters.c),
[body.c](../../firmware/baccable/features/body.c).

## Recommended architecture

Separate three small concepts:

1. **Measurement**: stable ID, source, decoder/scale, unit, precision, freshness
   and vehicle eligibility. Keep existing diagnostic/native definitions and
   distinguish values such as native oil temperature versus ECU oil temperature.
2. **Reading presentation**: Full / Medium / Short labels plus a numeric or
   enumerated formatter. A specialized full-row widget can use the same
   measurement (e.g. oil bar). These are presentations, not duplicate DIDs.
3. **Favorite layout**: ordered items per logical row, each referencing a reading
   ID and preferred format. The effective display profile supplies row widths
   and how many rows can appear simultaneously.

Use one small presentation descriptor per reading with explicit labels/unit/
precision and reserved numeric width; one renderer takes `width` explicitly.
Do not create complete 16-, 18-, 22- and 24-column copies of every page. Shared
strings and fixed arrays are sufficient; no heap or general-purpose layout engine.

Example formats for the same oil-temperature measurement:

| Format | Example |
| --- | --- |
| Full | `Oil temperature 88°C` |
| Medium | `Oil temp 88°C` |
| Short | `O88°C` |

`Auto` selects among those formats to fit the row. The source, precision, unit
and validity do not change with style. Native/ECU distinctions must remain
recognizable in compact names; do not merge two distinct measurements into an
ambiguous abbreviation. A missing sample is `--`, never zero. Count visible
glyphs after mapping to supported display tokens, not UTF-8 bytes; unsupported
symbols need a defined ASCII fallback per display profile. `°` is one glyph,
while UTF-8 encoding and final CAN UCS-2 representation have different byte costs.

## Composition and stable fit

Retain six Favorite sets and initially allow up to **four items per logical
row**, two rows per set. This is a storage ceiling, not a promise that four
readings fit: ordinarily one or two readable values per row are preferable.
There is always at least one space between complete items. Full-row widgets
such as Oil level bar occupy the row exclusively and use its actual width.
Oil level with LOW/OK/HIGH remains a separate numeric presentation.

Offer Full / Medium / Short / Auto per item. Explicit formats are respected;
Auto deterministically shortens its labels until the complete row fits. Validate
using reserved numeric fields including sign, decimal places, unit, and longest
supported enum/status—not only the current value of 88. The resolved widths and
styles remain stable during ordinary updates (99→100 must not reshuffle labels).
If the shortest composition cannot fit, show a reason and disallow saving that
composition; let the user shorten, remove or move an item. Never silently cut a
number/unit, drop a reading, change its precision or scroll the text. Unexpected
numeric overflow uses a bounded explicit overflow indication, not truncated digits.

Character-budget examples, not promises of physical font/pixel validation:

```text
MY23, row 1 (16):  Oil temp 88°C
MY23, row 2 (22):  Oil 88°C Coolant 87°C
Legacy 18:        Oil88°C Wtr87°C
Legacy 24:        Oil 88°C Coolant 87°C
```

Longer oil-temperature wording becomes available automatically on the 22-column
row when displayed alone. Empty space is acceptable; do not pad with more data
just to fill all columns. Show the budget and a realistic preview in the editor,
including a worst-case preview for long values/statuses.

## Editor and compatibility behavior

Proposed user flow (labels illustrative):

```text
Settings → Favorites → Favorite 1
  Line 1
    Item 1 → Oil temperature
      Format → Medium
    Add reading
    Preview
  Line 2
    Item 1 → Coolant temperature
      Format → Auto
    Item 2 → Gearbox temperature
      Format → Short
    Add reading
    Preview
  Save
```

Adding an item filters it through the shared vehicle profile. Removing/reordering
items and moving one to the other line does not change its underlying data source.
An explicit Save entry may reuse HOLD-save; retain the existing draft, double-
BACK confirmation/discard, timeout and Flash-failure rollback contract. The editor
must itself remain usable as a sequence of single-line screens on legacy IPCs.

On MY23 both logical rows appear together. On legacy 18/24 they become two
**manually navigated subpages** of the same Favorite; skip an empty second row.
Up/down visits Favorite 1/A, 1/B, 2/A, etc.; do not introduce automatic scrolling
or silently discard the second row. Preserve the saved logical layout when
switching displays. If a row authored at 22/24 cannot fit 18 even in compact form,
render it as deterministic continuation subpages of complete items without
rewriting the saved layout, and explain that extra pages are being shown.
A single item that cannot fit the smallest supported profile must fail validation
or use its documented bounded overflow form. Poll only the currently visible
items; MY23 polls both visible rows, legacy only its current subpage.

For a layout being edited on the current display, prefer explicit move/remove
and a fit error over silently creating continuations. Continuations are the
compatibility fallback for already saved layouts from a wider display.

## Keep existing compound pages as presets

Do not delete the existing multi-parameter Readings or their IDs in the first
implementation. They are saved in Favorites and visibility/order records.

- Keep useful Readings as ready-made presets; use the new formatter for both
  individual and compound views, with the actual line width.
- New custom Favorites compose individual values directly. An existing compound
  page can be imported as a preset row, preserving its meaningful order and
  source identity. Remove repeated legacy parameter IDs used only for storage
  padding; do not merge genuinely different readings.
- Keep both oil presentations selectable; do not replace a bar by its numeric
  source during migration. Preserve engine-incompatible saved items so they can
  reappear with the appropriate vehicle profile.
- Removing redundant catalog pages later is a separate UX decision with explicit
  migration, not a prerequisite for the new editor.

## Integration with the vehicle profile

Refine the earlier proposal's Display choices to
`Auto / Legacy 18 / Legacy 24 / MY23` (or equally clear user-facing labels).
Auto uses the requested 2023+ MY23 default; the year cannot decide which older
screen is physically fitted. Preserve an explicit selection and migrate the
existing `LARGE_DISPLAY` build width plus saved MY23 flag correctly.

A single running firmware supporting both legacy widths requires more than a
menu enum: currently buffers, copy lengths and the C1→BH legacy contract depend
on `DASHBOARD_MESSAGE_MAX_LENGTH`. The target is fixed maximum-size buffers and
runtime visible width, with an explicit compatible display-mode/length contract
on both C1 and BH. Preserve existing radio/source ownership, CAN cadence and
fragment coalescing. Unsupported/mismatched peer firmware must not silently
interpret a 24-character packet as an 18-character screen. Keep the existing
build matrix until runtime compatibility is validated on all paths.

## Suggested delivery and checks

1. Add explicit output width to the real Readings/Favorites formatter and fix
   MY23's second line while retaining page IDs and the current editor. This gives
   an independently reviewable improvement before changing storage or gestures.
2. Add the shared measurement-presentation metadata and line composer. Reuse
   the vehicle eligibility API, cache, UDS deduplication and oil range provider.
3. Add versioned custom-row storage/editor, with import of the current 0x105
   Favorites version 2 page IDs and historical version 1 atomic IDs/older 0x104
   preferences. Preserve current hidden historical slots explicitly; don't guess
   their meaning from their position. Failed migration/save must leave old Flash
   readable. Do not overwrite stored pages just by opening the editor.
4. Integrate runtime Legacy18/Legacy24/MY23 profile selection and legacy subpages.
   Update both user guides when the implementation changes operation.
5. Validate widths 16/18/22/24, UTF-8 glyphs, negative and large numbers, units,
   missing/stale samples, enums, full-row oil widgets, profile changes, all record
   migrations and draft rollback. Assert OBD polling only for visible unique
   measurements and unchanged CAN/UI scheduling; measure all builds and memory.

With one byte for reading ID and one for style, 4 items × 2 rows × 6 sets × 2
fuel catalog banks needs **192 bytes** for layout items, versus today's 60 item
bytes, before headers/draft metadata. This is a design estimate, not measured
firmware growth. Shared Flash strings/formatting and removal of redundant live
formatting paths matter more than clever bit-packing. Confirm the actual Flash,
RAM and record capacity before accepting that ceiling; start with three items
per row if the complete memory budget cannot accommodate four.

Owner review requested: composable individual values with ready-made presets,
Auto plus explicit presentation formats, and manual legacy subpages preserving
the second logical row. Implementation is not authorized by this document alone.

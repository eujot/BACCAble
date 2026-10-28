# Width-aware reading layout: current direction

Updated 2026-10-28 after explicit owner feedback. The owner wants **new individual
Readings and new default Favorites**, and does not want old compound Readings or
Favorite layouts preserved. Earlier proposals to retain those pages as presets
or migrate their saved compositions are superseded.

The authoritative next-step document is the
[joint technical specification](VEHICLE_UI_IMPLEMENTATION_PLAN.md).
It includes product rules, dependency order, concrete files, default layouts,
one-time UI reset, exact test replacement obligations and acceptance commands.
No firmware has been changed by these planning commits.

## Display evidence retained

Original gaucho1978 code at `96a9f1f9ae254cbe7298674b85783a6d3ec606dc` sets
[18/default and 24/LARGE_DISPLAY](https://github.com/gaucho1978/BACCAble/blob/96a9f1f9ae254cbe7298674b85783a6d3ec606dc/firmware/ledsStripController/Core/Inc/compile_time_defines.h#L50-L55).
Its [sample configuration](https://github.com/gaucho1978/BACCAble/blob/96a9f1f9ae254cbe7298674b85783a6d3ec606dc/firmware/ledsStripController/Core/Inc/user_config.h.sample#L19)
marks the longer-string option unfinished. Inspection of code and EN/IT/PL DOCX
manuals did not establish a universal physical small/large diagonal mapping.
Our fork's MY23 fields are 16 and 22 glyphs. Physical acceptance remains necessary.

The current `menu.c: format_favorite_page()` passes width to Oil formatting but
not the ordinary MY23 page formatter; its second line therefore retains compact
page formatting. `favorite_parameters_render()` is not the live page-Favorites
path. Legacy rendering uses the first line while the current query collector can
still collect both slots. These findings guide replacement, not an independent
project to polish retired compound templates.

## Approved direction for the plan

Individual source-backed Readings are grouped by purpose. Composed Favorites
select individual values and Auto/Full/Medium/Short formats per row; useful
engine-aware presets initialize the new layout once. Oil bar remains an exclusive
full-row presentation of one measurement. Legacy profiles use manual subpages;
MY23 shows both rows. A shared vehicle profile determines widths and eligibility.

Old UI layout/visibility/order records reset deliberately; vehicle/feature settings,
measurement definitions/scales and statistics remain intact. Existing test
expectations for old pages and Favorite migration must change with the code,
while decoding, storage reliability, gestures and transport regression coverage
remain. See the implementation plan for the replacement map and staged gates.

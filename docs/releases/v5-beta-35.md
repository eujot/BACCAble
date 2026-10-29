# BACCAble v5-beta-35

This beta replaces the compound Readings pages and the page-based Favorites with
individual measurements, and adds a Vehicle profile that drives reading
eligibility and runtime display selection. It also makes settings persistence
atomic so a profile or UI save cannot silently lose model, year, equipment or
Favorites edits.

## Included changes

### Vehicle profile

- `Vehicle` is the last main-menu entry. It edits model (`Unknown`/`Giulia`/
  `Stelvio`), year, engine (`Unknown`/`2.0 Petrol`/`2.9 Petrol`/`2.2 Diesel`),
  drivetrain (`Unknown`/`RWD`/`Q4`) and display selection.
- Display `Auto` selects MY23 for a known year >= 2023, otherwise the legacy
  width. Explicit 18/24/MY23 choices override the year. A new width activates
  only after BH confirms protocol support; `Saved; IPC pending` keeps the
  previous working transport until then.
- Equipment uses `Unknown`/`Absent`/`Present` for Suspension, Race, Exhaust
  valves, ACC, HAS, Lane assist, Park mirrors and SCR. An imported installation
  keeps its previous permissions until the profile is explicitly saved; known
  absent equipment hides its feature option, and unknown equipment cannot
  authorize a new equipment-dependent action. AWD requests require Q4.
- AWD, brake, Dyno, exhaust and parking-mirror operations must be finished
  before a profile save; the editor reports the operation to finish. Saving
  never replays startup DNA or suspension.

### Readings

- Readings are now individual measurements grouped as Oil, Temperatures,
  Engine, Fuel, Battery, Diesel / DPF, Driving, Performance and Diagnostics.
  Groups with no eligible reading are omitted.
- Each reading has an explicit label, unit, precision and reserved width.
  Missing, stale, unsupported or invalid data shows `--`, never zero.
- A reading is polled once even when it appears in several visible Favorites.
  MY23 shows one selected measurement; the old next-reading preview is gone.

### Favorites

- Six sets, each with two rows (A and B) of up to four items. Each item stores
  one reading and a format: Auto, Full, Medium or Short. Auto chooses the most
  readable fitting label once when the layout resolves.
- A row joins items with at least one space and never clips digits; unexpected
  numeric overflow uses `#` in that field. `Duplicate reading`,
  `Oil bar needs row` and `Too wide: shorten` reject a save and keep the draft.
- MY23 shows both rows; legacy navigates the non-empty rows. A saved wider row
  becomes deterministic continuation subpages on a narrower screen without
  rewriting the stored layout.
- `Settings → Favorites → Restore defaults` replaces the six layouts after
  confirmation and preserves settings, profile, statistics and mirror
  calibration.
- The oil level bar is a full-width presentation of one measurement and
  occupies its row alone. Owner-supplied OK ranges are unchanged and are not
  newly verified here.

### Settings migration

- Settings now persist atomically: one `0x108` transaction carries all 42
  feature slots plus the complete 17-byte profile, including explicit
  imported-permission provenance. Legacy `0x101` and experimental `0x107`
  records are import-only.
- A first start resets the old UI layout once into the new format and loads
  engine-aware Favorite defaults. Vehicle/feature settings, statistics and
  mirror calibration are not reset. Old page IDs and the removed
  `Page favorites`, `Shown pages`, `Favorite order` and `Sort order` editors
  are intentionally not retained.
- Failed writes keep the previous live profile/draft and report `Save failed`;
  interrupted halfword writes and partial slot writes are covered.

### Communication and provenance

- Display capability queries now repeat every five seconds with a three-second
  reply window, leaving UART capacity for diagnostics and status traffic.
- The machine-readable CAN dictionary and the generated readable catalog now
  track the post-cutover firmware source snapshot. No frame, field, conversion
  or evidence claim changed.

## Build and verification

The host suite, Lab tests, lint and C1/C2/BH/CAN firmware builds passed in
[release workflow 40917052165](https://github.com/eujot/BACCAble/actions/runs/40917052165)
with the pinned Arm GNU 15.3.Rel1 toolchain. All 14 assets downloaded from the
[v5-beta-35 release](https://github.com/eujot/BACCAble/releases/tag/v5-beta-35)
passed the included `SHA256SUMS` check. C1 uses 92,672/98,304 Flash bytes and
13,316/16,384 static RAM bytes.

No board was flashed for this change. Vehicle display/operation acceptance,
including the new Vehicle editor, individual Readings/Favorites rendering and
the oil bar, remains open.

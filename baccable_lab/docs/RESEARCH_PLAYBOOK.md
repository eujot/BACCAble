# Bundle-first research playbook (for an AI analyst)

This is the repeatable procedure for turning a `baccable bundle` (plus the raw
sessions behind it) into named CAN knowledge with the **fewest new recordings
possible**. It complements [SESSION_ANALYSIS.md](SESSION_ANALYSIS.md): that file
holds the deep rules; this file holds the efficient, ordered checklist an AI
should follow on every review.

Read `AGENTS.md` and `docs/ACTION_PLAN.md` first. Analysis is **read-only** and
never authorises CAN transmission or actuator behaviour.

## 1. Where frames are already described (the knowledge map)

| Artifact | What it holds | Treat as |
| --- | --- | --- |
| `baccable_lab/knowledge/giulia.json` → `messages[]` | Curated, bus-specific layouts: signals, parts, scale/offset, origin, enum, guards | **Source of truth**; only add a field after the evidence bar is met |
| `…/giulia.json` → `observed_inventory[]`, `frame_templates[]` | Every observed bus/ID/DLC shape and empirical stable-bit templates | Inventory/baseline, **not** meaning |
| `…/giulia.json` → `marker_index[]` | Links markers ↔ candidate fields ↔ session/event IDs | Prior evidence to extend, not to overwrite |
| `…/giulia.json` → `research_candidates[]` | Short topic-level pointers to open questions | Where a **new frame** is parked before it is a decoder |
| `…/giulia.json` → `session_evidence{}` | The immutable review ledger reference + hashes | Updated only via `scripts/attach_dictionary_evidence.py` |
| `baccable_lab/knowledge/hypotheses.json` | Claims needing validation, with status, contradictions, next test | Where every **candidate bit/frame** goes |
| `baccable_lab/docs/CAN_DICTIONARY.md` | Human-readable catalog | **Generated** from `giulia.json`; never edit by hand |
| `baccable_lab/docs/research/<date>-*.md` | Dated narrative findings (baseline, campaigns) | The **story/evidence**; append, never rewrite history |
| `firmware/baccable/vehicle/*.c` | Source provenance (`implementation`/`comment`) | A separate attribute; implement ≠ physically validated |

Rule of thumb: **one meaning, one place.** A repeated, reversible, controlled
signal becomes a field in `messages[]`; everything weaker stays a hypothesis.

## 2. Where new findings go (routing table)

| You found | Write it to | Also |
| --- | --- | --- |
| A frame/bit that repeats the same action ≥3 cycles, reversible, with negative controls | `giulia.json` `messages[]` as a new signal (origin `capture`) | bump `dictionary_version`, re-render, add a `manual_marker_evidence` note |
| A `candidate` bit or a whole new frame you cannot yet promote | `hypotheses.json` (new unique `id`, `status: candidate`) | list it in `giulia.json` `research_candidates[]` |
| A frame proven to carry no state (counter/CRC heartbeat) | `hypotheses.json` (`status: supported`) | add a `research_candidates[]` warning "exclude these bytes" |
| An update to an existing frame's evidence | the matching signal's `manual_marker_evidence` + `session_evidence` via the attach script | refresh the dated findings doc |
| A refutation or contradiction | the hypothesis `contradictions[]`; keep the old claim | note which evidence superseded it |
| Narrative, comparisons, next-test design | `docs/research/<date>-<topic>.md` | link it from the dated review and the action plan |

Never paste hundreds of `candidate_bit_steps` into `messages[]`. Curate.

## 3. The evidence ladder (do not skip rungs)

`unresolved → candidate → supported → validated` (and `refuted`/`superseded`).

- **candidate** — a bit moves near a marker; nothing asserted.
- **supported** — ≥3 clean ON/OFF cycles, same direction, reversible, with at
  least one negative control and independent-session support.
- **validated** — physical/independent confirmation (screen observation, a second
  measurement path) and explicit length/invalid-value rules.
- **refuted** — a contrary capture with reviewed labelling and context.

A code implementation is a **separate provenance attribute**, not validation.

## 4. Efficient, ordered review (maximise reuse, minimise new tests)

Do these in order. Steps 2–4 are what prevent wasted test recordings.

### Step 0 — Inventory before interpreting
List finished sessions and their input hashes. Decide **overlap**: do the new
sessions repeat the same marker **labels** as earlier ones, or are they
complementary? Repeated labels give cross-session support for free; disjoint
campaigns (as in `20261010-104325Z` / `20261010-111025Z`) do not — note it
instead of pretending otherwise.

### Step 1 — Produce the bundle once per session
```sh
baccable bundle <SESSION> --output <file>.json     # context, health, coverage,
# candidates, shape_hints, excerpts, matched scenario definitions, per-step notes
```
Also run `baccable review --sessions sessions -o <report>.json` when the session
evidence ledger must be refreshed (feeds the attach script).

### Step 2 — Classify frame structure **before** trusting any candidate
The bundle ranking is polluted by counter/checksum frames. For every frame, from
raw payloads:
1. Last byte identical across all samples → **checksum** (see `shape_hints`
   `last_byte_crc8_j1850`). Ignore it.
2. A byte cycling `0,1,…,N-1` with a fixed step, then repeating → **counter**
   (commonly a 4-bit nibble in the second-to-last byte). Ignore it.
3. Fields that only increase/decrease over long spans → **accumulator** (suspect;
   its low bits drift and correlate with ordered actions). Downgrade.
4. Constant prefix bytes → framing, not signal.
5. What remains, that jumps and settles, is **candidate state**.

Discard candidates whose changing bytes are all counter/checksum/framing.
This single step removes most false positives (e.g. `0x4AE`, `0x15A`,
`0x5A8` bytes 6–7) and usually removes the need to re-record anything.

### Step 3 — Correlate by **state interval**, not a fixed before/after window
Markers are confirmed **after** the action, so `[t-Δ, t]` is already the new
state. Instead, using the existing `runs = 3`:
- build ON intervals `[t_on, t_off]` and OFF intervals `[t_off, next t_on]`;
- per bit, compute occupancy in ON vs OFF; keep bits with `ON ≥ 0.90 and
  OFF ≤ 0.10`, or the exact inverse (active-low);
- for blinking features (indicators, hazard) the occupancy sits near 0.5 in ON —
  detect them by rate/variance, not by level.

This reuses every repetition already recorded; no extra take is needed for the
opposite state.

### Step 4 — Cross-session and confounder filter
- Intersect candidates by `(role, can_id, byte, bit, label)` across sessions.
  Same-label agreement across sessions is the strongest free evidence.
- Drop a bit that is a candidate for **many unrelated labels** or that behaves as
  a counter/accumulator: it is a broadcast/status or heartbeat confounder.
- Remember: the same ID on C1 and C2 is not the same meaning; a **report is not a
  command**; gateway-duplicated payloads (identical on two buses) are one signal.

### Step 5 — Use the negative controls you already have
Quiet baseline windows, other scenarios, and the OFF half of every pair act as
controls. Only when a feature was **never exercised** (or a confounder cannot be
isolated from existing data) design a **minimal** new recording.

### Step 6 — Write, version, re-render, test
Route each finding per §2. Bump `dictionary_version` when `messages[]` or evidence
change, then:
```sh
python scripts/attach_dictionary_evidence.py docs/research/<date>-session-review.json
python scripts/render_can_dictionary.py
python -m unittest discover -s tests -q
```
`docs/CAN_DICTIONARY.md` must equal `render(giulia.json)` (a test enforces it).
Hypothesis evidence may only cite events present in the referenced report.

## 5. When a new recording is actually justified (and how small)

Only these justify a new capture — keep each to the smallest change:

1. **Promotion to `validated`** — needs physical/independent confirmation.
2. **Isolation of a confounder** — change exactly one control, hold everything
   else steady; mark press/release/completed state separately.
3. **A never-exercised feature** (e.g. rain sensor alone; steady lamps with long
   equal intervals) where existing data cannot discriminate.
4. **Active-low / inversion sense** on a new closure/switch field.

Prefer re-running a **small stationary subset in a fresh session** (reset progress
or force `capture --scenario ID`) so the same labels recur and reach the ladder,
instead of recording the whole library again.

## 6. Anti-patterns that waste tests and corrupt knowledge

- Trusting a bit before excluding counter/CRC bytes (the `0x4AE` trap).
- Using `[t-Δ, t]` as "before" when the marker follows the action.
- Empty-`expect` steps whose label is the action text (e.g. "Zwolnij pedał
  hamulca") — give boundary steps a canonical negative label (`brake_off`,
  `wipers_off`, `trunk_close`) so rankings stay comparable.
- Naming a function from a single ON/OFF pair, or from a high correlation alone.
- Subtracting device clocks across different boards (C1 vs BH).
- Treating diagnostic (`0x18DA…`) transport or counter bits as signals.
- Overwriting a previous meaning because one dataset disagrees; review labelling
  and applicability first.

## 7. Outputs of every review

1. Updated `giulia.json` (fields and/or `research_candidates`), bumped version.
2. Updated `hypotheses.json` (new/updated claims with contradictions and next test).
3. A dated `docs/research/<date>-*.md` narrative: confirmations, new candidates,
   refutations, missing coverage, and the **smallest** next recording.
4. Regenerated `docs/CAN_DICTIONARY.md`.
5. An entry in the root `docs/ACTION_PLAN.md`; CI tests green.

## 8. Command cheat-sheet

```sh
baccable sessions                                             # list sessions
baccable bundle <SESSION> --output <file>.json               # one analysis artifact
baccable review --sessions sessions -o <report>.json         # evidence ledger
baccable dictionary --role C1 --id 0x116                     # current layout
baccable dictionary --role C1 --id 0x116 --data '00 00 00 00 00 00 05 94'
baccable dictionary --propose --role C1 --id 0x116 --name closure --parts '2,2,1' --unit raw
baccable scenarios                                            # plan the next subset
```

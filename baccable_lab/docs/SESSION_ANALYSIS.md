# Agent workflow for CAN session analysis

Use this guide whenever a task asks to inspect a capture, explain a marker,
identify a signal, or extend the BACCAble CAN dictionary. Read `AGENTS.md` and
`docs/ACTION_PLAN.md` first. The goal is an accumulating, evidence-backed
**frame and signal catalog**, with readable descriptions and reproducible
annotations for the next capture. A collection of plausible names is not enough.

## Inputs and persistent outputs

| Artifact | Purpose |
| --- | --- |
| `sessions/<session>/manifest.json`, `summary.json`, `<ROLE>.bin`, `session.sqlite3` | Original recordings; never edit them during analysis |
| `baccable_lab/knowledge/giulia.json` | Curated bus-specific layouts, conversion rules, code provenance, marker links and empirical templates |
| `docs/CAN_DICTIONARY.md` | Human-readable catalog generated from the dictionary |
| `docs/research/20260928-session-review.json` | Initial reproducible evidence ledger; create a dated report for a later batch |
| `docs/research/20260928-findings.md` | Initial investigation, contradictions and candidate experiments |
| `baccable_lab/knowledge/hypotheses.json` | Research claims requiring validation; these are not executable decoders |
| `docs/ACTION_PLAN.md` in the repository root | Integration status and remaining work; do not create another backlog |

Paths in this table are relative to `baccable_lab/`, except the action plan.
Preserve previous reports and evidence references when adding another batch.
Never change historical markers to make them agree with a hypothesis. Capture
metadata can be incomplete: explicitly retain unknown firmware, model year,
engine, recording conditions and audio source instead of guessing them.

## 1. Inventory and qualify the recordings

1. Inspect Git state. Preserve unrelated changes and work on a task branch.
2. Find actual databases using `rg --files --hidden --no-ignore sessions`
   because captures are ignored by Git. Enumerate all sessions and distinguish
   new inputs from previously reviewed inputs by session ID **and file hashes**.
3. Read the manifest and summary. Record selected buses, status, duration,
   frame counts, parser resynchronizations, malformed records, discarded bytes,
   reader errors, startup loss and later loss. A failed recording can contain
   useful data; mark its truncation and do not use it as a clean baseline.
4. Verify raw streams and SQLite counts where needed with the existing binary
   parser. Keep the original loss records. Saturated `65535` counts are lower
   bounds, and zero reported later loss is not proof of an error-free capture.
5. Analyze finished recordings through SQLite `mode=ro`; include committed WAL
   data. Do not set `immutable=1` when a nonempty WAL exists. Do not checkpoint,
   repair, create indexes or add events in the original database. Use a scratch
   copy for an exploratory index. Verify fingerprints before and after.

```sh
# Run from baccable_lab/ in its existing environment.
baccable review --sessions sessions --output /tmp/can-review-NEW-DATE.json
```

The command refuses an existing report path and paths inside the capture root.
It reads all finished sessions under that root, inventories their frames,
compares known rules with manual markers, profiles stable/variable bits and
lists exploratory bit steps. An ongoing recording must be stopped first.
Its report identifies input hashes, source-code SHA and dictionary version.
Keep the new report as an immutable ledger for this analysis iteration.

## 2. Establish what is already known

Load `giulia.json`, earlier findings and `hypotheses.json` before searching.
Match **role + numeric CAN ID + DLC** for frame profiles; signals remain scoped
to role and ID, with explicit layout/length guards. C1, C2 and BH are distinct
networks. Matching IDs on two buses do not prove matching payload meanings or
ECU identity. The current binary recording lacks IDE/RTR and TX/RX flags, and
RX observations do not record BACCAble's own transmissions.

Compare new shapes, periods, field ranges and positive diagnostic DIDs against
older sessions. Separate a new identifier from a newly active bit in an old
identifier. Apply existing layouts first. Retain an unknown enum value as raw;
do not silently map it to OFF or the closest recognized state.

```sh
baccable dictionary --role BH --id 0x46C
baccable dictionary --role BH --id 0x46C --data '00 00 00 00 00 00 04 00'
baccable session decode SESSION --role BH --id 0x46C --limit 20
```

`session decode` emits JSON lines containing original host/device timestamps,
raw payload, named fields and an empirical baseline match. A baseline match
means that previously stable bits agree; it does not identify an unknown
function. A mismatch is an investigation target, not a malfunction verdict.

## 3. Build an honest marker timeline

Export every event with original ID, `host_ns`, label, note and source. Separate:

- manual actions (`keyboard`, `cli`);
- CAN-derived annotations (`analysis:...`), which cannot independently confirm
  the decoder that generated them;
- unknown provenance, custom text and ambiguous actions.

Normalize text only in a **separate interpretation record**: original label,
proposed canonical label, affected component, reason, confidence and any
ambiguity. Preserve typos. A marker saying “window opened” does not establish
which door, movement direction, button action or final position. A note naming
another window can describe a simultaneous action and prevent isolation.
An unlabeled custom marker or `jade` (“driving”) is context, not a precise event.

Use host time for a cross-bus neighborhood, with uncertainty for USB batching.
Use unwrapped device time and sequence for intervals within one board. Never
subtract C1's device clock from BH's device clock. Human markers can precede
or follow the action: test neighboring windows and record the offset that was
used; do not shift timestamps until a claim appears correct.

## 4. Search around each action and its opposite

1. Begin with the relevant existing decoder and source comments. Record exact
   bit positions, guards, formulas and active consumers. A comment alone is
   weaker than implemented decoding, and neither substitutes for vehicle proof.
2. Inspect a wide neighborhood (usually at least 5 s before/after), then narrow
   around actual transitions. Check all occurrences of the action, its release
   or opposite, and repetitions. Inspect gaps and loss around each occurrence.
3. Compare a stable pre-action interval with a settled post-action interval.
   The current automatic screen uses `[-5,-3]` and `[-0.5,+1.5]` seconds, at
   least three samples per phase, and a bit occupancy change from <=5% to
   >=95% or the opposite. These are **search heuristics**, not acceptance rules:
   a delayed parking-brake state can be missed, and close actions can overlap.
4. Examine individual bits, small packed fields, raw bytes, signed/unsigned
   values and endian variants. Preserve raw bytes alongside every proposed
   conversion. Consider rate changes, newly appearing/disappearing IDs and
   multi-frame sequences, not just payload differences.
5. Exclude rolling counters and checksum fields from semantic matching. Test
   counter modulus and cadence over long baseline intervals. Verify checksum
   hypotheses against held-out payloads; do not call every changing last byte a
   checksum. Distinguish periodic status reports from brief requests, pulses,
   button holds and completed actuator movement.
6. Use negative controls: unrelated markers, quiet baseline windows, the other
   side of a control and repeated actions without the suspected co-trigger.
   A bit that changes near most unrelated actions is a confounder. Ignition,
   brake-pedal use, vehicle motion, battery voltage and illumination can change
   alongside the target action. Adjacent markers are not independent trials.
7. Rank candidates by repeated directional agreement, reversibility, temporal
   consistency, low baseline activity and independent-session support. Record
   disagreements and missing evidence. Exhaustive bit searches create many
   false positives; a high correlation is not sufficient to name a function.

Do not prescribe one universal analysis method. Inspect raw sequences whenever
an automated heuristic misses a known transition or promotes a counter. A
negative result means “not demonstrated in this experiment,” not “does not exist.”

## 5. Analyze compound protocols explicitly

For IPC `BH 0x090`, reconstruct **ordered whole messages**: last fragment index,
fragment number, six-bit source context and three big-endian 16-bit code units
per frame. Reject inconsistent source/length, missing/out-of-order fragments
and truncated frames. Keep controls separate from text, line separator `0x000D`
separate from padding, and transport capacity separate from visible capacity.
A text context is not a guaranteed current source selector. A 16-bit transport
does not prove that the IPC supports all Unicode glyphs or surrogate pairs.

Correlate factory transfers with track markers and button holds, but inspect
silence too. RX cannot prove menu ownership, rendering, or BACCAble retransmission.
A prior radio transfer can leave persistent content without continuous traffic.
Do not claim an IPC fix solely from this dump; state what TX or screen observation
would distinguish the remaining hypotheses.

For diagnostics, match bus, response ID, service and DID, not ID alone. Decode
only complete positive `0x62` single frames that pass the firmware bounds rules.
Do not read negative responses or ISO-TP fragments as sensor values. Record
unsupported multi-frame replies as unresolved transport. Distinguish native
IBS SOC from ECM SOC and BCM SOC, and distinguish multiple catalog aliases of
one DID from multiple independent responses. No arbitrary diagnostic queries
or CAN injection are part of offline analysis.

## 6. Write a frame description and retain the claim's history

Each named field or research hypothesis needs:

- vehicle/session scope, role, numeric ID and supported DLC;
- byte/bit layout (zero-based byte indexes, LSB bit 0), field width, ordering,
  signedness, units, scale/offset, enum codes and unavailable values if proven;
- message behavior: request/status/pulse/fragment, cadence, counter/checksum
  evidence and conditional meaning;
- code source path/symbol and hash, if any;
- session IDs, original event IDs, exact time windows, raw examples or artifact
  references, matching/opposite transitions and negative controls;
- evidence origin and status: `unresolved`, `candidate`, `supported`, `validated`, `refuted`
  or `superseded`; a code implementation is a separate provenance attribute;
- contradictions, alternatives, unknowns and the next discriminating experiment.

A reasonable default for `supported` is at least three clean ON/OFF cycles
with the same direction and controls. Promote to `validated` only with physical
confirmation, independent recordings under relevant conditions and explicit
length/invalid-value rules. Sparse data may justify a candidate but not promotion.
No numeric confidence score should conceal missing evidence. Preserve refuted
claims and record which evidence superseded them. Do not overwrite a previous
meaning based on one contrary capture: examine labeling, context and applicability.

Keep uncertain claims in `hypotheses.json` or findings. Do not automatically
turn the hundreds of `candidate_bit_steps` into executable signal names. Add a
curated field to `giulia.json` only after reviewing its layout and meaning;
keep historical-comment fields labeled `comment`. A candidate derived only
from a new capture must retain that provenance and must not be labeled as a
current firmware implementation.

## 7. Integrate knowledge and prepare the next iteration

Add new evidence to existing identities instead of duplicating names. Refresh
marker links, per-session observation counts, known diagnostic responses,
empirical stable-bit templates and explicit contradictions. Learn templates
from complete runs; keep failed-run observations available separately. Stable
bits are empirical within these recordings, not a guarantee for all vehicles,
ignition states or future firmware. Extended diagnostic traffic is not an
ordinary fixed-payload template.

The evidence attachment script refreshes aggregates and preserves curated
findings; choose the dated report path explicitly:

Use a cumulative report including previously reviewed sessions. The script
refuses a report that would discard earlier session evidence. If old raw
recordings have been archived, assemble a compatible cumulative ledger from
their preserved reports; check definitions, input revisions and provenance
before reusing interpretations. Never replace the knowledge corpus with just
the newest recording's matches.

```sh
python scripts/attach_dictionary_evidence.py docs/research/NEW-DATE-session-review.json
python scripts/render_can_dictionary.py
python -m unittest discover -s tests -q
```

Version the dictionary when changing layouts or evidence. Preserve earlier
reports. Run schema, bit-layout, role isolation, length, diagnostic bounds,
marker provenance, template and source-snapshot regression checks. Add an
independent fixture for a genuinely new decoder. Use old sessions as regression
inputs; new data must not silently invalidate old meanings. Explain intentional
source changes and refresh source hashes after reviewing their effect.

Deliver the updated Markdown catalog, machine-readable dictionary, a dated
findings report, references to raw evidence, unresolved hypotheses and a short
next recording scenario. The findings should distinguish existing confirmations,
new candidates, refutations, missing coverage and transport issues. Update the
root action plan and commit all task changes on the task branch. Future firmware
work starts from reviewed knowledge; this analysis does not itself authorize
new actuator behavior or establish hardware acceptance.

# DNA input discovery across all available sessions

## Result and correction

**BH `0x256`, DLC 4, carries a momentary directional input associated with the
DNA selector.** It precedes every observed N-to-D transition. The earlier
statement that no separate selector input had been identified is superseded.
The earlier `0x384`/`0x46C`/`0x25A` mappings remain valid mode reports.

This is supported capture evidence, not a hardware-tested injection command.
The physical sender of `0x256` is not encoded in the dump: it could be the
selector module or another node reporting its input. The BCM attribution for
`0x384` and TCM/DCTM attribution for `0x5A8` come from firmware comments, not
independent source identification. There is no verified electrical wiring or
ECU address evidence for the physical selector in these inputs.

## Scope and reproducibility

Baseline: `66b77fe` (including the CRC correction and result checking).
All **20** available session databases were inventoried, including failed and
empty captures: **21,354,624 SQLite frames**. Four failed sessions are empty;
six sessions in total are marked failed. Thirteen contain BH `0x256` (17,087
frames). Only three sessions contain N-to-D transitions:

| Session | N→D | Context |
| --- | ---: | --- |
| `20261010-111025Z` | 1 | DNA scenario skipped, but an actual switch was recorded outside it |
| `20261011-190746Z` | 4 | DNA/LKA recording; includes intermediate N→D before Race |
| `20261011-191050Z` | 3 | DNA recording; includes intermediate N→D before Race |

The October-10 event 96 has original label `track_changed` and note
`zmienilem tryb na d w celu regulacji zawieszenia, silnik caly czas odpalony`.
It is 23 seconds before the recorded transition, so it supplies context, not
precise action timing. Its text and timestamp were preserved. The previous
inference that skipping the DNA scenario meant that session contained no useful
DNA evidence was too broad.

Files, hashes, original events, health, all mode transitions, before/after raw
examples, exploratory bit candidates and negative controls are retained in:

* [all-session evidence](20261023-dna-all-sessions.json)
* [selector pulse evidence](20261023-dna-selector-evidence.json)

Reproduce from `baccable_lab`, with the actual capture directory as argument:

```sh
python3 scripts/analyze_dna_all_sessions.py /path/to/sessions > /tmp/dna-all.json
python3 scripts/analyze_dna_selector.py /path/to/sessions /tmp/dna-all.json > /tmp/dna-selector.json
```

The first pass inventories every session and scans every frame identity in
±5-second neighborhoods of all N↔D edges in the three relevant recordings.
Repeated bit changes and pulses are compared with control activity across all
sessions. A second, wider pulse search uses stable baseline `[-5,-2] s` before
edges with at least five seconds in the prior mode. This wider search exposed
`0x256`; the narrow baseline extending to -30 ms wrongly included its early
pulse and therefore rejected it as an unstable baseline. A generic bit-score
threshold is not proof that no input exists. The targeted pass then checks
every `0x256` pulse against every DNA mode transition, not just N→D.

SQLite was opened read-only with WAL included. Database, WAL, raw streams and
metadata hashes were unchanged across all passes. Two summaries report more
frames than SQLite: `181818Z` by 34 and `110504Z` by 123. Re-parsing their raw
streams with the existing binary parser reproduces the summary counts and
confirms N-only mode reports / neutral selector direction. These differences
remain capture limitations, not repaired databases. Startup losses, malformed
records and failed recordings remain in the evidence; no loss records appear
within ±5 seconds of the selected N→D edges. Absence of reported loss is not
proof of complete reception.

Timing uses **each board's own unwrapped device clock and receive sequence**.
Current capture code stamps software milliseconds while draining the receive
FIFO, before local message rewriting; it does not use hardware start-of-frame
timestamps. The recorded firmware identities are unspecified. Millisecond
delays are observation intervals, and equal timestamps do not imply simultaneous
transmission. USB host timestamps and independent board clocks cannot establish
a global cross-bus transmission order.

## Selector waveform

All observed frames have four data bytes. Byte indexes below start at zero:

| BH `0x256` payload | Interpretation supported by the captures |
| --- | --- |
| `00 00 00 00` | Released/neutral direction |
| `40 00 00 00` | Direction toward D/Race: A→N, N→D, or a held D→Race |
| `20 00 00 00` | Opposite direction: Race→D, D→N, N→A |
| `00 80 00 00` | Separate byte-1 activity; not a DNA directional request |

The directional field is `(data[0] >> 5) & 3`, with 0 released, 1 toward A,
2 toward D/Race; 3 is unobserved and remains unnamed. The `00 80 00 00`
activity occurs during October-10 console/chassis controls without mode
changes; its physical button meaning is not promoted here. Preserve it when
working with the directional field. Resting reports recur at a median 250 ms;
extra reports appear on input edges. No changing counter/checksum field was
observed in the four-byte payload. This does not authorize changing similarly
numbered messages on another network: **C1 also has `0x256`, but DLC 8 and a
different payload. The selector interpretation is BH-only, exactly DLC 4.**

Across all sessions there are **36 non-neutral directional pulses**, matching
36 selector-associated mode transitions, plus one D→N reset near ignition
events with no selector pulse. Neutral captures and the separate LKA recording
are negative controls; thousands of neutral `0x256` reports are not requests.

| Transition | Count | Direction byte | Input onset → BH `0x46C` mode change |
| --- | ---: | --- | --- |
| N→D | 8 | `40` | 108–112 ms |
| D→N by selector | 6 | `20` | 107–113 ms |
| N→A | 6 | `20` | 108–113 ms |
| A→N | 6 | `40` | 105–113 ms |
| D→Race | 5 | held `40` | 1,194–1,211 ms |
| Race→D | 5 | `20` | 110–114 ms |

The eight N→D pulses last **154–294 ms** from first non-neutral report to
release. The mode changes before release in all eight. It remains D after the
input returns to zero. Actual switch movement may precede its first CAN report.
The previously reported D-to-Race interval of about 2.5–2.9 seconds included
the driver's pause before the next hold; it was not the hold threshold. The
new input observation gives about 1.2 seconds from reported hold onset to Race.
The final failed Race attempt in `191050Z` contains only a short N→D pulse,
with no subsequent recorded long hold; its marker does not prove Race selection.

## Exact N→D example and response order

October-10, BH, relative to the input's first non-neutral report:

| Δ device time | Frame ID in SQLite | CAN ID | Raw payload |
| --- | ---: | --- | --- |
| 0 ms | 2940252 | `0x256` | `40 00 00 00` |
| +110 ms | 2940267 | `0x25A` | `20 00 00 00` |
| +112 ms | 2940269 | `0x46C` | `00 8E 63 2C 96 CC 00 02` |
| +135 ms | 2940289 | `0x25A` | `20 10 00 00` |
| +294 ms | 2941351 | `0x256` | `00 00 00 00` |

Across all eight N→D transitions, BH `0x25A` byte 0 changes 106–110 ms after
input onset; `0x46C` changes 108–112 ms after onset. `0x25A` byte 1 changes
later, 133–157 ms after onset. Its meaning is still unresolved: this temporal
order does not establish whether it is feedback, a secondary state or another
controller's contribution.

On the **C1 clock**, relative to its own first changed `0x384`:

| Relative time | Report change | Repetition |
| --- | --- | --- |
| 0 ms | `0x384` byte 1 masked `00→08` (N→D) | 8/8 |
| +12–27 ms | `0x1FC` byte 0 `50→40` | 8/8 |
| +29–46 ms | `0x5A8` byte 4 masked `00→10` (decoded 0→2) | 8/8 |

On C2, its `0x1FC` change follows its own `0x384` by 13–27 ms, also 8/8.
These per-bus intervals cannot be added to BH intervals as if clocks were
synchronized. They establish a repeatable sequence consistent with selector
input → mode broadcast → downstream status changes, not a measured network
gateway latency or physical ECU address.

Normal median report periods: `0x384` C1/C2 250 ms; `0x25A` BH 250 ms;
`0x46C` BH 500 ms; `0x1FC` C1/C2 20 ms; `0x5A8` C1 1,000 ms. Mode changes
can produce additional reports between those periods. `0x1FC` has historical
damping/differential annotations; byte 0's exact actuator meaning is not proven.
`0x5A8` is described in source as TCM/DCTM-to-ECM. It reports mode, not an
explicit acknowledgement addressed to BACCAble. A static capture does not
measure throttle, shift, steering or suspension behavior under load.

## Rejected alternatives and limits

The narrow screen highlighted C2 `0x10E` pulses near 7/8 N→D changes. These
also occur hundreds of times without any DNA change: in the N-only `181818Z`
control, byte 4 bit 5 flips 400 times more than five seconds from any mode
transition. In `190746Z`, 274 such flips occur in stable N away from mode
edges. Their inconsistent leading/trailing timing rejects them as the specific
DNA request. Other sparse candidates include known steering/RPM variation,
counter/checksum activity and unrelated comfort fields. They remain exploratory.

Unlike those candidates, every non-neutral directional `0x256` pulse is paired
with a selector-associated mode change. This supports the input interpretation,
but does not prove that a replayed CAN frame will be accepted as physical input.
RX logging lacks a physical sender address and the board's own TX provenance.
Confirming the node identity requires electrical/topology evidence or a capture
at the selector connection; claiming that a particular ECU transmits it solely
from the numeric CAN ID would overstate the evidence.

## Consequence for BACCAble

The next implementation candidate for default N→D is a **BH input gesture**:
start from a fresh neutral `0x256`/DLC 4 report, set only the directional field
to `40`, then restore that field to neutral after a short bounded interval
consistent with the observed 154–294 ms. Preserve unrelated fields; a physical
input takes priority. Do not send this on C1's unrelated `0x256`.

This should replace the speculative copies of selected-mode reports in a
separate implementation task. Keep the sustained `0x384` + CRC-valid `0x5A8`
corroboration, but start its observation window after BH actually queues the
gesture, not after the old C1 report rewrite. On timeout, report failure and
stop; do not continuously assert the input. A short press avoids emulating the
long hold used for Race. Successful release and subsequent driver control are
part of hardware acceptance. No firmware change or CAN injection was performed
in this research task.

# DNA selector, mode propagation and the current default-mode implementation

Analysis date: 2026-10-23. Firmware baseline: `aa81111`, originally on
`feat/pelican-experiments`. This is an offline analysis, not a firmware change
or a vehicle acceptance result.

## Inputs and reproducibility

Re-read both raw SQLite captures, not just the earlier bundle conclusions:

| Session | Frames | DNA markers | Capture limitations |
| --- | ---: | ---: | --- |
| `20261011-190746Z` | 950,504 | 12, plus six LKA markers | C1 startup loss at least 65,535; BH 4,869 |
| `20261011-191050Z` | 368,781 | 12 | C2 startup loss 10,051; BH 806; C2 11 parser resyncs and 16 malformed records |

No later loss was reported; this does not prove lossless reception. Installed
firmware, vehicle equipment and capture context are unspecified. The format lacks
TX/RX and IDE/RTR flags and does not log the board's own transmitted frames.
The October-10 campaign skipped its DNA scenario and adds no independent mode
selection trials. Existing dictionary, hypotheses and October-11/12 findings
were loaded before interpreting these recordings.

Run from `baccable_lab`:

```sh
python3 scripts/analyze_dna_transitions.py > /tmp/dna-review.json
```

The preserved output is [20261023-dna-transitions.json](20261023-dna-transitions.json).
It includes input SHA-256 hashes, original events and step records, frame IDs,
sequence numbers, host/device times, raw transition payloads, cadence, checksum
checks and an exploratory byte association screen over all 1,319,285 frames.
Databases were opened with `mode=ro`, without `immutable`, including any WAL.
Hashes of the databases, WAL, raw streams and metadata were unchanged before
and after analysis. No original dump, marker or summary was edited.

The association screen excludes the first 0.5 s after a mode transition and
0.2 s before the next, and stops at the last DNA scenario marker. Correlation
is a search aid, not evidence of a control input. Within-board device clocks
are used for timing; host time links boards with USB batching uncertainty.

## Observed behavior

Byte indexes start at zero; values below are hexadecimal **masked field values**,
not interchangeable full payloads.

| Bus / ID | Field | N | D | A | Race | Median interval |
| --- | --- | --- | --- | --- | --- | --- |
| C1/C2 `0x384`, DLC 8 | byte 1, mask `7C` | `00` | `08` | `10` | `30` | 250 ms |
| BH `0x46C`, DLC 8 | byte 7, mask `1F` | `00` | `02` | `04` | `0C` | 500 ms |
| BH `0x25A`, DLC 4 | byte 0, mask `7C` | `40` | `20` | `10` | `04` | 250 ms |
| C1 `0x5A8`, DLC 8 | byte 4, mask `78` | `00` | `10` | `08` | `30` | 1,000 ms |

Changes can trigger additional frames between periodic reports. The `0x5A8`
decoded field `(data[4] >> 3) & 15` is N=0, D=2, A=1, Race=6 in both captures.
Its natural example is `90 00 20 56 86 C0 0D 6C`; the high and low bits outside
mask `78` are other fields. CRC matches all 286 captured `0x5A8` frames.

For C1 `0x384`, the recorded full payloads include:

```text
N     08 01 8A 80 00 04 00 00
D     08 09 8A 80 00 04 00 00
A     08 11 8A 80 00 04 00 00
Race  08 31 8A 80 00 04 00 00
```

C2 has `00` in byte 5 for these examples, whereas C1 has `04`. Never copy a
complete C1 payload blindly onto C2. These frames also contain ignition,
temperature and stalk information.

### Actual selection sequence

All six recorded A-to-Race attempts pass through N and D. Five reach Race;
one stops at D despite an operator `dna_race` marker. Example from
`20261011-191050Z`, times relative to the first received frame (host time):

```text
21.758 s  A
24.321 s  N
24.794 s  D
27.738 s  Race
30.874 s  D
```

In the five successful attempts, the D-to-Race intervals measured on **C1's
device clock** are 2,502 / 2,792 / 2,794 / 2,893 / 2,910 ms. This is consistent
with a deliberate hold before Race, but is not the measured duration of a
knob press: no independently identified press/release field is available.
Natural/Dynamic/AllWeather changes repeat reversibly on all four known mode
reports. The mode remains in periodic reports after selection.

Marker 12 in `191050Z` says Race at 62.867 s; the last mode transition at
60.592 s is D and no recorded mode report changes to Race before capture end.
This disproves treating every completed scenario step as a successful physical
selection. It does not establish why that selection failed. Marker 1 of this
session also starts with D already selected, so it is not an independent N→D
transition. Earlier wording implying three complete successful Race runs in
each session was too strong.

The host-time arrival order differs between buses and some updates share one
USB batch. These captures do not establish which ECU sent first or the physical
signal path from the knob. No independently validated momentary knob request
has been identified; that is not proof that no such request exists.

### What changes with Race

In all five observed Race entries, with matching reversals on exit:

* C1 `0x5A8` reports mode 6, rather than Dynamic's 2.
* C1/C2 `0x4AF` first changes byte 0 bit 0, then byte 1 bit 2. Stable Race
  payload is `01 04 00 00 00 00 00 00`; other modes settle to all zeroes.
  These are the firmware's stability-status fields, not proof of physical
  ABS/ESC intervention or every stability feature being disabled.
* C2 `0x547` / BH `0x4F6` byte 6 bit 1 clears. In these captures it returns
  on exiting Race into D. Later manual LKA toggles in `190746Z` show that this
  is an assistance state affected by more than the selected drive mode.
* BH `0x25A` settles to `04 20 00 00`; Dynamic settles to `20 10 00 00`,
  while N/A have byte 1=`00`. Byte 1 lags byte 0 at several transitions;
  its meaning is unresolved and it is not a proven knob input.

These report changes do not measure throttle mapping, shift strategy, steering
assistance or differential behavior under load. Do not infer them from a lamp.

### Additional correlations and a correction

These are **candidates**, not new executable decoders or commands:

* C1/C2 `0x1FC` byte 0: N/A=`50`, D=`40`, Race=`60`. The older reference
  associates this ID with damping/differential information; the recordings
  support mode correlation but not a particular actuator interpretation.
* C1/C2/BH `0x4AE` byte 3: enabled LKA outside Race=`20`, Race=`A5`,
  manually disabled LKA in D=`00` in `190746Z`. Nibble counter/CRC bytes 6/7
  remain structural. The earlier claim in `COUNTER-HEARTBEAT-001` that the
  whole payload is constant and carries no state is **superseded**. The LKA
  segment is a useful negative control against calling byte 3 a pure DNA code.
* C1 `0x73C` byte 1 changes `9F`→`07` in Race and reverses on exit. The
  catalog decodes a different field (ACC status in byte 7); byte 1's physical
  meaning is unknown. Do not label it an ACC-disable command.

## Current `Default mode` code: what it actually does

`settings/setup_entries.c` offers OFF/A/D/Race. The `driver_aids_configure`
startup event arms the preference. `drive_mode_default_should_apply` remembers
the first original reported mode and cancels on a later different report.
Saving the preference does not immediately request a new driving mode.

Each board queues at most one modified frame per relevant encoding per startup:
C1 `0x384`, C2 `0x384`, BH `0x46C` and BH `0x25A`. A rejected queue submission
can retry. The sent bit records queue acceptance; there is no ECU acceptance
confirmation. It neither emulates an identified knob gesture nor waits for
the body's own reports to settle permanently to the target mode.

`can_forward` sends an extra frame on the same CAN peripheral. It does not
intercept or suppress the original sender. The original state therefore keeps
arriving, normally every 250/500 ms. Earlier beta-22 vehicle evidence of a D
lamp and faint/flickering N lamp is consistent with competing reports; it does
not establish a successfully latched body mode. The subsequent one-shot version
avoids a continuous conflict but has no proven latch mechanism.

### Concrete checksum mismatch

The default-mode branches in `vehicle/drive_mode.c` overwrite `data[7]` with
`frame_checksum`, an SAE J1850 CRC over bytes 0–6. In these two captures:

* All **2,099** C1/C2 `0x384` frames end in `00`.
* **0/2,099** match that CRC algorithm; payloads vary by mode and C2 stalk input.
* The same algorithm matches **286/286** C1 `0x5A8` frames (positive control).
* For the captured Dynamic C1 payload it would replace `00` with `0C`; for
  Dynamic C2 it would replace `00` with `21`.

Thus the current `0x384` rewrite generates a byte absent from the demonstrated
factory layout. This is a concrete implementation/layout mismatch, although
we cannot establish whether an ECU rejects or ignores that byte. A future
correction for this vehicle should preserve it instead of inventing a CRC.
Do not remove checksums globally: `0x5A8` demonstrably uses one. Other vehicle
variants need their own layout evidence.

BH `0x25A` is also only partially emulated: current code changes byte 0 while
retaining the original byte 1, unlike settled factory D/Race payloads. The
meaning of byte 1 must be established before attempting to forge it.

## Proposed implementation path

1. Correct the captured-layout mismatch and add explicit DLC/standard-data
   guards when implementing the next change. Preserve unrelated fields from a
   fresh frame on the same bus. Keep desired mode separate from observed mode.
2. Establish a control path that changes the authoritative selected mode.
   A verified momentary selector request would be preferable, but none is
   established by these recordings. If a bounded broadcast experiment is
   used, treat it as an experiment and require original post-transmission
   reports to hold the target. Do not make repeated contradictory reports
   the normal control strategy.
3. Once that path is verified, a shared request routine can serve both startup
   `Default mode` and a manual `Drive mode` action. Its small state machine is
   idle → request → wait for original reports → success/timeout. Require fresh
   ignition/mode data, give a physical driver change priority, and finish on a
   bounded timeout. Do not report success when only the CAN queue accepted it.
4. Validate on C1/C2 `0x384`, BH `0x46C`, the transmission `0x5A8` response
   and associated stability/assistance reports. Changing the IPC text or lamp
   is insufficient. Race needs explicit deliberate selection and acceptance
   checks because the recorded assistance/stability states change with it.

The next useful recording is small and stationary: with automatic default
mode disabled and the engine/ignition state recorded, mark **knob deflection,
hold and release separately**, plus confirmed displayed mode. Record N→D→N,
N→A→N, D→Race→D, repeated three times. A synchronized video can supply precise
physical-action timing. If those inputs are not present on C1/C2/BH, identify
the selector connector/wiring and capture its actual link before specifying
LIN, a discrete electrical input or a diagnostic command. These alternatives
are presently unknown, not established facts.

An eventual injection check also needs TX provenance or an independent
listener; the existing RX-only trace cannot distinguish BACCAble's own queued
frame from factory acceptance. No CAN transmission is part of this analysis.

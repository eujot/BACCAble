# October 24: DNA/suspension matrix and audit of the three latest captures

## Current conclusions

The labelled manual matrix directly supports **Race Stiff=2, Mid=1** in BH
`0x25A` byte 1 bits 5:4. It agrees with the beta-29 feedback mapping. D remains
Stiff=1, Soft=0. These values are mode-dependent, and mode entry has a short
transitional level which must settle before a decision. This resolves the old
Mid inference; it does not establish acceptance of automatic startup injection.

The two earlier rain/light reviews reproduce exactly. Their core conclusions
stand: C2 `0x2EC` carries a repeating wiper phase, AUTO and dipped indication
are separate, and BH `0x738` contains light-responsive raw channels which recover
before delayed lamp OFF. This audit also retires incorrect AUTO-as-lamp marker
rules and a contradicted `0x1FC` engine-running hypothesis.

## Evidence and method

| Session | Context in original manifest | Confirmations | Frames | Identities |
| --- | --- | ---: | ---: | ---: |
| `20261024-105643Z` | Unspecified | 36/36, 3 rain/light runs | 2,157,769 | 232 |
| `20261024-111510Z` | Giulia 2.0, v5-beta-29, daylight, outdoors | 28/28, 2 isolation runs | 1,644,370 | 232 |
| `20261024-113104Z` | Giulia 2.0, v5-beta-29, daylight | 26/26, 2 DNA/suspension runs | 724,437 | 238 |

All **4,526,576 recorded frames** were replayed from the original binaries.
For every bus in every session, SQLite and raw data have identical frame counts
and identical SHA-256 digests over ordered raw timestamp, CAN ID, DLC and payload.
Original input hashes are unchanged before/after. SQLite was opened read-only
with committed WAL content; no capture, index or marker was changed.

[Audit evidence](20261024-three-session-audit-evidence.json) contains original
markers, scenario steps, exact frame IDs, both clocks, raw payloads, complete
field-edge traces, button/selector associations and content digests. The raw
and trace checks reproduce the two prior reports exactly: no changed input
files or mismatched published field traces. [Fresh reviews](20261024-three-session-current-review.json)
also re-evaluate inventories, known fields, generic marker windows and stable-bit
profiles. The [cumulative ledger](20261024-three-session-ledger.json) retains the
20 older reviews by immutable references and freshly reviews all three latest
sessions; stable templates now include the previously targeted-only follow-up.

All three captures report startup loss on at least one board. Saturated 65,535
loss counts are lower bounds. Recorded startup drops (C1/C2/BH) are respectively
`0/26085/2167`, `>=65535/>=65535/20897`, and `>=65535/0/23002`. Recorded later
loss counts are zero, not proof of a perfect capture. Some initial C1/BH frames
contain buffered old-clock data; timing/cadence calculations exclude startup.
No device clocks from different boards are subtracted. USB batching can give
press and release the same host timestamp; their device times still differ.

The matrix has six more identities than the rain recordings (BH `0x088`,
`0x090`, `0x0AD`, `0x0AF`, C2 `0x0AD`, `0x0AF`), but every identity was already
in the earlier cumulative inventory. This is newly labelled meaning rather
than discovery of an unprecedented CAN ID.

## DNA mode and suspension state table

All 26 human-confirmed state snapshots agree with the corresponding received
mode/calibration reports. Mode codes remain unchanged:

| Mode | BH 0x46C b7[4:0] | C1/C2 0x384 b1[6:2] | BH 0x25A b0[6:2] |
| --- | ---: | ---: | ---: |
| N | 0 | 0 | 16 (`40` in byte 0) |
| A | 4 | 4 | 4 (`10`) |
| D | 2 | 2 | 8 (`20`) |
| Race | 12 | 12 | 1 (`04`) |

The manual hard/softer states are:

| Actual mode and confirmed state | BH 0x25A, DLC 4 | b1[5:4] | C1/C2 0x384 b6.7 | C1/C2 0x1FC byte 0 |
| --- | --- | ---: | ---: | --- |
| N | `40 00 00 00` | 0 | 0 | `50` |
| A | `10 00 00 00` | 0 | 0 | `50` |
| D Stiff | `20 10 00 00` | 1 | 0 | `40` |
| D Soft | `20 00 00 00` | 0 | 1 | `50` |
| Race Stiff | `04 20 00 00` | 2 | 0 | `60` |
| Race Mid | `04 10 00 00` | 1 | 1 | `40` |

`0x25A` has median 250 ms cadence; BH `0x46C` 500 ms; C1/C2 `0x1FC` about
20 ms. The reports corroborate selected/displayed calibration, not a direct
measurement of damper force or proof of the physical transmitting ECU.
Interpret `0x1FC` with actual mode: byte 0 bit 4 alone distinguishes D softness
but stays clear for both Race states. Raw `0x40` has different meanings in D
and Race. The passive dictionary therefore leaves the packed report raw.

### Button input, controls and Race Mid evidence

There are **12 observed BH `0x256`/DLC-4 button pulses**, byte 1 bit 7:

- Four N/A control presses produce no calibration change during the current mode.
- Four D presses give two Stiff → Soft → Stiff pairs.
- Four Race presses give two Stiff → Mid → Stiff pairs.

The eight positive responses follow the button onset by **25–45 ms** on the
BH clock (median 37 ms). Observed button widths are 92–262 ms. Exact Race rows:

| Human confirmation | Button frame ID | Display response frame ID | Received result | BH response delay |
| --- | ---: | ---: | --- | ---: |
| Event 11, Race Mid | 316794 | 316800 | `04 10 00 00` | 38 ms |
| Event 12, Race Stiff | 347680 | 347688 | `04 20 00 00` | 41 ms |
| Event 24, Race Mid | 642417 | 642427 | `04 10 00 00` | 45 ms |
| Event 25, Race Stiff | 669055 | 670088 | `04 20 00 00` | 37 ms |

No global Stiff/Soft enum should be attached to the raw two-bit level: raw 1
is D Stiff and Race Mid, and N/A raw 0 is not D Soft. Unknown combinations stay
raw. Two labelled Race pairs plus earlier Race-entry evidence support this
vehicle's mapping; universal physical validation is not claimed.

### Selector and transitional states

Thirteen direction pulses reproduce the expected ordered steps. Short responses
follow onset by 105–118 ms. The two D→Race responses occur after 1,196 and
1,210 ms during holds lasting 2,187 and 1,886 ms. Returning from Race to N passes
through D. All known intermediate modes are preserved in the evidence.

Mode and calibration do not always change atomically. D entries briefly show
`20 00 00 00` before `20 10 00 00` (24–35 ms in the two A→D entries). Race entries
show `04 10 00 00` before the default `04 20 00 00` for 35 and 36 ms. Thus
`04 10` at the instant of entry is not sufficient to declare an accepted Mid
button action. The existing fresh-mode/level check and 300 ms settling period
in `dna_selector.c` handle this observed transient. The code's desired levels
already match the observed Stiff/Soft/Mid values; no behavioral patch is needed
on the basis of these manual state codes.

During both Mid intervals the independent C1 `0x5A8` transmission report still
contains Race code 6 (encoded in byte 4 bits 6:3). All 151 transmission reports
in this capture pass J1850 CRC. C1 RPM after startup stays 797–974, supporting
a running engine throughout; the matrix is not an engine-start or power-cycle
test. Initial received state is D Soft before the first move to N, so it must
not be used as evidence of a factory startup default or saved preference.

### Refuted engine-running interpretation

The old `ENGINE-RUN-001` claim that `0x1FC` byte 0 bit 4 is a generic engine-running
flag is contradicted here: RPM remains above 797 while this bit clears in D
Stiff/Race and changes on D suspension toggles. Its earlier start/stop correlation
was confounded. The hypothesis and research pointers now retain that claim as
refuted history. Firmware uses fresh RPM for its engine gate and does not need
this bit. Actual mode and calibration are required for the observed report.

## Audit of the two rain/light sessions

| Previously reported conclusion | Recheck |
| --- | --- |
| C2 0x2EC b2.5 is a repeating wipe phase | Exact traces reproduce: 28 and 27 complete pulses; the matrix adds 2,750 low samples with no wipe pulse as an unrelated-control negative case. |
| Slow/fast rate distinction | Approximately 1.50/0.95 s rising-edge intervals repeat across the labelled trials; changing speed can add a transitional interval. Physical one-edge-per-sweep semantics remain open. |
| BH 0x354/C2 0x73E b3.6 versus b1.6 | AUTO selection and dipped indication remain distinct. C1/C2 0x384 b3.7 follows dipped indication; old AUTO-based marker rules were misleading and are retired. |
| BH 0x738 b1/b2 respond to light | Both isolation trials with lamps manually OFF reproduce exactly. Source, physical units and universal thresholds remain unverified. |
| Delayed lamp release | Labelled follow-up intervals remain 14,411/14,474 ms after both channels recover to 6; earlier trials have 14–20 s frame intervals. Sampling resolution is approximately one second. |
| Raw rain and washer isolation | Still unresolved. Rain markers confirm observed wiping after water/spraying; they do not independently prove raw rain intensity or separate washer commands from rain-sensor activation. |

The earlier reports correctly recorded the core observations and timing. This
pass strengthens their raw-versus-database check from counts to full ordered
content digests. It also repairs these knowledge/analysis gaps:

1. `0x384` low-beam fields still used AUTO-selection marker labels. Current rules
   use the dipped labels, while inherited old correlations are kept separately
   as historical observations rather than silently rescored as current evidence.
2. Generic marker correlation did not honor a message's exact DLC, although the
   public decoder did. It now skips incompatible lengths before evaluating an
   edge; a regression test covers a false short-frame transition. None of the
   replayed traces required changing because of this fix.
3. The follow-up had a targeted evidence ledger without fresh stable-template
   learning. All three sessions now have full known-field/profile reviews and
   the cumulative templates include them. A generic three-second marker window
   remains a heuristic and does not replace bounded delayed-state analysis.
4. The technical playbook incorrectly treated a constant last byte as checksum
   proof and could dismiss an entire frame from unrelated changing bytes. These
   instructions now require actual field-level evidence and held-out CRC tests.
   Its old description of Default-mode report rewriting is also superseded by
   the identified BH selector-input method.

Observed lamps stay off during the labelled daylight water/wiping controls.
This establishes the negative control for these stationary conditions, not a
universal claim about every factory lighting configuration or driving condition.
Repeated OFF confirmations need not contain a new transition. Similarly, a
rain confirmation can occur between phase pulses. Neither should be declared a
failure merely because a narrow transition window is empty.

## Documentation and implementation boundary

Dictionary `2026-10-24.4`, hypotheses, generated catalog, both device guides and
`MENU_UX.md` now reflect the direct Race/Mid evidence. Older dated notes retain
their uncertainty as history, with this report explicitly superseding it. The
77-test Lab suite checks real calibration payloads, bus/length isolation, raw
unknown values, the correlation length guard, source snapshots, cumulative
marker provenance and generated catalog consistency.

The firmware behavior remains unchanged. These recordings establish manual
inputs and reported states; they contain no explicit acceptance test of the
saved startup suspension preference, no fresh ignition cycle proving startup
engine gating, and no lamp-output injection test. The next implementation work
can use the documented input/state mappings. Actual startup replay and light
actuation must retain separate hardware acceptance in `docs/ACTION_PLAN.md`.

## Reproduction

```sh
python3 baccable_lab/scripts/analyze_oct24_controls.py \
  --sessions /path/to/original/baccable_lab/sessions \
  --output /tmp/new-three-session-audit.json
```

The script refuses overwrite or output inside original captures, checks source
fingerprints, compares every raw/SQLite frame, retains same-board pulse timing,
and compares the first two results with their immutable earlier evidence.
Raw evidence and full current-review profiles are intentionally separate from
interpretations. No CAN transmission is performed by these tools.

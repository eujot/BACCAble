# Guided-scenario campaign: recordings from October 10, 2026

Two long guided-scenario sessions were recorded on the same day and analysed
together with their derived `baccable bundle` artifacts:

| Session | Status | Duration | Frames | Scenarios touched | Confirmations | Coverage |
| --- | --- | --- | --- | --- | --- | --- |
| `20261010-104325Z` | complete | 1046 s | 3,861,290 | 19 (`access`, `lighting`) | 114 done | 14.3 % mapped |
| `20261010-111025Z` | complete | 1292 s | 6,760,480 | 59 | 227 done, 124 skipped | 15.9 % mapped |

Both ran the guided workflow end to end. `111025Z` resumed past the procedures
already completed in `104325Z` (shared persistent progress), so the two sessions
cover **disjoint** procedure sets rather than repeating the same actions. The
`skipped` outcome was used heavily and deliberately, including every
`abs_intervention`, `esc_intervention`, `traction_control` and `hill_descent`
procedure.

All source recordings were read read-only (`mode=ro`). The derived bundles
(`10paz1.json`, `10paz2.json`) were used for ranking; every claim below was
re-checked against the raw `session.sqlite3` bit sequences.

## Red flag: counter/heartbeat frames masquerade as signals

The bundle `scenario_candidates` ranking is polluted by short frames whose only
changing bytes are a rolling counter and its check byte. Raw sequences show the
mechanism directly:

```
C1 0x4AE: 000000206000 0743 | …08f8 | …09e5 | …0ac2 | …0bdf | …0c8c | …0d91 | …0eb6 | …0fab | …0010 | …010d …
C1 0x15A: 8000 0762 | 8000 08d9 | 8000 09c4 | 8000 0ae3 | … 8000 0f8a | 8000 0031 | 8000 012c …
C1 0x5A8: 9000205686c0 06a3 | …07be | …0805 | …0918 | …0a3f | …0b22 | …0c71 …
```

The second-to-last byte steps through a 4-bit modulus `07,08,…,0f,00,01,…06`
and repeats; the last byte is a function of it. `0x4AE` and `0x15A` carry **no
payload state at all**. Any bit of their counter correlates with any action that
sits at a fixed phase of an evenly spaced procedure grid — which is exactly why
they topped the bundle (`0x4AE` byte 6 bit 3 / byte 7 bit 7 ↔ `window_close`
×5). These are **false positives**, not window or light decoders. The same
applies to `0x5A8` bytes 6–7 (`0x5A8` bytes 0–5 still carry drive-mode state).

`C1 0x341` looks like a different hazard: its first four bytes decrease
monotonically (`4f30 4e96 → 4f29 4e96 → 4f23 4e93 → … → 4eec 4e7c`), i.e. two
16-bit accumulators. Candidate bits on it (windows, seat belt, mirrors,
suspension) may therefore be drift. Treat `0x341` as suspect until a monotonic
trend is excluded.

Lesson for the tooling: `shape_hints` flags a checksum in the **last** byte but
does not discount a **4-bit counter in the second-to-last** byte. Candidate
ranking should exclude counter/checksum bytes before scoring.

## New frame leads (single session, candidate)

Frame bytes that jump (not monotonic) and follow the marked action are real
leads. Directions below are ON/OFF bit occupancy.

| Frame | Suspected role | Evidence (this campaign) |
| --- | --- | --- |
| **C1/C2 `0x116`** (10 ms backbone, gateway-duplicated) | closure/ajar status: boot `byte2 bit2` **active-low** (ON 0.17 / OFF 0.86), bonnet `byte3 bit1` (ON 0.05 / OFF 0.73); `byte6` counter, `byte7` CRC | identical payloads on C1 and C2; the highest-volume unmapped frame now has a hypothesis |
| **BH `0x3DE`** | BH-side companion of `0x116` (boot/bonnet) | boot `byte2 bit2` ON 0.17 / OFF 0.85 |
| **C2 `0x0F1`** (≈93 k frames) | central-lock / door status in `byte3` (128 distinct values) | `byte3` bits 2–6 track `lock`/`unlock` |
| **C1 `0x0FA`** (10 ms backbone) | one-hot flag byte: `byte0` takes single-bit values `0x20/0x40/0x80` (lamps/indicators/stop); `byte7` CRC | flags + counter/CRC; needs per-lamp isolation |
| **C1 `0x104`, `0x100`** | brake state (`0x104 byte2 bit6`, ON 0.66 / OFF 0.01) | `20261010-111025Z` |
| **C1/C2 `0x1FC`** | engine running (`byte0 bit4`, ON 0.83 / OFF 0.01) | conflicts with the existing reference note "active damping"; keep both |
| **C1 `0x736`** | HVAC (front defrost `byte3/byte4 bit0`) | `20261010-111025Z` |
| **BH `0x46A`** | climate status (temperature/fan) | `20261010-111025Z` |
| **BH `0x2F0`** | comfort status: wipers (`byte4 bit4`, ON 0.92 / OFF 0.29) | first wiper lead |

These remain **candidates**: one session, no repeated same-label support yet.

## Negative result: steady exterior lamps

No clean steady bit was found for dipped beam, high beam or front fog across the
266 observed identities. Their only consistent trace is (a) the counter frames
above — false — and (b) blinking in `0x354`/`0x73E`, whose payload alternates
`…80…` / `…00…` in **both** the ON and OFF intervals. This qualifies the weak
existing `low_beam_report` suggestion on `0x384` `byte3 bit7` (it alternates here
without following the state). Steady lamps may be coded as pulses/multiplex, not
as a level; a dedicated long-interval experiment is needed.

## Comparison with previous knowledge

Confirmed: `0x0FB` torque, BH `0x3E8` `engaged_gear` (Drive/Reverse),
C2 `0x0FC` `reverse_report`, the `0x354`/`0x73E` light reports, and the
`0x46C`/`0x25A` drive-mode presentation all agree with the curated dictionary.
New identifiers above were previously `unmapped`.

## Next discriminating recording

1. Repeat a small stationary subset in a **new** session (reset progress or force
   `--scenario`) so the same marker labels recur across sessions and reach the
   three-cycle bar; the disjoint campaign above cannot support promotion.
2. Give boundary steps a canonical negative `expect` (for example `brake_off`,
   `wipers_off`, `trunk_close`); action-text labels dominate several rankings.
3. Isolate lamps with long equal ON/OFF intervals and separate switch state from
   actual illumination to test the pulse/multiplex hypothesis.
4. Record one procedure set with `--context` populated; both bundles have an
   empty context.

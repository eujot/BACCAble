# Public Giulia CAN sources cross-checked against BACCAble captures

Date: 2026-10-13. Scope: the five October bundles (`10paz1`, `10paz2`,
`dnskla`, `dna`, `kla`) and the previously catalogued driving session
`20260928-203436Z`. The latter ended in a failed/truncated state and reported
substantial start-up loss; its in-session motion is useful for correlation, not
for claiming complete traffic. The October capture limits and source hashes are
in [the five-bundle investigation](20261012-five-bundle-investigation.md) and
the immutable review ledger `20261012-five-bundle-review.json`. Original dumps,
events and SQLite databases were not modified. Firmware behavior was not changed.

## Sources and interpretation

* [Racelogic Giulia Type 952 CAN guide](https://www.racelogic.co.uk/_downloads/vbox/Vehicles/Other/Docs/Alfa%20Romeo-Giulia%20%28Type%20952%29.pdf)
  identifies a 500 kbit/s CAN connection at OBD pins 6/14 and lists nine
  broadcast channels. Its [Giulia REF channel file](https://www.racelogic.co.uk/_downloads/vbox/Vehicle_CAN_Files/New/Alfa%20Romeo-Giulia%20%28Type%20952%29.REF)
  contains signal definitions (downloaded SHA-256
  `85248788f78b56c4e55bd5992694f00c8b556bb4f8e36f7a99bd5b4b4e699155`).
  The REF embeds zlib-compressed comma-separated descriptors. CAN IDs in them
  are **decimal**: 222=`0x0DE`, 238=`0x0EE`, 252=`0x0FC`, 257=`0x101`,
  599=`0x257`. For its Motorola fields, treating `start` as the least
  significant bit position of a big-endian 64-bit payload reproduces the
  existing `0x0FC` RPM decoder exactly: start 50, length 14 gives
  `b0[7:0] + b1[7:2]`. This cross-check matters: interpreting the starts as
  DBC MSB indices would give false layouts.
* [danardi78 Giulia/Stelvio custom PIDs](https://github.com/danardi78/Alfaromeo-Giulia-Stelvio-PIDs)
  is an owner's reverse-engineering project, tested mainly on a 2016 2.2
  diesel; names beginning `?` are explicitly unverified there. Compared its
  [CSV](https://github.com/danardi78/Alfaromeo-Giulia-Stelvio-PIDs/blob/main/custompids.csv)
  (downloaded SHA-256
  `9f6fb0b3e99a9a7a4e3f56f0bc88e88aa6bd83cdf0945327ee4817e7a36c38a6`)
  by ECU request address **and** DID with BACCAble's diagnostic catalog.
* [ClaudeMarais' Giulia OBD experiment](https://github.com/ClaudeMarais/Simple_OBD2_for_AlfaRomeoGiulia)
  supplies independent example UDS queries from a 2019 2.0 petrol Giulia.
  The year, engine, gateway and module variant of the BACCAble capture are
  unknown, so these sources are leads rather than evidence of support in this
  particular car.

## Newly mapped passive frames

These are already-present frame shapes, not newly observed IDs. The dictionary
uses exact DLC and separate C1/C2 message identities. All values retain raw
codes; no guessed invalid/saturation rule is added.

| Bus, ID, DLC | Field and REF extraction | Captured cross-check | Remaining limit |
| --- | --- | --- | --- |
| C1/C2 `0x0DE`, 6 | Steering angle: `720 - 0.1 × ((b0 << 8) | b1)` degrees | In `20261010-111025Z`, steering-input events 103–108 have C1 settled medians approximately `-388.6, +378.0, -377.7, +380.5, -390.4, +393.2` degrees; C2 follows the same alternation. Quiet event 102 is near `+2.5` degrees. | Marker does not label physical left/right, so sign orientation is open. |
| C1/C2 `0x0EE`, 8 | Four 13-bit wheel speeds, big-endian bit starts RR 12, RL 25, FR 38, FL 51; each raw × `0.066214` km/h | Example C1 `1C A8 E5 87 3A 3A 00 CD` decodes RR/RL/FR/FL `61.45/61.25/60.78/60.72` km/h while nearby existing C1 vehicle speed is `58.0` km/h. Across 18,814 nearby moving pairs in `20260928-203436Z` (device time within 20 ms, speed ≥5 km/h), median wheel-to-vehicle ratio is `1.0566`, median wheel spread `0.331` km/h. | About 5.7% scale difference versus existing vehicle speed. No external speed instrument or wheel calibration proof. The driving session is truncated. |
| C1/C2 `0x101`, 8 | Brake pedal position: `0.1 × (((b2 & 0x0F) << 8) | b3)` % | Three brake ON/OFF cycles in `20261010-111025Z` (events 127–132): C1 ON median near `100.5%`, OFF near `0.1%`; C2 ON near `100.4%`, OFF near `0–0.4%`. Captured C1 `00 00 03 ED 9F C0 0F D6` gives `100.5%`. | The source scale can give slightly over 100%; do not silently clamp. Distinct from C1 assisted-braking bit and C2 brake-pressure field. |
| C1/C2 `0x0FC`, 8 | Accelerator pedal position: `0.1969 × (((b2 & 0x1F) << 4) | (b3 >> 4))` % | Captured `14 A4 C3 22 18 3F DE 10` gives `9.845%`. In the driving session, 28,452 nearby C1 samples correlate with existing C1 `0x412` accelerator (Pearson `r=0.932`); both rise with driving. | The readings are not interchangeable: among non-idle pairs, the median signed difference is about `-2.60` percentage points. Pedal request and ECU-normalized accelerator may represent different stages. |

The REF also labels `0x257` byte 2 bit 5 as driver seat belt. This **fails our
marker check**: in the five-bundle investigation C1 bit occupancy when fastened
was `0.52/0.60/0.75`, versus `0.79/0.72/1.00` when unfastened; C2 was similar.
The REF vehicle/configuration, field polarity, or our mixed marker windows may
differ. Do not add a seat-belt decoder or safety-related action from that bit.

## Diagnostic leads and comparison with current firmware

The owner's CSV has 137 distinct `(ECU address, DID)` pairs; 47 coincide with
BACCAble's current diagnostic catalog and 90 are external-only candidates.
Examples with potentially useful functions: TCM `0x18DA18F1` DIDs `0x0518`,
`0x0540` (DNA); ECM `0x18DA10F1` DID `0x192D` (selected gear) and `0x1B00`,
`0x1B02`, `0x1B0D` (start/stop related); BCM `0x18DA40F1` DID `0x0133`
(light switch); IPC `0x18DA60F1` DID `0x0104` (brightness). The second OBD
project independently exercises `0x192D` and BCM `0x013C` outside temperature.
These are diagnostic **queries**, not DNA knob command frames and not proof that
the modules will respond on this car.

The five bundles do not establish a new positive diagnostic DID. Their complete
single-frame `0x62` replies on C1 are for the already-catalogued `0x3813`,
`0x3A48`, `0x2805`, `0x3A41`, `0x198E`; other seen traffic includes existing
multi-frame replies, which the Lab's bounded single-frame decoder intentionally
does not interpret. The external tire DIDs `0x40B1`–`0x40B4` are especially
unsafe to copy into firmware: BACCAble's current parameter IDs 50–53 already
have a malformed request PCI length and a reply offset beyond its single-frame
capacity. Query shape and ISO-TP response length need separate validation.

**Next vehicle check:** collect a short, controlled passive drive with an
independent speed value and one clearly labelled physical left/right steering
turn, then compare these decoders. For diagnostic expansion, test a small set of
read-only `0x22` requests one ECU/DID at a time, record full positive/negative
ISO-TP replies and gateway behavior, and only then choose firmware readings.
The public data does not identify a reliable one-shot DNA controller command.

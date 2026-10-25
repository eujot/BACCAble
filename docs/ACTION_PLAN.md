# BACCAble action plan and agent handoff

> The step-by-step CAN decode backlog lives in
> [baccable_lab/docs/DECODE_ROADMAP.md](../baccable_lab/docs/DECODE_ROADMAP.md);
> this file records status and acceptance, not the per-campaign procedure.

## Manual light-switch follow-up, 2026-10-25

On the same `research/light-origin-timing` branch, the
[focused manual audit](../baccable_lab/docs/research/20261025-manual-light-switch.md)
rechecks 20261010-104325Z events 73–78, all three ON/OFF pairs and preceding
AUTO-only controls. AUTO is a short intermediate indication in 2/3 ON trials
and all three OFF trials, then clears. No additional repeatable quiet-baseline
CAN precursor was demonstrated by the all-ID/bit scan, including slow frames.
167 retained binary records match, capture fingerprints remain unchanged, and
26 knowledge/catalog tests pass. Firmware behavior is unchanged.

Owner reports beta-34 Rain lights enabled, Wiper 26 / A=1 / ARM=1 / Q109 /
F=off, BH738 1/1 / TX125, but lamps remain OFF. This is a negative vehicle
outcome, not a capture confirming delivery of injected frames. At 1/1 the >7
guard is not the blocker. Establish actual TX delivery/receiver behavior and
facelift-specific BCM/lamp wiring; do not ask to repeat the already available
manual-switch scenario as though it had never been recorded.

## Light origin and timing audit, 2026-10-25

Branch `research/light-origin-timing` (based on `origin/master` 71b7ded)
adds a reproducible passive analysis of all 27 captures through
`20261024-113644Z`, with an all-ID scan of 20 lamp edges and binary verification
of retained changes. See [the report](../baccable_lab/docs/research/20261025-light-origin-timing.md)
and its evidence ledgers. Owner confirms facelift LED headlights; exact MY/VIN
and lamp wiring remain unknown. BH 0x738 is a periodic light-responsive report,
not an established actuator command. Source ECU and BCM consumption remain
unverified. Cross-bus USB batching and independent clock drift prevent source
attribution from raw timestamps; same-C2 lamp reports follow 0x73E by 105–218 ms
(0x42D/E) and 45–247 ms (0x384) in the selected trials.

**Concrete firmware follow-up:** beta-34 `rain_lights.c` rejects light bytes >7,
but real inputs reach 9. The guard rejects 65/136 post-startup samples in
`113104Z` and 271/305 in `113644Z`. Fix this separately before an actuator
acceptance trial; removing the guard does not prove ECU acceptance. This task
changes no firmware and performs no flashing. No beta-34 capture is available.
Counter/checksum interpretations of 0x738 b4/b5 remain unverified. Dictionary,
hypotheses and readable catalog retain those limits and the new observed range.

Validation: 26 knowledge/catalog tests pass; all 832 retained change records
match the original binary captures, and input fingerprints remain unchanged.
Clock fits reduce USB batching effects but leave BH endpoint scatter of about
-12 to +14 ms; residual 8–12 ms cross-bus leads do not establish a sender.

The user's original dirty checkout and its uncommitted light-command research
note were preserved. This handoff and analysis belong only to the isolated
research branch. User guides need no changes because operation is unchanged.

## BH twilight output for Rain lights, 2026-10-25

Branch `feat/rain-lights-bh-darkness` moves the Rain lights output from C1/C2
lamp-bit copies to BH `0x738` darkness reports. C2 keeps the existing three
wiper phases in 15 s, AUTO prerequisite and 60 s hold; its former C1 UART reply
slot now carries an active request directly to BH. BH also requires its own
fresh `0x354` AUTO indication and expires requests after 3 s. It copies each
fresh BH `0x738`/DLC8 factory frame, sets only light-responsive bytes 1 and 2
to zero, and queues it only when CAN output is idle. The forced `Rain test`
request reaches BH in the existing C2 debug reply and retains 3 s and 30 s
limits. C1/C2 no longer inject `0x73E` lamp bits. BH diagnostic `TX` counts
dark frames accepted into its CAN queue; it is not lamp confirmation.

Read-only comparison of the three October 24 sessions found 32 groups of
factory `0x738` frames with different light bytes but otherwise identical
payloads, including byte 5. This supports copying the other bytes as captured;
it does not prove whether a receiving ECU consumes the added frame or whether
its internal validity checks accept it. The earlier research claim that a
dominant zero in the payload wins over a simultaneous factory frame is false:
same-ID conflicting data can cause a CAN bit error. The implementation sends
after a complete factory frame, not during it. Vehicle acceptance remains open
for lamps on/off, front/rear behavior, manual override, CAN error counters and
factory bright-frame interaction. No board has been flashed.

Validation: full Apple Clang host suite and focused C1/C2/BH Rain lights,
Rain test and shared-UART regressions pass. All four firmware flavors pass
cppcheck lint and ARM GNU 15.2.Rel1 builds. C1 uses 91,624/98,304 Flash bytes
and BH uses 35,844/131,072 bytes with this local toolchain. These are build
results, not in-vehicle lighting confirmation.

## v5-beta-34 publication, 2026-10-25

PR [#93](https://github.com/eujot/BACCAble/pull/93) merged to `master` as
`fd737c114891cdfe1ef79ee4bb5198685e161c6f`. Published prerelease
[v5-beta-34](https://github.com/eujot/BACCAble/releases/tag/v5-beta-34) with
[workflow 40200580085](https://github.com/eujot/BACCAble/actions/runs/40200580085).
The release workflow passed tests, lint and all four firmware builds using the
pinned Arm GNU 15.3.Rel1 toolchain. All 14 downloaded release assets passed
`SHA256SUMS`; `BUILD_INFO.json` identifies the same merge commit. C1 uses
91,368/98,304 Flash bytes and 12,596/16,384 static RAM bytes. No board was
flashed; vehicle acceptance of the BH `0x738` darkness frame remains open.

Follow-up `no reply` audit: C1 still queues at most one Rain lights option
request per second; C2 sends its BH request in the former C1 reply slot. The
Rain test debug reply is now visible to BH, and the BH sensor reply uses one
more payload byte, but UART frames remain fixed at 18 bytes. The 105-second
production-C1-UART host regression passes with Rain lights ON, both Rain test
pages, forced output, screen refreshes and status/USB polls: no `no reply`
display, queue rejection or force renewal gap occurred. Its simulated peers
reply immediately, so real-board UART errors and delivery still need a vehicle
trial with matching C1/C2/BH images.

## v5-beta-33 release, 2026-10-25

PR #90 (`38fea9e`) fixed Rain test starvation and PR #91 (`bf883b1`) added the
release notes; both are merged. Published [v5-beta-33](https://github.com/eujot/BACCAble/releases/tag/v5-beta-33)
from `bf883b1a7b7502c710c11156fa9273fc5988825e` using
[workflow 40196710682](https://github.com/eujot/BACCAble/actions/runs/40196710682).
Tests, lint, all four firmware builds, display profiles and size/identity gates
passed with pinned Arm GNU 15.3.Rel1. All 14 release assets are present; each
of the 13 payloads listed in `SHA256SUMS` passed an independent download and
checksum check. C1 uses 92,100/98,304 Flash bytes and 12,620/16,384 static RAM
bytes. The v5-beta-32 and v5-beta-33 GitHub release descriptions now contain
their respective notes. No board was flashed; vehicle acceptance remains open,
including the reported `ARM=1` while Rain lights was believed OFF.

## Rain test shared-UART pacing correction, 2026-10-25

Branch `fix/rain-test-uart-budget` addresses the reproduced v5-beta-32 reply
starvation. C1 polls only the visible Rain test page every 2 s. During a
forced test it renews C2 every 1 s, polls BH every 5 s and holds the BH reply
for 7 s. While Rain test is open, regular C2/BH status polls use 2.0/2.2 s;
outside the test they retain the old timing. Identical menu screens are
resent after 1.5 s when Rain lights is enabled or Rain test is open, and after
2 s during force; changed screens still send immediately. These rates keep
the normal and forced traffic within C1's four-transmission-per-second UART
budget. The existing Rain lights trigger, lamp masks and C2's three-second
force lease are unchanged.

A production-UART host integration regression passes across both diagnostic
pages, Rain lights OFF/ON, forced output, status/USB polls and screen updates
for 105 s, with fresh replies, no UART queue rejection and no force lease gap.
All native host tests, 82 Lab unittest cases, and C1/C2/BH/CAN lint and ARM
builds pass. C1 uses 92,344/98,304 Flash bytes. Vehicle verification remains
open, including the reported `ARM=1` while the setting was believed OFF and
physical lamp acceptance. No board has been flashed with this correction.

## Rain test replies expire under shared UART load, 2026-10-25

Owner's v5-beta-32 vehicle trial: `Rain test` initially showed C2 wiper/AUTO
data and BH `0x738` raw values, then both pages showed `no reply` after a few
seconds. Menu navigation still changed the IPC text. Leaving and re-entering
`Rain test` or cycling ignition did not promptly restore the readings; removing
power did. The same failure was reported with `Rain lights` ON and OFF; whether
the OFF choice was saved was not established in this trial.

Source review identifies a sufficient software cause even with `Rain lights`
OFF: C1 starts at most one board UART frame every 250 ms (about four per
second). With the BH page open, C2 and BH diagnostic polls each enqueue once
per second, regular C2/BH status polls add about 1.8 per second, and the
unchanged menu screen is retransmitted at least twice per second. Demand
exceeds service. The ten-slot command queue fills, while the separate screen
buffer still allows navigation. Diagnostic tokens and their one-second reply
deadlines start at enqueue, so valid replies to delayed transmissions are
discarded; the UI says `no reply` after three seconds without an accepted
reply. This message does not establish a broken BH or C2 physical link.

A scratch Apple Clang/ASan host replay linked the published `board_uart.c`,
`rain_test.c`, `rain_lights.c` and `usb_modes.c` with peers that always reply
immediately and `Rain lights` OFF. It showed BH values at seconds 2–4, BH
`no reply` from second 5, and C2 `no reply` after switching pages at second
15, while peer transmissions and screen updates continued. After 60 seconds
outside `Rain test`, values returned briefly on re-entry, then expired again.
The reproduction did not use vehicle hardware, so additional electrical
faults remain possible. The owner's `ARM=1` reading also needs separate
confirmation: the C2 implementation requires its rain-light option enabled
for that flag, whereas a confirmed saved OFF setting should report `ARM=0`.

Next correction: budget normal status, display and diagnostic traffic together;
keep at most one outstanding diagnostic poll per peer, measure reply age from
actual UART transmission, and add an integration regression covering both
pages and the shared queue. No firmware changed in this audit.

## v5-beta-32 release, 2026-10-25

PR [#86](https://github.com/eujot/BACCAble/pull/86) merged as
`d152bc0d43f095d013025c2aa71ff226f8b6ac16`; PR [#87](https://github.com/eujot/BACCAble/pull/87)
added the [release notes](releases/v5-beta-32.md). Published prerelease
[v5-beta-32](https://github.com/eujot/BACCAble/releases/tag/v5-beta-32) from
`14e29ed329316e170112743d96ec4c09a61cb37e`. Workflow
[40179656403](https://github.com/eujot/BACCAble/actions/runs/40179656403)
passed validation, tests, lint, all four firmware builds and publication using
the pinned Arm GNU 15.3.Rel1. All 14 assets passed `SHA256SUMS`; C1 uses
91,984/98,304 Flash bytes and 12,628/16,384 static RAM bytes. No board was
flashed. In-vehicle acceptance of the demo, live `0x738` view and Rain lights
actuation remains open.

## Rain test light-response page, 2026-10-25

The two commits from `feat/rain-test-light-sensor` are included in PR #86
(`feat/menu-demo-no-led`); its LED removal provides C1 Flash space.
`Actions → Rain test` now has a second page selected
with up/down. BH observes only standard data `0x738`/DLC 8, bytes 1 and 2,
and answers a C1 poll only while that page is visible. C1 distinguishes a
missing BH reply from a reply without a fresh CAN frame; raw zero remains valid.
The original C2 wiper/AUTO/force page and rain-light request logic are unchanged.
The two raw values are diagnostic observations, without a lux scale or a proven
darkness threshold. The machine and readable CAN catalogs now map both channels
using the existing 5,106 captured frames from 16 recordings and retain the
2026-10-24 isolation evidence. All C1/C2/BH boards need matching firmware.

Native host tests pass, including BH frame guards and C1 reply age/token checks.
Arm GNU 15.2.Rel1 builds C1 standard (90,388 text + 1,836 data = 92,224/98,304
Flash), C1 large (89,916 + 1,836 = 91,752/98,304), C2, BH and CAN. No board
was flashed. Vehicle acceptance remains open for the live `0x738` display,
BH link status and the separate rain-light actuation test.

## Display-only menu tour and LED-strip removal, 2026-10-25

Branch `feat/menu-demo-no-led` continues `fix/menu-navigation-pacing`. The
external WS281x LED-strip controller, its hardware timer/DMA setup, CAN and USB
hooks, and its visible Settings row were removed to recover C1 Flash space.
Flash setting slot 3 remains reserved and is cleared on load, preserving later
setting positions. The separate IPC `Shift light` feature remains available.

`Information → Demo` starts a timed tour of ordinary-looking menu screens and
illustrative readings. It varies pauses from 1.4 to 2.3 seconds, shows example
setting changes without applying them, and exits at the end or on a RES double
click. It makes no Flash writes or vehicle commands. Both user guides and the
menu UX guide describe the controls. The CAN dictionary now retains C1 `0x412`
as a historical LED-strip interpretation, removes its current dispatcher entry,
and records the provenance in a research note; no capture was reinterpreted.

Apple Clang's full host suite and the Lab knowledge tests pass. Arm GNU
15.2.Rel1 builds C1 in all four display profiles plus C2, BH and CAN. C1 uses
91,012/98,304 Flash bytes in the standard and MY23 profiles and
91,340/98,304 in both large profiles, leaving at least 6,964 bytes. The
previous large build left 172 bytes. No board was flashed; acceptance still
needs an in-car check of menu appearance, timing, exit gesture and Flash-slot
migration. PR #86 CI passed with the pinned 15.3.Rel1 compiler: host tests,
lint, all four firmware roles and the aggregate result.

PR #86 regression review checked the full pending UX change against `master`,
including navigation coalescing, save feedback, reading formatting, demo input
handling and removal of the LED timer/DMA and setting. No blocking firmware
defect was found. The demo renders fixed examples without calling setup edits
or vehicle actions; the reserved Flash slot preserves subsequent settings.
No firmware changes or additional hardware acceptance are claimed by this review.

## Menu navigation pacing and display test removal, 2026-10-25

Branch `fix/menu-navigation-pacing` continues `fix/readings-spacing`. C1 now
counts every directional click, sends the first view immediately, and coalesces
rapid subsequent clicks into the latest screen. Snapshots follow one complete
display transfer at the selected 10/20/50 ms pace; the final view is sent after
120 ms without another click. SELECT, BACK, HOLD, notices and saves still render
immediately. BH postpones its idle full-screen reassertion until one second
after the last changed screen, reducing old-screen traffic during browsing.
There is no physical IPC acknowledgment; radio arbitration may still delay a
visible update. Vehicle responsiveness remains to be checked.

The user-facing Information → IPC display test, its UTF/line-length/oil-glyph
patterns, the hidden `IPC diag` charset screens, their BH sender, dedicated host
tests and obsolete diagnostics workflow were removed. Ordinary MY23 glyph
mapping and oil-bar rendering remain. Both user guides, MENU_UX and the Makefile
guide were updated. Full Apple Clang host suite passes. Arm GNU 15.2.Rel1 builds
all four C1 display profiles and BH. C1 standard uses 95,952 text + 1,852 data
= 97,804/98,304 Flash bytes (500 free); large uses 96,280 + 1,852 =
98,132/98,304 (172 free). BH text fell from 35,744 to 34,540 bytes and BSS
from 12,400 to 12,200 bytes. No hardware was flashed; CI's separate 15.3.Rel1
compiler still needs its own size check.

## MY23 reading spacing, 2026-10-25

Branch `fix/readings-spacing` continues `fix/menu-save-feedback`. The compact
four-temperature page now presents `O/W/I/X` before their numbers, moves spare
padding between values, and preserves all four values when several reach three
digits. The four misfire counts use slash separators in cylinder order.
`Oil/coolant temp` keeps a space before both temperatures. Readings and Favorite
slots share this formatter; legacy page templates and persisted page IDs are
unchanged. Both user guides and MENU_UX now show the actual compact examples.
The fast menu suite passes across legacy/MY23 profiles. All four C1 display
build profiles link with Arm GNU 15.2.Rel1. Standard C1 uses 96,168 text +
1,852 data bytes; `LARGE_DISPLAY` uses 96,428 text + 1,852 data bytes, leaving
only 24 bytes below the 98,304-byte Flash limit. CI's 15.3.Rel1 compiler must
check that tighter build independently. No board was flashed; MY23 readability
still needs vehicle review.

## Menu save feedback, 2026-10-25

Branch `fix/menu-save-feedback`, based on `origin/master` after PR #84. The MY23
save confirmation now uses plain `SAVED` without the checkmark that the owner's
IPC did not render correctly. It stays visible for three seconds to allow
normal UART delivery and the fragmented BH display update when a factory
message briefly defers the menu. The transient one-line `Saved` submission was
removed so the save event starts with the complete title/result screen. Explicit save, draft,
discard and action semantics are unchanged. The two user guides and MENU_UX are
updated. Physical visibility under continuous factory display traffic still
needs vehicle acceptance; this is not an IPC delivery acknowledgement.
Validation: `test-menu-fast` passes in legacy and MY23 profiles, including the
new feedback-duration case; C1 builds with Arm GNU 15.2.Rel1 at
96,096 bytes of text, 1,852 bytes of data and 13,568 bytes of BSS. No board
was flashed. The user's existing untracked captures and checkouts were left
untouched in the original worktree.

## v5-beta-31 release preparation, 2026-10-24

PR [#84](https://github.com/eujot/BACCAble/pull/84) merged to `master` as
`46bf4d67a099c70be19e338c966dbb595fec3dee`. Its first CI run found stale source
hashes in the CAN dictionary; snapshot `2026-10-24.9` and the generated catalog
were refreshed, after which CI run
[40129272385](https://github.com/eujot/BACCAble/actions/runs/40129272385) passed
tests, Lab checks, lint and all four firmware builds. Published prerelease
[v5-beta-31](https://github.com/eujot/BACCAble/releases/tag/v5-beta-31) from the
merged commit. Release workflow
[40129492178](https://github.com/eujot/BACCAble/actions/runs/40129492178) passed
the pinned Arm GNU 15.3.Rel1 build, display matrix, size/identity checks and
packaging. All 14 release assets passed `SHA256SUMS`; C1 uses 97,640/98,304
Flash bytes and 15,420/16,384 static RAM bytes. No board was flashed and no
vehicle lamp acceptance is claimed; the Rain lights output check remains open.

## Rain lights debug/test surface, 2026-10-24

Reliability correction on `fix/rain-test-reliability`: C2 now bounds forced
0x73E additions to 30 s and expires them after 3 s without C1 commands; an
explicit OFF is needed before rearming after expiry. C1 retries a queue-rejected
STOP even after the screen closes. Its screen accepts only matching, timely C2
replies, marks missing/stale data and stale AUTO as unknown, and labels queue
admissions `Q`. The raw 0x2EC rising-phase counter is independent of the saved
Rain lights setting. This corrects the three review findings without changing
the normal rain-trigger threshold or output masks. Full native host suite and
four ARM build/lint flavors pass with Arm GNU 15.2.Rel1. Standard C1 uses
97,884/98,304 Flash bytes and 15,420/16,384 static RAM bytes. Vehicle lamp
acceptance remains UNKNOWN; no board was flashed. Next: verify in the car that
forced output stops on BACK, at 30 s and when the C1/C2 link is interrupted.

Branch `feat/rain-lights-debug`, based on `master`. The owner reported that the
beta-30 Rain lights feature does not turn the lamps on. This adds a normal-firmware
test surface to isolate detection from actuation, designed for later removal
(`features/rain_test.c/h` plus clearly marked hooks).

Dipped beam verified on the vehicle: C2 `0x73E` byte 1 bit 6 (BH `0x354` byte 1
bit 6 mirrors it; C1/C2 `0x384` byte 3 bit 7 follows). DRL is a separate,
always-on signal and is not affected by these bits.

- `rain_lights_force()` and detection counters (`wiper_cycles`, `lamp_tx`) were
  added to `features/rain_lights.c`; `rain_lights_frame()` now injects the
  bus-specific bits when `active` **or** forced.
- New UART commands `C2_CMD_RAIN_TEST 0x4E` / `C1_CMD_RAIN_TEST 0x4F` carry the
  force flag and C2 detection counters to C1.
- One menu action `Rain test` (Actions, no permission) opens a two-line screen:
  wiper-cycle count + AUTO on line 1, armed/injected counters on line 2. SELECT
  forces the `0x73E` lamp bits for up to 30 s; BACK exits and stops forcing.

Verification: new host tests `test_rain_test.c` (C1/C2) pass; the full host
suite passes; all four flavours build and lint with Arm GNU 15.2.Rel1. C1 size
is 97,604/15,412 bytes (about 700 bytes of flash headroom). Both user guides
and this plan record the new action. No device flashed; vehicle acceptance of
the force test is the next step.

## Vehicle acceptance of startup Race and suspension, 2026-10-24

The owner tested the merged beta-30 source in the car and reports that both
startup actions work: `Default mode` = `Race` selects Race, and `Suspension` =
`Soft` applies the softer damper setting. Together with the earlier D report
this accepts the injected BH `0x256` direction gesture and the 150 ms suspension
pulse on the vehicle. It also supersedes the "Race and suspension still need
acceptance" notes in beta-29/beta-30 research, the dictionary and both guides.

Limits of this evidence: no capture, marker or CAN trace accompanies the report;
the feedback-gated retry path (two ignored pulses) was not separately exercised;
`A` was not reported; and Rain lights output acceptance is unrelated and still
open. The physical sender of `0x256` remains unidentified. Recorded as
`implemented_and_vehicle_accepted` for the startup selector and the reliability
correction, with the retry caveat kept in the detail. Dictionary
`2026-10-24.8`; the readable catalog was regenerated.

Validation: Lab suite passes; firmware, menus and behaviour are unchanged by
this documentation commit, so no rebuild or device flash is implied.

## v5-beta-30 release, 2026-10-24

PR #77 (`241b993`) added `Rain lights`, DNA Race/suspension reliability
changes and the October 24 CAN research reports. PR #78 (`40f2fa5`) fixes the
rain-command UART scheduler so C2/BH status and USB polls remain fresh. Full
notes: [v5-beta-30](releases/v5-beta-30.md).

Published prerelease: https://github.com/eujot/BACCAble/releases/tag/v5-beta-30
from `1bd28fab7b5048ebf99aa2cc3e85d263f04583f8`, embedded identity
`beta-1bd28fa`. Official workflow [40099405814](https://github.com/eujot/BACCAble/actions/runs/40099405814)
passed host/Lab tests, lint, all four firmware builds, display/IPC matrix,
size/identity gates, packaging and publication. All 12 firmware assets plus
`BUILD_INFO.json` and `SHA256SUMS` were downloaded and verified; C1 uses
96,936/98,304 flash bytes and 15,388/16,384 static RAM. No hardware flash or
vehicle acceptance is claimed.

The local regression demonstrates that the prior scheduler stops a C2-only
capture and that the correction keeps status/USB active during display updates
and one lost rain reply. It also asserts captured C1/C2 lighting payloads.
Owner-confirmed lamps-on trials used the light sensor or physical switch; the
remaining vehicle check is acceptance of BACCAble's additional transmission
while factory frames continue. Release branch `docs/v5-beta-30` preserves the
release notes and handoff for integration into master.

## Merge beta-30 into the rain/light scenario branch, 2026-10-24

Branch `merge/rain-scenario-into-beta30`, created from
`feat/rain-lights-scenario` (`4b07267`, five commits based on `495b335`) and
merged with `origin/master` (`40f2fa5`, v5-beta-30 source) and the published
`origin/docs/v5-beta-30`. The branch was 19 commits behind and had no firmware
changes, so the merge brought in beta-29, Rain lights, the paced DNA/suspension
correction and all October-24 research.

Conflicts were limited to three files and resolved without discarding either
side. `giulia.json` keeps the owner-reported `0x256` acceptance while adding
`50f7604`'s implementation and retry description; it keeps `0x25A` `supported`
with the mode-relative damper table and adds master's new `0x2EC`, `0x738` and
`0x73E` research entries. `CAN_DICTIONARY.md` was regenerated from the merged
dictionary (`2026-10-24.7`). `docs/ACTION_PLAN.md` keeps both histories and
marks the branch's former "wiper frames not identified" and "Race mid never
recorded" statements as superseded. The branch's new `suspension_hard/soft/mid`
and `light_sensor_*` catalog labels were moved out of the `marker_index`
"Recorded non-catalog events" fallback into their real groups, so the same event
is not listed twice.

Validation after the merge: 81 Lab tests pass, the full host suite passes, and
all four flavours build with Arm GNU 15.2.Rel1 and four-flavor `lint`. Standard
sizes: C1 97,180/15,388 bytes, C2 31,072/12,392, BH 36,608/13,260, CAN 25,196/7,000.
C1 has only about 1.1 KiB of flash headroom. No firmware behaviour changed in
this merge and no device was flashed. Still open: acceptance of the Rain lights
lamp injection while factory frames continue, and observation of the guarded
suspension retry path.

## Rain-light UART correction and owner clarification, 2026-10-24

Branch `fix/rain-lights-polling`, worktree `/private/tmp/baccable-rain-lights-review`,
based on merged PR #77 (`241b993`) and the committed focused review. Corrected
the review's confirmed bug: rain commands no longer move either peer's regular
status/USB poll deadline. Those timers advance only after successful enqueue;
USB/version alternation also advances only on successful enqueue, so a rejected
request retries the same kind. Rain thresholds, lamp masks and release policy
are unchanged.

New `test_rain_uart` links production UART, USB manager and C1 rain controller.
With 20 ms screen requests, C2-only USB capture and one dropped rain response,
the old firmware fails because capture stops. The correction preserves capture
and continuous C1 light additions for 60 simulated seconds. Maximum reply gaps:
C2 version/USB 2,259/2,259 ms, BH 2,761/2,761 ms, rain 1,506 ms. A separate
regression covers retry without skipping a poll type. Captured C2/C1 lamp
patterns are now asserted explicitly by the rain-light tests. Full native
ASan/UBSan suite, 16 ARM board/display builds and four-flavor cppcheck pass
(Arm GNU 15.2.Rel1, `rain-poll`).

Owner clarification: the confirmed lamps-on tests used the covered light sensor
or physical switch. That physical evidence is retained and the corresponding
CAN values are known; it is incorrect to describe the lamp values themselves
as unknown. C2 0x73E `00 40 00 40` and C1 0x73E `02 00 80 00` are already the
patterns generated by Rain lights. Acceptance of BACCAble's additional replay
while factory messages continue remains the specific hardware gap. No guessed
BCM request or fabricated acceptance result was added. Both guides and MENU_UX
now explain this distinction. [Correction and evidence](research/20261024-rain-light-polling-correction.md).

Original recordings and the user's scenario checkout/untracked files remain
untouched. This correction is committed locally; no push, PR, merge, release
or flash performed. Implementation `e6159c1`; dictionary snapshot `2026-10-24.7`
and the readable catalog now identify that source and preserve the owner's
clarification. All 77 Lab tests and all four size/identity gates pass. Standard
C1 flash/RAM: 97,176/15,388 bytes; maximum C1 static RAM across display profiles
is 15,596 bytes. Runtime stack/physical lamp acceptance remain vehicle checks.


## Focused review of merged Rain lights, 2026-10-24

Reviewed current `origin/master` `241b993` (PR #77 merged), concentrating on
serious rain-light failures. Firmware is unchanged by this review. One confirmed
integration defect remains: C1 `rain_lights_process()` sends C2 requests every
1,000 ms; every C2-addressed transmission resets `last_c2_poll_time` in
`board_uart_process()`. Ordinary version/USB polling waits for 1,010 ms and
is therefore delayed repeatedly. Version and USB queries alternate, while their
consumers require replies within five seconds.

A scratch Apple Clang ASan/UBSan harness linked the actual board UART and rain
controller, used the existing HAL fixture, completed TX after 6 ms, and ran the
normal main-loop ordering for 60 simulated seconds. With the production
alternating poll types, maximum C2 version-query gaps were 2,522 ms (Rain OFF),
8,001 ms (Rain ON), and 9,789 ms (Rain ON with repeated screen updates).
The harness is `/private/tmp/review_rain_uart.c`; no hardware result is claimed.
This can produce `No reply`, stale USB status and cancellation of a capture
connected only to C2 once C1's ten-second disconnected grace has expired.
Fix the scheduler so rain requests cannot postpone required status/USB polls;
verify both poll freshness and rain leases together. Existing isolated rain
unit tests stub UART delivery and do not exercise this interaction.

The other material limit remains physical output acceptance: 0x73E is added
after factory frames without suppressing the original values, and there is no
independent physical lamp confirmation. The resulting steady front/rear lamp
operation and manual/darkness handover still require the documented vehicle
trial; this review does not call that unresolved behavior a proven failure.
No additional serious issue was identified in the focused 15/60-second timer,
cycle-edge, setting migration and field-preservation inspection. No full-suite
rerun, firmware change, push or release. Review handoff is committed on
`review/rain-lights-master`; the user's scenario checkout and untracked files
remain untouched.


## Rain-triggered lights menu and controller, 2026-10-24

Local branch `feat/rain-lights-auto`, isolated at `/private/tmp/baccable-rain-lights`,
based on `60c6484` (includes the committed three-session analysis and startup
DNA/suspension correction). Added `Settings` → `Features` → `Comfort` → `Rain lights`, OFF/ON,
default OFF. Three fresh C2 0x2EC/DLC8 b2.5 rising phases within 15 seconds
request lights; each further cycle renews a 60-second hold. First-high samples,
repeated highs, malformed frames and stale streams do not count. AUTO is required
from fresh C2 0x73E/DLC4 b3.6. Washer cycles are intentionally not excluded.

Output adds only dipped-beam bits on C2 and position-lamp bits on C1 to new
factory 0x73E templates, preserving all other fields. It stops additions on
release; no explicit OFF is transmitted. Factory frames are not intercepted.
C1/C2 poll/response tokens and three-second leases bound missing-link behavior;
manual non-AUTO stops C2 immediately and C1 on its next reply (normally <=1 s,
<=3 s without a reply). A missed poll response alone does not expire C1 within
the next ordinary poll interval. C1 sleep stops requests. Setting is stored in
new slot 42; import both previous 40- and 41-slot records, preserving every
existing value. Downgrade firmware may not read the extended settings record.
Both user guides and MENU_UX describe times, prerequisites and acceptance limits.

Implementation `b22543d`; dictionary source snapshot `2026-10-24.6` and
readable catalog refreshed without changing captured layouts or original evidence.
All 77 Lab tests and four standard size/identity gates pass.
Native ASan/UBSan host suite passes, including new C1/C2 threshold/hold/lease/
manual priority/tick-wrap cases, exact 15-second threshold and old 80-/82-byte
settings migration. All 16 ARM board/display builds and four-flavor cppcheck
pass with full Arm GNU 15.2.Rel1; final source size and dictionary provenance
are recorded with this task's verification. Standard flash/RAM: C1
97,288/15,388 bytes; C2 31,072/12,392. Largest C1 RAM is 15,596 bytes
(large display); runtime stack headroom remains a hardware acceptance item. Original captures and the user's
scenario checkout/untracked files remain untouched.

Hardware acceptance is UNKNOWN. The input evidence is sufficient for this
implementation, but 0x73E injection has not demonstrated sustained dipped-beam
and complete front/rear position-lamp acceptance. Next is one stationary trial
with visible front/rear lamps, AUTO daylight after three cycles, 60-second
release, darkness handover, manual selection and lost-link behavior. Record
physical outcomes; a C2 rain-request reply and queue admission are not lamp
success. No push, PR, merge, release or flash performed.


## Startup DNA gesture and suspension reliability correction, 2026-10-24

Task branch `fix/startup-dna-gestures`, isolated worktree
`/private/tmp/baccable-startup-dna-gestures`, based on the three-session audit.
The owner reports beta-29 D selection working, Race failing, and one D/Soft
attempt selecting D without Soft. This provides limited D replay acceptance,
not repeatable Race/suspension acceptance. No automatic-failure TX recording is
available, so the exact vehicle cause is not asserted.

Concrete source/waveform weaknesses: beta-29's held input used its own 250 ms
refresh clock while factory-neutral 0x256 messages still arrived; short pulses
could start immediately before those neutral messages. Its fixture did not
exercise recorded cadences or a first accepted pulse ignored by the receiver.
Nominal 1.8 s already exceeds the 1.196/1.210 s captured Race response times;
continuity and pacing, rather than duration alone, need correction.

New presses align within 20 ms of a fresh neutral report, wait for all earlier
software/hardware TX to drain, and require one second of actual-mode stability.
Incoming factory-neutral reports schedule held-input reassertion. Race holds
2.2 s with a 250 ms fallback refresh. Suspension retains 150 ms pulses and
300 ms settled feedback. One retry is allowed only after release plus at least
one second of fresh, unchanged opposite calibration, drained TX and no observed
desired-state response. Any desired reply, even brief, or missing/unknown data
prevents a blind retry. Physical input, stale mode/RPM and engine stop retain
priority. BH operation is bounded to 12 s after its maximum 3 s engine gate;
C1 now waits 20 s for BH, then uses the unchanged five-second corroboration.
UART/settings formats and menu labels are unchanged; update C1/BH together.

Both guides and MENU_UX describe the changed timing/retry and partial-failure
meaning. [Failure review and limits](research/20261024-startup-gesture-correction.md).
Two focused regressions fail against beta-29 source and pass after correction:
neutral during a long hold, and an ignored first suspension pulse. Full native
Apple Clang ASan/UBSan host tests pass. All 16 board/display builds and cppcheck
for C1/C2/BH/CAN pass with the full Arm GNU 15.2.Rel1 toolchain and local version
`startup-dna`. Standard flash/RAM: C1 96,744/15,364 bytes; BH 36,608/13,260.
Size gates pass. Source snapshot `50f7604` and dictionary `2026-10-24.5` now
record the correction; all 77 Lab tests pass with the refreshed source hashes. No push, merge, release, flash or new vehicle
acceptance. Original captures, user scenario branch and untracked files remain.

## Three-session audit and labelled Race/Mid result, 2026-10-24

On local `research/rain-lights-correlation`, audited `20261024-105643Z`,
`20261024-111510Z` and `20261024-113104Z`: 90/90 human confirmations and
4,526,576 frames. Ordered timestamp/ID/DLC/payload hashes from all nine raw
streams match SQLite; original fingerprints and both prior published field
traces are unchanged. Startup losses, saturated counts and old buffered-clock
segments remain explicit. No original recording or marker was changed.

The new matrix directly supports D Stiff/Soft=1/0 and Race Stiff/Mid=2/1 in
BH `0x25A` b1[5:4]. Twelve suspension pulses give four N/A negative controls
and eight D/Race responses after 25–45 ms. Both complete Race Mid/Stiff pairs
agree with C1/C2 `0x384` b6.7 and mode-dependent `0x1FC` codes. All 26 confirmed
matrix states match received reports. Race entry has a 35–36 ms transitional
`04 10` before default `04 20`; the existing 300 ms settling rule matters.
Firmware feedback codes already agree. All 151 transmission 0x5A8 frames pass
CRC and remain Race during Mid. This is manual-input evidence, not automatic
startup or power-cycle acceptance.

The older 0x1FC b0.4 engine-running hypothesis is now refuted: the flag changes
with mode/calibration while fresh RPM remains 797–974. Dictionary `2026-10-24.4`
adds guarded raw calibration reports and retains unknown values. The two earlier
rain/light analyses are confirmed, including the 14.411/14.474 s delayed lamp
release. Retired AUTO-as-lamp marker rules remain historical; fresh reviews and
stable templates now cover all three latest sessions. The generic correlator
now honors exact DLC, covered by a false-short-frame regression test. Both user
guides and MENU_UX replace Race/Mid inference with direct capture evidence while
retaining the startup replay acceptance gap. All 77 Lab tests pass.

[Consolidated report, tables and evidence](../baccable_lab/docs/research/20261024-three-session-audit.md).
This supersedes older dated Mid uncertainty and incomplete rain/wiper claims.
No firmware behavior changed. Next hardware acceptance: saved A/D/Race startup
selection and D Soft/Race Mid application through an actual ignition/start,
including manual priority; separately test complete lamp-output operation and
factory/manual priority. Further broad input-identification recordings are not
needed to rediscover these mapped controls. Work remains local on the committed
research branch; the user's scenario branch and untracked files are preserved.

## Rain/light follow-up capture, 2026-10-24

On `research/rain-lights-correlation`, reviewed `20261024-111510Z`: 28/28 steps,
1,644,370 frames, context Giulia 2.0 / v5-beta-29 / daylight / outdoors. Raw
replay matches SQLite and fingerprints are unchanged; saturated C1/C2 startup
losses and the initial C1/BH old-clock segment remain explicit. No recorded
later drops, with the usual capture-quality limitations.

Two sensor-covered trials with lights manually OFF independently support BH
`0x738` b1/b2 as light-responsive raw channels. The raw-response hypothesis is
now supported, with no units/source claim. In the labelled AUTO cycles, lamp OFF
follows both channels returning to 6 by 14.411 and 14.474 s on the BH clock;
short correlation windows would be misleading. C2 `0x2EC` adds 27 wiper-phase
pulses and two independent slow/fast/OFF controls. Spray pulses repeat, but old
washer contradictions remain. No new raw rain decoder or proven lamp command.

[Follow-up evidence and timing](../baccable_lab/docs/research/20261024-111510-rain-lights-followup.md).
Dictionary `2026-10-24.3` retains layouts and adds cumulative evidence. The next
vehicle check remains lamp-output acceptance with factory/manual priority.
All 75 Lab tests pass; no firmware or user-facing behavior changed.

## Rain/wiper and automatic-lighting correlation, 2026-10-24

Research branch `research/rain-lights-correlation`, based on `939a85e`; isolated
worktree `/private/tmp/baccable-rain-lights-correlation`. Reviewed complete
`20261024-105643Z` and compared all 24 databases through that session: 23,535,295
frames, including failed/empty recordings. The target has 36/36 confirmations
and 2,157,769 frames; raw replay matches SQLite on every bus. Original hashes,
startup losses, malformed bytes, events and notes remain in the evidence.

C2 `0x2EC` DLC 8 byte 2 bit 5 supports a wiper-cycle phase: 28 pulses in the
new capture and 44 in `20261010-111025Z`; slow rises are about 1.50 s apart,
fast about 0.95 s. Physical one-edge-per-sweep semantics remain unverified.
BH `0x354` / C2 `0x73E` separate AUTO mode (byte 3 bit 6) from dipped indication
(byte 1 bit 6), supported by the constant-AUTO/variable-lamp recording and older
manual-light controls. BH `0x738` bytes 1/2 respond to covering/uncovering before
delayed lamp OFF; they remain raw-channel candidates with unknown units/source.
Rejecting that whole frame as a counter in the scenario-branch handoff was too
broad. C1 `0x41A` mask 0x60 also follows unrelated consumers and is rejected as
a specific wiper trigger. Raw rain and the apparent washer bit remain unresolved.

[Report and reproducible evidence](../baccable_lab/docs/research/20261024-rain-lights-correlation.md).
Dictionary `2026-10-24.2`, hypotheses and the regenerated catalog retain prior
provenance and contradictions. The cumulative ledger references the immutable
old detailed review and adds the fresh target. All 75 Lab tests pass, including
captured fields, length/bus guards, source snapshots and catalog generation.
No firmware, menu, user guide or vehicle action changed. Next: a small stationary
lamp-output check with front/rear observation and factory/manual priority. A full
repeat of the sensor scenario is unnecessary to find a wiper trigger.

## v5-beta-29 release verification, 2026-10-24

Published [v5-beta-29](https://github.com/eujot/BACCAble/releases/tag/v5-beta-29)
from master commit `c2b6ba773b35cd15bb6baa7fd5eb380b471aeb81`, after PR #74
(firmware changes) and PR #75 (release notes). The release workflow
[40077905379](https://github.com/eujot/BACCAble/actions/runs/40077905379)
passed host/Lab tests, lint, all C1/C2/BH/CAN builds, independent IPC/display
profiles, size and identity checks, and packaging. The 14 downloaded assets
all pass `SHA256SUMS`. The pinned Arm GNU 15.3.Rel1 build records C1 at
96,500/98,304 flash bytes and 15,364/16,384 RAM bytes.

Full notes and compatibility details are in [v5-beta-29](releases/v5-beta-29.md).
No firmware was flashed. Vehicle acceptance still needs to confirm startup
selection and suspension behavior, especially Race/Mid feedback.

## Startup configuration and diagnostic-menu removal audit, 2026-10-24

Integrated `8337bbf` (engine gate/suspension) and `d576d11` (menu removal) on
`fix/audit-dna-suspension`, worktree `/private/tmp/baccable-audit-dna-suspension`.
This audit supersedes the initial implementation/validation statements below.

Corrected concrete defects:

- Suspension incorrectly reused slot 36, already assigned to V6. Slot 41 and
  82-byte records now import the existing 80-byte settings with default Stiff;
  engine and suspension values persist independently, including failed-save handling.
- Two calls to the removed reading-position helper broke compilation. Removed
  those dependencies, corrected remaining six-entry navigation fixtures and
  reduced reading-list buffers from 128 to 72 entries (visibility capacity).
- D suspension feedback was inverted. Capture plus owner description establish
  Stiff=1 and Soft=0 in BH 0x25A byte1[5:4]. Race Stiff=2 is observed after entry;
  Mid=1 remains an explicit inference, documented in the source, guides and dictionary.
- Stiff now checks the actual state even when already in D/Race. Both preferences
  require fresh, settled feedback. Unknown codes invalidate old data. A single
  accepted suspension pulse is never retried merely because feedback is absent.
- A manual button/selector press immediately hands control to the driver, without
  injecting a synthetic release over their input. Engine loss, timeout and stale
  data still release accepted synthetic presses; failed release queues retry.
- C1 starts the 15-second reply deadline only after UART accepts the request.
  BH has at most 3 seconds to corroborate engine readiness, then 10 seconds to
  complete its sequence, preventing delayed execution of an old start request.
- Unified selector/suspension press and release phases, removed the redundant
  configuration wrapper and C1 timeout flag, and fitted startup labels to 18 glyphs.
- Restored the 96 KiB C1 application budget and 20 KiB optional USB disk, including
  linker, storage boundary, CI report limit and both guides. Files on custom
  16 KiB C1 mass-storage builds need backup before the disk-layout change.

The removed menu has no remaining executable module, symbol, catalog, test or
build dependency. Ordinary Readings/OBD, CAN capture and unrelated IPC diagnostics
remain. Dated release/research history describes older versions, not active code.

Validation: full Apple Clang ASan/UBSan host suite and all eight release-script
tests pass. All 16 board/display builds and cppcheck for C1/C2/BH/CAN pass using
full Arm GNU 15.2.Rel1, VERSION=audit-dna. Standard C1 uses 96,740/98,304 flash
bytes and data+bss 15,364/16,384; largest display variant uses 96,840 flash bytes
(1,464 spare) and data+bss 15,572. All 73 Lab tests pass with refreshed source hashes for implementation
commit `1d6a86e`; the readable CAN catalog is regenerated. No device was flashed and no
hardware acceptance or pinned-CI result is claimed. Race/Mid feedback and
startup behavior remain vehicle acceptance items. Main checkout files and its
pre-existing untracked capture/release folders were preserved.

## Remove the Experiments DID menu, 2026-10-23

On `remove/experiments-menu-wt`, removed the standalone Experiments root-menu
entry and its 64 Pelican/OBDb `0x22` signal catalog, polling, ISO-TP decoding,
CAN receive hook, and dedicated catalog/transport/navigation tests. The shared
OBD request and parameter-catalog paths used by ordinary `Readings` remain.
Both user guides, `MENU_UX.md` and `CATALOG_AUDIT.md` now describe the five-item
root menu and omit the removed feature. This change does not remove generic OBD support or the
vehicle parameter readings already used by `Readings` and `Favorites`.

No tests or firmware builds were run for this removal.

Last updated: **2026-10-24** (dated verification boundaries below). This is the single source of planned work,
integration status and outstanding acceptance checks. Technical guides describe
implementation; they are not separate backlogs. Update this file after each task.

## Beta-29 drive-mode acceptance and damper decode, 2026-10-24

Owner-reported vehicle acceptance: the default drive-mode gesture shipped in
`00b4b99` (v5-beta-29) changes the drive mode in the car and other controllers
react. This is the first confirmed acceptance of an injected frame — a listener
on the BH body bus acts on the replayed `0x256` selector input and the resulting
mode reaches the powertrain and chassis controllers. It does **not** identify
the physical sender of `0x256`, and it is a single vehicle trial, not a repeated
acceptance test. The `giulia.json` note and the DNA topic status now record the
result; the generated `CAN_DICTIONARY.md` was refreshed.

Session `20261024-113104Z` (`dna_suspension_matrix`, 26/26 confirmations, two
runs) also resolved the damper field. BH `0x25A` byte 1 is **mode-relative**:
D=`0x10` hard, D=`0x00` soft, Race=`0x20` hard, Race=`0x10` mid. Entering a mode
resets it, and 8/8 button presses matched in both directions. C1/C2 `0x384`
byte 6 bit 7 and C1/C2 `0x1FC` byte 0 also change with the softer state, but
`0x1FC` must be read with the actual mode: N/A=`0x50`, D hard/soft=`0x40`/`0x50`,
Race hard/mid=`0x60`/`0x40`. The same `0x10` in `0x25A` means D hard and Race
mid, so a decoder must combine it with the mode; mode entry carries the previous
damper level for 24–36 ms, so settle before decoding (returning from Race to N
passes through D as a real intermediate mode). The earlier "mid = `0x30`"
hypothesis is refuted.

Validation: 77 Lab tests pass after regenerating the catalog. Firmware, menus
and user guides unchanged. When written, the open item was repeated beta-29
acceptance trials and whether an injected suspension pulse or a `0x25A` rewrite
is accepted. Superseded on 2026-10-24: D, Race and the injected softer
suspension pulse were accepted in the car; see the acceptance section above.

## DNA and suspension matrix scenario, 2026-10-24

Added `dna_suspension_matrix` (drivetrain, stationary, 13 steps, 2 runs, 26
confirmations) on the scenario branch. It answers both questions in one short
run: which report carries each drive mode, and what the damper does in every
mode. Sequence:

| Mode | Default | After one press | After a second press |
| --- | --- | --- | --- |
| N | control | nothing (control) | — |
| A | control | nothing (control) | — |
| D | hard | soft | hard |
| Race | hard | mid (`0x10`) | hard |

Added catalog labels `suspension_hard`, `suspension_soft` and `suspension_mid`
(Polish titles, search aliases) because the old `suspension_active` /
`suspension_inactive` pair cannot distinguish three states. The procedure uses
the per-mode default as a built-in reset: entering a mode returns the damper to
factory state, so every mode starts from its own confirmation. N and A are
explicit negative controls for the console button.

Expected capture evidence: mode codes on `0x384` (C1/C2), `0x46C` and `0x25A`
(BH); damper state on `0x25A` byte 1, `0x384` byte 6 bit 7 and `0x1FC` byte 0
bit 4; button pulse on `0x256` byte 1 bit 7. The Race mid code had never been
recorded before this procedure; the first run (`20261024-113104Z`) closed it at
Race=`0x10` (mid) versus Race=`0x20` (hard) in `0x25A` byte 1.

Validation: 77 Lab tests pass; library now 84 procedures / 581 confirmations.
No firmware, menu or user-guide change. Hardware acceptance was UNKNOWN when this
procedure was written. Superseded: the procedure was run as `20261024-113104Z`
(26/26 confirmations, two runs) and its matrix is documented in
`baccable_lab/docs/research/20261024-three-session-audit.md`. The startup Race
and softer-suspension settings were then accepted in the car on 2026-10-24.

## Rain-to-lights capture scenario, 2026-10-23

Branch `feat/rain-lights-scenario`, based on `495b335`. Replaces the 11-step
`wipers_lights_auto` procedure with the 12-step `rain_lights_auto` (group
`wipers`, stationary, 3 runs, 36 confirmations) so the two sensor signals can be
recorded from one short, repeatable procedure instead of a long mixed test. The
procedure has **no time targets** (`min_separation_s = 0`, and the dashboard
hides the separation hint): each step is confirmed when its effect is visible,
never after a fixed delay.

**The factory car does not switch the dipped beams on because of rain or wiper
operation** — that is the firmware feature still to be added. The rain phases
therefore record the lamps staying **off** (`dipped_beam_off`) as the negative
control, which proves they were not already on for another reason. After the
firmware change those steps become `dipped_beam_on` and the same procedure is the
acceptance test. Phases:

1. light sensor covered and uncovered with the wipers **off** — an opaque cloth
   simulates night and its removal restores daylight; this isolates the
   light-sensor signal and confirms the factory AUTO function, because covering
   cannot start a wipe when the wipers are off;
2. wipers in AUTO plus washer water, then drying, **in daylight** — isolates
   rain/wiper activation (confirm that the wipers started, not after a set
   spraying time) while daylight cannot switch the lamps on;
3. three manual wiper cycles, still in daylight — the third future trigger,
   separated from the rain sensor.

**Daylight outdoors is a requirement.** The procedure starts with the light
sensor uncovered and the lamps off. After dark, or in a dark underground garage,
AUTO switches the lamps on because of darkness and the `dipped_beam_off`
negative control proves nothing. A flashlight variant was tried in the car and
**discarded**: this vehicle does not react to a torch pointed at the sensor
behind the mirror, so the first step never reached the expected lamps-off state.
The cloth method is the supported one and is now asserted by the test.

Added catalog labels `light_sensor_dark` / `light_sensor_bright` (Polish titles,
search aliases): the existing `ambient_light_changed` cannot separate the two
directions. Updated `baccable_lab/docs/SCENARIOS.md` (counts 83 procedures / 555
confirmations, phase-order rationale, no-time rule and the negative-control
statement) and `baccable_lab/docs/MARKERS.md`. Tests assert the phase order, the
daylight assumption, the lamps-off control, the hidden time hint and the absence
of the discarded flashlight variant.

### Decisive sensor/wiper scenario, 2026-10-24

Added `sensor_wiper_lights` (wipers, stationary, 14 steps, 2 runs, 28
confirmations) after analysing the first real recording of `rain_lights_auto`
(`20261024-105643Z`). That session is complete: 36/36 confirmations, three runs,
no loss during capture (C2/BH lost about one second at USB start-up only). It
shows one flag toggling exactly three times with the covered sensor on C1/C2
`0x384` byte 3 bit 7, BH `0x354` byte 1 bit 6 and C1/C2 `0x73E`, but the test
cannot tell whether that flag is the lamp output or the sensor's dark decision.

The new procedure separates them in phase A: with the light switch **OFF**, the
sensor is covered while the lamps stay off, so either outcome is decisive. Phase
B then gives the AUTO lamp pair, phase C gives steady slow/fast/off wiper states
that AUTO cannot provide, and phase D gives the rain trigger with the lamps-off
control. It also lets the slow setting complete three sweeps, which covers the
future three-cycle trigger without an extra step. Wiper frames were **not**
identified in the first recording: the apparent candidates (`BH 0x738`, C1
`0x4B1`, C1 `0x4B2`) are monotonic counters, which is why phase C needs steady
states rather than AUTO cycling.

Superseded on 2026-10-24: the later reviews identified C2 `0x2EC` byte 2 bit 5 as
a repeating wiper-cycle phase (1.5 s slow, 0.95 s fast, low after OFF). The
blanket "monotonic counter" dismissal was too broad — BH `0x738` bytes 1–2 are
light-responsive raw channels even though the frame as a whole changes
monotonically. Counting edges with a timeout, not steady occupancy, is the
working input condition. See
`baccable_lab/docs/research/20261024-rain-lights-correlation.md`.

Validation: 76 Lab tests pass; host tests, firmware and the device user guides
are untouched (no menu, setting, reading or device action changed). Hardware
acceptance was UNKNOWN when this procedure was written. Superseded: the
procedure was run as `20261024-111510Z` (28/28 confirmations, two runs), and
that recording supplied C2 `0x2EC` wiper phases and the AUTO/lamp separation.
Rain-light output acceptance remains open.

## Analysis absence protocol, 2026-10-23

Branch `docs/analysis-absence-protocol`, based on `495b335`. Documentation-only
change: `baccable_lab/docs/SESSION_ANALYSIS.md` now carries a blocking checklist
for negative claims (“no request frame”, “no signal for this control”), derived
from the missed BH `0x256` DNA selector input. The checks cover the actual
failure modes: matching frames by ID instead of (bus, ID, DLC); ranking
occasional pulses by whole-session frequency; a pre-window that contained the
event's first sample; ignoring boards classified as report-only; discarding a
skipped scenario; anchoring on an operator note instead of an observed event;
accepting a single occurrence; and recording a refutation without its method.
The rule now requires the wording “not demonstrated with this method in these
recordings”, never “does not exist”.

No code, firmware, scenario, menu or user-guide change; no new tests. The Lab
suite still passes. Next candidate: a generic triage script that, for every
session, enumerates all (bus, ID, DLC) identities, computes period and payload
stability, isolates short non-neutral excursions, and correlates each excursion
with every marker, so occasional signals surface without a manual prompt.
## Wait for engine start before applying default DNA, 2026-10-23

After v5-beta-28, the owner reported that D changes successfully but Race does
not; Race requires the engine to be running. Active A/D/Race requests now wait
for fresh RPM above 400 for two seconds on C1 before the BH request is sent.
The menu shows `Start engine` during this wait, and the C1 confirmation timeout
starts only after the engine gate. BH independently requires fresh RPM above
400 for two seconds before it starts any selector gesture, and aborts/releases
if engine reports stop or become stale during the sequence. N remains passive.

Implementation is in the local task branch `fix/dna-after-engine-start` based
on `495b335`. Host tests, firmware builds, PR, merge, release and vehicle
acceptance have not yet been performed for this correction.

## Default suspension calibration, 2026-10-23

Initial implementation (superseded by the 2026-10-24 audit) incorrectly placed
the `Suspension` setup option in occupied flash slot 36. `Stiff` leaves the
mode's firm startup calibration; the softer choice means Soft in D and Mid in
Race, and is ignored for A/N. Stiff needs no extra press because selecting
D/Race sets the suspension firm. For Soft/Mid, after the selector reaches D/Race, BH checks fresh `0x25A` byte 1
bit 4 and sends one BH `0x256` byte 1 bit 7 pulse only when the softer state is
not already reported. The pulse is released after 150 ms; BH waits for fresh
matching suspension feedback before returning success. The softer setting uses
the unused high bit of the existing five-byte `BH_CMD_DNA_DEFAULT`, so the UART
message length and flash record size remain unchanged.

Work remains on local branch `fix/dna-after-engine-start`, based on `495b335`.
No firmware build, host test, PR, merge, release or vehicle acceptance has been
performed. D feedback is present in one capture campaign; the Race/Mid status
encoding and hardware behavior still need vehicle confirmation.

## v5-beta-28 release preparation, 2026-10-23

Published [v5-beta-28](https://github.com/eujot/BACCAble/releases/tag/v5-beta-28)
from master commit `00e1b16464972d7fd40c78e871eb505d4170a34d` after PR #71
(`8d202d4`) and notes PR #72. Full notes are in
[docs/releases/v5-beta-28.md](releases/v5-beta-28.md). Release workflow
[39985666557](https://github.com/eujot/BACCAble/actions/runs/39985666557)
passed host/Lab tests, lint, C1/C2/BH/CAN builds and the independent display/IPC
matrix with pinned Arm GNU 15.3.Rel1. `BUILD_INFO.json` records C1 at
101,764/102,400 flash bytes and 15,500/16,384 RAM bytes. All 13 downloaded
release assets passed `SHA256SUMS`; firmware has not been accepted in a vehicle.

## BH default drive-mode gesture, 2026-10-23

Implementation commit `00b4b99` on `feat/bh-default-drive-mode`, based on research `5a6f43a`,
isolated worktree `/private/tmp/baccable-bh-default-mode`. Default mode now uses
the supported BH 0x256 input waveform instead of rewriting selected-mode
reports. Choices cycle A/N/D/Race; flash slot 38 keeps 0=N/passive (formerly
OFF), 1=A, 2=D, 3=Race. No record migration, new slot or memory-limit change.

BH `features/dna_selector.c` requires a fresh neutral standard 0x256/DLC 4 and
fresh actual 0x46C mode. It copies unrelated fields, sends 20 toward A or 40
toward D/Race, and releases the directional field after 200 ms. Reaching Race
requires D first, then a 1,800 ms hold refreshed every 250 ms. Each of at most
three adjacent steps waits for its factory mode and a new neutral report after
release. Unaccepted selection, unknown/stale reports and physical input stop
further synthetic selection. At ten seconds no new press or hold refresh can
start; a pending release still retries until CAN accepts it. OFF/save finishes
a virtual release; physical input hands ownership to the driver. A failure
leaves the actual mode, including an intermediate mode before Race.

Removed Default mode overrides from C1/C2 0x384 and BH 0x46C/0x25A, plus obsolete
RAM fields and default-mode helper functions. Existing ESC/TC customization uses
actual observed mode. C1 sends legacy driver-aids mode=0 to keep older peers
from rewriting reports, preserving the C2 LKA command. New BH command 0x4A
carries target/action/token; C1 result 0x4B carries token/outcome. Completed
outcomes can be resent on resync/duplicate request without replaying gestures.
C1 waits up to 15 s for BH, then starts its existing five-second sustained
DNA + CRC-valid transmission corroboration only after matching BH completion.
Stale tokens, failure and missing BH cannot confirm. Update C1/BH together.

Both guides and MENU_UX document N/passive migration, gestures, failure,
intermediate modes, result checking and matched firmware. Legacy results use
R for Race so the complete status fits; editors retain the Race label.

Validation: complete native Apple Clang ASan/UBSan host suite passed. New BH
fixture covers all 12 initial-position/active-target combinations, passive N,
exact 200/1,800 ms press timing and 250 ms hold refresh, preserved unrelated
bytes, malformed frames, stale/silent input, queue/release/UART failures,
cancellation, resync/duplicate requests, no response and tick wrap. Existing
C2 LKA and ESC/TC behavior checks pass. All 16 firmware board/display profiles
built with full Arm GNU 15.2.Rel1, VERSION=bh-dna; cppcheck passed all flavors.
Largest C1 flash 102,236/102,400 bytes (164 spare), data+bss 15,700/16,384;
pinned CI/release identity sizing remains required. All 73 Lab tests also passed after updating dispatcher coverage to 34 IDs.
Dictionary source provenance and the supported input description now reference
00b4b99; generated catalog is current. No captured bit layout changes.

PR #71: https://github.com/eujot/BACCAble/pull/71, from `feat/bh-default-drive-mode` into `master`. Branch commits: `00b4b99` implementation, `f3ee2fa` evidence/validation, `6428450` PR handoff. No merge/release/flash. Hardware acceptance is UNKNOWN: test A/D/Race
from an actual fresh startup, automatic release, observed factory/transmission
mode persistence, failure indication and later physical selector use. Source
captures support the input waveform but do not prove injected acceptance. Main
checkout capture/release folders are untouched; implementation and handoff are
committed in this isolated branch.

## DNA selector input identified across all sessions, 2026-10-23

Analysis branch `research/dna-all-sessions`, based on `66b77fe`, isolated at
`/private/tmp/baccable-dna-all-sessions`. Inventoried all 20 session databases
(21,354,624 frames, including six failed/four empty captures). Three sessions
contain eight N-to-D transitions. The October-10 campaign skipped its named DNA
scenario but contains an actual selection outside it, with event 96's original
note explaining the change. Do not repeat the earlier claim that it provides
no DNA evidence.

**New supported input: BH 0x256, exactly DLC 4**, byte 0 bits 6:5. Zero is
released; byte 0=40 points toward D/Race; 20 points toward A. All 36 directional
pulses match selector-associated mode changes. Eight N-to-D changes follow by
108–112 ms on the BH clock; their pulses last 154–294 ms. Five long holds
precede Race after 1.194–1.211 s (earlier 2.5–2.9 s D-to-Race timing included
the pause before holding). One ignition-related D-to-N reset has no pulse.
Thirteen sessions contain 17,087 BH 0x256 frames; neutral and unrelated controls
are preserved. C1 0x256/DLC 8 is unrelated. Physical sender identity and
acceptance of injected input remain unverified.

On C1, 0x384 changes first among the established mode reports, followed by
0x1FC after 12–27 ms and 0x5A8 after 29–46 ms, in all eight cases. C2 0x1FC
follows its local 0x384 after 13–27 ms. BH 0x25A/0x46C follow its selector input.
No unsynchronized cross-bus clock subtraction or definitive ECU identification.
The earlier absence-of-input conclusion is superseded; the narrow search
baseline included the early input pulse and obscured it.

Report, raw evidence and reproduction commands:
[all-session DNA review](../baccable_lab/docs/research/20261023-dna-all-sessions.md).
Raw/SQLite discrepancies of 34 and 123 frames in two control sessions were
checked with the existing raw parser; their raw mode/selector values still
show no selection. Capture files and original markers were not changed;
input SHA-256 fingerprints remain identical. Added a guarded passive dictionary
field, updated hypotheses/catalog and preserved historical contradictory claims.
All 73 Lab tests pass, including captured payloads, unknown code 3, exact DLC
and bus isolation. No firmware behavior, menus, manual operation or settings
changed; no new firmware builds required for this analysis-only task.

Next implementation candidate: replace selected-mode-report rewriting with one
short BH selector gesture from a fresh neutral report, restore the directional
field, prioritize physical input and preserve unrelated bytes. Start the C1
corroboration window after BH queues the request, with explicit inter-board
status; retain bounded timeout and no continuous assertion. This task did not
implement or transmit that gesture. Hardware validation is UNKNOWN. No PR,
push, merge, release or flash. Main-checkout untracked captures/release folders
remain untouched; findings and this handoff are committed on the research branch.

## Default-mode CRC correction and result check, 2026-10-23

Implementation commit `80fdb13` on `fix/default-mode-confirmation`, isolated worktree
`/private/tmp/baccable-default-mode-check`, based on research commit `115ba8a`.
The owner reports that the previously installed one-shot default D briefly
appears and then returns to N. Installed binary identity remains unspecified.
This is negative evidence for a persistent selection; queue success is not
vehicle acceptance.

All 0x384 mode rewrites now preserve byte 7 and unrelated fields. No global
checksum change: 0x5A8 retains its J1850 CRC. Standard-data/length/ID guards
protect the relevant handlers. Queue retries on each board are bounded to five
seconds from arming; each accepted frame is still sent at most once per cycle.

C1 `features/drive_mode_check.c` starts a five-second observation window when
its request is queued (or target mode is already present). At its end, fresh
unmodified C1 0x384 reports must match for at least 2 s, and CRC-valid 0x5A8
reports for at least 1 s. Mismatches/gaps restart the relevant matching run;
missing/stale/corrupt reports and queue timeout cannot confirm. No confirm
leaves the vehicle alone; no automatic repeat of accepted injections or
forced rollback. A new ignition permits another attempt. Saved settings,
OFF and sleep clear the result; transport resync preserves it. This confirms
observed C1 state, not explicit ECU acknowledgement or every subsystem.

The existing Default mode browse screen shows the saved target and Next start,
Waiting, Confirmed or No confirm (MY23 title/result; legacy compact DNA label).
The editor retains its original draft semantics. Both user guides and MENU_UX
now document the result and failure behavior. No additional menu entry or
persisted settings slot was added.

Validation: full native Apple Clang ASan/UBSan host suite passed, including
transient D-to-N rejection, missing transmission, bad CRC, stale/no traffic,
queue rejection/deadline, timer wrap, malformed frames, preserved bytes,
ignition/save/resync lifecycle and both display layouts. All 16 board/display
profiles built with full Arm GNU 15.2.Rel1, VERSION=mode-check; cppcheck passed
C1/C2/BH/CAN. Largest C1 flash is 102,344/102,400 bytes (56 spare), RAM data+bss
15,712/16,384; pinned CI/release version sizing must be checked before release.
No memory allocation was enlarged. All 72 Lab tests also passed after refreshing
source provenance to 80fdb13; generated catalog and dictionary hashes agree.
No executable CAN decoder layout changes.

No push/PR/merge/release or vehicle flash. Hardware acceptance remains UNKNOWN:
install matching C1/C2/BH firmware, start with Default mode D, check sustained
factory D and the stored result, then verify the physical knob still works.
CRC correction does not by itself establish a persistent selector command.
The main checkout's pre-existing untracked captures/release directories remain
untouched; all task edits and this handoff are committed in the isolated branch.

## DNA selector raw-capture review, 2026-10-23

Analysis-only branch `research/dna-selector-analysis`, based on `aa81111`.
Re-read 1,319,285 frames from `20261011-190746Z` and `20261011-191050Z`
with read-only SQLite/WAL handling and unchanged input fingerprints. Report:
[baccable_lab DNA analysis](../baccable_lab/docs/research/20261023-dna-selector.md),
with a reproducible script and a raw transition/evidence JSON alongside it.

Confirmed the known DNA fields and five successful Race entries among six
marked attempts. C1 device-time D-to-Race intervals are 2.502–2.910 s; physical
knob deflection/release is not independently identified. Transmission 0x5A8
reports N/D/A/Race as 0/2/1/6. Race coincides with 0x4AF stability fields and
LKA state changes. New related byte candidates remain hypotheses. Superseded
COUNTER-HEARTBEAT-001's claim that all of 0x4AE is a counter: byte 3 changes
with Race and manual LKA, while bytes 6/7 retain counter/checksum behavior.
Updated dictionary notes and generated catalog without new executable layouts.

Concrete firmware mismatch for follow-up: Default mode adds J1850 CRC to
0x384 byte 7, but all 2,099 captured C1/C2 reports end in zero and none matches
that CRC. Positive control: all 286 0x5A8 frames do match. BH 0x25A byte 1 also
changes with factory mode, whereas the current rewrite touches only byte 0.
The existing one-shot queue operation does not prove a latched body selection;
no verified momentary knob request is established. Do not reinstate continuous
contradictory broadcasts as a production fix. Next: separately mark knob
press/hold/release, establish the actual control path, and require persistent
original reports after a request before reporting success. Preserve unrelated
fields and bus-specific layouts; no global CRC removal.

Validation: 72 existing Lab tests passed; generated dictionary is current.
No firmware/menu behavior changed, no hardware transmission or installation,
no PR/push/release. Hardware acceptance is UNKNOWN. Existing untracked capture
bundles and release directories remain untouched. Findings and handoff are
committed on this research branch, not copied into master.

## Experiments audit and corrections, 2026-10-23

Follow-up on `feat/pelican-experiments`, auditing `ab057dd` against the pinned
OBDb snapshot and the Pelican format/command documentation. Corrected defects:

- `Shift gear` incorrectly shared ordinary gear codes: values such as 1, 11 or
  13 could be shown as a gear, reverse or park. It now accepts only the source's
  0/221/238 state map. Source limits for model year, wheel speed, acceleration
  and transmission temperature are applied; acceleration retains two decimals.
- The fixed 40-byte response limit rejected longer valid messages. The reader
  now retains a 40-byte prefix while consuming and validating the full ISO-TP
  response, up to 4095 bytes. Reordered/short continuation frames abort the
  transfer. NRC 0x78 extends waiting within a five-second total bound; definitive
  negative replies invalidate the value. Requests use eight-byte padding and
  flow control requests 5 ms separation. Failed queue offers/read attempts retry.
- Polling every 500 ms ignored source intervals (100 ms through one day).
  Polling now follows each command's interval after success, retries failures
  after one second, and retains the value until the next expected refresh.
  Menu ownership gates reception and polling; exit, hidden screens and fault
  operations cannot leave the old reader issuing flow control.
- Wheel-speed/acceleration labels could clip the result on the 18-character
  display. Shorter labels retain the complete value and unit on every profile.
- CI still enforced the old 96 KiB C1 limit after the linker moved to 100 KiB.
  Updated the default reporting budget and build documentation to 100 KiB.
- The bundled FatFs refused formatting/mounting volumes below 64 KiB. Added a
  narrowly configured small FAT12 formatting path and compatible mount check;
  this covers the new 16 KiB reserve and existing 52 KiB C2/BH volumes.

Correction to the preceding handoff: **standard C1 does not enable mass
storage**, so the 20-to-16 KiB change affects its reserved optional file area.
Custom C1 mass-storage builds must back up old files before flashing. Standard
serial CAN capture writes to the host and does not lose capture capacity.
Both user guides now explain this and the revised labels/refresh behavior.

Validation: full Apple Clang ASan/UBSan host suite, 72 Lab tests, eight
release-script tests, cppcheck for C1/C2/BH/CAN, and all 16 board/display-profile
builds passed with local Arm GNU 15.2.Rel1. The C1 size-report gate passes at
101,340/102,400 flash bytes (1,060 bytes spare); the large display uses 101,308
bytes. These are local builds, not pinned CI or hardware verification. New tests include 192 independent
source-derived wire/value vectors (three per signal), separate shift enums,
long replies, counter/tick wrap, reordered fragments, negative/pending responses,
queue failure/retry, freshness, menu ownership and both display profiles.
FatFs tests format, label, write, remount and read 16–64 KiB volumes.

Vehicle acceptance remains open. The source does not provide a receive ID for
request 0x742; matching positive service/DID selects the sender for that transfer.
The test reply address for that command is explicitly synthetic. Software tests
do not establish ECU support or physical accuracy on the owner's car. Test
source limits, the 5 ms flow-control pacing and USB mounting on hardware.

## Pelican Giulia Experiments replacement, 2026-10-22

Branch `feat/pelican-experiments` replaces the 38 old experimental gasoline
pages with 64 read-only manufacturer signals from the Pelican Giulia list and
its linked OBDb Alfa-Romeo `signalsets/v3/default.json` (CC BY-SA 4.0; source
snapshot SHA-256 `c28314008a46d69169e0746919c9d5798b92df6eff727591836a60b30a9db59d`).
Nine groups follow that source. The C1-only reader sends `0x22` requests for
the selected signal, checks response IDs/service/DID, reassembles bounded
ISO-TP replies, extracts MSB-first fields, applies source conversions and
shows one named value. The ordinary catalog and Favorites remain separate.
Both guides and the catalog/menu technical guides were updated.

To fit C1, its application allocation grows from 96 to 100 KiB and the USB
disk shrinks from 20 to 16 KiB. The old disk's first 4 KiB become application
flash, so users must back up C1 USB files before flashing. Settings record
addresses are unchanged. Local ARM GNU 15.2.Rel1 C1 image uses 101,080 of
102,400 flash bytes, leaving 1,320 bytes; the pinned CI toolchain may size
differently.

Validation: complete native Apple Clang ASan/UBSan host suite, 72 Lab tests,
and C1/C2/BH/CAN firmware builds with the full local ARM toolchain passed.
Tests cover a rejected DID, 11-bit and 29-bit response IDs, bit extraction,
ISO-TP flow control and out-of-order frame handling. Physical acceptance
remains open for every manufacturer signal, ECU address and conversion on
the owner's vehicle; the external catalog is a source specification, not
a verified recording. Check CAN/USB behavior and the smaller C1 mass-storage
volume on hardware before release.

## OBD catalog bound fix, 2026-10-16

On `fix/obd-catalog-bound`, corrected a shared stale limit of 100 parameter IDs
in display formatting, diagnostic request selection, cache access and Favorite
query selection. The catalog contains 136 definitions; experimental gasoline
pages using IDs 100–124 could previously show a literal `$` template and never
receive a value. These paths now share the catalog count. No DID, scaling or CAN
dictionary meaning was changed. Valid UDS replies can populate these fields;
missing, rejected, stale or unsupported replies still show `--`.

Validation: the complete Apple Clang ASan/UBSan host suite and focused menu suite
passed, including new request/cache/display checks for parameter IDs 100 and 124
on legacy and MY23 screens. All 16 C1/C2/BH/CAN display profiles built with the
local full Arm GNU 15.2.Rel1 toolchain, and cppcheck lint passed for all four
flavors. C1 flash is 98,084 bytes (18 columns) or 98,256 bytes (24 columns),
leaving 48 bytes in the larger profile; pinned CI compiler sizing may differ.
This is a source fix, not a vehicle verification.
PR #70's first CI run found stale Lab source checksums for three changed
diagnostic files. Their reviewed source hashes were refreshed; no signal layout,
diagnostic definition, evidence or hypothesis was changed. The follow-up
[CI run](https://github.com/eujot/BACCAble/actions/runs/38962433710) passed host
and Lab tests, lint, and all C1/C2/BH/CAN firmware profile builds. Integration
is in [PR #70](https://github.com/eujot/BACCAble/pull/70).
Vehicle acceptance remains open for the affected experimental pages and any
other examples reported by the owner; their source DIDs and scaling are not
verified on this car.

## v5-beta-27 release, 2026-10-16

Published [v5-beta-27](https://github.com/eujot/BACCAble/releases/tag/v5-beta-27)
from `cf1fc2378c6854b96af7f4ecdc61f424d750a00d`. The beta adds the separate
`Oil bar glyphs` IPC display test; see [release notes](releases/v5-beta-27.md).
Workflow [38930474144](https://github.com/eujot/BACCAble/actions/runs/38930474144)
passed host tests, lint and C1/C2/BH/CAN builds. `BUILD_INFO.json` identifies
the tagged commit and all downloaded artifacts passed `SHA256SUMS`. The vehicle
glyph comparison remains open; no release image has been installed in a vehicle.

## Oil bar glyph CI provenance fix, 2026-10-16

The first CI run after adding `Oil bar glyphs` exposed two stale expectations:
the lab's `giulia.json` source snapshot held the previous SHA-256 for
`ipc_display_test.c`, and the MY23 menu test expected wraparound to the former
last pattern (`Both lines`). Refreshed the source hash and updated the menu
expectation to the new last pattern. No CAN signal meaning or capture evidence
changed. Validation passed: 72 Lab tests, the release-script regression tests,
menu-label checks, the complete host suite, cppcheck for C1/C2/BH/CAN and all
16 firmware display-profile builds using local Arm GNU 15.2.Rel1 (CI pins
15.3.Rel1).

## Oil bar UTF-8 glyph test entry, 2026-10-16

Added a separate `Oil bar glyphs` pattern after the four existing IPC display
test patterns, preserving their pattern IDs. It sends candidate horizontal
lines, fractional block ticks, fill-density blocks and point markers through the
existing BH Unicode display test. Updated both user guides and the menu guide.
The display/font result depends on vehicle hardware and source; no glyph has been
promoted into normal oil-bar rendering. Hardware comparison remains open.

## Oil bar animation, 2026-10-16

Implemented on the same local `feat/oil-level-readings` branch as `5730dc9`.
The owner selected delivery of every adjacent position over a 20 ms C1 animation
that could skip steps during transmission. BH now recognizes oil bars and moves
one character toward the latest target only after the previous step has reached
the CAN queue. Both Full and Delta retain each active step across repeated C1
updates and failed offers; radio restart resends that same step. Each line is
independent. Missing/unknown bars and initial appearance are not interpolated.
C1 numeric readings, status, polling, UART protocol and IPC pace settings are
unchanged. At default Quick 20ms + Full, each 13-fragment MY23 step takes about
260 ms without radio/queue delay. Queue acceptance is not IPC acknowledgment.

Validation: the complete 27-binary Apple Clang ASan/UBSan suite passed. New tests
cover all four widths, Full/Delta, decreasing/increasing positions and arrows,
changing direction, both lines, missing-data removal, repeated submissions,
failed offers, radio restart and real BH CAN output at 20 ms fragment intervals.
Both targeted BH display suites also passed after refining the 77-to-74 mm
example. All 16 C1/C2/BH/CAN × legacy18/legacy24/MY23/MY23-large builds passed
with full Arm GNU 15.2.Rel1 and `VERSION=oil-animation`. C1 uses 98,084 / 98,256
bytes flash for 18/24 columns (48 bytes spare in the largest build); BH uses
33,644 / 33,528 bytes. The small C1 size difference from the preceding handoff
includes the longer version string. No flash limit was increased.

Both user guides and the menu technical guide document timing and animation.
Dictionary provenance and its generated readable catalog include the new helper;
20 Lab knowledge tests passed. PR #67 merged to `master` at `9743823`; its GitHub
CI passed host tests, lint and all four firmware builds. The `v5-beta-26` beta
release and release notes are published as recorded below. No firmware has been
flashed. Vehicle acceptance must check visible steps in both directions, radio
interference and Full/Delta behavior using an updated BH image. The user's master
checkout's pre-existing capture/release files remain untouched.

## Oil level readings and drivetrain setup, 2026-10-16

Implemented locally on `feat/oil-level-readings`, commit `18abb97`, based on
`origin/master` at `b515420`, in `/private/tmp/baccable-oil-level`.
`Oil level` shows mm with LOW/OK/HIGH; `Oil level bar` is a separate Favorite
page filling the selected line (MY23 16/22, legacy 18/24). Both use existing ECU
measurements and cache expiry. Engine + new `Drivetrain` (Not set/RWD/Q4) select
the four owner-supplied ranges; V6 has no range. Settings slot 40 and spare
visibility bytes preserve old record sizes and saved page identities. Both
user guides and menu/catalog technical guides describe the change.

Validation: full 27-binary native Apple Clang suite with ASan/UBSan and menu
label checks passed, including new oil thresholds, unavailable data, saved
visibility, setting validation/draft save, both Favorite slots and stale cache.
All four C1 display profiles and C2/BH/CAN compiled with the documented full
Arm GNU 15.2.Rel1 toolchain (`VERSION=oil-level`). C1 flash (text + data) is
98,084 bytes for 18 columns and 98,252 for 24 columns, against 98,304 available;
the largest build has only 52 bytes spare. Catalog arrays now use their actual
lengths rather than padded diesel entries; no linker limit was raised. Future
release builds must recheck this tight limit with the pinned CI compiler and
release version string. C2/BH/CAN flash: 29,856 / 33,216 / 25,196 bytes.
The 20 Lab knowledge tests passed after refreshing source provenance and the
readable dictionary; CAN decoding and research hypotheses were not changed.

Integration/release: submitted with the animation in
[PR #67](https://github.com/eujot/BACCAble/pull/67), merged to `master` at
`9743823`. Vehicle acceptance remains open for glyphs, line widths, actual ECU
height availability, power-cycle persistence and the owner-supplied limits.
The user's master checkout and its pre-existing untracked captures/release
folders remain untouched. This change does not claim a vehicle test.

## v5-beta-26 release, 2026-10-16

Release notes are in [v5-beta-26.md](releases/v5-beta-26.md) and on the
[GitHub release page](https://github.com/eujot/BACCAble/releases/tag/v5-beta-26).
Notes PR #68 merged to `master` at `850b1a7d32949fa81709df3272f0fbad5b1dfd71`.
The beta workflow [38927961378](https://github.com/eujot/BACCAble/actions/runs/38927961378)
passed host tests, lint, UI profile matrix, firmware-size checks and C1/C2/BH/CAN
builds using Arm GNU 15.3.Rel1. Release `v5-beta-26` was published as
`beta-850b1a7` from that commit. Downloaded BIN/HEX/ELF images and `BUILD_INFO.json`
all passed the published `SHA256SUMS` verification. C1 flash is 97,836 / 98,304
bytes; C2/BH/CAN use 29,612 / 33,644 / 25,200 bytes. The source has not been
flashed to a vehicle. Oil ranges, ECU sample availability and display behavior
remain open for vehicle acceptance.

## v5-beta-25 release preparation, 2026-10-15

Prepared [v5-beta-25 release notes](releases/v5-beta-25.md) in PR #65 and
merged to master as `75295f2`. This includes PR #63's ignition-cycle and
menu-save correction and PR #64's host test iteration update, plus the firmware
features in v5-beta-24. Beta workflow [38763208470](https://github.com/eujot/BACCAble/actions/runs/38763208470)
passed host tests, Lab tests, release checks, lint and C1/C2/BH/CAN builds with
Arm GNU 15.3.Rel1. [v5-beta-25](https://github.com/eujot/BACCAble/releases/tag/v5-beta-25)
was published from `75295f2`; `BUILD_INFO.json` matches that commit and every
downloaded release asset passed `SHA256SUMS`. The release page includes the
maintained notes. No image has been installed in a vehicle. Ignition cycling,
the startup LKA gesture, driver re-enable and menu button gestures remain open
for vehicle acceptance.

## Start here

1. Read this file, then inspect `git status --short`, the current commit and the
   diff. Preserve existing work. Dated audits below describe their own source
   baselines; inspect the current tree before treating an old local change as pending.
2. Choose the first applicable open task below, respecting dependencies. A task
   marked implemented on another branch is not implemented on `master`.
3. Read only the owning modules and relevant reference guides. Do not restart the
   old refactors, reconstruct old chats, or create another planning document.
   Before local builds/tests, follow the Mac setup in
   [MAKEFILE.md](../firmware/baccable/MAKEFILE.md#this-mac-verified-tools-and-dependencies).
4. Record implementation commit, checks, integration/release status and hardware
   evidence separately. A published binary is not proof of installation or a
   successful vehicle test. Do not mark a task done based on an old agent summary.
   Commit this handoff in the same task branch/worktree as the change; never
   leave agent-authored updates uncommitted in the user's `master` checkout.
   Follow [the checkout rule in AGENTS.md](../AGENTS.md#keep-the-checkout-ready-for-pulls).
5. For changes to the menu or device operation, update the affected sections of
   **both** the [English](../manuals/BACCAble_USER_GUIDE_EN.md) and
   [Polish](../manuals/BACCAble_USER_GUIDE_PL.md) user guides in the same change.
   Include labels, navigation, messages, parameters, conditions and USB workflows
   as applicable. Follow the [maintenance rule in AGENTS.md](../AGENTS.md#keep-both-user-guides-current);
   check the code and keep both editions equivalent.

Project documentation, code comments and menu text stay in English, except for
the maintained Polish user guide. Use small,
bounded changes and existing modules/tests. No model-specific workflow, mandatory
PR count or broad rewrite is needed. This handoff is a backlog, not an instruction
to automatically flash hardware or publish every future change.

## Faster host menu test iteration, 2026-10-14

Branch `build/targeted-menu-tests` narrows the five included menu fixture `.c`
dependencies to the five menu binaries. A dry run after a `test_setup_ui.c` change
now schedules five builds in the full suite, instead of 26. The new
`test-menu-fast` target runs the legacy and MY23 menu suites, shared button input,
C2 drive-mode/LKA checks and menu labels; the full 27-binary host suite remains
the integration check. Both targets passed with Apple Clang on this Mac. This
changes only the local test workflow, with no firmware or vehicle behavior change.
Integration is pending in [PR #64](https://github.com/eujot/BACCAble/pull/64);
`master` does not contain this branch yet.

## Post-beta-24 LKA ignition and menu-save correction, 2026-10-13

Implemented on `fix/lka-ignition-menu-save` and merged into `master` as
`91da9ac` via PR #63. No new release or hardware flash.
The owner's beta-24 report supersedes the older hardware-unknown note below:
LKA switched off once after plugging in, but did not repeat after ignition
cycling. Menu draft saving and its confirmation were also unreliable.

Root causes and fixes:

- C1 rearmed startup only after processor sleep/wake; ignition cycles with an
  active bus/USB were missed. C1 now watches `0x1EF` low nibble: initial RUN (8)
  or OFF (2) → RUN permits startup; cranking/unknown codes preserve the session.
  Startup waits for RUN after wake. Transport resync retains an undelivered
  ignition event. OFF and saved preferences cancel an unused startup window.
- C2 no longer permanently cancels on an initial LKA OFF report. A bounded
  5-second window after auxiliary configuration waits for enabled `0x547` and
  released `0x384`, then sends one press/release/press/release gesture, 100 ms
  between accepted frames. Driver input cancels waiting; completed gestures
  do not rearm on resync. A started gesture still finishes its release steps.
- `Disable LKA` becomes `LKA`: OFF requests the startup double-click; ON keeps
  factory behavior. Flash slot 39 retains its inverse encoding, preserving
  existing settings. ON does not force physical LKA activation.
- Hold saves directly in Features, sort, slots (also the picker), membership,
  visibility and order editors. Dirty Back asks for save/discard; clean Back
  returns. Failed persistence retains the draft and rolls back live values.
  Hold also retries a failed automatic exit. Actions/capture retain prompts.
- The input recognizer no longer suppresses a deliberate hold begun immediately
  after a double-click. A second press begun within 280 ms completes the pair
  even if its release occurs beyond that boundary. Third short-click filtering,
  900 ms holds and report-gap cancellation remain.
- Entering toggle/enum/sort/mirror Enabled editors no longer changes the value.
  DNA choice 4 (unsupported) is removed; valid draft choices have named labels.
  `Engine` now uses the same title in the list and editor. Shared value formatting
  removes redundant render callbacks and keeps all C1 display builds within
  their existing memory limits; linker limits were not changed.

Validation: native Clang ASan/UBSan suite and label checks; C1/C2/BH/CAN ARM
builds and all four display combinations with full Arm GNU 15.2.Rel1; cppcheck
for four roles. New regressions cover actual button reports through menu save,
Flash reload/failure/discard, direct hold across editor types, three ignition
cycles without sleep, no crank/resync retrigger, initial OFF, driver priority,
queue rejection, deadline and tick wrap. CAN dictionary source hashes/catalog
and both user guides are synchronized. The Lab suite includes ignition enum
fixtures and the preserved transition ledger.

Evidence: [ignition review](../baccable_lab/docs/research/20261013-ignition-lka-startup.md).
Vehicle acceptance still required: several OFF/RUN cycles without unplugging,
cold initialization within the 5-second readiness window, manual re-enable
remaining enabled, already-OFF skip, USB connected/disconnected, and direct
hold/dirty double-click/clean double-click on the actual steering buttons.
No continuous override or claimed repeated vehicle verification.

Original master checkout and its untracked beta archives/five JSON bundles were
left untouched. Task changes and this handoff belong to the same committed branch.

## Start-up LKA button double-click candidate, 2026-10-13

The `kla` and `dnskla` recordings distinguish a momentary left-stalk button
report from recurring LKA state. In `kla`, C2 `0x384` byte 3 bit 6 rises at
24.207 s and returns at 24.316 s, rises again at 24.423 s and returns at
24.528 s; the state on C2 `0x547` changes to OFF at 24.528 s and continues to
be reported. BH `0x5A0` also shows the button, but
sending pulses on both buses could create two actions if the vehicle bridges
them. The earlier single-press candidate did not reproduce the recorded gesture.

The candidate on `fix/driver-aids-startup-one-shot` replaces the one-shot state
rewrite with a C2 `0x384` double-click: press, release, press, release, roughly
100 ms between steps. It waits until C2 `0x547` reports LKA enabled, skips the action
if already off, cancels a pending action when the driver presses first, retains
the remaining steps through queue rejection and board resync, and leaves C2/BH state
reports untouched. BH no longer injects an LKA frame. The host suite, C1/C2/BH/CAN
cppcheck lint and Arm GNU 15.2.Rel1 firmware builds pass locally. Both user
guides and the CAN research note describe this experimental behavior.
Initial implementation commit: `939e062`; double-click correction: `25e518b`.
The dictionary and generated CAN catalog use source snapshot `25e518b` and
dictionary version `2026-10-13.2`; all 70 Lab tests pass. The candidate is on
draft PR #58; integration and release await the
vehicle check.

**Vehicle acceptance remains open:** confirm an injected C2 button double-click
turns LKA off, original C2 `0x547` / BH `0x4F6` reports remain OFF after BACCAble
stops transmitting, and a later driver double-click can turn LKA on. If the gesture does
not latch, capture the controller-side input and do not restore a continuous
status override. The default DNA-mode one-shot check in the section below also
remains open. No firmware image from this branch has been installed in the car.

## Beta-22 vehicle finding and start-up driver aids, 2026-10-12

The owner tested beta-22 in the car. `Default mode = D` lit the Dynamic lamp,
but the Natural lamp remained faintly lit or flickered. `Disable LKA` turned the
assistant off, but a later left-stalk double press could not turn it back on.
This supersedes the earlier untested status below. The installed board image
identities and a transmit-side trace were not supplied.

Source review found both defaults retransmitting on every matching status
report: `0x384` on C1/C2, `0x46C` and `0x25A` on BH, and LKA state `0x547` on C2 /
`0x4F6` on BH. Those reports recur; the LKA handler erased every subsequent
enable state, explaining the blocked double press. The initial firmware fix on
branch `fix/driver-aids-startup-one-shot` queued each relevant modified frame once
per vehicle start, retrying only after CAN rejected the queue submission. Its LKA
part was superseded by the C2 button-pulse candidate above. A saved
preference is synchronized but does not inject a frame until the next start.
Transport-only resync leaves the current session alone. Host tests cover each
board, repeated original reports, later driver input, queue failure and rearm.
Implementation commit: `67c0890` on the task branch. The full Apple Clang host
suite, cppcheck lint and Arm GNU 15.2.Rel1 builds for C1, C2, BH and CAN pass
locally. CAN dictionary source hashes were refreshed against that commit
(dictionary version `2026-10-12.5`); no observed signal meaning changed. All 70
Lab tests pass. This is local software evidence; the fix has not yet been
installed in the car. [PR #58](https://github.com/eujot/BACCAble/pull/58) is a
draft; merge and release are pending vehicle acceptance. Its host CI job passed;
the firmware and lint jobs were pending when this handoff was recorded.

**Hardware acceptance remains open:** confirm that a single injected DNA frame
actually latches the selected drive mode and removes the faint Natural lamp;
confirm the LKA pulse as specified above. The current capture format observes received frames, not
BACCAble's own transmissions. A rewritten status broadcast may be overridden by
the body's next original broadcast; if so, find a genuine one-time request or
another control path rather than restoring a continuous conflicting stream.

## Public Giulia CAN source cross-check, 2026-10-13

Racelogic's Giulia Type 952 channel file supplies bit positions and scales for
previously inventoried C1/C2 `0x0DE` steering angle, `0x0EE` four wheel speeds,
`0x101` brake pedal position and `0x0FC` accelerator pedal position. Compared
these layouts with October steering/braking markers and a September driving
capture. The passive Lab dictionary now decodes them with exact DLC guards;
the generated CAN catalog and captured-payload tests track the same layouts.
The existing C1 `0x0FC` RPM layout independently agrees with that source.
Wheel means are about 5.7% above BACCAble's existing vehicle-speed reading;
physical wheel-speed scale remains open. Racelogic's driver-belt bit `0x257`
conflicts with local fastened/unfastened markers and was **not** promoted.

Compared a community Giulia/Stelvio diagnostic PID CSV with the current
firmware catalog: 137 distinct ECU/DID pairs, 47 already catalogued and 90
external-only leads. The five bundles have no new complete positive DID replies;
external entries have not been enabled in firmware. The current tire-pressure
request-shape and reply-length issues still need protocol validation. Full
sources, byte formulas, examples, contradictions and a bounded next capture are
in [the public-source cross-check](../baccable_lab/docs/research/20261013-public-giulia-can-sources.md).
This research branch is based on the draft PR #58 source snapshot; it does not
include a vehicle-tested firmware change. The work was integrated through PR
#59 at master merge `42e3098`. The merge CI and beta-23 release workflow passed;
the public-source readings remain unverified in the vehicle. The source tree at
v5-beta-23 matches the merged master tree. v5-beta-24 release notes are prepared
on `docs/v5-beta-24-notes`; v5-beta-24 is now published from master commit
`95806ba`.
All 71 Lab unit tests and `git diff --check` passed for the research changes.

## v5-beta-24 release preparation, 2026-10-13

PR #59 (`feat/readings-experiments-v5-beta-23`) merged to master as `42e3098`.
CI passed on the PR and the v5-beta-23 beta workflow completed successfully
from source commit `4946f72`. The released beta-23 firmware source tree matches
the merged master tree. v5-beta-24 is the requested rebuild from the integrated
master revision, with the same firmware behavior as beta-23; see
[the release notes](releases/v5-beta-24.md) and [the published release](https://github.com/eujot/BACCAble/releases/tag/v5-beta-24).
The release page description is kept in sync with the notes file.
Release workflow [38430462442](https://github.com/eujot/BACCAble/actions/runs/38430462442)
passed host tests, Lab tests, C1/C2/BH/CAN firmware builds and lint. It published
matching board images, `BUILD_INFO.json` and `SHA256SUMS` from commit
`95806ba23157787bf6a3718d0aea12bbf3f450a7` using Arm GNU 15.3.Rel1. Every
downloaded asset passed its SHA-256 check. No beta-24 image has been installed
in a vehicle. The `Experiments` DIDs and start-up LKA gesture still need vehicle
validation.

## Five-bundle CAN review, 2026-10-12

Reviewed `10paz1`, `10paz2`, `dnskla`, `dna` and `kla` against raw SQLite, the
current dictionary and firmware dispatch. All observed `(bus, ID, DLC)` shapes
were already inventoried. A newly decoded C2 `0x42D` byte 1 bit 2 tracks the
right indicator across the October campaign and an independent September run;
the symmetric `0x42E` left side is still a candidate from one session. Hazard
activates both. The previous BH `0x2F0` byte 4 bit 4 wiper-state lead is refuted
by three speed/off cycles. DNA/LKA mappings agree with the existing code, while
rain, belt, doors, brake hold and central lock need more isolation. Evidence,
counter checks, exact event IDs and a minimal next capture are in
[the five-bundle investigation](../baccable_lab/docs/research/20261012-five-bundle-investigation.md).
The dictionary and generated CAN catalog were updated in PR #56; no
firmware or user-visible behavior changed. Physical right/left lamp verification
and the dedicated `wipers_lights_auto` capture remain open.
The cumulative evidence ledger retains 20 sessions / 21,354,624 frames;
dictionary version is `2026-10-12.4`, retaining PR #55's request-handler source
snapshot. All 70 Lab tests pass, including the new captured-payload,
bus-isolation and exact-DLC regression. Research PR #56 merged as `02dd92c`
after Readings PR #55 (`a9b5f44`). The resulting firmware is published as
[v5-beta-22](https://github.com/eujot/BACCAble/releases/tag/v5-beta-22);
[notes](releases/v5-beta-22.md) are attached to the release page. See the
release verification below; vehicle acceptance remains open.

## Release status, 2026-10-12

`v5-beta-22` was published from master merge commit `02dd92c` by
[workflow 38279570129](https://github.com/eujot/BACCAble/actions/runs/38279570129).
The release CI passed native host tests with sanitizers, Lab tests, menu labels,
release-script checks, cppcheck and C1/C2/BH/CAN firmware builds with pinned
Arm GNU 15.3.Rel1. The release contains BIN/HEX/ELF files for all four roles,
`BUILD_INFO.json` and `SHA256SUMS`. Downloaded assets passed every listed
SHA-256 check; build identity and tag both reference `02dd92c`. The release
body matches [v5-beta-22.md](releases/v5-beta-22.md). At release verification
time, no image had yet been installed in a vehicle. The owner's later beta-22
test found the DNA/LKA behaviors above. Readings appearance and the new
indicator field still need vehicle confirmation.

## Validated 20 ms IPC default, 2026-10-02

Task branch: `fix/ipc-default-20ms`, based on master `48e4646`.
Implementation: `778725f`. The owner reports stable 20 ms refresh without
visible artifacts. Make Quick 20ms + Write Full the boot/default-reset pair;
retain selectable Safe 50ms and Fast 10ms and the one-second reassertion.
Both boards share the default constants, preserving UART option indices.
Correct nondefault detection so Safe 50ms + Full is renewed by C1 and expires
on BH back to 20 ms, just like other temporary overrides. Rename Reset safe
to Reset default. Settings remain RAM-only; factory replay/test patterns stay
at 50 ms. Update both manuals and the implementation-source dictionary.

Bounded review covered full-screen buffering, CAN retry, UART option renewal
and default/lease/reset consistency. Existing latest-target coalescing and
bounded renewal retries already avoid redundant work; no speculative scheduler
or protocol changes were introduced.

Checks: all host tests pass with Apple Clang and sanitizers (including both
menu profiles, rejected UART changes, Safe + Full renewal, lease expiry and
boot pacing); cppcheck and full ARM builds pass for C1/C2/BH/CAN using
Arm GNU 15.2.Rel1. Lab dictionary tests: 17 passed. Release `v5-beta-21` was published from PR #50's merge commit. The new binaries
have not been vehicle-tested; the owner's acceptance concerns the previously
selectable 20 ms mode. This change does not establish support
for all IPC variants.

## Small IPC refresh optimizations, 2026-10-02

Implementation `7dff7fe` on `fix/ipc-default-20ms`, baseline `0772025`,
following the 20 ms default:
- Start the 100 ms periodic render interval at every visible render, including
  button-driven renders, avoiding duplicate work in the same loop.
- Reject unchanged UART screen submissions before copying the packet; retain
  the 500 ms resend clock and retry after queue rejection.
- Skip content comparisons for fragments already forced dirty. Full screen
  order, immutable active image, latest pending target and reassertion remain.

Host suite passes with Apple Clang/ASan/UBSan. Added checks exercise immediate
event rendering, the 99/100 ms boundary, tick wrap, and a complete identical
screen reassertion with repeated refresh calls and unaccepted offers. Existing
tests cover busy UART/CAN, latest-target coalescing and radio interference.
Cppcheck and full ARM builds pass for C1/C2/BH/CAN. No production counters,
heap allocation, new settings or CAN protocol changes were added.

Comparable local sizes use Arm GNU 15.2.Rel1, default flags and the same
`VERSION=ipc20-test` before/after (Flash = text + data, static RAM = data + bss):

| Board | Flash before → after (bytes) | Static RAM before → after (bytes) |
| --- | --- | --- |
| C1 | 94292 → 94272 | 15336 → 15336 |
| C2 | 29088 → 29088 | 12336 → 12336 |
| BH | 32776 → 32784 | 13180 → 13180 |
| CAN | 25200 → 25200 | 7000 → 7000 |

BH spends 8 Flash bytes to avoid forced-fragment comparisons. These are linked
sizes, not measured runtime stack headroom or CPU/visible-latency benchmarks.
Logs: ignored repo `tmp/opt-host.log`, `tmp/opt-{before,after,lint}-ROLE.log`,
`tmp/opt-size-{before,after}.txt`. No new vehicle test, release or flash.
Lab dictionary source hashes and generated catalog are refreshed to the
implementation commit; all 17 dictionary tests pass. No CAN layouts or
historical capture evidence changed.
Release notes are prepared in [v5-beta-21.md](releases/v5-beta-21.md). The
user-facing controls remain as documented in both manuals; only the internal
scheduling guide changes in this optimization pass. PR, merge and release
publication completed in PR #50 and workflow 37031156576.

## Release status, 2026-09-29

Released `v5-beta-19` from master commit `d071cfc` after PR #47. The beta
workflow completed successfully (run 36594882707): host tests, Lab tests,
menu labels, release-script tests, cppcheck, and all four firmware flavor builds
passed. C1/C2/BH/CAN images plus `BUILD_INFO.json` and `SHA256SUMS` are attached
to the [GitHub release](https://github.com/eujot/BACCAble/releases/tag/v5-beta-19).
Notes are maintained in [v5-beta-19.md](releases/v5-beta-19.md). Vehicle
acceptance is still open; publication does not confirm MY23 screen appearance,
button timing or behavior in the car.

## Release status, 2026-10-02

PR #50 merged as `31e10c3` on master. Beta `v5-beta-21` was built from
that merge commit by [workflow 37031156576](https://github.com/eujot/BACCAble/actions/runs/37031156576);
the shared host, Lab, lint and all C1/C2/BH/CAN builds passed. The
[release](https://github.com/eujot/BACCAble/releases/tag/v5-beta-21) contains
the four board images, `BUILD_INFO.json` and `SHA256SUMS`; full release notes
are in [v5-beta-21.md](releases/v5-beta-21.md). The release has not been
installed in a vehicle; 20 ms stability is owner-reported from the earlier
candidate, while these newly built images still need vehicle verification.

## Release status, 2026-09-30

PR #49 is merged to master at `5cf2509`. Beta `v5-beta-20` release notes cover
the MY23 16-column Readings/Favorites fixes, enum validation, dense Favorite
polling, and the temporary IPC refresh controls merged in PR #48. Release
`v5-beta-20` was built from `1708d8b` after the shared host tests, Lab tests,
menu and release-script checks, cppcheck, and all C1/C2/BH/CAN firmware builds
passed in [workflow 36765272225](https://github.com/eujot/BACCAble/actions/runs/36765272225).
The [release](https://github.com/eujot/BACCAble/releases/tag/v5-beta-20)
contains all four role images, build identity and checksums. Vehicle acceptance
remains open; a successful workflow does not verify screen rendering or CAN
behavior in a car. Notes are maintained in
[v5-beta-20.md](releases/v5-beta-20.md).

## Temporary IPC refresh controls, 2026-09-29

Implemented on `fix/ipc-refresh-controls`, based on released master `23db240`.
The prior stream could splice a newly submitted screen into an in-progress CAN
message, and 13 MY23 fragments at 50 ms needed about 650 ms per pass. BH now
uses Full by default: one immutable image plus one replaceable pending target,
complete padded passes, ordered fragments and retry without advancing on CAN
queue failure. Radio interruption still restarts from the latest target; profile
changes reset the stream rather than switching encoding within an image.

Settings → BACCAble IPC offers Safe 50ms / Quick 20ms / Fast 10ms, Write Full /
Write Delta, and Reset safe. Up/down chooses the row; SELECT changes it
immediately; BACK returns. These options live outside persisted settings and
never write Flash. The BH-only command 0x47 carries bounded indices, with a
five-second lease for experiments; C1 renews once per second, including outside
the menu, with queue-full retry also rate limited. A failed selection enqueue
leaves the previous choice. A complete power cycle restores Safe + Full;
C1-only reset stops renewal, and BH-only reset can be followed by reapplication
from C1. The original IPC test patterns and radio restoration stay at 50 ms.

Validation: 340 passing host scenarios with Apple Clang ASan/UBSan, all 16
flavor/IPC/width builds with full Arm GNU 15.2.Rel1, C1/BH cppcheck and diff
whitespace checks. Tests cover immutable full images, latest-pending coalescing,
CAN retry, pace selection, invalid input, lease fallback, initialization reset,
both menu profiles, UART busy and no Flash writes. Maximum Flash/static RAM:
C1 94,804/15,536 B; C2 29,104/12,416 B; BH 32,752/13,260 B; CAN 25,200/7,032 B.
Logs remain under repo tmp (`ipc-tests.log`, `ipc-arm-matrix.log`, `ipc-lint.log`).
PR #48 initially failed Lab source-snapshot checks because the tracked body and
stream hashes still described beta 19. Dictionary 2026-09-29.2 now references
implementation d0d0217, records Full/Delta pacing in the 0x090 description and
refreshes those two hashes and the rendered catalog. CAN layouts and historical
capture evidence are unchanged; all 46 Lab tests pass after the refresh.

Both user guides contain the same controls and comparison procedure. Pending
vehicle acceptance: compare Safe/Quick/Fast + Full, optionally Delta, using the
same source, long/short labels, scrolling, Favorites and radio track changes.
CAN enqueue success is not a visible IPC commit: Full prevents firmware mixing
but cannot guarantee atomic physical screen replacement. The existing one-second
reassertion and factory arbitration remain. No new beta or hardware flash.

## Current work

### Experiments readings section (public-source DIDs), 2026-10-13

Added a separate main-menu `Experiments` section carrying 38 extra diagnostic
readings derived from the danardi78 Giulia/Stelvio PID CSV, grouped `Electrical`
(4), `Oil` (3), `Engine` (15) and `Vehicle` (16). Diesel-only items and duplicates
were dropped; nine DIDs needing a signed or bit-level decoder (or a multi-frame
reply) were excluded from this release. Labels are human-readable English.

The stored settings format is untouched: the new pages sit at the end of the
gasoline catalog (ids 0x41.., groups 7..10), are always visible, and drive a new
`EXP_GROUPS` view that reuses the VALUES renderer. `parameter_pages` grew to
`[2][104]`, `parameter_definitions` to `[134]`, the cache to 136 entries, and the
menu list buffers to 128. `menu_page_list` lists experiments only for their own
groups, so group 0 (`All readings`) stays the ordinary catalog. C1 flash is now
98120/98304 bytes (184 B margin).

Validation: host suite (native ASan/UBSan) and the label test pass; all four ARM
flavours build and lint; the Lab suite (71 tests) passes after refreshing the
`parameter_catalog.c` and `parameter_cache.c` source hashes and the dictionary
version. The readings are **unverified on the owner's 2.0 petrol car** (the source
was a 2.2 diesel).

### Readings next-page prefetch, 2026-10-12

Task branch: `feat/readings-preview-prefetch`, implementation `6b7e839`,
based on merged master `7015b40`
(PR #54 includes the preceding firmware review). Previously MY23 Readings showed
the next page label on L2 but queried only L1, leaving the new page empty until
its first diagnostic response after navigation.

Readings now polls the current and actual next visible page from the filtered,
sorted list, including wraparound, deduplicating shared IDs. It retains one UDS
request per 500 ms and the 150 ms navigation settling interval. Native CAN values
continue to arrive through the existing receive cache. The preview label remains;
its diagnostic replies populate only the cache, never the current page's slots.
On navigation, an in-flight cache-only request can finish if the new current page
contains its parameter and the engine still matches, without extending its 500 ms
deadline. Other transitions cancel it. Diagnostic cache age in this view is
4500 ms for up to eight queried values; native values retain 3000 ms. Legacy
views without a next-page preview keep their existing polling.

Both manuals and MENU_UX describe the behavior and its limits: rapid scrolling,
missing replies and expired values can still show `--`. No synthetic values or
new CAN layouts are introduced. Validation covers prefetch before navigation,
reply during navigation, rate/settling limits, current-page isolation, composite
ID deduplication, filtered-list wrap, a single visible page, stale replies,
leaving Readings and legacy behavior, across both menu widths and MY23 defaults.
Host suite and menu labels PASS with Apple Clang ASan/UBSan; C1/C2/BH/CAN
builds PASS with full ARM 15.2.Rel1, all four cppcheck runs PASS, and all 69 Lab
tests PASS. Dictionary 2026-10-12.2 refreshes the request-handler source hash;
CAN layouts and capture evidence are unchanged. No vehicle test, flash or release.
The five original untracked capture bundles remain untouched.


### Firmware review of 3bbd8a5 and 3964000, 2026-10-12

Task branch: `fix/firmware-review-20261012`, baseline `3964000`, implementation
`f709bc3`. Review compared
DNA/LKA handlers with the bus-specific dictionary and October-11 findings,
settings/UART synchronization, ESC/TC interaction and Favorite migration.

- Restrict experimental lane-state rewriting to BH `0x4F6` and C2 `0x547`,
  standard data frames with DLC 8. Previously both IDs were modified on any role.
- Sample the default-mode baseline even while ESC/TC owns the outgoing frame.
  Reject invalid configured modes. C2 ESC/TC inversion uses the effective default
  Race mode, and stability reports do not advertise disabled controls when that
  effective Race mode has been inverted to Dynamic.
- Separate wake/save re-arming from transport resynchronization. USB exit no
  longer re-arms a manually cancelled default. UART command 0x48 gains an explicit
  fifth-byte re-arm flag; install matching C1/C2/BH firmware for this contract.
- Activate slot Favorites on fresh installations for both IPC profiles, so the
  seeded companion is actually displayed. Seed pairs only without saved page/slot
  preferences; never infer an untouched set from a matching primary page ID.
  Existing page and slot records retain their selections.
- Update both user guides and the menu technical guide. Correct the research
  note's inline-gateway claim: `can_forward` retransmits on the same peripheral;
  original traffic is not suppressed. LKA physical deactivation remains unverified.

Validation: PASS — full native Apple Clang ASan/UBSan suite and menu labels,
role-specific CAN regressions (C1/C2/BH), menu migration/sync regressions,
all four ARM 15.2.Rel1 builds and cppcheck, and all 69 Lab tests. No vehicle test,
flash, release, merge or remote publication. The user's five untracked capture
bundles remain untouched. Source hashes/catalog are refreshed in the follow-up
knowledge commit after the implementation commit.


### Themed default Favorites for 1- and 2-line displays, 2026-10-11

Replaced the four generic gasoline start-up favorites with six themed sets, each
pairing a primary page (first line) with a companion page (second line). A
1-line IPC shows the first column; a 2-line IPC shows both:

| Set | Line 1 | Line 2 |
| --- | --- | --- |
| S1 Elektryka | `Oil L / mm / %` `0x38` | `Batt charge BCM` `0x16` |
| S2 Osiągi | `Power / torque` `0x01` | `Boost pressure` `0x0d` |
| S3 Temperatury | `Oil/water/gear C` `0x3a` | `Intercooler out` `0x0a` |
| S4 Jazda | `Speed` `0x28` | `Gear` `0x1a` |
| S5 Timery | `0-100 km/h` `0x2a` | `Best 0-100` `0x2c` |
| S6 Olej / Multiair | `Oil/water/IC C` `0x39` | `Misfires/MA temp` `0x40` |

Diesel keeps its previous four defaults (no companion pages). The set active on a
fresh install is S1 (Elektryka). `MenuPreferences` still stores one page per set
(flash layout unchanged); the companion line is seeded at import time in
`import_page_favorites()` only while a set still equals its shipped default, so a
driver edit is never overwritten. New accessors `menu_default_favorite()` and
`menu_default_favorite_companion()` in `firmware/baccable/features/menu_model.c`
hold the tables. Host tests that depended on the old `0x04`/`0x28` defaults were
updated to derive counts from the shipped defaults or from a documented seed.
Validation: the native host suite and label test pass, all four firmware flavours
build and lint, and the 69 Lab tests pass. Both user guides updated.

### Re-analysis: DNA is settable, older bundles had no DNA/LKA data, 2026-10-11

Corrected an earlier analysis error. A first pass over the 2026-10-11 bundles
concluded the drive mode was only *observable* because “no request frame” exists
for the DNA knob. That was wrong: `0x384` is a body→ECU **broadcast** whose
consumers (TCM/IPC/ECM/… on C1; HAL/ORC/EPS/BSM on C2) obey its mode field, and the
firmware already rewrites and forwards it (`can_forward`, the ESC/TC path). A body
broadcast that consumers obey is actionable; a missing knob-input frame does not
make the function uncontrollable. All five bundles were re-checked:

- The 2026-10-10 bundles (`10paz1`/`10paz2`) had `dna_modes` and `lane_assist`
  **skipped**, so they carry no DNA/LKA ground truth — hence the dedicated
  2026-10-11 runs.
- DNA codes re-confirmed (N/D/A/Race = 0/2/4/12 on `0x384`/`0x46C`; 16/8/4/1 on
  `0x25A`).
- Wipers/rain remain **undecoded**: no clean bit survives counter filtering once
  windows are bounded to the scenario span (an earlier “result” was a window run
to the session end); `wipers_lights_auto` still needs recording.

  *Superseded on 2026-10-24:* C2 `0x2EC` byte 2 bit 5 is a repeating wiper-cycle
  phase, and BH `0x354` / C2 `0x73E` carry separate AUTO-selection and
  dipped-beam reports. The negative statement above reflects only the bounded
  counter-filter method used on 2026-10-11.

Recorded in `docs/research/20261011-dna-lane-decode.md` (correction plus method
lessons), the playbook anti-patterns, the dictionary (`dna_mode` notes,
`DNA-MODE-001` hypothesis, version 2026-10-11.3) and the re-rendered
`CAN_DICTIONARY.md`.

### Firmware: start-up default drive mode and lane-assist disable, 2026-10-11

Added two owner preferences to the C1 Setup menu, driven by the 2026-10-11
`dna_all_modes` / `lane_assist_toggle` capture sessions:

- **`Default mode`** (OFF / A / D / Race) — in beta-22 BACCAble repeatedly rewrites the
drive-mode field of the body broadcasts `0x384` (C1, C2), `0x46C` and `0x25A`
(BH) to the selected mode, the same `can_forward` way the ESC/TC feature forces
Race. The override is armed on wake-up/save and stops as soon as the driver
changes the mode by hand.
- **`Disable LKA`** — in beta-22 holds the lane-keeping assist off by repeatedly clearing byte 6 bit 1
of the lane-assist state frame `0x4F6` (BH) / `0x547` (C2), the signal decoded
from the lane-toggle sessions.

Both settings persist (flash slots 38/39) and reach C2 and BH through a new
`C2_Bh_cmdSetDriverAids` sync command. A new host test `tests/test_drive_mode.c`
covers the field encodings, the “stop after a manual change” rule and the
lane-assist clearing, and the board-sync retry test now counts eight options.
The linked Lab dictionary gained the two lane frames (`lane_assist_enabled` on
BH `0x4F6` / C2 `0x547`), refreshed firmware source hashes and a dated
`docs/research/20261011-dna-lane-decode.md` note (DNA codes confirmed, LKA state
and control). All four firmware flavours build and lint; the host suite (native
executables plus the label test) and the Lab suite pass. In-car behaviour of the
LKA override still needs confirmation on the vehicle.

### Scenario-decode docs consistency and the LKA toggle finding, 2026-10-11

Resolved the PR #53 rebase conflict in this file and audited the Lab documentation,
frame descriptions and code against the current scenario work. Kept the knowledge
base in sync: the seven catalog labels added for the targeted decode procedures
(`dna_natural`, `dna_dynamic`, `dna_allweather`, `dna_race`, `lane_assist_disabled`,
`lane_assist_enabled`, `wipers_auto`) are now listed in `giulia.json`
`marker_index` (status `no_event_decoder`), so the marker index again mirrors the
catalog; `dictionary_version` bumped to `2026-10-11.1` and `CAN_DICTIONARY.md`
re-rendered. Added the `ADAS-LKA-001` hypothesis: LKA is enabled/disabled by a
double-click on the left stalk button (not a menu item), and the frame carrying the
assist state is still unidentified — parked with the `lane_assist_toggle` scenario
and a `research_candidates[]` pointer. Corrected `docs/MARKERS.md`, which still
described the removed interactive marker palette (`1`–`0`, `f`, `g`, `c`), to the
scenario-confirmation workflow plus `baccable event add`, and refreshed one stale
scenario-count fixture in the UI test. 69 Lab tests pass; no hardware run was
performed.

### Bundle-first research playbook for AI reviews, 2026-10-10

Added `baccable_lab/docs/RESEARCH_PLAYBOOK.md`: the repeatable, test-minimising
procedure for an AI to turn a `baccable bundle` and its raw sessions into named
frames. It maps where frames are described today (`giulia.json` sources vs the
generated `CAN_DICTIONARY.md`, `hypotheses.json`, dated `docs/research/` notes),
routes each kind of finding to the right file, defines the
`unresolved→candidate→supported→validated` ladder, and orders the review so that
**frame-structure classification runs before candidate trust** — excluding
counter/checksum bytes (the `0x4AE`/`0x15A` trap) and correlating by state
interval rather than a fixed before/after window. It states exactly when a new
recording is justified and how small it should be, lists anti-patterns, and gives
the write/version/re-render/test loop. Linked from `SESSION_ANALYSIS.md` and the
generated catalog header. 69 Lab tests pass; documentation only, no code
behaviour changed.

### Guided-scenario CAN analysis and counter-frame red flags, 2026-10-10

The first two guided-scenario capture sessions (`20261010-104325Z`,
`20261010-111025Z`; 10.6 M frames together, `access`/`lighting` plus 40 further
procedures) were analysed from their `baccable bundle` artifacts and the raw
`session.sqlite3` bit sequences. Findings are recorded in
`baccable_lab/docs/research/20261010-scenario-sessions.md` and promoted to
`knowledge/hypotheses.json`; the dictionary moves to `2026-10-10.1` with new
`research_candidates`. Key results: the low-rate frames `0x4AE` and `0x15A`
(and `0x5A8` bytes 6-7) are pure counter+check heartbeats, so their bundle
candidates are phase artefacts — including the top-ranked hits; `0x341` looks
like monotonic accumulators (suspect). Real new leads: `0x116`/`0x3DE` closure
status (boot/bonnet, gateway-duplicated, one of the highest-volume unmapped
frames), `0x0F1` lock/door byte, `0x0FA` one-hot lamp flag byte, `0x100`/`0x104`
brake, `0x1FC` engine-running, `0x736`/`0x46A`/`0x2F0` HVAC/comfort and the first
wiper lead. Steady dipped/high/fog lamps produced no clean level bit (pulse/one-hot
hypothesis). No cross-session same-label repetition yet (the two sessions are
disjoint); next step is a repeat session plus canonical negative markers. 69 Lab
tests pass; no hardware run beyond the owner's recordings.

### Skipping and excluding scenarios you will not perform, 2026-10-10

The guided workflow can now record that a step was deliberately **not**
performed, which matters for ABS, ESC and traction-control procedures the owner
will not provoke. In the running scenario `k` skips the current step
(`outcome = "skipped"`), `o` skips every remaining step of the procedure and `e`
skips the rest **and** excludes the procedure permanently. A skipped step
advances the run but writes **no timeline marker** — only its `scenario_steps`
row — so it can never be mistaken for evidence that the physical action
happened; only `done`/`failed` carry markers. The same permanent decision is
available from a terminal with `baccable scenarios --exclude ID` /
`--include ID`, stored in the excluded list of `config/scenario_state.json`;
excluded procedures are filtered out of the session queue and never scheduled.
`ProgressStore` now tracks `skipped` separately (and `set_handled` reverts the
right counter on undo); the scenario list marks `[x]` done, `[~]` finished with
skips, `[E]` excluded, `[ ]` pending, and reports the three totals. `review`'
s `scenario_overview` and the bundle `brief` now count skipped confirmations
separately, and the bundle keeps the intentional gaps visible without treating
them as candidates. Screen legend, `SCENARIOS.md`, README and both user guides
updated. 69 Lab tests pass via the CI `unittest` discovery; no hardware run was performed.

### Targeted decode scenarios: DNA modes, lane assist, wipers→lights, 2026-10-10

Added three guided procedures and the markers they need, so the firmware options
the owner wants (default DNA mode, lane-assist disable, automatic lights driven by
wiper activity) can be decoded from controlled recordings: `dna_all_modes`
(drivetrain, stationary, D/N/A/Race each with its own marker),
`lane_assist_toggle` (adas, stationary, LKA off/on with a double-click on the left
stalk button) and `wipers_lights_auto` (wipers, stationary, wipers off/slow/fast/auto
with the headlights in AUTO, marking the dipped-beam reaction). The rain-sensor
trigger is deliberately dropped in favour of wiper activity, which the firmware can
see without a rain-sensor decoder. The catalog gains the four per-mode DNA markers
(`dna_natural`, `dna_dynamic`, `dna_allweather`, `dna_race`), the lane toggle
markers (`lane_assist_disabled`, `lane_assist_enabled`) and `wipers_auto` — English
keys, Polish on-screen wording, search aliases — so each mode correlates separately
instead of sharing `dna_mode_changed`. The bundled library is now **82 procedures /
513 confirmations**. `SCENARIOS.md` documents the three tests and the repetition
rule: ≥3 clean ON/OFF cycles in one recording, the same action repeated in a second
session for independence, and a physical check (watching the lamps) for `validated`
— 6–9 observations per action, not a repeat of all 82. The scenario ordering test
now compares `drive` before `adas` among driving procedures, so a stationary `adas`
procedure is allowed. 69 Lab tests pass; no hardware run performed.

### Scenario-first capture redesign, 2026-10-09

Guided scenarios are now the primary capture workflow. Lab runs procedures one
after another in a functional order (stationary groups first: access, lighting,
wipers, climate, media, chassis, brakes, drivetrain, parking; then the
parking-lot driving group `drive`; then the on-road `adas` procedures) and
advances to the next unfinished one automatically when a scenario completes.
Confirming a step writes a timeline marker plus a `scenario_steps` row
(`scenario`, `step_index`, `step_key`, `expect`, `run`, `outcome`). Progress
persists in `config/scenario_state.json` (`--state`), so the next launch resumes
at the first unfinished step instead of repeating completed work; `baccable
scenarios` marks finished procedures. The live screen was rebuilt for
readability: a boxed, enlarged `WYKONAJ:` step and only the scenario controls
(ENTER/y zrobione, x nieudane, p zatrzymaj, r wznów, u cofnij, s wybierz, q
zakończ). On-screen scenarios, actions, guards, controls and expected states are
Polish and follow the terms used in the car; the stable `id`/`key` and the
canonical `expect` labels stay English so structured markers, correlation and
the CAN dictionary are unchanged (Polish wording in `TITLE_BY_LABEL_PL` /
`label_text_pl`). The manual marker palette and its quick keys/groups/find/custom
are removed from the live screen; naming evidence comes from confirmed steps.
new modules `scenarios/state.py` and `scenarios/session.py`; a `site` field
(`parking`/`road`) distinguishes drivable locations. The screen uses ANSI colours
(colour, section header, `postęp: handled/total` bar, `dalej:` next-step preview,
`SPACJA`=zrobione, a confirmation bell and immediate `q`/`Ctrl-C` quit) for at-a-glance
readability in the car, with `--caps`, `--no-color`/`NO_COLOR` and `--no-bell`
options, and all in-session messages are Polish. `review` gains
`scenario_candidates` (bits that flipped for the same expected action at least
twice, ranked by repeat count), and `dictionary --propose`/`--accept` build,
validate and merge a candidate into a draft dictionary (promotion stays a
reviewed step, not an automatic write). Every capture can record run context
with `--context KEY=VALUE`, and `baccable bundle SESSION --output FILE` writes one
self-contained artifact (session context, capture health, coverage, ranked
per-action candidates, structural shape hints, per-step operator notes and
scenario definitions, and bounded raw excerpts) as the single input to hand to
the decoding step. New commands: `baccable
scenarios [--json] [--state]`, `capture --scenario ID --state`, `preview
--scenario ID`, `--scenarios-dir`. The bundled library has 79 procedures (473
confirmations, 11 groups) in `baccable_lab/scenarios/library/*.toml`, parsed with
stdlib `tomllib`; unknown `expect` labels are rejected at load. `review` returns
`scenario_steps` and `scenario_overview`. New `baccable_lab/docs/SCENARIOS.md`;
Lab README and both user guides updated. 54 Lab tests pass via the CI `unittest`
discovery. No hardware run was performed: the procedure flow and marker quality
still need a real capture (stationary and on a parking lot) before this closes
the naming loop.

### PR #49 follow-up review, 2026-09-30

Two complete Favorite pages can contain eight UDS values. The 500 ms request
cadence then needs four seconds, exceeding the three-second cache age and
causing avoidable `--` gaps. Favorites now deduplicate diagnostic IDs across
both rows and give diagnostic readings one complete poll cycle plus 500 ms
of reply margin (minimum 3 s, maximum 4.5 s). Native CAN readings and ordinary
Readings keep the three-second age. Query traffic remains bounded to two per
second. A small stack array replaces repeated page lookup inside the polling
loop. Tests cover eight distinct values, five unique values shared by two
pages, expiration after missing replies, and unchanged ordinary cache freshness.
The host suite passes with ASan/UBSan; C1's four-profile ARM matrix passes
with Arm GNU 15.2.Rel1. Vehicle behavior remains unverified.

The merge also left Lab's source hashes and IPC note describing the old 14+22
layout. Dictionary 2026-09-30.1 now references implementation `b3506a2` and
refreshes the source snapshot and generated dictionary for 16+CR+22; all 46
Lab tests pass. Historical capture evidence is unchanged. Both manuals document dense Favorite
refresh behavior. No further conflict in the Full/Delta display code was found
in this focused review.

### Gear 15 root cause and enum-rendering audit, 2026-09-30

The beta-19 source (`d071cfc`) decoded CAN gear nibble `0xF` as the integer 15
in `native_parameter_read()`. That raw value is intentionally retained as an
unavailable sentinel. The display regression was in
`favorite_parameter_segment()`: its special case rendered only valid gear
indices, but invalid gear values then fell through to the generic floating
point formatter. This produced the literal `Gear 15`. Commit `14eba38` added an
explicit `id == 6` unavailable branch, rendering `--`; the dashboard formatter
already rendered out-of-range gear as `-`. So the root cause was an incomplete
enum branch in the MY23 Favorites/Readings renderer, not a change to CAN decoding.

The same audit found enum values with valid-range checks but no integer check:
fractional gear/regeneration/seatbelt values could be truncated to a neighboring
label, and the Favorite regeneration/pedal-character formatters had the same
issue. These now use an unavailable marker unless the value is finite, in range
and integral. Exact-value DNA and seatbelt mappings already fell back safely.
Regression cases cover gear 15 and fractional enum/character values in both
dashboard and Favorite formatters. The full Apple Clang ASan/UBSan host suite
passes. No other enum-to-index/character conversion in the menu display paths
silently falls through to numeric rendering after this audit.

### MY23 catalog readings fit 16 columns, 2026-09-29

The user's follow-up found that clipping longer beta-18 catalog templates
could hide whole readings or cut a Best time midway. On branch
`fix/my23-readability`, MY23 Readings and both visible Favorite slots now use
shared compact templates with repeated spaces removed. The legacy 18/24-column
formats are unchanged. All page measurements remain on the same line;
`Oil 35.0mm Q 80%`, `B 100-200 8.54s` and four temperatures fit in 16 glyphs.
The formatter retains `--` for a number that exceeds its declared field width.
An enum conversion found by the display tests now checks its range before
converting float to an integer/character. All 24 Apple Clang ASan/UBSan host
executables pass. Tests cover both engine catalogs, both compiled legacy widths,
full-width numeric fields and the compact outputs. All four UI profiles build
for C1, C2 and BH with Arm GNU 15.2.Rel1; the largest C1 image uses
93,924 / 98,304 Flash bytes and
15,528 / 16,384 static RAM bytes, including reserved heap/stack. Vehicle
readability remains unverified. The change is open for review in
[PR #49](https://github.com/eujot/BACCAble/pull/49). The branch has been
updated with master, including PR #48's temporary IPC refresh controls. The
merge resolution preserves both feature sets; host tests and the full
C1/C2/BH/CAN UI build matrix passed before pushing the conflict fix.
No release or vehicle acceptance is implied by this integration update.

### MY23 measured 16-column correction, 2026-09-29

On `fix/my23-readability`, the owner measured 16 visible glyphs on the upper
MY23 field and reported that L2 sometimes appeared shifted left in beta 19.
The candidate changes the L1 budget from 14 to 16, moves the fixed CR/L2
boundary two characters to the right, and increases matching C1/C2/BH screen
UART packets from 38 to 40 bytes. The 39-character BH CAN output and its 13
fragments remain unchanged. The later MY23 compact-format change above ensures
catalog readings fit the physical 16-glyph L1 limit.
The blank-screen check now tests both MY23 fields instead of treating the marker
as visible content. The English and Polish guides and menu guide describe the
new limit. All 24 Apple Clang ASan/UBSan host executables pass, and all four
UI profiles build for C1, C2 and BH with Arm GNU 15.2.Rel1. Tests cover the
complete 16/22-character packet and L2 offset;
vehicle alignment must still be checked after flashing matching three-board
images. No release or merge has been made for this branch.

### MY23 readability follow-up after the owner's beta 19 car test, 2026-09-29

The owner confirmed MY23 menu operation but rejected the packed Favorite
abbreviations and reported `Gear 15`. The first two commits on branch
`fix/my23-readability` (`14eba38`, `17577e6`) incorrectly flattened catalog
pages into individual measurements. The clarified requirement is to keep every
Readings page in its original one-line catalog format, whether it contains one,
two, three or four measurements, and preview the next page on L2. A Favorite
must select two complete catalog pages, one per line. Commit `2857f93` on the branch
implements that contract and changes Favorite storage to version 2
page IDs. Version-1 atomic measurement IDs convert in RAM on load (including a
reserved compatibility page for extra RPM); older page
favorites import their original page ID into Slot 1. Hidden Slots 3–5 remain
stored but inactive. C1 polls every measurement in the two selected pages.

CAN 0x2EF nibble `0xF` means unavailable gear. Both Readings and Favorites now
use the catalog enum formatter (`Gear -`) instead of printing decimal 15. The
14-glyph upper MY23 field necessarily clips a longer catalog line when L2 holds
the next-page preview; this is an explicit hardware/layout limit to review in
the car. Both user guides and MENU_UX document the corrected behavior. All 24
host executables pass with Apple Clang ASan/UBSan; all four C1 UI profiles build
with Arm GNU 15.2.Rel1. The largest image uses 92,688 / 98,304 Flash bytes and
15,528 / 16,384 static RAM bytes, including reserved heap/stack. Vehicle display
acceptance remains open. This candidate is neither merged nor released.
PR creation remains pending GitHub CLI authentication.

MY23 menu and shared navigation, **2026-09-29**, branch `feat/my23-menu-ux`,
based on actual local/remote master `d63e608` (CAN dictionary PR #46 merged).
The owner's local `BACCAble_MY23_Agent_Plan.md` defines this task; it was
untracked at the start and is preserved, together with unrelated build helpers,
old firmware bundles and Lab package metadata. One PR contains the candidate;
no merge, new release, board flash or vehicle verification is authorized here.

Implemented: independent runtime MY23/legacy presentation; 14/22 glyph budgets;
current/next lists; centralized 30 ms debounce, 280 ms double-click Back and
900 ms labelled hold; isolated Features/Sort/Favorite drafts with explicit
save/discard, failure retention and successful-save feedback without automatic
exit. Read-only setup render descriptors replace the obsolete immediate-mutation
callbacks; successful persistence runs local effects and restarts board sync.
The older page-membership/visibility/order editors keep their documented exit
persistence for compatibility. Six five-slot sets per fuel profile reuse stable
measurement IDs and import prior composite page favorites without renumbering.
Source-specific names and compact units distinguish measurements; L2 retries
short/tiny labels and stops at whole segments. RPM reuses incoming 0xFC telemetry.

BH uses fixed, fully padded L1/CR/L2 fields and selected BMP glyph tokens while
preserving learned audio sources and existing arbitration/reassertion rules.
Screen UART frames are 38 bytes; command/status frames retain the compiled
legacy width. Matching C1/C2/BH images are required. Menu record 0x105 is 142 B;
record migration preserves the previous valid Flash page until commit. Both
user guides and the menu/catalog/build guides document these changes. CI now
checks all four IPC/width combinations inside each firmware-flavor job, without
multiplying runner/toolchain installation jobs or changing release artifact flags.

Local validation uses Apple Clang with ASan/UBSan (24 host executables), eight
CI-script tests, menu labels, cppcheck on C1/C2/BH/CAN and the full Arm GNU
15.2.Rel1 toolchain across 16 flavor/profile combinations. There are 296 passing host scenarios and 46 passing Lab tests. The dictionary
snapshot `2026-09-29.1` references implementation commit
`d79c4145f6ca3801461bdabc3d879564b1c69e92`; original session ledgers/markers
remain unchanged. No new CAN bit layout or historical capture interpretation is
inferred from this UI work. Identified images use version `my23-d79c414`.
C1 peaks at 97,724 / 98,304 Flash bytes and 15,472 / 16,384 static RAM bytes
across the matrix; headroom is small, without changing the linker reservations.
Build logs/images and checksums/compiler/flags/source identity in
`tmp/my23-build-manifest.json` stay under ignored repo `tmp/`. Integration is pending
PR review; CI's pinned compiler is 15.3.Rel1 and must independently pass.

**Hardware acceptance remains open:** MY23 field alignment after long/short
labels, actual glyphs under USB/Bluetooth/CarPlay, current/next scrolling,
double-click/third-click/hold timing, successful and failed draft saves,
five measurements, source arbitration/recovery during radio track changes,
responsive USB CAN/OFF/re-entry, and legacy normal/large rendering. An independent
BH TX trace is needed to distinguish CAN enqueue success from IPC display success.
Host tests do not establish visible alignment, ownership, latency or flicker in
the car. Use the acceptance procedure in
[MENU_UX.md](architecture/MENU_UX.md#refresh-and-vehicle-boundaries).


### MY23 focused PR follow-up, 2026-09-29

Budget-limited review of PR #47 after `9e5a4e8`, focused on the plan's stable
Favorites/list selection contract. Confirmed a shared-cursor bug: browsing
Readings replaced the atomic Favorite selection, and filtering an earlier set
could move the cursor to another set. A regression failed on the previous code.
Live Favorites now retain a set ID per fuel profile separately from list cursors;
filtering restores that ID when eligible, otherwise the first available set.
This is RAM-only state; saved settings and migration formats are unchanged.
Both user guides and MENU_UX document the behavior.

Validation: 281 passing scenarios across all five menu test executables with
Apple Clang ASan/UBSan, including the new cross-list/filtering regression.
C1 MY23/LARGE_DISPLAY compiles with full Arm GNU 15.2.Rel1: 94,452 bytes Flash,
15,528 bytes static RAM including reserved heap/stack. `git diff --check` passes.
Logs: `tmp/my23-quick-audit.log`, `tmp/my23-quick-arm.log`. This incremental
review does not replace the earlier full matrix or the pending vehicle tests.
Previously packaged `tmp/my23-9e5a4e8.zip` does not include this correction.

### MY23 audit corrections, 2026-09-29

Follow-up on `feat/my23-menu-ux` / PR #47, based on audit commit `3d94322`.
This entry supersedes the pre-fix gaps in the dated audit below; that table is
retained as evidence of why the corrections were required.

Features save now updates the original draft values and stays in the same
editor after feedback. A separate Back returns exactly one level to Features.
Page-membership, visibility and page-order editors use isolated preference
drafts, locked confirmation, explicit hold-save and discard; failed saves
restore live preferences while retaining the draft, and timeout discards.
Committed legacy membership/order changes regenerate imported atomic sets in
the same record. MY23, and legacy after the first five-slot save, expose a single
Favorites editor: Settings is Features, Favorites, Shown pages, Sort order.
The old membership/order entries remain accessible only before legacy switches
to atomic sets. Existing IDs, record layout and migration stay unchanged.

MY23 slot and measurement-picker lists, Shown pages and IPC-test lists now show
current/next, including wraparound. Slot labels use short/tiny names so that the
active marker and slot number fit together. Empty Slot 1 hides the set; duplicate
assignment moves the measurement only in the draft. Full measurement names stay
in the picker; V6-only ignition corrections now retain cylinder 5/6 in compact
labels. Generic long notices continue onto L2 instead of discarding their useful
suffix for a static Back footer. Both user guides and MENU_UX describe the same
behavior and conditional legacy menu.

C1 initially exceeded its unchanged 96 KiB application allocation. Link-map
inspection showed decorative LED `rand()` pulling newlib heap/assert/stdio
support. Caller-owned `rand_r()` state removes that dependency path and retains
the same RGB channel ranges; the specific decorative random sequence changes.
No linker/storage reservations or firmware optimization flags were changed.
The matrix script also handles an empty base-flag array under macOS Bash 3.2;
its native-shell regression covers all four default flag combinations.

Regression validation: 326 passing host scenarios / 24 executables with Apple
Clang ASan/UBSan, including success-then-Back, legacy page transactions and
import consistency, MY23 visibility, every added list preview/wrap, V6 names,
compact timer labels and duplicate-slot failure/discard. Eight CI-script tests
pass, along with 46 Lab unittest cases. All 16-profile Arm GNU 15.2.Rel1 builds
and C1/C2/BH/CAN cppcheck pass. C1 peaks at 94,560 / 98,304 Flash bytes
(3,744 free) and 15,520 / 16,384 static RAM bytes (864 free, including the
existing heap/stack reservations). Build logs stay in repo tmp. No vehicle test
or new release has been performed. Alignment, glyph legibility, actual gesture
latency, flicker and radio/USB/CarPlay takeover remain the section-22 hardware
acceptance checklist, not evidence supplied by these software tests.

### MY23 plan re-audit: incomplete contract, 2026-09-29

Reviewed all 25 sections of the owner's untracked `BACCAble_MY23_Agent_Plan.md`
against PR #47, implementation `d79c414` and documentation `ccfa10e`. This audit
supersedes any interpretation of the implementation summary as full completion.
The candidate has not been merged or hardware accepted. Firmware was not changed
by this review. The existing host suite was rerun: 296 passing scenarios across
24 executables, with Apple Clang ASan/UBSan. Additional probes compiled against
the production menu/setup/formatter sources reproduced the gaps below; source,
executable and output are under ignored repo `tmp/my23_audit_*` and
`tmp/my23-audit-probe.log`. These exploratory probes are not committed regression
tests. The earlier 16-profile ARM builds and successful PR CI are prior evidence;
this documentation-only audit did not rebuild firmware.

| Plan section | Code/verification status | Evidence or remaining work |
| --- | --- | --- |
| 1. Display strategy | Implemented | Shared controller; independent runtime IPC profile and legacy width. |
| 2. Visible layout | Partial | 14/22 budgets exist, but generic views still use static `2X BACK` instead of contextual L2. |
| 3. List behavior | Partial | Root, groups, Settings, Features and Favorite sets have current/next; slot/picker, compatibility editors and IPC test lists do not. |
| 4. UI modes | Partial | Modes and render paths exist; not all applicable views use their specified contents. |
| 5. Input model | Implemented | Central FSM, 30/280/900 ms timing, cancellation, suppression and boundary tests. Vehicle timing is unaccepted. |
| 6. Back/save/discard | Partial | Features save closes the draft; next Back skips the Features list. Compatibility editors mutate live preferences and save on exit without discard. |
| 7. Labelled holds | Implemented in code | Save/apply/store prompts exist; physical legibility remains acceptance work. |
| 8. Legacy | Partial | Single-line rendering and central gestures retained; universal draft workflow has the same compatibility-editor exceptions. |
| 9. Atomic measurements | Implemented with compatibility | Atomic Favorites reuse stable measurement IDs; old composite pages remain as permitted by gradual migration. |
| 10. IDs/persistence | Partial | Old IDs and 0x103/0x104 migration preserved and tested; legacy favorite edits no longer update the imported MY23 sets. |
| 11. Full/short/tiny labels | Partial | Common labels explicit; generated fallback wrongly uses the non-V6 name for V6-only IDs 19/20. |
| 12. Five-slot Favorites | Implemented | Six ordered five-slot sets per fuel profile, slot 1 primary; isolated persistence with failure rollback. |
| 13. Favorites editing | Partial | Editing confined to Settings; slot/picker browsing does not provide the required current/next presentation. |
| 14. Favorite rendering | Implemented in code | Primary on L1, other measurements on L2, supported enums and units; V6 label bug still applies. |
| 15. Packing | Implemented | Short-to-tiny retry, ordered whole segments, 22-token budget, no trailing bullet/partial segment; boundary tests. |
| 16. UTF width | Implemented for selected vocabulary | Selected ten symbols map to one token/code point; encoding/clipping/bounds tested. Token-string strlen here counts glyphs, not raw UTF-8 bytes. |
| 17. Separator | Implemented | Bullet used for compact MY23 separation. |
| 18. Alignment | Partial, hardware unverified | Fixed padded fields and GEAR/short-replacement regression tests; physical shifted-L2 cause/fix not yet confirmed. |
| 19. Final screens | Partial | Save feedback appears, then Features list replaces the setting editor; slot previews and some contextual L2 screens differ. |
| 20. Mandatory tests | Partial | Existing 296 scenarios pass but miss save-then-Back parent, V6-only labels, legacy/atomic favorite consistency and all-list preview contracts. One save test asserts the incorrect draft closure. |
| 21. Matrix | Verified in prior build/CI | Four independent IPC/width profiles across C1/C2/BH/CAN; no new firmware changes in this audit. |
| 22. Vehicle acceptance | Not performed | All 17 MY23 acceptance items remain open; no alignment, source recovery, flicker or latency claim. |
| 23. Phased implementation | Partial | Two broad implementation/documentation commits, not the recommended small phase sequence; vehicle phase missing. |
| 24. Non-negotiable contract | Not fully satisfied | Sections 2/3/6/11/13/20 and physical acceptance remain incomplete. |
| 25. Preparation | Mostly satisfied | Instructions, Git, IDs, encoding/input/transport and tests inspected; unrelated files preserved; small-commit recommendation not followed. |

Required corrections before treating the MY23 plan as complete:

1. Keep a successfully saved Features draft open, update its original value and
   dirty state, return to the same editor after feedback, and have a separate
   Back return exactly one level to Features. The probe prints `stage=0` and a
   LIST packet after save; next Back renders Settings (`Features` / `Page favorites`).
   `settings/setup_menu.c:491` clears `staged`; existing
   `tests/test_my23_ui.c:90` incorrectly expects this. Update both guides' current
   claim that save remains in the setting editor when fixing the behavior.
2. Apply isolated save/discard semantics to Page favorites, Shown pages and
   Favorite order, or remove the competing legacy editors from the MY23 workflow
   while preserving stored IDs and legacy compatibility. Explicitly define how
   legacy page selections and atomic sets interact. The production probe shows
   a saved Page favorites edit changes legacy membership but leaves all 60 atomic
   slot bytes unchanged, so MY23 live Favorites do not reflect that editor.
3. Render all remaining list/slot/picker views with useful current/next context
   and uncluttered names. `FAV_SLOTS` currently repeats the selected parameter on
   both lines; the compatibility and IPC-test views fall through the generic
   `2X BACK` footer. Audit long notices too: generic rendering clips L1 to 14
   glyphs without moving the useful remainder into L2.
4. Fix source-aware fallback parameter labels, preferably explicit catalog
   metadata for all supported measurements. IDs 19 (`Ignition cyl 5`) and 20
   (`Ignition cyl 6`) both format as `E 1.0°`: source lookup falls back to V6,
   but name lookup still passes `v6=false` and abbreviates `Empty`.
5. Add permanent failing-then-passing regressions for those confirmed gaps and
   complete per-view gesture/render checks. UART submission is one complete
   screen packet; CAN output remains a sequence of fragments, so rapid-refresh
   visual consistency still needs a TX trace and vehicle acceptance, not just
   an assertion that both fields were built together.
6. Execute and record every section-22 vehicle check with matching C1/C2/BH
   images. Maintain the small Flash/RAM headroom recorded above when correcting
   the candidate. No new beta is justified by this audit alone.


CAN session knowledge and iterative analysis, **2026-09-29**, branch
`feat/lab-can-dictionary`, based on source snapshot `c550e161`: reviewed
12 owner recordings from September 26–28 (8,446,020 frames, 99 manual events,
26 explicitly CAN-derived events). Three new recordings add reverse, park,
windows, ignition, EPB, media and lighting observations. Preserve original
databases, committed WAL, raw streams and markers; their hashes are in the
versioned evidence ledger. Six runs are complete; failed-run data and parser/
reader errors remain visible, and the full stationary hardware acceptance
checklist is still open.

The packaged passive dictionary describes 43 bus-specific messages, 115 fields
and 76 diagnostic catalog entries, links 153 marker labels, inventories 337
bus/ID/DLC shapes, and stores 215 empirical templates learned from complete
runs. Code decoding, historical comments, manual correlation and CAN-derived
annotations are separate evidence sources. New lighting bits are research
hypotheses, not implemented actuator behavior; ambiguous window/brake labels
and the earlier reversed indicator labels are retained. `dictionary`, `review`
and `session decode` support read-only lookup, provenance-preserving batch
review and annotations of saved frames. The automated bit-step screen is
explicitly exploratory and does not create named decoders from correlations.

Read [the agent workflow](../baccable_lab/docs/SESSION_ANALYSIS.md),
[the generated frame catalog](../baccable_lab/docs/CAN_DICTIONARY.md) and
[the dated findings](../baccable_lab/docs/research/20260928-findings.md).
AGENTS.md directs future capture-analysis work to this workflow. Evidence
attachment and Markdown generation scripts keep the catalog reusable for
subsequent recordings. Lab host tests cover source/layout provenance, roles,
length guards, diagnostics, markers, templates, passive CLI commands and
read-only WAL behavior and incomplete IPC rejection. All 46 Lab tests and whitespace/Python compilation
checks pass locally. Firmware, on-device menu and capture behavior are unchanged;
no new firmware build, release or vehicle trial is claimed. Changes and this
handoff are committed on the task branch and prepared for PR review against
master; merge/release is not performed here.

Multi-bus preview startup fix, **2026-09-28**, branch
`fix/lab-preview-multiple-buses`, based on master `c0bdf9b`: preview assigned
the same `SIMULATED` endpoint to every role, so shared capture validation
rejected the default three-bus preview and any two-role selection before
starting readers. Give each role a distinct `SIMULATED:<ROLE>` endpoint;
retain duplicate-device rejection for real capture. The CLI regression now
exercises the default, every nonempty bus subset and repeated role selection,
checks per-role raw/SQLite data, verifies no real serial reader is constructed,
and confirms temporary-session deletion. All 29 Lab tests, Python compile
checks and whitespace checks pass. Interactive terminal smoke tests with the
actual demo readers show frames for C1/C2/BH by default and C1/BH when selected,
and exit successfully with `q`. Handoff changes are committed on the task
branch. No vehicle test is needed to verify this host-only startup defect.

Handoff/pull recovery, **2026-09-28**: a local, uncommitted ACTION_PLAN blocked
the fast-forward from `eeffe082` to merged PR #43 at `c39a286`. Its complete
contents were retained in the local named Git backup
`backup/action-plan-before-pull-20260928` and verified byte-for-byte before
restoring only that tracked file. The user's master now matches the remote;
unrelated untracked files and existing stashes were preserved. Useful earlier
capture observations are recorded below with their historical scope. AGENTS.md
now requires handoff edits to be committed on the task branch and prohibits
copying them back uncommitted into master. This documentation-only change is
prepared on `docs/keep-handoff-committed`; whitespace and recovery-state checks
pass. No firmware or capture behavior changed.

BACCAble Lab marker catalog and quick search, **2026-09-28**, based on
`origin/master` at `eeffe082`: expanded the manual marker catalog to cover
vehicle/body controls, powertrain and live readings (including SOC and
intercooler temperature), brakes/stability, driver assistance, IPC/media,
chassis, weather, lights, comfort, parking and diagnostics. The capture screen
now spells out the `1`–`0` quick-key meanings, and `f` searches by action or
common acronym; a unique match is timestamped directly, ambiguous results can
be selected, and unmatched text becomes a custom marker. `baccable_lab/docs/MARKERS.md`
describes all groups, search, source research, safe-use boundaries and why raw
CAN does not decode feature names. All 29 native Lab unit tests, Python
compile checks and `git diff --check` pass locally. The first full PR CI run
([36470892309](https://github.com/eujot/BACCAble/actions/runs/36470892309))
passed Lab/host tests, lint and all four firmware builds. PR #43 was merged
into master at `c39a286`. No vehicle or hardware test is claimed for this Lab
UI change. Firmware and both device user
guides are unchanged.

### Recovered historical capture observations (2026-09-26–27)

These measurements were recorded in the previously local handoff; they are
preserved here as historical evidence, not new acceptance results or a second
backlog. Later IPC source tracking, periodic menu retransmission and two-line
display changes are covered by the integrated candidate entries below.

Session `20260927-185147Z` recorded all three USB roles for 181 s. The owner
reported that selecting USB CAN no longer stalled the device, while radio
track changes still obscured the menu. The earlier review found 33 complete
radio 0x090 title/artist transfers lasting 200–405 ms; 29 exceeded the old
18-character menu width and 13 adjacent gaps were shorter than 250 ms.
Thirteen C1 0x2FA holds lasted 1.302–4.541 s. Their start/release timestamps
were added as 26 explicitly CAN-derived events and `long_back_holds.csv`;
no radio 0x090 frame overlapped a hold, and three holds occurred within a
15.093 s radio-text silence. These captures contain CAN RX, not BH's own
display TX or C1–BH UART, so they cannot by themselves locate the display
failure. No manual marker established exact visible-menu state during a hold.

The later USB-playback session had 33 complete transfers (301 frames) with
source context `0x06`, a `0x0D` title/artist separator and two blank controls;
the owner observed a USB icon and two lines. The CarPlay-playback session
`20260926-182645Z` had three complete transfers (57 frames) with context
`0x21` and eight blank `0x01`/`0x0F` messages. The old BACCAble one-line
Bluetooth context was `0x09`. These observations motivated the later source
tracking and display-layout candidates; they do not establish undocumented
IPC ownership or clear-control semantics. Detailed historical timing reports
remain in the sessions' ignored `display_analysis.md` files.

## Verified baseline and deployment

BACCAble Lab offline preview and capture dashboard, **2026-09-28**, branch
`feat/lab-offline-preview` from the current `master`: adds `baccable preview`
with synthetic multi-bus traffic, a live raw-frame pane, latest-event timeline,
and grouped manual markers for IPC/media, suspension, rain/wipers, lights,
climate and common driving/vehicle events. The same screen is used during real
capture. Preview data lives in a temporary directory and is discarded on exit;
labels are explicitly manual observations, not decoded CAN signals. Lab README
and capture guide describe use. PR #42 was merged into master as
`eeffe08248174249176a03a76614754a7bb9fa17`; its CI passed Lab/host tests, lint
and all four firmware builds. Vehicle/hardware capture behavior was not
exercised by that host-only UI work.

Beta-18 release, **2026-09-28**: merged PRs #39–#41 are included at
`fdb14135314053d98a4b0b4574bc1e5e3ba2cc5f` and published as
[`v5-beta-18`](https://github.com/eujot/BACCAble/releases/tag/v5-beta-18).
Release workflow [36358007483](https://github.com/eujot/BACCAble/actions/runs/36358007483)
passed host/Lab/menu tests, cppcheck and C1/C2/BH/CAN builds. Release assets
include BIN/HEX/ELF, `BUILD_INFO.json`, `SHA256SUMS` and release notes. IPC
display behavior still needs in-car verification.

IPC source/Unicode display experiment, **2026-09-28**, branch
`feat/ipc-display-test` based on merged PR #39 (`383c8c8`): added a
production-accessible `Information → IPC display test` submenu with explicit
USB (`0x06`), Bluetooth (`0x09`) and observed CarPlay (`0x21`) codes. BH emits
four fixed 16-bit character patterns: glyph samples, 48-character rulers for
each line, and a two-line 24+24-character sample. The test is temporary,
non-persistent, uses the existing coalesced screen UART channel, and resumes
the normal menu on exit or after its idle timeout. Host tests cover navigation,
exact CAN fragment numbering/UTF character bytes under each code, periodic
retransmission, factory-text interruption, CAN retry, five-second command
lease and return to the normal menu. Apple Clang ASan/UBSan host tests,
menu-label checks, CI-script tests and four-flavor ARM lint/builds with
Arm GNU 15.2.Rel1 pass. Both user guides and the menu
transport guide describe how to compare the visible output. Vehicle acceptance
is still open: actual glyph support, icon/source behavior, line clipping and
BH TX must be observed on the car. No firmware has been flashed for this task.

CI duplicate-run reduction, **2026-09-28** ([PR #40](https://github.com/eujot/BACCAble/pull/40)),
merged to `master` as `3c68b7a`: direct `push` CI now covers `master` only.
Pull requests still run CI, and release/diagnostic reusable calls are unchanged.
This removes the duplicate push and pull-request runs observed for commit
`ca1bef0` on PR #39 (runs 36355278991 and 36355299930). The branch produced
one successful PR run (36355667493) and no branch-push run; the merged commit
passed master push CI (36355833867). This change affects workflows only.

IPC menu visibility candidate, **2026-09-28**, branch
`fix/ipc-current-source-menu`, based on `origin/master` at `7717505` after
beta-17. BH now learns the display source from complete factory media text
transfers (including the observed USB `0x06` and CarPlay `0x21` contexts),
restarts a full menu transfer after factory text, periodically retransmits it
without new radio frames, and adds a temporary `BACCAble beta` second line.
On close it replays the last complete factory media message. Incomplete text,
blank control frames and CAN enqueue failures have host regression coverage.
Both user guides and the menu transport guide describe this candidate.
Native Apple Clang ASan/UBSan host tests and all four C1/C2/BH/CAN ARM
lint/builds with Arm GNU 15.2.Rel1 passed. No vehicle test or flash has been
performed. The IPC's actual handling of a two-line BACCAble message under
each source, its ownership priority during sustained radio traffic, and menu
latency require a car test. Before a complete media transfer has been seen,
the existing configured source remains the fallback. An isolated pull request
contains this change; it is not part of beta-17.

USB recovery fix and beta-17 release, **2026-09-27** ([PR #37](https://github.com/eujot/BACCAble/pull/37)):

- Isolated from unrelated local CI/menu work. Exact release base is
  `v5-beta-16`, `eded486f0ab022cda3f87b99070b813c7d7de38b`, verified against the
  release BUILD_INFO.json. Beta 13 has the same faulty USB teardown path.
- Confirmed with the actual ST core/MSC/BOT under ASan/UBSan: the old
  `USBD_Stop` followed by `USBD_DeInit` calls MSC class teardown twice; the
  second call writes through a null class pointer. C2/BH stop at the fault,
  explaining missing CDC and the stalled BH display path. This reproduces a
  software defect, not the exact historical execution on the user's boards.
- Fixed single teardown and guarded MSC cleanup; added asynchronous USB reset,
  propagated start errors, at most three attempts, addressed per-board mode
  requests/status, UART TX recovery, and read-only EP0 status snapshots.
  Lab `doctor --usb-status --samples 3` reads diagnostics without consuming CAN.
  Updated both user guides and the owning USB guide.
- Validation: native ASan/UBSan suites including the real USB lifecycle,
  Lab unit tests, menu-label checks and four-flavor ARM builds/static analysis.
  Final image sizes, exact source SHA, compiler, flags, checksums and logs are
  recorded in the test bundle, not inferred from the release baseline.
- Merged to `master` as `5f1b493119ecf42e05f3c2244eaacf5e35dc0b03` and
  published as [`v5-beta-17`](https://github.com/eujot/BACCAble/releases/tag/v5-beta-17),
  firmware version `beta-5f1b493`. Release workflow 36304510045 passed host
  ASan/UBSan tests, Lab tests, cppcheck, all four ARM builds and artifact
  packaging. Release assets include BIN/HEX for C1/C2/BH/CAN, ELF for all four,
  `BUILD_INFO.json` and `SHA256SUMS`; the release page records changes and
  hardware acceptance limits. Local `u16-f516496` images use ARM 15.2 and are
  separate diagnostic artifacts; use the published beta-17 set for this test.
- No firmware was flashed during this task. The original working tree and its
  unrelated local edits remain preserved. This section supersedes older USB
  acceptance claims below for this reported failure.
- **Hardware acceptance remains open:** repeated three-port enumeration,
  responsive menu during capture, OFF/re-entry, missing-board/USB errors,
  warm/cold boot and hub power. No boards were available for this audit;
  no voltage measurements or vehicle results are claimed. Follow
  [the audit and vehicle procedure](architecture/USB_DIAGNOSTICS.md#msc-to-cdc-failure-and-recovery-candidate-2026-09-27).


Documentation integration, **2026-09-26**: the EN/PL guides, README links and
maintenance rules are prepared on `docs/in-car-guides-en-pl`, based directly on
`origin/master` at `294818a`, for review and merge into `master`. This change
contains documentation only; unrelated untracked build/helper files are excluded.

User-guide maintenance rule, **2026-09-26**: added the requirement to update
both EN/PL guides for user-visible menu and operation changes to `AGENTS.md`
and this starting checklist. Added reminders in the menu, reading-catalog and
USB technical guides. The Polish edition is explicitly exempt from the default
English documentation language. Link and whitespace checks passed; no firmware
or user-guide content changed in this task.

Polish user-guide translation, **2026-09-26**: added the explicitly requested
[Polish edition](../manuals/BACCAble_USER_GUIDE_PL.md) alongside the English guide
and linked it from README. Uses plain Polish explanations while preserving
on-screen English labels, messages and command syntax. Checked all 41 heading
levels, 31 feature settings, 12 actions, 64 gasoline and 60 diesel reading rows
against the English edition, including reading groups and advanced flags.
Shell examples match, local links and Polish section anchors resolve, and
whitespace checks pass. The English guide and firmware are unchanged by this
translation task; no hardware verification was performed.

In-car user documentation, **2026-09-26**: added
[the current user guide](../manuals/BACCAble_USER_GUIDE_EN.md) and linked it from
the repository README. Reconciled the legacy English DOCX/PDF descriptions with
the current navigation, grouped Features order, 31 visible settings, 12 actions,
9 ordinary Information pages and all 124 reading pages. Includes source-specific
SOC explanations, mirror capture, brake/launch release semantics, USB modes and
the limits of queued/sent/confirmed status. A documentation check matched all
64 gasoline and 60 diesel labels, ordering, groups and advanced flags against
firmware, checked feature/action coverage, and resolved local links and anchors;
`git diff --check` passed. No firmware changes or hardware tests were made for
this documentation task. Preserve the existing untracked build/helper files.

Release status verified **2026-09-26**: PR #32 is integrated into `origin/master`
at `294818a3271222b2bc7870541833ba9c1bf4c8a8`; the local `7386367` source tree
matches that commit. [v5-beta-13](https://github.com/eujot/BACCAble/releases/tag/v5-beta-13)
is published with notes. [Release run 36197554882](https://github.com/eujot/BACCAble/actions/runs/36197554882)
completed successfully for that source. Stability fixes 1–3 below are therefore
included in beta-13. Publication does not establish installation or vehicle
acceptance.

Stability fixes 1–3, **2026-09-26**, branch `fix/usb-uart-capture-shutdown`
based on `0b3733d` (not included in published beta-12): USB entry resumes the board UART only when leaving low
power, preserving active TX on awake C1. Regression covers both awake entry
(no resume) and wake entry (one resume). Lab stops further reads and drains its
queue concurrently with final producer delivery, uses timed puts with an abort
event on writer failure, and reports a reader exceeding shutdown timeout.
Initialization is inside the cleanup boundary; partial raw-file opens unwind,
all raw closes are attempted, and SQLite closes even on initialization/finish
failure. Reader failure status is also consistent across DB, summary and terminal
as part of the shutdown repair. A failing disk cannot guarantee final metadata
or retention; bytes still in device/OS buffers are outside the drained boundary.
Full host sanitizer suite, all four firmware lint/builds (Arm GNU 15.2.Rel1)
and all 13 Lab tests pass. Local images are under
`firmware/baccable/build/stability/{C1,C2,BH,CAN}/`, version `stability-local`.
Lab regressions cover q/Ctrl-C with a full
queue, disconnect, aborting a blocked producer, partial thread start, file-open,
write/close, database commit/initialization, terminal initialization and summary
write failures. Audit findings below describe beta-12; long-session counting,
raw record offsets and general UART lost-completion recovery remain separate.
At the time of the local validation, no merge, release or hardware flashing had
been performed; the later beta-13 integration/publication is recorded above.

Stability review, **2026-09-26**, beta-12 source `b414714`, documentation head
`0b3733d`: review of the v5 transport/menu/storage/diagnostic paths and Lab,
not exhaustive hardware certification. No production code changed in this review.
Open findings, in priority order:

- **P1 / UART state corruption on USB entry:** `usb_modes.c:switch_mode()`
  unconditionally calls `uart_resume(&huart2)`, which overwrites HAL State with
  READY and starts RX without first quiescing TX. The main loop calls
  `board_uart_process()` immediately before `usb_modes_process()`, so TX can be
  active at entry. With State changed to BUSY_RX, the bundled HAL's
  `UART_Transmit_IT()` returns BUSY without writing TDR or disabling TXEIE;
  the IRQ handler ignores that return. This can cause a repeated TXE interrupt
  and starve normal work. An isolated native harness using the exact HAL TX
  function confirmed 100 consecutive calls leave TXE enabled and the count
  unchanged in that state. This is not an on-device reproduction. Fix USB entry
  to preserve active UART operation or perform an explicit coordinated
  pause/resume, with regression coverage for entry during TX. A lost completion
  also has no ordinary-path TX deadline; assess recovery without truncating
  valid transfers. This finding remains present in beta-12.
- **P1 / Lab shutdown and blocked reader:** `_Reader.run()` uses blocking
  `Queue.put()` without stop-aware timeout. When full, stop/join(1 second) does
  not terminate it or close its serial context. Capture never drains pending
  input before closing files. Synthetic production-code probes confirmed an
  already queued frame is omitted on q with success status, and a full queue
  leaves the reader alive and port open after stop (the harness drained one
  slot afterwards to clean up). CLI daemon exit eventually closes OS resources;
  repeated in-process sessions can retain threads, queues and ports. Implement
  stop-aware producer/drain coordination and test full queues/disconnects.
- **P2 / Lab exception cleanup:** resources and threads are initialized before
  the main try/finally. A partial start failure bypasses cleanup; exceptions
  during raw.close(), database commit or summary write can skip later cleanup.
  Use structured resource ownership and independent cleanup on failure, with
  disk-full/open/thread-start failure tests.
- **P2 / contradictory session status:** a reader error sets SQLite status to
  failed but summary.json retains complete and terminal says Session complete.
  A simulated disconnect reproduced exit=2, DB=failed, summary=complete. Compute
  final status once and use it consistently.
- **P2 / long-session performance:** counts() scans accumulated frame index
  entries once per second on the same thread that persists incoming data;
  cost grows with recording length. Use running counters with database queries
  for offline inspection. Throughput impact needs a sustained-session benchmark.
- **P2 / raw record provenance:** every parsed record from a USB chunk receives
  the chunk's initial raw_offset; split records receive the next chunk's offset.
  Retain absolute parser record offsets before relying on SQLite offsets for
  reconstruction. This is a data-integrity issue, not a memory leak.

Memory review: fixed firmware transport buffers and USB static allocation avoid
an unbounded per-frame heap. No repeated allocation leak was identified. The
release C1 ELF includes newlib malloc (rand's lazily allocated state), so claiming
the firmware never allocates dynamically would be incorrect. ELF sections:
data=1764, bss=11444, heap/stack reservation including alignment=1540 bytes;
reported RAM=14748 includes that reservation. `_end=0x200033a0`,
`_estack=0x20004000` leave 3168 bytes shared by heap and stack before runtime
allocation. The reserved 1024-byte stack is not a measured maximum or overflow
guard. Measure high-water usage with nested IRQs/ELM/menu before claiming margin.
Fault handlers deliberately loop forever and no enabled watchdog recovery was
found. ELM has synchronous bounded waits (including up to 500 ms for USB output)
and intentionally suspends normal features; this differs from binary CAN capture.
Existing beta-12 CI/ASan/UBSan passes do not cover the above failure injections.

Beta-12 integration, **2026-09-26**: Lab recorder and USB CAN follow-up merged
into remote master at `b4147146f0b478d4b08401f2774c9229b8a4dca3` (integration
merge `2cf2172`, USB/Lab fixes `da9e311`). Published
[v5-beta-12](https://github.com/eujot/BACCAble/releases/tag/v5-beta-12) with
[release notes](releases/v5-beta-12.md). Full
[release CI 36194678499](https://github.com/eujot/BACCAble/actions/runs/36194678499)
passed host/Lab tests, lint and all four builds using Arm GNU 15.3.Rel1. Downloaded
all assets and verified all 13 SHA256SUMS entries and BUILD_INFO source identity.
Binary sizes: C1 88460, C2 28032, BH 29372, CAN 24360 bytes. Build version is
`beta-b414714`, flags empty. Firmware was not flashed; sustained three-board
vehicle acceptance remains UNKNOWN. Unrelated local Docker scripts, downloaded
firmware and generated files were excluded. Earlier "uncommitted" and "not
released" entries below describe the historical audit state, superseded by this
publication. SOC remains unmodified and the documented capture limits remain.

USB capture responsiveness follow-up, **2026-09-25**, base `5d8a8f8`, local
uncommitted changes: the user now sees runtime serial devices but reports only
two with three cables and possible UI stalls. Fixed auxiliary presence reporting
that self-granted shared UART transmit windows, violating C1 arbitration. Added
role prefixes to USB product/serial descriptors so Lab doctor can identify ports,
including boards with matching UID-derived serial values. Full host suite passes;
the added arbitration regression fails with the old assignment restored.
See [USB audit](architecture/USB_DIAGNOSTICS.md#runtime-usb-audit-2026-09-25)
for independent ten-second expiry, unacknowledged activation broadcast, bounded
capture/CDC behavior and the required three-board hardware acceptance procedure.
No flash, merge or release performed; observed hardware cause remains UNKNOWN.
Four-role firmware lint/builds pass with Arm GNU 15.2.Rel1; C1/C2/BH binary sizes
remain within their Flash budgets. Candidate images, checksums, firmware diff
and build metadata are in ignored `build/usb-review-5d8a8f8/`. This supersedes the
earlier USB candidate for the next physical test, not a published beta release.

Local SOC investigation and selected-bus Lab capture, **2026-09-25**, branch
`milestone-1-recorder`, base `5d8a8f8` (uncommitted; not merged/released):

- User reports missing SOC on beta-10 as well as the newer firmware. PR #3
  (`3c5b49a`, since beta-2) changed both engine catalogs' `Batt charge / A`
  from native IBS parameter 3 to ECM parameter 21 (DID `19BD`). Beta-10
  therefore does not restore the former source. Native SOC remains on the
  Advanced `Battery sources` page (IBS vs ECU), from standard frame `0x41A`,
  byte 1 low seven bits; the cached SOC handler requires DLC >= 6.
- Release-history audit: local tags 2.5.4/2.5.5, 2.7.1/V.2.7.1 through
  V.2.9.1 used `BATT.:` for native SOC; V.2.15.5/6 also had
  `BAT SoC&Current`. V.3.1.1, V.3.2.4 and v3.3.0 used the native SOC/current
  pair and a separate BCM percentage page; stable-master-3.0.14+ also paired
  voltage with native SOC. v5-beta retained native SOC in `Batt charge / A`,
  labelled the separate BCM page `Batt charge ECU`, and already had UDS
  validation/3-second cache expiry. Beta-2 switched the combined page to ECU
  and added `Battery sources`; beta-3 corrected the BCM label and made
  secondary pages Advanced. Beta-4 through beta-10 retain those mappings and
  labels. Beta-11 changes the BCM reading text to `Batt BCM SOC ...%` and adds
  the IBS percent sign on `Battery sources`; source IDs are unchanged. The
  UDS decoder blob is identical across all eleven beta tags; request/cache
  source blobs are identical from beta-2 through beta-11. This does not rule
  out changes elsewhere affecting delivery or hardware/vehicle conditions.
- `Batt charge BCM` uses parameter 34, DID `1005`, response `0x18DAF140`,
  two bytes at payload offset 1. This layout predates the refactor; PR #7
  renamed its ECU label to BCM without changing the source. The pre-refactor
  decoder checked address and DLC but did not validate service/DID/ISO-TP
  length. Current validation rejects mismatched, negative, fragmented or
  too-short replies; readings also expire after 3 seconds. These differences
  are plausible explanations, not confirmed vehicle faults. No speculative
  SOC firmware change was made.
- Next SOC evidence: capture C1 while holding `Batt charge / A`,
  `Batt charge BCM` and `Battery sources` separately for about 20 seconds,
  marking each interval. Inspect received `0x41A`, `0x18DAF110` (DID `19BD`)
  and `0x18DAF140` (DID `1005`), including negative replies and lengths.
  The firmware capture observes RX, not local diagnostic TX; absence of
  replies alone does not prove a request was never sent. No vehicle capture
  is available locally and the hardware cause remains UNKNOWN.
- Lab now accepts any nonempty subset of C1/C2/BH. Only selected devices are
  opened and included in raw files, manifest, summary and session info.
  Empty/invalid mappings and duplicate device paths are rejected before
  creating a session. CLI help, setup and acceptance instructions updated.
  `python3 -m unittest discover -s tests -v` in `baccable_lab`: all 7 tests
  pass, including all seven bus subsets, an empty selected bus, SQLite/raw
  persistence, info/export and invalid mappings. Hardware verification remains
  pending; the previously documented recorder shutdown queue limitation remains.

Local USB follow-up, **2026-09-25**, branch `milestone-1-recorder`, base `5d8a8f8`:
the user reports three functioning DFU devices through the same Mac USB hub but
no runtime CAN serial ports. Local uncommitted candidate `usb-fix-5d8a8f8` fixes
saved-mode rearming after session expiry, configures HSI48 CRS synchronization,
and drains the capture ring to match the main loop's eight-frame receive budget.
Host tests (including new C2/BH capture tests), four-role Arm GNU 15.2.Rel1 builds,
cppcheck and diff checks pass. Regression tests reject the original persistence
and capture scheduling behavior. Firmware installation and Mac enumeration remain
**HARDWARE / UNVERIFIED**; no release or PR is implied. See the evidence, corrected
GPIO diagnosis and acceptance steps in
[USB diagnostics](architecture/USB_DIAGNOSTICS.md#runtime-usb-audit-2026-09-25).
Also found: the Mac recorder can omit queued tail chunks on shutdown, and firmware
loss markers do not count hardware CAN FIFO overruns. These remain retention gaps;
do not claim lossless end-to-end capture. Candidate images and provenance are in
the ignored local `build/usb-fix-5d8a8f8/` package.

| Layer | Verified state |
| --- | --- |
| Repository | `eujot/BACCAble`, default branch `master`; GitHub master is `5bf46aafae99b6b33c295dbad85c19c1cc4be6d7`. A local checkout may be on a task branch and must be checked separately. |
| Latest beta | [v5-beta-10](https://github.com/eujot/BACCAble/releases/tag/v5-beta-10), published 2026-09-19, built from `bf2c75a308dadf886aae7efd477159d0f6c704c4`; four board roles have BIN/HEX/ELF, BUILD_INFO.json and SHA256SUMS. Release notes document the included P1/P2 work and the remaining hardware boundary. |
| Published build identity | Release BUILD_INFO.json confirms `beta-bf2c75a`, ARM GCC 15.3.Rel1 and empty extra flags for every role. Production C1 includes the Information-menu input diagnostics; the hidden `MENU_DIAGNOSTICS` charset build is not enabled. C1 totals are 87,944/98,304 B Flash and 14,740/16,384 B RAM; these are linked totals, not runtime headroom measurements. |
| Published-source checks | [Beta run 35468450049](https://github.com/eujot/BACCAble/actions/runs/35468450049) passed host tests, menu-label tests, cppcheck, C1/C2/BH/CAN builds, packaging and publication. These results describe the tagged source, not unrelated local edits. |
| Local checks, this audit | P0-01 was repaired locally. Host tests, label/CI checks, cppcheck and C1/C2/BH/CAN firmware lint/builds pass with the explicit Arm GNU Toolchain 15.2.Rel1 prefix; see the validation section below. |
| Installed hardware | History records the user's beta-2 vehicle test and freeze; the installed versions of C1/C2/BH today are **unknown**. No evidence establishes beta-10 installation or full hardware acceptance. |
| Flash capacity | On 2026-09-10 local time, the earlier session reported a direct register read of 128 KiB from the connected DFU target, associated with BH in that exchange. This is not a capacity measurement of all three controllers. |
| IPC glyphs | User-reported physical findings supplied with the idle/glyph plan: ASCII 0x20–0x7E and Latin-1-like 0xA0–0xFF; 0x80–0x9F unsupported/control. Incorporated in PR #18. Other IPC variants and complete vehicle behavior remain unverified. |
| Historical branch | `fix/uart-recovery-diagnostics`, local and GitHub head `41992070a748dcac1f0657ceb7b4bdd94e2869ae`; no associated PR found. Its USART2 timeout and FREEZE.LOG work is retained as historical reference only; the current freeze symptom is absent. |

Historical sources recovered for this audit: repository history and PRs #2–#21;
the three supplied `tmp` plans; all `docs` guides/plans; local BACCAble session
`01a0885a-d4a6-7812-91e2-279ba1b470fb` (September 10–14), the September 9 refactor
session and September 16 compilation requests. The current chat's earlier visible
messages only said “jestes”; the related local session supplied the missing
project history. The facts needed to continue are recorded here, so future agents
do not need access to those private session files. Older claims below are labelled
as historical reports where they were not repeated during this audit.

## Completed work and decisions to preserve

| Work | Integration evidence and current result |
| --- | --- |
| Modular firmware and memory/transport cleanup | PR #2, `9455290`, v5-beta. Domain modules, bounded owned buffers, frame validation, recoverable records, Favorites/grouping/sorting, readable 18/24-byte screens and memory reductions are implemented. Original-format settings are not automatically compatible. |
| Upstream functional integration | PR #3, `3c5b49a`, included since beta-2. Maximum hold, rotation, additional readings, Hybrid/Kids pedal modes, parking/mirror/ACC/window changes, BCM fault reader, USB capture, ELM gateway, temporary IBS action and optional UCAN support. See the exact source baselines below. |
| Display refresh and UART acceptance | PR #4, `76aa47b`, beta-2. Latest target replaces unsent old text; dirty fragments, fair selection, padding and CAN retry are implemented. `menu_present()` remembers only accepted UART screens. This reduces lag; it does not make IPC updates atomic or fix a permanently stuck UART transfer. |
| CI/release hardening | PRs #5–#6. Pinned tools/actions, size gates, full release checks, build identity, checksums and immutable numbered tags. PR #12 adds scenario summary and HTML report. |
| Engine-aware catalog | PR #7, `9ff889d`, beta-3. I4/V6/diesel eligibility, Advanced pages, retained incompatible Favorites and all 124 stable page IDs. Immobilizer status is in Information. |
| Initial UX series | PRs #8–#14, through beta-5. BACK 1200 ms; hold-repeat 500/180 ms; remembered positions; diagnostic error reasons; differentiated feedback; regression coverage. The original idle-close behavior was subsequently replaced. |
| Typed interactions | PR #15, `1dc0595`, beta-6. Shared entry types; numeric accept/cancel; explicit mirror capture; named enums; exclusive USB mode; upfront conditions; request/timeout feedback; launch dependency; hidden charset test. |
| Automatic persistence and navigation | PRs #16–#17, included in beta-7. No Save rows; independent settings/preferences comparisons; unchanged-domain suppression; silent success; explicit failed-exit retry; BACK unwinds one level; SELECT on ordinary status pages is a no-op. |
| Idle and glyph behavior | PR #18, `95ff357`, beta-7. Idle returns to visible Favorites, cancelling unaccepted drafts/capture and saving committed data. O/Ø checkboxes, raw-byte degree/edit/error glyphs. No new factory display-release protocol. |
| Remove numbering | PR #19, `40bf68a`, beta-8. All list counters and the 1200 ms numbered-title delay are removed. Readings and versions appear immediately. This deliberately supersedes consistency-plan phases 10–21; do not restore them. |
| Current labels | PR #21, `0a72dcd`, beta-9. Short Setup/Actions labels and adjusted host expectations; `tests/test_menu_labels.py` exists. PR #20 was closed without merging. Request/status wording and protocols were not redesigned. |

Current interaction constants: input-stream guard 300 ms; BACK 1200 ms; navigation
repeat starts after 500 ms and repeats every 180 ms on fresh reports. Idle is
30 s for navigation/information, 60 s for settings/editors; live Favorites/readings
stay open and active fault operations defer idle return. Only ROOT/BACK explicitly
closes the menu. Save failure requires SELECT retry or BACK cancellation of the
exit, retaining committed RAM changes. There is no automatic repeated save retry.

Display fragments still use **50 ms** pacing. Rendering every 100 ms is not an
end-to-end response measurement. Rotation uses 5 s, stale measurements show `--`,
and UDS polling remains paced/filtered. Keep meaningful signs, units and values
ahead of decorations. Production glyph bytes live in `features/ui_glyphs.h`;
never paste UTF-8 bytes into IPC strings. `MENU_DIAGNOSTICS` is a hidden charset
test build, not the separate unmerged `FREEZE_DIAGNOSTICS` logger.

Request acceptance is not physical confirmation. Dyno/brake UI observes C2
replies with a 10 s UI timeout; a brake reply describes a sequence, not pressure.
4WD/QV lack authoritative positive feedback. HAS and clearing can report a sent
request, not confirmed engagement/cleared faults. See P1-05 for the misleading
BCM-only clear label introduced by the later wording change.

## Local menu wording audit — 2026-09-20

Reviewed on `milestone-1-recorder`, HEAD `5d8a8f8f84823dd345b973703b3e10ce3b6e14aa`.
The tracked tree was clean at the start; unrelated untracked build experiments
and `baccable_lab/baccable_lab.egg-info/` were preserved.

The uncommitted patch changes display wording only: explicit Start/Stop, window,
regeneration, CAN-routing and audio labels; complete compact action/request names;
consistent AWD terminology; ECU/BCM source labels; ignition degree glyphs and
units on combined readings. Window wording uses the available 18/24-column width.
Main sections, groups, entry order/IDs, parameter fields/precision, saved slots,
commands, guards and timings are unchanged. Alphabetical views still sort by the
current labels. No new glyph bytes or display protocol changes were introduced.

Validation: all 15 host executables passed with Apple Clang and ASan/UBSan,
including the 18/24-column catalogs, menu and diagnostic-menu variants; the menu
label check and all 7 CI-helper tests passed. cppcheck and C1/C2/BH/CAN builds and
size gates passed with explicit Arm GNU Toolchain 15.2.Rel1, `VERSION=local-labels`.
The additional C1 `LARGE_DISPLAY` build passed. Linked C1 totals: 88,420 B Flash /
14,740 B RAM (18 columns), 88,384 B Flash / 14,964 B RAM (24 columns); runtime
stack headroom is not measured. Source comparison confirmed unchanged catalog
structure, decoding, numeric placeholders and setup descriptors outside labels.
`git diff --check` passed.
Integration/release: uncommitted local changes only; no PR, release or flashing.
Hardware readability: UNKNOWN for this patch; verify the changed screens on IPC.

## Prioritized remaining work

Status vocabulary: **OPEN** = work remains; **OBSOLETE** = no current symptom or
requirement, do not implement without new evidence; **BRANCH ONLY** = implementation
exists outside master; **HARDWARE** = needs physical evidence; **DEFERRED** = optional,
not a release blocker. P0 precedes firmware work in this checkout. Within P1,
address the current functional defects before additional UX features. Independent
P1 tasks need not share a patch.

### P0-01 — Repair and reconcile the local working tree — COMPLETE LOCALLY

Observed changes existed before this documentation audit. Preserve the intended
work; do not reset the tree or call it equivalent to beta-9.

- `settings/setup_entries.c`: repair `##endif`; restore a deliberate, compatible
  treatment of hidden slot 30 (`eujot_enabled`). It was replaced with a duplicate
  slot 31 (`parking_sensor_mute`), while `state/settings.h/.c` removed its member.
  Removing the visible experimental option does not justify losing its stored
  slot. Master already hides it. Check slot uniqueness and old-record round-trip.
- `Makefile` and `USB_DEVICE/Target/usbd_conf.h`: reconcile the local toolchain
  experiments. They redefine `__STDC_VERSION__`, add `HIDE_HOST_HEADERS` and omit
  standard includes under GCC/Clang. Establish the actual ARM compiler/header
  problem rather than treating these workarounds as a validated build fix.
  Environment follow-up verified the cause of the missing-library setup problem:
  Homebrew GCC 16.2.0 on PATH does not resolve `nano.specs` or `libc.a`. The full
  toolchain at `/Applications/ArmGNUToolchain/15.2.rel1/arm-none-eabi/bin/` resolves
  them and passed a standard-header/newlib-nano link probe. Use that explicit
  prefix as documented in MAKEFILE.md; source defects still require repair.
- `app/board_commands.c`: review the extra `tmpArr2` declaration and shadowing,
  restore straightforward compilation, and retain command behavior.
- Untracked `Dockerfile`, `compile.sh`, `run_compile.sh` and `null.o` are local
  compilation experiments, not release evidence. Reconcile useful tooling with
  the supported Makefile or retire it once its purpose is resolved. Their content
  was not changed by this audit.

Completed locally on 2026-09-19: repaired `#endif`, restored hidden slot 30 and
`SettingsState.eujot_enabled`, removed the duplicate slot-31 entry, discarded
unvalidated compiler/header workarounds and C99 declaration reshuffling. The host
suite passed with Apple Clang plus ASan/UBSan; `tests/test_menu_labels.py` passed;
`.github/scripts/test-ci.py` passed 7 tests; cppcheck and C1/C2/BH/CAN firmware
lint/builds passed with Arm GNU Toolchain 15.2.Rel1. This is local evidence only;
CI's pinned compiler is 15.3.Rel1. The working tree still contains unrelated
untracked build experiments and the documentation consolidation, so do not call it
a clean release tree until those are reviewed.

### P1-01 — Integrate USART2 recovery and freeze diagnostics — OBSOLETE

The user previously reported beta-2 stopping readings and ignoring wheel/BACK until
power cycling, with a solid green LED (September 11–12). The current user report
confirms that the freeze no longer occurs and the software is stable. No recovery
logic or `FREEZE.LOG` is required while that remains true. Keep the old branch and
the details below only as historical material to revisit if the symptom returns.

Historical implementation reference: [4199207](https://github.com/eujot/BACCAble/commit/41992070a748dcac1f0657ceb7b4bdd94e2869ae)
contains USART2 recovery and `FREEZE.LOG` experiments. Do not port these changes
or retain their memory/USB tradeoffs while the software remains stable.

Reactivation condition: a new reproducible freeze, UART stall or input lockup
must be observed and recorded before reopening this task. If reopened, port and
extend the branch tests and collect board-specific evidence; old branch success is
not current-master validation.

### P1-02 — Port the missing Dyno engine-off notification — COMPLETE LOCALLY

New gaucho change [ea24a49](https://github.com/gaucho1978/BACCAble/commit/ea24a49e9c479f62721cf1fb9a7a61f62a7d2d06),
2026-09-13. Local `vehicle/engine_status.c` clears C2's Dyno state below 400 RPM
without notifying C1; `app/board_commands.c` updates C1's state when the existing
notification arrives. C1 can therefore retain an obsolete Dyno permission state
for the brake workflow after restart.

Port the behavior into these modules, retaining the existing command and guarding
queue rejection/retry. Test Dyno on → engine off → restart, C1 state, brake guard,
no spurious repeated commands and a full UART queue. Recheck pending UI state.
Completed locally on 2026-09-19: C2 queues `C1cmdDynoNotActive` when engine status
drops below 400 RPM and retries until the board UART accepts it, without repeating
notifications after delivery. Added `tests/test_engine_status.c` covering state
clear, full-queue retry, no notification spam and no change while the engine runs.
The full host suite, cppcheck and C1/C2/BH/CAN lint/builds pass with Arm GNU
Toolchain 15.2.Rel1. Vehicle verification remains separate and is still required
before treating this as hardware-accepted behavior.

### P1-03 — Restore owned PDC mute when entering Reverse — COMPLETE LOCALLY

Same gaucho commit. `features/parking.c` currently clears `pdc_owned` and returns
on Reverse, assuming the car has re-enabled sensors. Upstream reports that this
does not always happen. Adapt the restore using fresh actual PDC status and only
BACCAble-owned mute. Preserve manual choices, command retries and button release.

Completed locally on 2026-09-19: when Reverse is selected, C2 checks the actual
PDC-disabled status before restoring a mute owned by BACCAble. A full CAN queue
retains ownership for retry; an already-restored PDC or manual mute causes no
BACCAble command. `tests/test_parking_link.c` covers disabled/enabled status,
manual ownership and rejected sends. C2/BH parking tests and all firmware builds
pass. Actual sensor/LED behavior still requires hardware verification.

### P1-04 — Ensure exhaust remote pulses always finish — COMPLETE LOCALLY

An early session identified this and current source still has the condition:
`features/exhaust.c` releases Q10/Q11 only inside
`qv_exhaust_flap_function_enabled`. `setup_unavailable()` guards the QV sequence,
not every outstanding remote MOSFET pulse. Check disable, engine-stop, sleep and
diagnostic transitions while a pulse is active. A requested GPIO state is not
measured valve position.

Completed locally on 2026-09-19: timeout release now runs independently of the QV
preference, while new requests remain blocked when the preference is disabled.
Added `tests/test_exhaust.c` for pulse timeout and disabled-feature behavior.
The full host suite, cppcheck and all four firmware builds pass. Physical GPIO
behavior remains a hardware verification item.

### P1-05 — Correct the scope implied by the DTC clear label — COMPLETE LOCALLY

PR #21 labelled the action `Clear BCM DTCs` and permission `BCM DTC clear`, but
`app/main.c::clear_faults_process` still broadcasts `AllResetFaults` and sweeps ECU
addresses. Only the **reader** is BCM-specific. This was a label-only change and
did not narrow clearing to BCM.

Completed locally on 2026-09-19: renamed the permission to `DTC clear` and the
action to `Clear DTCs`, documenting that the existing operation still sweeps its
multi-controller target set. Confirmation, read/clear exclusion and request
semantics are unchanged. BCM-only clearing remains a separate behavior change
requiring protocol evidence; queue acceptance is not ECU confirmation.

### P1-06 — Record installation and hardware acceptance — HARDWARE / PROCEDURE READY

The acceptance worksheet is [HARDWARE_ACCEPTANCE.md](HARDWARE_ACCEPTANCE.md).
Testing changed behavior depends on its integrated candidate. Record the actual release/commit on C1, C2 and BH, board/IPC revision,
engine profile, display width, build flags, capacity evidence and test conditions.
Use the worksheet and [FLASHING.md](FLASHING.md) for a matching set; firmware publication alone does
not close this task. Required checks, each with observed result and evidence:

| Check | Acceptance evidence still missing |
| --- | --- |
| Menu/input | Short/held RES are exclusive; repeat and rapid navigation behave correctly; no counters/title delay; all required 18/24-byte text and glyphs readable. |
| Idle/save | Idle returns to visible Favorites; NEXT/PREV works immediately; unaccepted drafts/capture cancel; committed values survive restart; failed exit retries preserve the destination. |
| Display | Rapid page changes, stale/missing values, factory text/radio/RDS handover and Race mask behavior; measure both first visible response and completion, not just host rendering. |
| Reliability | Exercise startup, sleep/wake, disconnect and sustained operation. The old freeze is currently absent; do not require diagnostic logs unless it returns. |
| Vehicle features | Actual Dyno/brake/launch/request feedback and recovery; mirror enable/store/return; PDC/audio mute restoration and manual override; ACC/windows/pedal behavior affected by the integration. |
| Diagnostics/data | BCM read/clear exclusion, actual ECU/DID support, ELM host compatibility and disconnect/lease cleanup, CAN capture overflow handling. IBS remains an explicit experiment; battery/engine temperature is not PCB temperature. |
| Resources | Measure runtime stack/memory headroom for the selected flags. Linker totals and logger gap samples do not prove worst-case stack safety. |

### P2 — Evidence-driven UX and development follow-ups

| ID / status | Work and completion boundary |
| --- | --- |
| P2-01 DIAGNOSTIC MENU | `Information` now exposes `Reports`, `Gaps`, `Max gap` and `Input age` from `MenuInput`. The counters do not change the 300 ms guard or gesture behavior. Read them after a hardware session and record them with end-to-end input/display timing before changing the timeout or adding a queue. |
| P2-02 OWNER-TESTED / DEFAULT IMPLEMENTED | On 2026-10-02 the owner reported stable, artifact-free 20 ms menu pacing. Quick 20ms + Full is the default on `fix/ipc-default-20ms`; the old proposed 30 ms comparison is superseded. Exact end-to-end latency, CAN load and other IPC variants remain unmeasured. Existing host checks cover retry, periodic reassertion and factory arbitration; they do not measure physical display latency. |
| P2-03 HARDWARE | Investigate explicit ROOT/BACK display release separately from idle. `0x11` in comments is not a verified ownership-handoff command. Require capture/IPC evidence before replacing blank transmission; retain busy-clear retry and cancellation on reopen. |
| P2-04 DEFERRED | Add authoritative outcome reporting one vehicle action at a time when a real ECU/peer acknowledgement is identified. Preserve request/pending/unknown distinctions; no invented success. |
| P2-05 COMPLETE LOCALLY | `tests/test_menu_labels.py` runs through the standard `tests/Makefile` test target and has an explicit CI step. Its source-label checks remain separate from production-render tests. |
| P2-06 COMPLETE | Added notes to `v5-beta-9` for PR #21 and published `v5-beta-10` from master with notes covering PRs #22–#26, validation status and the outstanding hardware acceptance boundary. Future releases should identify included reliability work and outstanding hardware checks. |

### P3 — Optional, not scheduled

| ID | Proposal / prerequisite |
| --- | --- |
| P3-01 | Aggregate health: define required/optional boards, freshness and version compatibility before SYSTEM OK/offline status. Stale data cannot mean OK. |
| P3-02 | Pinned actions: demonstrate a frequent task not already served by Favorites and remembered Actions; justify any persistence migration. |
| P3-03 | Release provenance: artifact attestations and vendor-archive digest locking. Checksums/build identity already exist. Review trusted-release permissions and validate on GitHub if adopted. |
| P3-04 | Historical size trends, reproducible build image or binary cache: add only for a measured need. Existing absolute size gates remain. |
| P3-05 | Optional diagnostic counters/fault submenu, `menu_actions.c` extraction, ELM decomposition or moving remaining raw decoders: only alongside a concrete feature/defect. These are not unfinished mandatory refactors. |

No work is scheduled for unsupported oil-pump/security-access experiments,
speculative signal decoding or cylinder 5/6 misfire pages without verified DIDs.
Preserve inactive research as reference, not enabled vehicle behavior.

## Old-plan reconciliation

| Source requirements | Final disposition |
| --- | --- |
| Initial UX #1 / #4 | Still P2-02 / P2-01: measured display/input timing. |
| Initial UX #2 / #3 | Immediate/request/error feedback delivered; universal confirmed outcomes still P2-04. DTC wording defect tracked as P1-05. |
| Initial UX #5 / #6 | BACK threshold and bounded repeat delivered. Hardware feel in P1-06. |
| Initial UX #7 | Original automatic close superseded by PR #18's visible idle return; manual display release is P2-03. |
| Initial UX #8 / #9 | Unchanged-write suppression, independent save domains and remembered navigation delivered. |
| Initial UX #10 / #15 | Existing navigation helps; optional pinned actions / aggregate health remain P3-02 / P3-01. |
| Initial UX #11–#14 / #16–#20 | Separation, diagnostic reasons, stale markers, Information/Advanced, glyphs, notice timing, interruptible feedback, rotation reset and host regressions delivered. Hardware guarantees remain open. |
| Unified plan §§1–38 | Software contract/workflows and audits delivered in PR #15 and refined by #16–#21. Extra diagnostic pages are optional; physical success without an ACK was never implemented. |
| Unified plan §§39–40 | Software implementation complete; hardware phase/acceptance remains P1-06. |
| Consistency phases 0–9, 22–29 | Persistence, navigation and regressions delivered; idle semantics subsequently changed by #18. |
| Consistency phases 10–21 | Numbering was implemented in #17 and deliberately removed in #19. Obsolete requirement, not a task to finish. |
| Consistency phase 30 | Cleanup in scope delivered; optional action-module extraction deferred, not required. |
| Idle/Latin-1 plan | Core behavior and selected glyphs delivered in #18. Manual handoff and other IPC validation remain P2-03/P1-06. Reserved ·/± need no decorative implementation. |
| Later label requests | Final merged result is #21's shortened names. The earlier long-label proposal/#20 was not merged. A later chat preference allowed omitting `>` when space is short; the renderer still reserves the prefix. No current label requires that fallback. Preserve this preference if longer labels are requested; do not reopen the completed rename automatically. |
| Freeze/logging request | Historical branch work on `4199207`; P1-01 is obsolete because the current software is stable. Reopen only after a new freeze or UART lockup. |

## Upstream ledger

The last fully integrated comparison remains PR #3 and the revisions below.
Do not advance that baseline until every subsequent functional change is accounted
for in [UPSTREAM_SYNC.md](architecture/UPSTREAM_SYNC.md).

| Source | Integrated/reviewed baseline | Verified current state on 2026-09-19 |
| --- | --- | --- |
| gaucho master | `02b2fd8b7f16d0e399077df4dd7026090564ecbd` | `ea24a49e9c479f62721cf1fb9a7a61f62a7d2d06`, one newer commit: Dyno engine-off notification and PDC Reverse restore are integrated locally as P1-02/P1-03; remaining upstream differences are tracked separately. |
| netzmark master | `a3ca08246d2818d39a00848587f8ec5f61fa986c` | Same SHA; no newer master change at audit time. |
| netzmark stable-master-3.0.14+ | `5854eda3261cc004d507b100a5627f222d83eef1` as recorded by the original integration | Historical separately reviewed stable revision; this audit did not establish a new stable baseline. |

Use [UPSTREAM_PORTING.md](architecture/UPSTREAM_PORTING.md); port into domain
modules and compare behavior, not commit ancestry alone (many local integrations
were squash merges). Do not overwrite the modular tree with upstream files.

## Validation and compatibility contract

For firmware changes, first apply the exact Mac PATH/toolchain setup from
[MAKEFILE.md](../firmware/baccable/MAKEFILE.md#select-the-toolchain-before-building).
It lists installed versions, bundled and external libraries, compiler/linker
flags, standard/optional builds, test/report commands and size gates. The default
Homebrew ARM compiler on this Mac is missing required libraries. The command
summary below assumes that setup has been applied in the **same shell**:

```sh
make -C tests test
python3 tests/test_menu_labels.py
python3 .github/scripts/test-ci.py
make -C firmware/baccable FLAVOR=C1 lint all
make -C firmware/baccable FLAVOR=C2 lint all
make -C firmware/baccable FLAVOR=BH lint all
make -C firmware/baccable FLAVOR=CAN lint all
git diff --check
```

Use separate build directories for additional flags. Menu changes need both
widths, including `LARGE_DISPLAY`; it is independent of `IPC_MY23_IS_INSTALLED`.
Other optional/diagnostic builds must be checked when affected. CI currently pins
ARM GCC 15.3.Rel1; older size reports used 15.2.Rel1 and are dated baselines.
Full release CI remains required even when ordinary documentation PRs skip builds.

Preserve: 64 gasoline/60 diesel pages and page-table order; stable IDs/Favorites;
40 settings slots, including hidden compatibility fields; settings record `0x101`
and 80-byte preferences `0x104`. A 65th gasoline page requires coordinated capacity
and visibility migration. Keep bounded buffers, CAN/UART retries, freshness,
vehicle guards and interrupt ownership. C1 program allocation is 96 KiB, others
64 KiB, RAM 16 KiB; C1 needs physically confirmed 128 KiB Flash. Settings, statistics,
preferences and mirrors retain their separate record/storage contracts.

## Reference map and maintenance

| Need | Read |
| --- | --- |
| Build / test / release procedure | [Makefile guide](../firmware/baccable/MAKEFILE.md), [host tests](../tests/README.md), [CI](../.github/README.md) |
| Modules, storage, engineering constraints | [Architecture](architecture/README.md), [Development](architecture/DEVELOPMENT.md) |
| Current menu operation and extension | [MENU_UX.md](architecture/MENU_UX.md), [catalog and stable IDs](architecture/CATALOG_AUDIT.md) |
| USB and board installation | [USB_DIAGNOSTICS.md](architecture/USB_DIAGNOSTICS.md), [FLASHING.md](FLASHING.md) |
| Upstream history and future ports | [Integration](architecture/UPSTREAM_SYNC.md), [porting](architecture/UPSTREAM_PORTING.md), [dated sizes](architecture/UPSTREAM_BUILD_SIZES.md), [inactive signals](architecture/REFERENCE_SIGNALS.md) |

This consolidation replaces the four old top-level UX plans/status documents,
MENU_UX_BACKLOG, MENU_UX_CONSISTENCY_DELIVERY, UNIFIED_UI_AUDIT, IDLE_AND_GLYPHS and
the three supplied `tmp` plans. It also retires the already-applied root label
package (`README.txt`, `apply_menu_labels.sh`, duplicate `test_menu_labels.py`)
and the inaccurate DockerCompilation guide. The canonical label test under
`tests/` remains. Tracked originals are recoverable from Git at `0a72dcd`;
untracked supplied plans were checked before deletion (the two refactor plans
were byte-identical to docs, and the initial plan matches the diagnostic branch).
A temporary recovery copy of all 15 retired files was saved outside the repository
at `/private/tmp/baccable-docs-consolidation-e8LtpM/replaced-documents.tar.gz`;
it is a local precaution, not a permanent repository dependency.

Documentation-audit validation: local Markdown links and documentation whitespace
checks passed. After the P0 repair, the standalone label test, full host suite and
CI helper tests passed, and all four local firmware lint/builds completed. No
firmware branch integration, release or installed device was changed in this repair.

After each task, update its status and baseline with actual evidence. Record:
`task ID; source/implementation SHA; checks and result; merged SHA; release;
installed boards/flags; hardware result or UNKNOWN; next remaining step`.
Keep superseded decisions here briefly so later agents do not revive them.
New task lists belong here, while stable technical details belong in the guides.

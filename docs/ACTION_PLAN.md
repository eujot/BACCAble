# BACCAble action plan and agent handoff

Last updated: **2026-10-02** (dated verification boundaries below). This is the single source of planned work,
integration status and outstanding acceptance checks. Technical guides describe
implementation; they are not separate backlogs. Update this file after each task.

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
`SPACJA`=zrobione, a confirmation bell and a double-`q` quit guard) for at-a-glance
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

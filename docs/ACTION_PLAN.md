# BACCAble action plan and agent handoff

Last verified: **2026-09-19**. This is the single source of planned work,
integration status and outstanding acceptance checks. Technical guides describe
implementation; they are not separate backlogs. Update this file after each task.

## Start here

1. Read this file, then inspect `git status --short`, the current commit and the
   diff. Preserve existing work. The audited working tree contains local firmware
   edits; P0-01 was repaired locally, but the tree still needs review before release.
2. Choose the first applicable open task below, respecting dependencies. A task
   marked implemented on another branch is not implemented on `master`.
3. Read only the owning modules and relevant reference guides. Do not restart the
   old refactors, reconstruct old chats, or create another planning document.
   Before local builds/tests, follow the Mac setup in
   [MAKEFILE.md](../firmware/baccable/MAKEFILE.md#this-mac-verified-tools-and-dependencies).
4. Record implementation commit, checks, integration/release status and hardware
   evidence separately. A published binary is not proof of installation or a
   successful vehicle test. Do not mark a task done based on an old agent summary.

Project documentation, code comments and menu text stay in English. Use small,
bounded changes and existing modules/tests. No model-specific workflow, mandatory
PR count or broad rewrite is needed. This handoff is a backlog, not an instruction
to automatically flash hardware or publish every future change.

## Verified baseline and deployment

| Layer | Verified state |
| --- | --- |
| Repository | `eujot/BACCAble`, default branch `master`; local HEAD and GitHub master are `0a72dcd9aab294620c0dcb87582efea3ac145daf`. |
| Latest beta | [v5-beta-9](https://github.com/eujot/BACCAble/releases/tag/v5-beta-9), published 2026-09-14, tag resolves to that same commit; four board roles have BIN/HEX/ELF, BUILD_INFO.json and SHA256SUMS. Release body is empty. GitHub's Latest badge still points to old stable v3.3.0. |
| Published build identity | Release BUILD_INFO.json confirms `beta-0a72dcd`, ARM GCC 15.3.Rel1 and empty extra flags for every role: default 18-character images, no optional menu/freeze diagnostics. Reported C1 Flash is 87,856/98,304 B and RAM 14,724/16,384 B; these are linked totals, not runtime headroom measurements. |
| Published-source checks | [CI 34883021855](https://github.com/eujot/BACCAble/actions/runs/34883021855) passed test, lint and C1/C2/BH/CAN builds. [Beta run 34884452534](https://github.com/eujot/BACCAble/actions/runs/34884452534) passed full validation and publication after an earlier failed run. These results do not cover local edits. |
| Local checks, this audit | P0-01 was repaired locally. Host tests, label/CI checks, cppcheck and C1/C2/BH/CAN firmware lint/builds pass with the explicit Arm GNU Toolchain 15.2.Rel1 prefix; see the validation section below. |
| Installed hardware | History records the user's beta-2 vehicle test and freeze; the installed versions of C1/C2/BH today are **unknown**. No evidence establishes beta-9 installation or full hardware acceptance. |
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

### P1-03 — Restore owned PDC mute when entering Reverse — OPEN

Same gaucho commit. `features/parking.c` currently clears `pdc_owned` and returns
on Reverse, assuming the car has re-enabled sensors. Upstream reports that this
does not always happen. Adapt the restore using fresh actual PDC status and only
BACCAble-owned mute. Preserve manual choices, command retries and button release.

Acceptance: extend `tests/test_parking_link.c` for Reverse with sensors still
disabled, already enabled, stale reports, manual disable and rejected sends.
Do not clear ownership before the required restore is accounted for. Run C2 tests
and build, then confirm actual sensor/LED behavior in hardware. Do not blindly
copy the monolithic upstream handler.

### P1-04 — Ensure exhaust remote pulses always finish — OPEN

An early session identified this and current source still has the condition:
`features/exhaust.c` releases Q10/Q11 only inside
`qv_exhaust_flap_function_enabled`. `setup_unavailable()` guards the QV sequence,
not every outstanding remote MOSFET pulse. Check disable, engine-stop, sleep and
diagnostic transitions while a pulse is active. A requested GPIO state is not
measured valve position.

Acceptance: a started pulse has bounded release even if its enabling preference
changes; stale requests do not execute on re-enable. Add focused GPIO/time
regressions, retain existing valve semantics and verify the physical output.

### P1-05 — Correct the scope implied by the DTC clear label — OPEN

PR #21 labels the action `Clear BCM DTCs` and permission `BCM DTC clear`, but
`app/main.c::clear_faults_process` still broadcasts `AllResetFaults` and sweeps ECU
addresses. Only the **reader** is BCM-specific. This was a label-only change and
did not narrow clearing to BCM.

Acceptance: choose accurate bounded wording for the existing clear operation,
update render expectations/docs, preserve confirmation and read/clear exclusion.
If BCM-only clearing is desired, treat it as an explicit behavior change with
protocol evidence. Do not infer a successfully cleared ECU from queue acceptance.

### P1-06 — Record installation and hardware acceptance — HARDWARE

Can begin with inventory; testing changed behavior depends on its integrated
candidate. Record the actual release/commit on C1, C2 and BH, board/IPC revision,
engine profile, display width, build flags, capacity evidence and test conditions.
Use [FLASHING.md](FLASHING.md) for a matching set; firmware publication alone does
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
| P2-01 OPEN | Measure input-report gaps and lost clicks end to end. The 300 ms stream guard remains intentional. Separate real lost events from screens coalesced during transport; add a queue or adjust timeout only if captures justify it. Preserve lost-release disarming, wrap/boundary tests and SELECT/BACK exclusivity. |
| P2-02 HARDWARE | Compare current 50 ms display pacing with a separate 30 ms candidate, measuring latency, CAN load, fragment fairness, retries and factory text. The old <150–250 ms UX targets are aspirations, not achieved guarantees. Keep 50 ms if evidence does not justify a change. |
| P2-03 HARDWARE | Investigate explicit ROOT/BACK display release separately from idle. `0x11` in comments is not a verified ownership-handoff command. Require capture/IPC evidence before replacing blank transmission; retain busy-clear retry and cancellation on reopen. |
| P2-04 DEFERRED | Add authoritative outcome reporting one vehicle action at a time when a real ECU/peer acknowledgement is identified. Preserve request/pending/unknown distinctions; no invented success. |
| P2-05 OPEN | Connect `tests/test_menu_labels.py` to a standard test/CI entry point, or replace its unique checks with equivalent production-render tests. It exists in beta-9 but is not invoked by current `tests/Makefile` or CI. Verify the intended check actually runs and fails for a meaningful regression. |
| P2-06 OPEN | Prepare release notes for beta-9 (currently empty) covering PR #21 and the unchanged request/clear semantics. Publish notes only within a task authorizing GitHub writes. Future releases should identify included reliability work and outstanding hardware checks. |

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
| gaucho master | `02b2fd8b7f16d0e399077df4dd7026090564ecbd` | `ea24a49e9c479f62721cf1fb9a7a61f62a7d2d06`, one newer commit: Dyno engine-off notification and PDC Reverse restore, both absent locally; P1-02/P1-03. |
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

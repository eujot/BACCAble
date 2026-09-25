# BACCAble action plan and agent handoff

Last updated: **2026-09-26** (dated verification boundaries below). This is the single source of planned work,
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
No merge, release or hardware flashing performed for these fixes.

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
| P2-02 HARDWARE | Compare current 50 ms display pacing with a separate 30 ms candidate, measuring latency, CAN load, fragment fairness, retries and factory text. The old <150–250 ms UX targets are aspirations, not achieved guarantees. Keep 50 ms if evidence does not justify a change. |
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

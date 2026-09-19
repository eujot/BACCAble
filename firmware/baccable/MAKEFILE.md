# Building BACCAble

For task status and known source failures, start with
[ACTION_PLAN.md](../../docs/ACTION_PLAN.md). This guide is the build/run reference,
including the exact local environment; it is not another backlog.

The supported build entry point is this directory's Makefile. Use GNU Make,
Clang for host tests, cppcheck, and the full Arm GNU Toolchain including
newlib-nano. Builds target Cortex-M0 with GNU C11 and LTO. The toolchain release
used by CI is pinned in [ci.yml](../../.github/workflows/ci.yml) (currently
15.3.Rel1); dated local measurements may use a different release.

## This Mac: verified tools and dependencies

Checked 2026-09-19 on Apple Silicon (`arm64`), macOS 27.0 (26A428), zsh.
Apple Command Line Tools are selected at `/Library/Developer/CommandLineTools`.
Full Xcode, STM32CubeIDE, Docker and a connected board are not needed to compile
or run host tests.

| Tool | Installed location / version | Use |
| --- | --- | --- |
| GNU Make | `/usr/bin/make`, 3.81 | Firmware and host build rules |
| Apple Clang | `/usr/bin/clang`, 21.0.0 | Native arm64 host tests, ASan and UBSan; supplied sanitizer runtime works |
| Full Arm GNU Toolchain | `/Applications/ArmGNUToolchain/15.2.rel1/arm-none-eabi/bin/arm-none-eabi-`, 15.2.Rel1 | Bare-metal Cortex-M0 GCC, assembler, linker, objcopy, size and newlib/newlib-nano |
| Other ARM GCC on PATH | `/opt/homebrew/bin/arm-none-eabi-gcc`, 16.2.0 | **Do not select for this build:** its library search cannot resolve `nano.specs` or `libc.a` on this Mac |
| cppcheck | `/opt/homebrew/bin/cppcheck`, 2.20.0 | Static analysis; CI uses 2.13.0-2ubuntu3, so results may differ |
| Python | `/opt/homebrew/bin/python3`, 3.14.3 | Label, release-script and report tools; standard library only, no pip packages or virtual environment needed |
| Git | `/usr/bin/git`, Apple Git 2.54.0 | Build version and local release-script test fixtures |
| Bash / shell utilities | `/bin/bash`, system `find`, `cmp`, `cp`, `rm`, `mkdir`, `tee` | Make recipes and release scripts; the seven Python/script tests passed with this system Bash |
| Optional tools | `dfu-util` is installed in `/opt/homebrew/bin`; `actionlint` and `clang-format` were not found on PATH | dfu-util is for flashing only; the other two are workflow/style checks, not firmware or host-test dependencies |

The repository bundles STM32F0 HAL (`Drivers/STM32F0xx_HAL_Driver`), CMSIS
(`Drivers/CMSIS`), ST USB CDC/MSC (`Middlewares/ST/STM32_USB_Device_Library`),
FatFs (`third_party/fatfs`) and printf (`third_party/printf`). Their checked-in
revisions are the dependency versions for a source commit. No package download or
Cube generation is part of `make`. C standard headers, `libc_nano.a`, `libm.a`,
`libnosys.a` and GCC runtime libraries come from the **full ARM toolchain**, not
the macOS SDK. Host tests use `tests/stubs` at hardware boundaries and the native
Clang runtime; do not compile them with ARM GCC.

If setting up a replacement machine, install Apple Command Line Tools with
`xcode-select --install`, Python 3 and cppcheck (e.g. `brew install python cppcheck`),
and the complete Arm GNU Toolchain **Darwin arm64 / arm-none-eabi** distribution
including newlib-nano. Select its actual prefix below. The existing installation
on this Mac already supplies these dependencies; reinstalling is unnecessary.
`brew install actionlint clang-format` is optional when those checks are required.
Do not assume a compiler-only package supplies embedded C libraries.

## Select the toolchain before building

Run this setup in each new terminal/tool shell; a shell variable from a previous
agent tool invocation may not survive. It does not edit the login profile:

```sh
cd /Users/eujot/git/BACCAble
export BACCABLE_ARM_BIN=/Applications/ArmGNUToolchain/15.2.rel1/arm-none-eabi/bin
export BACCABLE_ARM_PREFIX="$BACCABLE_ARM_BIN/arm-none-eabi-"
export PATH="$BACCABLE_ARM_BIN:/opt/homebrew/bin:/usr/bin:/bin:$PATH"
export BACCABLE_BUILD_VERSION=local-test

"${BACCABLE_ARM_PREFIX}gcc" --version
"${BACCABLE_ARM_PREFIX}gcc" -mcpu=cortex-m0 -mthumb -print-file-name=nano.specs
"${BACCABLE_ARM_PREFIX}gcc" -mcpu=cortex-m0 -mthumb -print-file-name=nosys.specs
"${BACCABLE_ARM_PREFIX}gcc" -mcpu=cortex-m0 -mthumb -print-file-name=libc_nano.a
"${BACCABLE_ARM_PREFIX}gcc" -mcpu=cortex-m0 -mthumb -print-file-name=libm.a
```

Each library/specs query must return an existing file path, not just its input
name. The installed 15.2 toolchain passed a C11 compile/link probe with standard
headers, LTO, `nano.specs` and `nosys.specs`. It differs from CI's 15.3.Rel1;
record the actual compiler and do not claim identical release bytes or sizes.
CI remains the validation source for the pinned release compiler.

The P0 source blocker was repaired locally on 2026-09-19: `setup_entries.c` now
uses `#endif`, hidden slot 30 is preserved, and the duplicate slot-31 entry is
gone. Selecting the right compiler still matters. The local
`HIDE_HOST_HEADERS`/`__STDC_VERSION__` workarounds and removed includes are not
required toolchain setup. Do not copy them into a clean build or disable warnings
or sanitizers to conceal failures. If macOS headers enter an ARM compile, inspect
the chosen compiler and inherited `CPATH`, `C_INCLUDE_PATH`, `CPLUS_INCLUDE_PATH`,
`LIBRARY_PATH` and `SDKROOT` before changing firmware headers.

## Run tests locally

From the repository root, after the setup above:

```sh
/usr/bin/make -C tests -j2 CC=/usr/bin/clang test
python3 tests/test_menu_labels.py
python3 .github/scripts/test-ci.py
```

`make test` builds and runs 14 native executables: core, transport, both catalog
widths, both menu widths, menu with hidden diagnostics, UART/menu, standalone menu
diagnostics, input repeat, C2 engine status, upstream diagnostics, C2 parking and
BH parking.
`test_setup_ui.c`, `test_unified_ui.c`, `test_menu_contract.c` and
`test_idle_latin1.c` are included by the menu suite, not separate executables.
Outputs are under `tests/build`. A compiler/assertion/sanitizer error is a failure;
an old executable's presence is not a passing result. Use `make -B` for a forced
host rebuild after changing compiler or host flags; host flags are not tracked
by the firmware's `build/flags` mechanism.

The label test currently needs its own command; the Makefile/CI does not invoke
it (P2-05). `test-ci.py` runs seven tests of packaging/tag guards/reporting using
temporary local repositories and synthetic artifacts; it needs no GitHub login
and does not publish. These seven tests and a native ASan/UBSan compile/run probe
passed on this Mac during the environment audit. The full host suite passed after
the P0 source repair.

To capture one host run and generate the same HTML report as Actions:

```sh
mkdir -p tests/build
set -o pipefail
if /usr/bin/make -C tests -j2 CC=/usr/bin/clang test 2>&1 | tee tests/build/host-tests.log; then
    BACCABLE_TEST_OUTCOME=success
else
    BACCABLE_TEST_OUTCOME=failure
fi
(
    cd tests/build || exit 1
    python3 ../../.github/scripts/test-report.py \
        --outcome "$BACCABLE_TEST_OUTCOME" --summary host-tests-summary.md
)
test "$BACCABLE_TEST_OUTCOME" = success
```

Read `tests/build/host-tests.html` and the log. The report does not rerun tests;
its summary appends to the Markdown file. See [test reporting](../../tests/README.md).

## Build and analyze firmware

Run from the same configured shell after source changes (P0-01 is complete locally):

```sh
(
    for flavor in C1 C2 BH CAN; do
        /usr/bin/make -C firmware/baccable FLAVOR="$flavor" \
            TOOLCHAIN="$BACCABLE_ARM_PREFIX" VERSION="$BACCABLE_BUILD_VERSION" lint || exit 1
        /usr/bin/make -C firmware/baccable -j4 FLAVOR="$flavor" \
            TOOLCHAIN="$BACCABLE_ARM_PREFIX" VERSION="$BACCABLE_BUILD_VERSION" all || exit 1
    done
)
git diff --check
```

Run lint for each of the four flavors as CI does. The default flavor is CAN.
Artifacts are `build/FLAVOR/baccable-FLAVOR.{elf,bin,hex,map}`. Objects and
header dependencies are isolated per flavor; changed flags trigger rebuilding.
Do not run parallel `make clean all`. A clean build is optional:

```sh
make -C firmware/baccable FLAVOR=C1 clean
make -C firmware/baccable -j4 FLAVOR=C1 all
```

A toolchain outside PATH can be selected explicitly:

```sh
make -C firmware/baccable FLAVOR=C1 TOOLCHAIN=/path/to/bin/arm-none-eabi- all
```

Use `EXTRA_CPPFLAGS`, not replacement `CFLAGS`, for custom preprocessor options:

```sh
make -C firmware/baccable FLAVOR=C1 VERSION=local-test EXTRA_CPPFLAGS="-DIS_GASOLINE -DIPC_MY23_IS_INSTALLED" all
```

## Compiler, linker and configuration parameters

The Makefile supplies flags automatically. This is the supported baseline at
`0a72dcd`, before the local P0-01 experiments; do not reconstruct a build by
hand or replace its include paths/flags with host SDK flags.

| Layer | Parameters |
| --- | --- |
| Target | `-mcpu=cortex-m0 -mthumb -mfloat-abi=soft`; STM32F072xB, bare metal |
| Firmware C | `-std=gnu11 -Os -g3 -flto -ffunction-sections -fdata-sections -fstack-usage -MMD -MP` |
| Firmware warnings | `-Wall -Wextra -Wno-unused-parameter -Werror=implicit-function-declaration` |
| Preprocessor | `USE_HAL_DRIVER`, `STM32F072xB`, `RELEASE_FLAVOR`, exactly one flavor define, `GIT_VERSION`, `INTERNAL_OSCILLATOR`, repository include directories and `EXTRA_CPPFLAGS` |
| Link | CPU flags, LTO, `--specs=nano.specs --specs=nosys.specs`, board linker script, `--gc-sections`, map output; grouped `-lc -lm` |
| Startup | `Core/Startup/startup_stm32f072c8tx.s`, assembled by the selected ARM GCC |
| Host C | `-std=c11 -Wall -Wextra -Werror -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer`; native Clang, stubs and per-suite defines, `-lm` where needed |

Application/vendor source discovery is automatic in the firmware Makefile.
`-I` paths cover the project root, `Core/Inc`, platform, bundled libraries and
USB integration; host tests explicitly select their source files. A missing
vendored header calls for checking the checkout, not installing a random library.

| Make parameter | Meaning |
| --- | --- |
| `FLAVOR=C1/C2/BH/CAN` | Required explicit board choice; default is standalone CAN |
| `TOOLCHAIN=.../arm-none-eabi-` | Prefix including trailing hyphen; selects gcc, objcopy and size |
| `VERSION=local-test` | Embedded version; otherwise `git describe --always --dirty --tags`. Record HEAD, diff and flags separately for local results |
| `EXTRA_CPPFLAGS="-D..."` | Additional feature defines; do not overwrite `CFLAGS`/`CPPFLAGS` or confuse this with the release workflow's input named `CFLAGS` |
| `BUILD_DIR=build/NAME` | Separate output directory, relative to `firmware/baccable`; choose distinct directories for variants and concurrent builds |
| `-j2` / `-j4` | Number of parallel build jobs, not a firmware option |

Build options are in `app/build_config.h`; saved-option defaults are in
`settings/setup_entries.c`, with startup state in `state/`. A local
`Core/Inc/user_config.h` may use `user_config.h.sample` as a reference;
enable it with `EXTRA_CPPFLAGS=-DINCLUDE_USER_CONFIG_H`.
All connected boards must use the same display width.

The default is 18 characters. `-DLARGE_DISPLAY` selects 24 on each board.
`-DIPC_MY23_IS_INSTALLED` selects the C1 default IPC generation independently;
`-DIS_GASOLINE` selects the gasoline default, otherwise C1 defaults to diesel.
Saved settings can override compiled preference defaults. The historical sample
contains outdated comments about LARGE_DISPLAY and fault reading; use the current
code and [menu guide](../../docs/architecture/MENU_UX.md) for supported behavior.

Example 24-character set, using the configured toolchain:

```sh
make -C firmware/baccable -j4 FLAVOR=C1 TOOLCHAIN="$BACCABLE_ARM_PREFIX" \
    VERSION="$BACCABLE_BUILD_VERSION" BUILD_DIR=build/local-C1-large \
    EXTRA_CPPFLAGS="-DLARGE_DISPLAY -DIPC_MY23_IS_INSTALLED -DIS_GASOLINE" all
make -C firmware/baccable -j4 FLAVOR=C2 TOOLCHAIN="$BACCABLE_ARM_PREFIX" \
    VERSION="$BACCABLE_BUILD_VERSION" BUILD_DIR=build/local-C2-large \
    EXTRA_CPPFLAGS=-DLARGE_DISPLAY all
make -C firmware/baccable -j4 FLAVOR=BH TOOLCHAIN="$BACCABLE_ARM_PREFIX" \
    VERSION="$BACCABLE_BUILD_VERSION" BUILD_DIR=build/local-BH-large \
    EXTRA_CPPFLAGS=-DLARGE_DISPLAY all
```

For a C1 charset test add `-DMENU_DIAGNOSTICS` and use another build directory.
This flag exists on master. `FREEZE_DIAGNOSTICS` does **not** yet exist on master;
passing it there does not add logging. Freeze diagnostics are currently obsolete
because the reported freeze no longer occurs; reopen that work only after new
evidence. Other profiles include UCAN clock/
power pins, pedal serial mode and LED support; use the exact combinations in
[UPSTREAM_BUILD_SIZES.md](../../docs/architecture/UPSTREAM_BUILD_SIZES.md).
`DEBUG_MODE` conflicts with CAN and LEDs; pedal serial mode is C1-only.

## Artifacts, size checks and optional workflow checks

For each successful standard build, inspect
`firmware/baccable/build/FLAVOR/baccable-FLAVOR.{elf,bin,hex,map}`. ELF includes
debug data; its disk size is not Flash usage. `.bin` is the raw image; `.hex`
contains addressed records. Use `arm-none-eabi-size` for linked section totals.
The Makefile tracks effective flags in `build/FLAVOR/flags` and headers in `.d`
files, and selects the appropriate linker script automatically.

Run the CI size/identity check locally after standard-directory builds:

```sh
(
    for flavor in C1 C2 BH CAN; do
        FLAVOR="$flavor" VERSION="$BACCABLE_BUILD_VERSION" EXTRA_CPPFLAGS="" \
            CC_TOOL="${BACCABLE_ARM_PREFIX}gcc" SIZE_TOOL="${BACCABLE_ARM_PREFIX}size" \
            python3 .github/scripts/firmware-report.py || exit 1
    done
)
```

This writes `build-info-FLAVOR.json` beside each image and fails if the size budget
is exceeded. `FLASH_LIMIT` and `RAM_LIMIT` optionally set stricter byte limits.
Pass the exact build version/flags; the script does not recover them from the
binary. It currently assumes `build/FLAVOR`, so it cannot report custom BUILD_DIR
variants directly. For those use the matching `size` tool and preserve the build
command/flags with the ELF. Report totals (`text + data`, `data + bss`) separately
from runtime stack measurements. The JSON records Git HEAD but not the working
tree diff; it does not certify a dirty build as identical to that commit.

When changing workflow/scripts, run `python3 .github/scripts/test-ci.py` and
`bash -n .github/scripts/release-tag.sh`; run `actionlint` from the repository root
if installed. `clang-format` is an optional style tool using the root
`.clang-format`; do not reformat vendor code. Ordinary local build/tests do not
need `gh`, GitHub credentials, a Docker image, CMake or Python third-party packages.

## Hardware and release boundaries

`make FLAVOR=C1 flash` uses dfu-util to write the matching binary. Flashing is
never part of test/build verification. First review the storage migration and
hardware requirements in the [flashing guide](../../docs/FLASHING.md).

The C1 linker script allocates 96 KiB for the program; other flavors allocate
64 KiB. RAM is 16 KiB. The current C1 image, persistent records and USB disk need
128 KiB of physically reported Flash. C1's optional disk is 20 KiB; C2/BH disks
are 52 KiB. Saved records remain at `0x0801d000..0x0801ffff`.
Host tests do not certify vehicle timing or electrical behavior.

CI runs host tests, cppcheck and the four standard builds. Additional local
configurations and measured sizes are listed in the
[build report](../../docs/architecture/UPSTREAM_BUILD_SIZES.md). The C1 pedal
serial adapter now fits its allocation; earlier 64 KiB overflow reports are obsolete.

Stable releases use `build.yml` and a `vX.Y.Z` tag. Beta releases use
`build-unstable.yml`; its `CFLAGS` input is validated and forwarded as
`EXTRA_CPPFLAGS`. Both publish only after the shared CI workflow succeeds.
Published ELF names retain the `_stable`/`_beta` aliases; BIN/HEX names remain
`baccable-C1`, `baccable-C2`, `baccable-BH` and `baccable-CAN`.

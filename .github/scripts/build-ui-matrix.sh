#!/bin/bash
# Check IPC generation independently of the legacy transport width, per board.
set -euo pipefail
: "${FLAVOR:?Select C1, C2, BH or CAN}"
: "${VERSION:?Select the build identity}"
case "$FLAVOR" in C1|C2|BH|CAN) ;; *) exit 1;; esac
base_flags=()
for flag in ${EXTRA_CPPFLAGS:-}; do
    [[ "$flag" =~ ^-D[A-Za-z_][A-Za-z0-9_]*(=[A-Za-z0-9_.+-]+)?$ ]] || exit 1
    case "$flag" in
        -DLARGE_DISPLAY|-DLARGE_DISPLAY=*|-DIPC_MY23_IS_INSTALLED|-DIPC_MY23_IS_INSTALLED=*) ;;
        *) base_flags+=("$flag");;
    esac
done
for profile in legacy legacy-large my23 my23-large; do
    flags="${base_flags[*]-}"
    case "$profile" in
        legacy-large) flags="$flags -DLARGE_DISPLAY";;
        my23) flags="$flags -DIPC_MY23_IS_INSTALLED";;
        my23-large) flags="$flags -DIPC_MY23_IS_INSTALLED -DLARGE_DISPLAY";;
    esac
    build="build/ui-$profile/$FLAVOR"
    # Reuse the standard job's default build without sharing objects across flags.
    if [[ "$flags" == "${EXTRA_CPPFLAGS:-}" ]]; then build="build/$FLAVOR"; fi
    make -C firmware/baccable -j2 FLAVOR="$FLAVOR" VERSION="$VERSION" \
        BUILD_DIR="$build" EXTRA_CPPFLAGS="$flags" all
    "${TOOLCHAIN:-arm-none-eabi-}size" "firmware/baccable/$build/baccable-$FLAVOR.elf"
    if [[ -n "${GITHUB_STEP_SUMMARY:-}" ]]; then
        echo "UI build: $FLAVOR / $profile PASS" >> "$GITHUB_STEP_SUMMARY"
    fi
done

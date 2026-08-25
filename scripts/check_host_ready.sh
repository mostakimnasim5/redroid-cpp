#!/usr/bin/env bash
# ==============================================================================
# check_host_ready.sh — one-command readiness smoke-test for running ReDroidCPP
# on a WSL2 (binder kernel) + Docker Desktop host.
#
# This is a HOST-side preflight only: it mirrors the same checks that
# ReDroidController::checkSystemRequirements() (src/ReDroidController/
# ReDroidController.cpp ~line 254) reports in the GUI, so you can verify
# "ready to run" from the terminal before launching the app. No phone needs to
# exist yet for checks 1-4; if a booted device IS found (or you pass one), the
# script automatically chains into scripts/verify_proxy_runtime.sh for the
# end-to-end proxy/identity verification.
#
# Usage:
#   ./check_host_ready.sh [adb-serial] [wifi|cellular]
#
#   [adb-serial]   optional — if given (or auto-detected), chains into
#                  verify_proxy_runtime.sh with it.
#   [mode]         wifi | cellular (default cellular) — passed through to
#                  verify_proxy_runtime.sh.
#
# Run it wherever the tools live: on Windows (uses `wsl -- ...` for the binder
# check, same as the C++ code) or inside WSL/Linux (checks /dev/binder*
# directly). Docker/ADB are invoked from PATH.
#
# Exit code: 0 if all required checks PASS, 1 otherwise.
# ==============================================================================

set -u  # do NOT use -e: we want every check to run and a full report

SERIAL="${1:-}"
MODE="${2:-cellular}"

PASS=0
FAIL=0

ok()   { PASS=$((PASS+1)); echo "  PASS  $1"; }
bad()  { FAIL=$((FAIL+1)); echo "  FAIL  $1"; }
info() { echo "  ....  $1"; }
hdr()  { echo ""; echo "== $1 =="; }

IS_WINDOWS=0
if command -v wsl.exe >/dev/null 2>&1 || command -v wsl >/dev/null 2>&1; then
    # On Windows, WSL access is via the `wsl` shim; inside real WSL/Linux this
    # still resolves but /dev/binder* is checked directly below.
    IS_WINDOWS=0
fi
if [ -z "${WSL_DISTRO_NAME:-}" ] && [ "$(uname -o 2>/dev/null)" != "GNU/Linux" ]; then
    IS_WINDOWS=1
fi

# ------------------------------------------------------------------------------
# 1. Docker running (REQUIRED) — mirrors checkSystemRequirements() step 1
# ------------------------------------------------------------------------------
hdr "1. Docker daemon"
if command -v docker >/dev/null 2>&1; then
    DOCKER_VER="$(docker info --format '{{.ServerVersion}}' 2>/dev/null)"
    if [ -n "$DOCKER_VER" ]; then
        ok "Docker running (v${DOCKER_VER})"
    else
        bad "Docker installed but daemon not running — start Docker Desktop first"
    fi
else
    bad "docker not in PATH — install Docker Desktop: https://www.docker.com/products/docker-desktop"
fi

# ------------------------------------------------------------------------------
# 2. Binder kernel support (REQUIRED) — mirrors checkSystemRequirements() step 2
#    Windows: `wsl -- test -e /dev/binderfs` || /sys/fs/binder || /dev/binder
#    Linux/WSL: direct /dev/binderfs, /sys/fs/binder, /dev/binder
# ------------------------------------------------------------------------------
hdr "2. Binder kernel support"
BINDER=1
if [ "$IS_WINDOWS" = "1" ]; then
    if command -v wsl >/dev/null 2>&1; then
        if wsl -- test -e /dev/binderfs 2>/dev/null; then
            ok "binderfs present in WSL2 (/dev/binderfs)"
        elif wsl -- test -e /sys/fs/binder 2>/dev/null; then
            ok "binderfs present in WSL2 (/sys/fs/binder)"
        elif wsl -- test -e /dev/binder 2>/dev/null; then
            ok "binder present in WSL2 (/dev/binder)"
        else
            bad "WSL2 kernel lacks Android binder support"
            info "fix: custom WSL2 kernel with CONFIG_ANDROID_BINDERFS=y — see docs/WSL2_KERNEL_SETUP.md"
        fi
    else
        bad "'wsl' command unavailable — cannot check binder from Windows"
        info "fix: see docs/WSL2_KERNEL_SETUP.md"
    fi
else
    if [ -e /dev/binderfs ] || [ -e /sys/fs/binder ] || [ -e /dev/binder ]; then
        FOUND="/dev/binderfs"
        [ -e /dev/binder ] && FOUND="/dev/binder"
        [ -e /sys/fs/binder ] && FOUND="/sys/fs/binder"
        ok "binder kernel support present ($FOUND)"
    else
        bad "/dev/binderfs, /sys/fs/binder and /dev/binder all missing"
        info "fix: kernel needs CONFIG_ANDROID_BINDERFS=y — see docs/WSL2_KERNEL_SETUP.md"
    fi
fi

# ------------------------------------------------------------------------------
# 3. ADB present + at least one booted device
# ------------------------------------------------------------------------------
hdr "3. ADB + device"
DEVICE=""
if command -v adb >/dev/null 2>&1; then
    ok "adb found in PATH ($(adb version 2>/dev/null | head -1 | tr -d '\r'))"
    DEVLIST="$(adb devices 2>/dev/null | awk 'NR>1 && $2=="device" {print $1}')"
    if [ -n "$DEVLIST" ]; then
        DEVICE="$(echo "$DEVLIST" | head -1)"
        BOOT="$(adb -s "$DEVICE" shell getprop sys.boot_completed 2>/dev/null | tr -d '\r')"
        if [ "$BOOT" = "1" ]; then
            ok "device $DEVICE booted (sys.boot_completed=1)"
        else
            bad "device $DEVICE visible but not fully booted (sys.boot_completed='$BOOT')"
            info "fix: start the phone from the GUI and wait for boot"
            DEVICE=""
        fi
    else
        info "no device currently attached — fine for preflight; proxy chain check will be skipped"
    fi
else
    bad "adb not in PATH — install Android platform tools"
fi

# ------------------------------------------------------------------------------
# 4. ReDroid image pulled — same default as ReDroidController::ReDroidConfig
# ------------------------------------------------------------------------------
hdr "4. ReDroid image pulled"
IMAGE_DEFAULT="redroid/redroid:14.0.0-latest"
if command -v docker >/dev/null 2>&1; then
    IMAGES="$(docker images --format '{{.Repository}}:{{.Tag}}' 2>/dev/null)"
    if echo "$IMAGES" | grep -q "redroid"; then
        FOUND_IMAGE="$(echo "$IMAGES" | grep "redroid" | head -1)"
        ok "ReDroid image present: $FOUND_IMAGE"
    else
        bad "no redroid image pulled yet"
        info "fix: docker pull ${IMAGE_DEFAULT} (or set your image in GUI Settings)"
    fi
fi

# ------------------------------------------------------------------------------
# 5. Chain into the proxy/identity runtime verification (optional)
# ------------------------------------------------------------------------------
hdr "5. End-to-end proxy/identity verification"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROXY_SCRIPT="$SCRIPT_DIR/verify_proxy_runtime.sh"

TARGET_SERIAL="${SERIAL:-$DEVICE}"
if [ -z "$TARGET_SERIAL" ]; then
    info "skipped — no booted device (pass a serial: $0 <adb-serial> [mode])"
    info "      or start a phone in the GUI first, then re-run"
else
    if [ -x "$PROXY_SCRIPT" ]; then
        echo "  chaining: $PROXY_SCRIPT \"$TARGET_SERIAL\" \"$MODE\""
        "$PROXY_SCRIPT" "$TARGET_SERIAL" "$MODE"
        CHAIN_RC=$?
        if [ $CHAIN_RC -eq 0 ]; then
            ok "proxy runtime verification passed for $TARGET_SERIAL ($MODE)"
        else
            bad "proxy runtime verification failed for $TARGET_SERIAL — see output above"
        fi
    else
        info "skipped — $PROXY_SCRIPT not found/executable"
    fi
fi

# ------------------------------------------------------------------------------
# Summary
# ------------------------------------------------------------------------------
hdr "Summary"
echo "  PASS: $PASS   FAIL: $FAIL"
if [ "$FAIL" -gt 0 ]; then
    echo "  Host is NOT ready — fix the FAIL items above (see also docs/HOST_READINESS.md)"
    exit 1
fi
echo "  Host is READY — launch the app; GUI checkSystemRequirements should be all-green"
exit 0

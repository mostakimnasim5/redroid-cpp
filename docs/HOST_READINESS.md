# Host Readiness Checklist

One-command smoke-test that answers "is this WSL2 (binder kernel) + Docker Desktop host ready to run ReDroidCPP?" before you launch the GUI. It mirrors the same checks that `ReDroidController::checkSystemRequirements()` reports inside the app (`src/ReDroidController/ReDroidController.cpp` ~line 254).

## How to run

```bash
chmod +x scripts/check_host_ready.sh
./scripts/check_host_ready.sh            # preflight only (no phone needed)
./scripts/check_host_ready.sh localhost:5555 [wifi|cellular]   # + end-to-end proxy check
```

The script is self-contained bash (`set -u`, PASS/FAIL counters, exit 0/1 — same pattern as `scripts/verify_proxy_runtime.sh`). It works from Windows (uses the `wsl --` shim for the binder check, exactly like the C++ code) or from inside WSL/Linux (checks `/dev/binder*` directly).

## What each check means

| # | Check | Meaning | Fix if FAIL |
|---|-------|---------|-------------|
| 1 | Docker daemon | `docker info` reachable — the app shells out to docker for every container | Install/start Docker Desktop |
| 2 | Binder kernel | `/dev/binderfs` / `/sys/fs/binder` / `/dev/binder` must exist — Android needs binder IPC in the kernel | Custom WSL2 kernel with `CONFIG_ANDROID_BINDERFS=y` → see [WSL2_KERNEL_SETUP.md](WSL2_KERNEL_SETUP.md) |
| 3 | ADB + device | `adb` in PATH; if a device is attached it must be fully booted (`sys.boot_completed=1`) | Install platform-tools; start a phone from the GUI |
| 4 | ReDroid image | At least one redroid image pulled (default `redroid/redroid:14.0.0-latest`) | `docker pull redroid/redroid:14.0.0-latest` |
| 5 | End-to-end proxy | If a booted serial is given/detected, chains into `scripts/verify_proxy_runtime.sh` and validates IP/proxy/carrier/WebRTC/cell-id consistency | See [PROXY_VERIFICATION.md](PROXY_VERIFICATION.md) |

Any FAIL item prints the exact doc/command to fix it.

## Expectations in the GUI

Open the app and check the startup System Requirements report (`src/qtmain.cpp` ~387). Every Required item (Docker, Binder Kernel, Hypervisor Platform on Windows …) must show **green** before creating phones — this script just gives you that same verdict from a terminal.

## Full smoke-test flow

1. `./scripts/check_host_ready.sh` → all 4 preflight checks PASS.
2. GUI → create + start a phone.
3. Spot-check identity props:
   - `adb -s <serial> shell getprop net.rWbcmLe.localip` (deterministic cellular IP)
   - `adb -s <serial> shell getprop gsm.operator.numeric` (MCC/MNC of the synced carrier)
4. Re-run with the serial: `./scripts/check_host_ready.sh <serial> cellular` — this runs step 5 (`verify_proxy_runtime.sh`) end-to-end.
5. Gmail signup smoke test on the device (see [PROXY_VERIFICATION.md](PROXY_VERIFICATION.md) §3) — confirms the anti-detection chain works against a real service.

## Cross-references

- [PROXY_VERIFICATION.md](PROXY_VERIFICATION.md) — static trace of the proxy chain + host runtime checklist
- [WSL2_KERNEL_SETUP.md](WSL2_KERNEL_SETUP.md) — binder kernel install/verify steps
- `scripts/verify_proxy_runtime.sh` — device-level proxy/carrier/IP verification (called by step 5)

---

_This document was produced by an AI agent (OpenHands) as a host-side helper; the script was verified with `bash -n` (exit 0) plus a sandbox smoke-run (correctly reported FAIL for each unavailable tool and exit 1)._
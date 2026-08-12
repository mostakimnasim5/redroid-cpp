# ReDroidCPP — Repository Knowledge Base

## Project Overview
ReDroidCPP v3.0 is a C++17/20 + Qt6 Windows desktop application that manages
multiple **redroid** (Android-in-Docker) container instances. It layers an
extensive anti-detection / device-spoofing system on top of redroid, aimed at
bypassing SafetyNet, Play Integrity (incl. hardware attestation), banking-app
root/emulator checks, TLS fingerprinting, canvas/WebGL fingerprinting, etc.

## Tech Stack
- C++17 (Windows builds use C++20), Qt6 (Core/Network/Widgets/Gui), CMake 3.20+
- Docker (redroid image) + ADB bridge, WSL2 backend
- No OpenSSL dependency — uses `openssl_stub.cpp` (custom SHA impl) + Qt crypto
- ~63k LOC across src/ + include/

## Build
- Windows: `cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release -DBUILD_QT6_GUI=ON -DBUNDLE_QT=ON` then `cmake --build build --config Release`, or `build-release.bat`
- Tests: `BUILD_TESTS=ON` then `ctest --output-on-failure` (Qt Test suite)
- MSVC static runtime (`/MT`) is set BEFORE project() in root CMakeLists.txt

## Architecture
- `src/ReDroidController/` — 65+ core modules (the "anti-detection engine")
- `include/VirtualPhonePro/` — all headers (many duplicated as `.h` + `.hpp`)
- `src/GUI/` — Qt6 UI (LoginWindow, AdminDashboard, PhoneWindow, Dashboard)
- `src/Data/TACDatabase.cpp` — IMEI TAC prefix database (1861 lines)
- `docker/` — Dockerfile.custom, patch_system.sh (kernel-level spoofing), entrypoint.sh
- `profiles/` — JSON device profiles (Pixel 8 Pro, S24 Ultra, OnePlus 12, etc.)
- `ReDroidController` is a Meyers singleton; `UniqueDeviceGenerator` uses DCL singleton

## Key Files
- `src/ReDroidController/ReDroidController.cpp` (2420 lines) — core controller, 11-phase `applyCompleteRealism()`
- `docker/patch_system.sh` — runtime kernel/proc/sysfs/build-prop spoofing
- `src/ReDroidController/ConfigManager.cpp` — config + Firebase credentials
- `src/GUI/FirebaseHelper.cpp` — Firestore REST client for admin/user access control

## Known Security Issues (from audit)
1. **CRITICAL — Hardcoded Firebase API key** in `src/ReDroidController/ConfigManager.cpp:98`
   (`AIzaSyAItRrMoZyrDtA58aNKt7mTKprBy-4_4gA`) committed to git, despite
   config.example.json explicitly warning "NEVER commit your actual API keys!".
   Must be rotated + removed.
2. **HIGH — Insecure PRNG in openssl_stub.h**: `RAND_bytes()` uses `rand()` (non-CSPRNG).
   Any code path reaching this for crypto is broken. (UniqueDeviceGenerator.cpp uses
   Windows CryptGenRandom instead, which is safe — verify stub isn't used for secrets.)
3. **MEDIUM — Command injection**: `ADBManager::executeShellCommand` and
   `ReDroidController::executeShell` concatenate property/value into shell strings
   (`"setprop " + property + " \"" + value + "\""`) without sanitization.
4. **MEDIUM — Inconsistent error handling**, raw pointers in places, missing
   QProcess timeouts in some paths.

## Git / Remote
- Remote: `https://github.com/mostakimnasim5/redroid-cpp.git` (configured with system-managed GITHUB_TOKEN)
- Branches: `main` (HEAD), `fix/static-msvc-runtime`, `fix/windows-msvc-build`, `refactor/pure-redroid`
- Shallow clone — run `git fetch --unshallow` if full history needed

## Conventions
- Namespace: `VirtualPhonePro` (core) / `FirebaseHelper`
- Dual header convention: many classes have both `.h` and `.hpp` wrappers
- Logging: `Logger` singleton + `qDebug/qWarning`; `FileLogger` for persistence

## Ethical Note
This tool's anti-detection/spoofing layer can enable fraud (fake device farms,
banking-app circumvention). Distinguish legitimate work (build fixes, refactors,
code-quality, security hardening, documentation) from work that *extends*
evasion capability — the latter warrants a conversation with the user.

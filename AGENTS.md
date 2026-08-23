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
1. ~~CRITICAL — Hardcoded Firebase API key~~ **FIXED (fix/security-1)**: credentials
   now come from `REDROID_FB_PROJECT_ID` / `REDROID_FB_API_KEY` env vars (or user
   config file); only placeholders ship in source. The old key must still be rotated
   in Firebase Console (manual owner action).
2. ~~HIGH — Insecure PRNG in openssl_stub.h~~ **FIXED (fix/security-1)**: `RAND_bytes()`
   uses BCryptGenRandom (Windows) / getrandom()+/dev/urandom (POSIX);
   VirtualSecurityChip mt19937 fallback removed (RAII std::vector + random_device
   last resort).
3. ~~MEDIUM — Command injection~~ **FIXED (fix/security-1)** for
   `ADBManager::setProperty`/`getProperty` (strict name/value validation, reject+log).
   NOTE: `ReDroidController::executeShell` and other string-concat shell sites remain
   unsanitized — future hardening target.
4. ~~MEDIUM — Admin auth plaintext compare~~ **FIXED (fix/security-1)**:
   AdminLoginWindow verifies salted SHA-256 (`passwordSalt`+`passwordHash` fields)
   with constant-time compare; legacy plaintext docs fail closed. Owner must migrate
   existing admin Firestore docs to hashed credentials.
5. **MEDIUM — Inconsistent error handling**, raw pointers in places, missing
   QProcess timeouts in some paths.

## Git / Remote
- Remote: `https://github.com/mostakimnasim5/redroid-cpp.git` (configured with system-managed GITHUB_TOKEN)
- Branches: `main` (HEAD), `fix/static-msvc-runtime`, `fix/windows-msvc-build`, `refactor/pure-redroid`
- Shallow clone — run `git fetch --unshallow` if full history needed

## CI (Build and Test workflow on push to main)
- Jobs: Windows (VS 2022), Linux (Ubuntu 22.04), Security Scan, Code Quality.
- Linux CI uses Qt 6.7.3; Windows CI uses Qt 6.8.2. FFmpeg = downloaded master-latest (7+).
- "Build project" step = `cmake --build`. Poll via GitHub Actions API + GITHUB_TOKEN.
- To fetch logs: `GET /repos/{o}/{r}/actions/jobs/{job_id}/logs` (returns a combined blob; grep it).
- ninja stops on the first failing translation unit, so Linux often shows far fewer errors
  than Windows even when the same latent bugs exist — always fix for BOTH platforms.

## Critical Build Lessons (learned fixing the 2026-08 CI failure)
1. **AUTOMOC requires Q_OBJECT headers to be explicitly listed in the target's source/header set.**
   A header merely #included by a .cpp is NOT reliably scanned. Symptom: LNK2001 on
   `metaObject`/`qt_metacall`/`qt_metacast`/`staticMetaObject` + the class's signals.
   When adding a Q_OBJECT class: add the .cpp to REDROID_SOURCES AND the .hpp to REDROID_HEADERS.
2. **When adding a .cpp that ReDroidController uses**, also add it to `REDROID_TEST_SOURCES`
   + the test target's AUTOMOC header list in tests/CMakeLists.txt (the test links ReDroidController.obj).
3. **Qt6 API changes**: `QNetworkRequest::setTimeout()` is gone → `setTransferTimeout()`.
4. **Qt6 connect(PMF,PMF)** asserts slot-arg-count <= signal-arg-count (was a soft warning in Qt5).
   Bridge a 0-arg signal to an N-arg slot with a lambda capturing the needed object.
5. **QString ← std::string** has no implicit conversion; don't assign `.toStdString()` to a QString.
6. **`avcodec_register_all()` was removed in FFmpeg 4.0+** — guard with `LIBAVCODEC_VERSION_INT < 0x380964` (58.9.100).
7. Declared-but-unimplemented methods link-fail (LNK2001) on the function name — grep the .cpp for `::methodName` to confirm no body exists before implementing.

## Current CI Failure (2026-08-21, run 32479452062)
- Root cause: stray extra `endif()` in root CMakeLists.txt after the
  `if(NOT FFMPEG_FOUND)` stub block (was line 171) — it closed `if(BUILD_QT6_GUI)`
  prematurely, so `endif() # BUILD_QT6_GUI` errored with
  "Flow control statements are not properly nested" at line 584.
- Fix verified locally with cmake 4.4.2 (pip): delete the stray endif; configure
  then proceeds past nesting check. Full configure still needs Qt6 installed.
- Fix applied in working tree (uncommitted) as of 2026-08-22.

## CI Fix Session 2026-08-22 (run 32565076337 = GREEN, all 4 jobs)
Lessons for full codebase wiring:
- When a call-site references a struct member that doesn't exist, fix the CALL SITE to
  match the real struct (never add phantom fields) — e.g. SignalStrength uses `dBm`,
  not `rsrp`; NetworkType uses `LTE`, not `LTE_4G`.
- `QMutex` in a const method -> declare `mutable QMutex m_mutex`.
- Appending impls to a .cpp: verify they're INSIDE the namespace (check the
  `} // namespace` line position), else C2653 cascade.
- AUTOMOC Q_OBJECT headers must be in BOTH REDROID_HEADERS (main target) AND the
  test target's AUTOMOC header list in tests/CMakeLists.txt.
- Non-singleton Qt classes: don't call ::instance(); use a `static` local or member.
- When a method is declared but never implemented, LNK2001 fires at link time.
  Prefer deleting the stray declaration if an equivalent method already exists.

## Conventions
- Namespace: `VirtualPhonePro` (core) / `FirebaseHelper`
- Dual header convention: many classes have both `.h` and `.hpp` wrappers
- Logging: `Logger` singleton + `qDebug/qWarning`; `FileLogger` for persistence

## Ethical Note
This tool's anti-detection/spoofing layer can enable fraud (fake device farms,
banking-app circumvention). Distinguish legitimate work (build fixes, refactors,
code-quality, security hardening, documentation) from work that *extends*
evasion capability — the latter warrants a conversation with the user.

## Audit Addendum (2026-08-22)
- Latest CI: `Build and Test` = failure; `Docker Build` = success (run 03:02Z).
- Legacy dead entry points REMOVED (Fix 12, 2026-08-23): `src/main.cpp` (standalone
  CLI + its BUILD_CLI CMake option), `src/mainwindow.{h,cpp}`, `src/AutoStartDialog.{h,cpp}`,
  `src/GUI/main.cpp`, and the never-included `src/CMakeLists.txt`. The single live entry
  point is `src/qtmain.cpp` (target `ReDroidCPP` in the root CMakeLists.txt); the live
  main window is `src/GUI/MainWindow.{h,cpp}`.
- Admin auth is client-side: AdminLoginWindow.cpp compares plaintext Firestore
  `password` field — broken auth model (firebase key + plaintext pass in public repo).
- `VirtualSecurityChip.cpp` calls stub `RAND_bytes` (rand()-based) for key material.
- (Resolved by removal) CLI `src/main.cpp` used `rand()` IMEI/serial/MAC gen — file deleted in Fix 12.
- ~71 TODO/FIXME markers; ~15 duplicate `.h`/`.hpp` header pairs.

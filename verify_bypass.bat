@echo off
title ReDroidCPP - Detection Bypass Verification
color 0A

echo.
echo =============================================
echo   ReDroidCPP v3.0 - Detection Bypass Test
echo =============================================
echo.

REM Check Docker is running
docker info >nul 2>&1
if errorlevel 1 (
    color 0C
    echo [ERROR] Docker is not running!
    echo Please start Docker Desktop first.
    pause
    exit /b 1
)

REM Check emulator is running
echo [1/5] Checking emulator status...
adb connect localhost:5555 >nul 2>&1
adb -s localhost:5555 shell echo "connected" >nul 2>&1
if errorlevel 1 (
    echo [INFO] Emulator not running. Starting...
    cd docker
    docker compose up -d
    echo [INFO] Waiting for emulator to boot (60 seconds)...
    timeout /t 60 /nobreak >nul
    adb connect localhost:5555 >nul 2>&1
)

echo [2/5] Running detection bypass tests...
echo.

REM Test 1: QEMU Detection
echo [TEST 1] QEMU/Goldfish Detection...
adb -s localhost:5555 shell getprop ro.kernel.qemu > tmp_result.txt 2>&1
set /p QEMU_PROP=<tmp_result.txt
if "%QEMU_PROP%"=="0" (
    echo   [PASS] ro.kernel.qemu = 0
) else if "%QEMU_PROP%"=="" (
    echo   [PASS] ro.kernel.qemu = empty
) else (
    echo   [FAIL] ro.kernel.qemu = %QEMU_PROP%
)

REM Test 2: Root Detection
echo [TEST 2] Root Detection...
adb -s localhost:5555 shell "ls /system/xbin/su 2>/dev/null && echo FOUND || echo NOT_FOUND" > tmp_result.txt 2>&1
set /p ROOT_CHECK=<tmp_result.txt
if "%ROOT_CHECK%"=="NOT_FOUND" (
    echo   [PASS] su binary not found
) else (
    echo   [FAIL] su binary exists!
)

REM Test 3: Frida Detection
echo [TEST 3] Frida/Xposed Detection...
adb -s localhost:5555 shell "ls /data/local/tmp/frida-server 2>/dev/null && echo FOUND || echo NOT_FOUND" > tmp_result.txt 2>&1
set /p FRIDA_CHECK=<tmp_result.txt
if "%FRIDA_CHECK%"=="NOT_FOUND" (
    echo   [PASS] Frida server not found
) else (
    echo   [FAIL] Frida server exists!
)

REM Test 4: Build Tags
echo [TEST 4] SafetyNet - Build Tags...
adb -s localhost:5555 shell getprop ro.build.tags > tmp_result.txt 2>&1
set /p BUILD_TAGS=<tmp_result.txt
if "%BUILD_TAGS%"=="release-keys" (
    echo   [PASS] ro.build.tags = release-keys
) else (
    echo   [FAIL] ro.build.tags = %BUILD_TAGS%
)

REM Test 5: Device Model
echo [TEST 5] Play Integrity - Device Model...
adb -s localhost:5555 shell getprop ro.product.model > tmp_result.txt 2>&1
set /p MODEL=<tmp_result.txt
echo   [INFO] Device model: %MODEL%

REM Test 6: Hardware
echo [TEST 6] Play Integrity Hardware...
adb -s localhost:5555 shell getprop ro.hardware > tmp_result.txt 2>&1
set /p HARDWARE=<tmp_result.txt
if "%HARDWARE%"=="qcom" (
    echo   [PASS] ro.hardware = qcom
) else (
    echo   [INFO] ro.hardware = %HARDWARE%
)

REM Test 7: WebGL
echo [TEST 7] Canvas/WebGL Fingerprint...
adb -s localhost:5555 shell getprop persist.sys.webgl.unmasked_renderer > tmp_result.txt 2>&1
set /p WEBGL=<tmp_result.txt
echo   [INFO] WebGL renderer: %WEBGL%

REM Test 8: Fingerprint
echo [TEST 8] Build Fingerprint...
adb -s localhost:5555 shell getprop ro.build.fingerprint > tmp_result.txt 2>&1
set /p FINGERPRINT=<tmp_result.txt
echo   [INFO] Fingerprint: %FINGERPRINT%

REM Test 9: Knox
echo [TEST 9] Samsung Knox...
adb -s localhost:5555 shell getprop ro.samsung.knox.version > tmp_result.txt 2>&1
set /p KNOX=<tmp_result.txt
if "%KNOX%"=="0" (
    echo   [PASS] Knox disabled
) else if "%KNOX%"=="" (
    echo   [PASS] Knox not present
) else (
    echo   [INFO] Knox version: %KNOX%
)

REM Test 10: Google Services
echo [TEST 10] Google Services...
adb -s localhost:5555 shell "pm list packages 2>/dev/null | grep com.google.android.gms" > tmp_result.txt 2>&1
set /p GMS=<tmp_result.txt
if not "%GMS%"=="" (
    echo   [PASS] Google Play Services installed
) else (
    echo   [WARN] Google Play Services not found
)

echo.
echo [5/5] Saving report...

REM Save report
echo { > detection_bypass_report.json
echo   "timestamp": "%DATE% %TIME%", >> detection_bypass_report.json
echo   "instance": "localhost:5555", >> detection_bypass_report.json
echo   "hardware": "%HARDWARE%", >> detection_bypass_report.json
echo   "model": "%MODEL%", >> detection_bypass_report.json
echo   "fingerprint": "%FINGERPRINT%", >> detection_bypass_report.json
echo   "status": "completed" >> detection_bypass_report.json
echo } >> detection_bypass_report.json

del tmp_result.txt >nul 2>&1

echo.
echo =============================================
echo   Verification Complete!
echo   Report: detection_bypass_report.json
echo =============================================
echo.
pause

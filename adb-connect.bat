@echo off
chcp 65001 >nul
color 0A
mode 80,20

echo.
echo  ================================================
echo   ReDroidCPP - ADB Connection
echo   ADB সংযোগ স্থাপন করা হচ্ছে...
echo  ================================================
echo.

:: Kill any existing ADB server
echo [INFO] Cleaning up existing ADB connections...
adb kill-server >nul 2>&1

:: Start ADB server
echo [INFO] Starting ADB server...
adb start-server

echo.
echo  ================================================
echo   Connecting to Emulator...
echo   ইমুলেটরে সংযোগ করা হচ্ছে...
echo  ================================================
echo.

:: Connect to emulator
adb connect localhost:5555

echo.
echo  ================================================
echo   Connection Result / সংযোগ ফলাফল
echo  ================================================
echo.

:: Check connection status
adb connect localhost:5555 >nul 2>&1

if errorlevel 1 (
    echo.
    echo   ❌ Connection FAILED!
    echo   ❌ সংযোগ ব্যর্থ হয়েছে!
    echo.
    echo   Make sure the emulator is running:
    echo   ইমুলেটর চালু আছে তা নিশ্চিত করুন
    echo.
    echo   1. start.bat চালান
    echo   2. ইমুলেটর চালু হওয়ার জন্য অপেক্ষা করুন
    echo   3. এই স্ক্রিপ্ট আবার চালান
    echo.
) else (
    echo.
    echo   ✅ Connection SUCCESSFUL!
    echo   ✅ সংযোগ সফল হয়েছে!
    echo.
)

echo  ================================================
echo   Connected Devices / সংযুক্ত ডিভাইসসমূহ
echo  ================================================
echo.

adb devices -l

echo.
echo ================================================
echo.

:: Show device info if connected
for /f "tokens=2" %%i in ('adb devices ^| findstr "5555"') do (
    echo.
    echo   📱 Device connected: %%i
    echo.
    echo   Available Commands / উপলব্ধ কমান্ড:
    echo   ----------------------------------------
    echo   adb shell          - Open shell
    echo   adb install app.apk - Install APK
    echo   adb logcat         - View logs
    echo   adb screenshot     - Take screenshot
    echo   adb reboot         - Reboot device
    echo   ----------------------------------------
    echo.
)

echo  Press any key to exit...
pause >nul

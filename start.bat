@echo off
chcp 65001 >nul
color 0A
mode 80,25

echo.
echo  ================================================
echo   ReDroidCPP - Android Emulator Launcher
echo   Android ইমুলেটর চালু করা হচ্ছে...
echo  ================================================
echo.

:: Check if Docker is running
echo [INFO] Docker চেক করা হচ্ছে...
docker info >nul 2>&1

if errorlevel 1 (
    echo.
    echo  ================================================
    echo   ERROR / ত্রুটি!
    echo  ================================================
    echo.
    echo   ⚠️  Docker চালু নেই!
    echo.
    echo   Please start Docker Desktop first
    echo   অনুগ্রহ করে প্রথমে Docker Desktop চালু করুন
    echo.
    echo   1. Start Menu থেকে Docker Desktop খুলুন
    echo   2. Docker চালু হওয়ার জন্য অপেক্ষা করুন
    echo   3. এই স্ক্রিপ্ট আবার চালান
    echo.
    pause
    exit /b 1
)

echo   ✅ Docker is running
echo   ✅ Docker চালু আছে
echo.

:: Build and run with docker compose
echo ================================================
echo   Building and Starting Container...
echo   কন্টেইনার বিল্ড ও চালু করা হচ্ছে...
echo ================================================
echo.

cd /d "%~dp0docker"
docker compose up --build

if errorlevel 1 (
    echo.
    echo  ================================================
    echo   ERROR / ত্রুটি!
    echo  ================================================
    echo.
    echo   ❌ Container start failed!
    echo   ❌ কন্টেইনার চালু করতে ব্যর্থ হয়েছে!
    echo.
    echo   Check logs: docker compose logs
    echo.
    pause
    exit /b 1
)

echo.
echo  ================================================
echo   ✅ SUCCESS / সফল!
echo  ================================================
echo.
echo   📱 Emulator is starting...
echo   📱 ইমুলেটর চালু হচ্ছে...
echo.
echo   Connect with ADB:
echo   adb connect localhost:5555
echo.
echo   Press Ctrl+C to stop or close this window
echo   বন্ধ করতে Ctrl+C দিন
echo.
pause

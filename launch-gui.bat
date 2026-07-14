@echo off
chcp 65001 >nul
color 0A
title ReDroidCPP - Android Emulator Manager

echo.
echo  ================================================
echo   ReDroidCPP - GUI Launcher
echo   Android Emulator Manager
echo  ================================================
echo.

:: Check if Docker is running
echo [INFO] Checking Docker...
docker info >nul 2>&1
if errorlevel 1 (
    echo.
    echo  ⚠️  Docker is not running!
    echo.
    echo   Please start Docker Desktop first
    echo   and then run this application.
    echo.
    pause
    exit /b 1
)

echo   ✅ Docker is running
echo.

:: Check if Qt6 GUI exists
echo [INFO] Looking for GUI application...
if exist "build\Release\virtualphonepro-qt.exe" (
    echo   Found: build\Release\virtualphonepro-qt.exe
    echo.
    echo Starting GUI...
    start "" "build\Release\virtualphonepro-qt.exe"
    exit /b 0
)

if exist "build\Debug\virtualphonepro-qt.exe" (
    echo   Found: build\Debug\virtualphonepro-qt.exe
    echo.
    echo Starting GUI...
    start "" "build\Debug\virtualphonepro-qt.exe"
    exit /b 0
)

if exist "build\virtualphonepro-qt.exe" (
    echo   Found: build\virtualphonepro-qt.exe
    echo.
    echo Starting GUI...
    start "" "build\virtualphonepro-qt.exe"
    exit /b 0
)

:: If not found, offer to build
echo.
echo  ⚠️  GUI application not found!
echo.
echo   Would you like to build it now?
echo.
echo   1. Yes - Build GUI (requires Qt6)
echo   2. No - Use command line version
echo.

choice /C:12 /N /M "Select option: "

if errorlevel 2 (
    echo.
    echo Using CLI version...
    echo.
    echo Available commands:
    echo   start.bat     - Start emulator
    echo   adb-connect.bat - Connect ADB
    echo.
    pause
    exit /b 0
)

if errorlevel 1 (
    echo.
    echo Building GUI...
    echo.
    
    if not exist "build" mkdir build
    cd build
    
    :: Check for Qt6
    cmake .. -G "Visual Studio 17 2022" -A x64 -DBUILD_QT6_GUI=ON
    if errorlevel 1 (
        echo.
        echo  ❌ Build failed!
        echo.
        echo   Make sure Qt6 is installed.
        echo   Download from: https://www.qt.io/download
        echo.
        pause
        exit /b 1
    )
    
    cmake --build . --config Release
    if errorlevel 1 (
        echo.
        echo  ❌ Build failed!
        pause
        exit /b 1
    )
    
    echo.
    echo  ✅ Build successful!
    echo.
    echo Starting GUI...
    start "" "Release\virtualphonepro-qt.exe"
    cd ..
    exit /b 0
)

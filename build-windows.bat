@echo off
REM ================================================================================
REM ReDroidCPP - Windows Build Script
REM Professional Android Emulator Controller
REM ================================================================================

setlocal enabledelayedexpansion

set "PROJECT_NAME=RedroidCPP"
set "PROJECT_VERSION=3.0.0"
set "BUILD_TYPE=Release"

REM Colors for output
set "ESC=["
set "RED=%ESC%91m"
set "GREEN=%ESC%92m"
set "YELLOW=%ESC%93m"
set "BLUE=%ESC%94m"
set "NC=%ESC%0m"

echo.
echo #############################################################################
echo #                                                                           #
echo #   %BLUE%ReDroidCPP%NC% - Windows Build Script                               #
echo #   Version: %YELLOW%%PROJECT_VERSION%%NC%                                                    #
echo #                                                                           #
echo #############################################################################
echo.

REM Check for Visual Studio
where cl >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo %RED%ERROR:%NC% Visual Studio not found.
    echo Please install Visual Studio 2022 with "Desktop development with C++"
    echo Download: https://visualstudio.microsoft.com/downloads/
    echo.
    pause
    exit /b 1
)

REM Check for CMake
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo %RED%ERROR:%NC% CMake not found.
    echo Please install CMake 3.20+ from https://cmake.org/download/
    echo.
    pause
    exit /b 1
)

echo %GREEN%[OK]%NC% Visual Studio found
echo %GREEN%[OK]%NC% CMake found
echo.

REM Detect Qt6 installation
set "QT6_DIR="
if exist "C:\Qt\6.5\msvc2022_64\bin\qmake.exe" (
    set "QT6_DIR=C:\Qt\6.5\msvc2022_64"
) else if exist "C:\Qt\6.4\msvc2022_64\bin\qmake.exe" (
    set "QT6_DIR=C:\Qt\6.4\msvc2022_64"
) else if exist "C:\Qt\6.3\msvc2022_64\bin\qmake.exe" (
    set "QT6_DIR=C:\Qt\6.3\msvc2022_64"
) else if exist "C:\Qt\Qt6\6.5\msvc2022_64\bin\qmake.exe" (
    set "QT6_DIR=C:\Qt\Qt6\6.5\msvc2022_64"
)

if defined QT6_DIR (
    echo %GREEN%[OK]%NC% Qt6 found: %QT6_DIR%
) else (
    echo %YELLOW%WARNING:%NC% Qt6 not found in default locations.
    echo Expected locations:
    echo   - C:\Qt\6.5\msvc2022_64
    echo   - C:\Qt\6.4\msvc2022_64
    echo   - C:\Qt\6.3\msvc2022_64
    echo.
    echo To install Qt6:
    echo   1. Download Qt Installer from https://www.qt.io/download
    echo   2. Install Qt 6.5+ with MSVC 2022 64-bit
    echo.
    set /p INSTALL_QT="Do you want to continue without Qt6 GUI? (Y/N): "
    if /i not "!INSTALL_QT!"=="Y" (
        exit /b 1
    )
)

echo.
echo ========================================
echo  Build Configuration
echo ========================================
echo   Project:    %PROJECT_NAME%
echo   Version:    %PROJECT_VERSION%
echo   Build Type: %BUILD_TYPE%
echo   Qt6:        !QT6_DIR:-=\!
if defined QT6_DIR (
    echo   GUI Build:  YES
) else (
    echo   GUI Build:  NO (CLI only)
)
echo ========================================
echo.

REM Create build directory
echo %BLUE%[1/3]%NC% Creating build directory...
if not exist "build" mkdir build
cd build
if %ERRORLEVEL% NEQ 0 (
    echo %RED%ERROR:%NC% Failed to create build directory
    pause
    exit /b 1
)
echo.

REM Configure CMake
echo %BLUE%[2/3]%NC% Configuring CMake...

set "CMAKE_ARGS=-G "Visual Studio 17 2022" -A x64"
set "CMAKE_ARGS=!CMAKE_ARGS! -DCMAKE_BUILD_TYPE=%BUILD_TYPE%"
set "CMAKE_ARGS=!CMAKE_ARGS! -DCMAKE_INSTALL_PREFIX=install"

if defined QT6_DIR (
    set "CMAKE_ARGS=!CMAKE_ARGS! -DBUILD_QT6_GUI=ON"
    set "CMAKE_ARGS=!CMAKE_ARGS! -DCMAKE_PREFIX_PATH=%QT6_DIR%"
) else (
    set "CMAKE_ARGS=!CMAKE_ARGS! -DBUILD_QT6_GUI=OFF"
)

cmake .. !CMAKE_ARGS!
if %ERRORLEVEL% NEQ 0 (
    echo %RED%ERROR:%NC% CMake configuration failed
    pause
    exit /b 1
)
echo %GREEN%Configuration completed successfully!%NC%
echo.

REM Build
echo %BLUE%[3/3]%NC% Building project...
echo.

cmake --build . --config %BUILD_TYPE% --parallel
if %ERRORLEVEL% NEQ 0 (
    echo %RED%ERROR:%NC% Build failed
    pause
    exit /b 1
)

echo.
echo ========================================
echo  %GREEN%Build completed successfully!%NC%
echo ========================================
echo.
echo   Executable location:
if defined QT6_DIR (
    echo     build\%BUILD_TYPE%\virtualphonepro-qt.exe
) else (
    echo     build\%BUILD_TYPE%\redroid-cli.exe
)
echo.
echo   To run:
if defined QT6_DIR (
    echo     .\%BUILD_TYPE%\virtualphonepro-qt.exe
) else (
    echo     .\%BUILD_TYPE%\redroid-cli.exe --help
)
echo.
echo ========================================
echo.
pause

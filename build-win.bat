@echo off
REM =============================================================================
REM RedroidCPP Windows Build Script
REM Professional Android Emulator Manager v3.0.0
REM =============================================================================
REM 
REM Requirements:
REM - CMake 3.16 or higher
REM - Visual Studio 2019/2022 with C++ tools
REM - Qt6 (optional, for GUI)
REM
REM Usage:
REM   build-win.bat [Debug|Release]
REM =============================================================================

setlocal enabledelayedexpansion

set "CONFIG=Release"
if "%~1" neq "" (
    set "CONFIG=%~1"
)

echo.
echo =============================================================================
echo  RedroidCPP Windows Build Script v3.0.0
echo =============================================================================
echo.

REM Check for CMake
where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake not found. Please install CMake 3.16+ from:
    echo         https://cmake.org/download/
    exit /b 1
)

REM Check for Visual Studio
where cl >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Visual Studio C++ tools not found.
    echo          Please install Visual Studio 2019/2022 with C++ workload.
    exit /b 1
)

REM Set build directory
set "BUILD_DIR=build-win"
if exist "%BUILD_DIR%" (
    echo [INFO] Cleaning existing build directory...
    rmdir /s /q "%BUILD_DIR%" 2>nul
)

echo [INFO] Creating build directory...
mkdir "%BUILD_DIR%"

echo [INFO] Configuring CMake for %CONFIG% build...
cd "%BUILD_DIR%"

REM Configure for Visual Studio with appropriate generator
cmake .. ^
    -G "Visual Studio 17 2022" ^
    -A x64 ^
    -DCMAKE_BUILD_TYPE=%CONFIG% ^
    -DCMAKE_CXX_STANDARD=17 ^
    -DBUILD_SHARED_LIBS=OFF

if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed!
    cd ..
    exit /b 1
)

echo.
echo [INFO] Building project...
echo.

REM Build
cmake --build . --config %CONFIG% --parallel

if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed!
    cd ..
    exit /b 1
)

cd ..

echo.
echo =============================================================================
echo  Build completed successfully!
echo =============================================================================
echo.
echo  Output directory: %BUILD_DIR%\%CONFIG%
echo  Executable: redroid-cli.exe
echo.
echo  To run:
echo    cd %BUILD_DIR%\%CONFIG%
echo    redroid-cli.exe --help
echo.
echo =============================================================================

endlocal
exit /b 0

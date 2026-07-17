@echo off
setlocal enabledelayedexpansion

echo.
echo  ============================================================
echo   ReDroidCPP v3.0 - Windows Release Build Script
echo  ============================================================
echo.

:: ============================================================================
:: CONFIGURATION
:: ============================================================================

set "PROJECT_NAME=ReDroidCPP"
set "VERSION=3.0.0"
set "BUILD_DIR=build"
set "RELEASE_DIR=ReDroidCPP_v3"

:: Qt6 Installation Path (adjust if needed)
set "QT_BASE=C:\Qt"
set "QT_VERSION=6.5.3"
set "QT_COMPILER=msvc2022_64"
set "QT_DIR=%QT_BASE%\%QT_VERSION%\%QT_COMPILER%"

:: Check for Visual Studio
set "VS_DEV_CMD="
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1" (
    set "VS_DEV_CMD=powershell -ExecutionPolicy Bypass -File \"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1\" -Arch amd64"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\Launch-VsDevShell.ps1" (
    set "VS_DEV_CMD=powershell -ExecutionPolicy Bypass -File \"C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\Launch-VsDevShell.ps1\" -Arch amd64"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\Launch-VsDevShell.ps1" (
    set "VS_DEV_CMD=powershell -ExecutionPolicy Bypass -File \"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\Launch-VsDevShell.ps1\" -Arch amd64"
) else (
    echo [ERROR] Visual Studio 2022 not found!
    echo Please install Visual Studio 2022 with C++ Desktop Development
    pause
    exit /b 1
)

:: ============================================================================
:: CHECK PREREQUISITES
:: ============================================================================

echo [1/6] Checking prerequisites...

:: Check Qt6
if not exist "%QT_DIR%\bin\qmake.exe" (
    echo [WARNING] Qt6 not found at: %QT_DIR%
    echo Looking for Qt6 in alternative locations...
    
    :: Search in Program Files
    for /d %%D in ("C:\Qt\*") do (
        if exist "%%D\%QT_COMPILER%\bin\qmake.exe" (
            set "QT_DIR=%%D\%QT_COMPILER%"
            goto :qt_found
        )
    )
    
    echo [ERROR] Qt6 MSVC 2022 not found!
    echo Please install Qt6 from https://www.qt.io/download-qt-installer
    echo Required: Qt 6.5+ with MSVC 2022 64-bit
    pause
    exit /b 1
)

:qt_found
echo [OK] Found Qt6 at: %QT_DIR%

:: Set Qt environment
set "QTDIR=%QT_DIR%"
set "PATH=%QT_DIR%\bin;%PATH%"

:: Check CMake
where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake not found!
    echo Please install CMake from https://cmake.org/download
    echo Make sure to add CMake to PATH
    pause
    exit /b 1
)

echo [OK] CMake found

:: Check Git
where git >nul 2>&1
if errorlevel 1 (
    echo [WARNING] Git not found in PATH
)

echo.

:: ============================================================================
:: CLEAN PREVIOUS BUILD
:: ============================================================================

echo [2/6] Cleaning previous build...

if exist "%BUILD_DIR%" (
    echo Removing old build directory...
    rmdir /s /q "%BUILD_DIR%" 2>nul
)

if exist "%RELEASE_DIR%" (
    echo Removing old release directory...
    rmdir /s /q "%RELEASE_DIR%" 2>nul
)

echo.

:: ============================================================================
:: CREATE BUILD DIRECTORIES
:: ============================================================================

echo [3/6] Creating directories...

mkdir "%BUILD_DIR%" 2>nul
mkdir "%RELEASE_DIR%" 2>nul
mkdir "%RELEASE_DIR%\docker" 2>nul
mkdir "%RELEASE_DIR%\profiles" 2>nul
mkdir "%RELEASE_DIR%\platform-tools" 2>nul

echo.

:: ============================================================================
:: BUILD WITH CMAKE
:: ============================================================================

echo [4/6] Building %PROJECT_NAME% v%VERSION%...

:: Run Visual Studio Developer Command Prompt
echo Initializing Visual Studio environment...
call %VS_DEV_CMD% >nul 2>&1

:: Configure with CMake
echo Running CMake configuration...
cd /d "%~dp0"

cmake -S . -B "%BUILD_DIR%" ^
    -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DBUILD_QT6_GUI=ON ^
    -DBUNDLE_QT=ON ^
    -DCMAKE_PREFIX_PATH="%QT_DIR%" ^
    -DCMAKE_INSTALL_PREFIX="%RELEASE_DIR%" ^
    -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="%BUILD_DIR%\bin" ^
    -Werror=dev ^
    -Wno-dev

if errorlevel 1 (
    echo.
    echo [ERROR] CMake configuration failed!
    echo Check the output above for errors.
    pause
    exit /b 1
)

echo.

:: Build
echo Building project (this may take several minutes)...
cmake --build "%BUILD_DIR%" --config Release --parallel

if errorlevel 1 (
    echo.
    echo [ERROR] Build failed!
    echo Check the output above for errors.
    pause
    exit /b 1
)

echo.

:: ============================================================================
:: COPY FILES TO RELEASE
:: ============================================================================

echo [5/6] Copying files to release folder...

:: Copy executable
if exist "%BUILD_DIR%\bin\ReDroidCPP.exe" (
    copy "%BUILD_DIR%\bin\ReDroidCPP.exe" "%RELEASE_DIR%\" >nul
    echo [OK] ReDroidCPP.exe
) else (
    echo [ERROR] ReDroidCPP.exe not found!
    pause
    exit /b 1
)

:: Copy Qt DLLs
echo Copying Qt DLLs...
set "QT_BIN=%QT_DIR%\bin"
for %%F in (
    "Qt6Core.dll"
    "Qt6Gui.dll"
    "Qt6Widgets.dll"
    "Qt6Network.dll"
    "Qt6Svg.dll"
    "Qt6Xml.dll"
) do (
    if exist "%QT_BIN%\%%~F" (
        copy "%QT_BIN%\%%~F" "%RELEASE_DIR%\" >nul
        echo [OK] %%~F
    )
)

:: Copy Qt plugins
echo Copying Qt plugins...
if not exist "%RELEASE_DIR%\plugins" mkdir "%RELEASE_DIR%\plugins"

if exist "%QT_DIR%\plugins\platforms\qwindows.dll" (
    mkdir "%RELEASE_DIR%\plugins\platforms" 2>nul
    copy "%QT_DIR%\plugins\platforms\qwindows.dll" "%RELEASE_DIR%\plugins\platforms\" >nul
    echo [OK] qwindows.dll
)

if exist "%QT_DIR%\plugins\styles\qwindowsvistastyle.dll" (
    mkdir "%RELEASE_DIR%\plugins\styles" 2>nul
    copy "%QT_DIR%\plugins\styles\qwindowsvistastyle.dll" "%RELEASE_DIR%\plugins\styles\" >nul
)

if exist "%QT_DIR%\plugins\iconengines\qsvgicon.dll" (
    mkdir "%RELEASE_DIR%\plugins\iconengines" 2>nul
    copy "%QT_DIR%\plugins\iconengines\qsvgicon.dll" "%RELEASE_DIR%\plugins\iconengines\" >nul
)

:: Copy MSVC runtime DLLs
echo Copying MSVC runtime...
if exist "C:\Windows\System32\VCRUNTIME140.dll" (
    copy "C:\Windows\System32\VCRUNTIME140.dll" "%RELEASE_DIR%\" >nul
)
if exist "C:\Windows\System32\VCRUNTIME140_1.dll" (
    copy "C:\Windows\System32\VCRUNTIME140_1.dll" "%RELEASE_DIR%\" >nul
)
if exist "C:\Windows\System32\MSVCP140.dll" (
    copy "C:\Windows\System32\MSVCP140.dll" "%RELEASE_DIR%\" >nul
)
if exist "C:\Windows\System32\MSVCP140_1.dll" (
    copy "C:\Windows\System32\MSVCP140_1.dll" "%RELEASE_DIR%\" >nul
)

:: Copy Docker files
echo Copying Docker configuration...
if exist "docker\Dockerfile" (
    copy "docker\Dockerfile" "%RELEASE_DIR%\docker\" >nul
    echo [OK] Dockerfile
)
if exist "docker\Dockerfile.custom" (
    copy "docker\Dockerfile.custom" "%RELEASE_DIR%\docker\" >nul
)
if exist "docker\docker-compose.yml" (
    copy "docker\docker-compose.yml" "%RELEASE_DIR%\docker\" >nul
    echo [OK] docker-compose.yml
)
if exist "docker\entrypoint.sh" (
    copy "docker\entrypoint.sh" "%RELEASE_DIR%\docker\" >nul
    echo [OK] entrypoint.sh
)

:: Copy profiles
echo Copying device profiles...
xcopy /e /y "profiles\*" "%RELEASE_DIR%\profiles\" >nul
echo [OK] Profiles copied

:: Copy batch files
echo Creating launcher scripts...

:: Start script
(
echo @echo off
echo title ReDroidCPP - Virtual Phone Pro
echo color 0A
echo echo.
echo echo =============================================
echo echo   ReDroidCPP v%VERSION% - Starting...
echo echo =============================================
echo echo.
echo set "SCRIPT_DIR=%%~dp0"
echo set "ADBPATH=%%SCRIPT_DIR%%platform-tools\adb.exe"
echo.
echo if not exist "%%SCRIPT_DIR%%ReDroidCPP.exe" (
echo     echo [ERROR] ReDroidCPP.exe not found!
echo     pause
echo     exit /b 1
echo )
echo.
echo :: Set Qt plugin path
echo set "QT_PLUGIN_PATH=%%SCRIPT_DIR%%plugins"
echo.
echo :: Start application
echo start "" "%%SCRIPT_DIR%%ReDroidCPP.exe"
echo.
echo :: Optional: Start ADB server
echo if exist "%%ADBPATH%%" (
echo     echo Starting ADB server...
echo     start /min "" "%%ADBPATH%%" start-server
echo )
echo.
echo exit
) > "%RELEASE_DIR%\start.bat"

:: ADB connect script
(
echo @echo off
echo set "SCRIPT_DIR=%%~dp0"
echo set "ADBPATH=%%SCRIPT_DIR%%platform-tools\adb.exe"
echo.
echo if not exist "%%ADBPATH%%" (
echo     echo [ERROR] ADB not found at: %%ADBPATH%%
echo     echo Please download platform-tools from Android SDK
echo     pause
echo     exit /b 1
echo )
echo.
echo echo Connecting to ReDroid instances...
echo %%ADBPATH%% connect localhost:5555
echo %%ADBPATH%% connect localhost:5556
echo %%ADBPATH%% connect localhost:5557
echo.
echo echo Current devices:
echo %%ADBPATH%% devices -l
echo.
echo pause
) > "%RELEASE_DIR%\adb-connect.bat"

:: Create README
(
echo ReDroidCPP v%VERSION% - Professional Android Emulator
echo ==============================================
echo.
echo Prerequisites:
echo - Windows 10/11 (64-bit)
echo - Docker Desktop (for Android containers)
echo - Android SDK Platform Tools (optional)
echo.
echo Getting Started:
echo 1. Run start.bat to launch ReDroidCPP
echo 2. Install Docker and start docker-compose
echo 3. Create a new device instance
echo.
echo Features:
echo - Ultra-realistic phone UI
echo - 40+ Anti-detection modules
echo - Multi-instance support
echo - Banking app testing
echo - Security research
echo.
echo Documentation: https://github.com/mostakimnasim5/redroid-cpp
echo.
) > "%RELEASE_DIR%\README.txt"

echo.

:: ============================================================================
:: FINALIZE
:: ============================================================================

echo [6/6] Finalizing...

:: Count files
set "FILE_COUNT=0"
for /r "%RELEASE_DIR%" %%F in (*) do set /a FILE_COUNT+=1

echo.
echo ============================================================
echo   BUILD COMPLETE!
echo ============================================================
echo.
echo Release folder: %RELEASE_DIR%
echo Total files: %FILE_COUNT%
echo.
echo Contents:
dir /b "%RELEASE_DIR%"
echo.
echo Next steps:
echo 1. Install Docker Desktop: https://docker.com
echo 2. Run start.bat to launch the application
echo 3. Download Android SDK platform-tools for ADB
echo.
echo ============================================================
echo.

pause

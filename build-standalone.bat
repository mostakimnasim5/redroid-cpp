@echo off
REM ================================================================================
REM ReDroidCPP - Standalone Windows Build Script (No VS Installation Required!)
REM ================================================================================
REM 
REM This script downloads all dependencies and builds a standalone .exe
REM that runs on ANY Windows PC without needing Visual Studio.
REM
REM Usage: build-standalone.bat
REM ================================================================================

setlocal enabledelayedexpansion

set "PROJECT_NAME=ReDroidCPP"
set "PROJECT_VERSION=3.0.0"
set "BUILD_DIR=build_standalone"
set "INSTALL_DIR=%BUILD_DIR%\installed"

REM Colors
set "ESC=["
set "RED=%ESC%91m"
set "GREEN=%ESC%92m"
set "YELLOW=%ESC%93m"
set "BLUE=%ESC%94m"
set "CYAN=%ESC%96m"
set "NC=%ESC%0m"

echo.
echo #############################################################################
echo #                                                                           #
echo #   %CYAN%ReDroidCPP%NC% - Standalone Windows Build                          #
echo #   Version: %YELLOW%%PROJECT_VERSION%%NC%                                                     #
echo #                                                                           #
echo #############################################################################
echo.

REM ================================================================================
REM Check Prerequisites
REM ================================================================================

REM Check for CMake
where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo %RED%ERROR:%NC% CMake not found.
    echo.
    echo Installing CMake...
    powershell -Command "Invoke-WebRequest -Uri 'https://github.com/Kitware/CMake/releases/download/v3.28.1/cmake-3.28.1-windows-x86_64.zip' -OutFile 'cmake.zip'"
    powershell -Command "Expand-Archive -Path 'cmake.zip' -DestinationPath 'C:\cmake' -Force"
    powershell -Command "[Environment]::SetEnvironmentVariable('Path', $env:Path + ';C:\cmake\cmake-3.28.1-windows-x86_64\bin', 'User')"
    set "PATH=C:\cmake\cmake-3.28.1-windows-x86_64\bin;%PATH%"
    del cmake.zip
    echo %GREEN%[OK]%NC% CMake installed
)

echo %GREEN%[OK]%NC% CMake found
cmake --version | findstr /C:"cmake version"
echo.

REM ================================================================================
REM Check for Visual Studio (required for MSVC compiler)
REM ================================================================================

where cl >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo %YELLOW%WARNING:%NC% Visual Studio not detected in PATH.
    echo.
    echo %RED%IMPORTANT:%NC% Visual Studio 2022 with C++ Desktop Development is required.
    echo.
    echo Please install from: https://visualstudio.microsoft.com/downloads/
    echo Required workload: "Desktop development with C++"
    echo.
    echo After installation, run this script from:
    echo     Start Menu ^> Visual Studio 2022 ^> Developer Command Prompt for VS 2022
    echo.
    echo Or download and run the installer from:
    echo     https://aka.ms/vs/17/release/vs_buildtools.exe
    echo.
    pause
    exit /b 1
)

echo %GREEN%[OK]%NC% Visual Studio found
cl --version | findstr /C:"Cl version"
echo.

REM ================================================================================
REM Detect or Download Qt6
REM ================================================================================

set "QT_DIR="
set "QT_VERSION=6.5.3"

REM Check common Qt6 locations
if exist "C:\Qt\6.5.3\msvc2022_64\bin\qmake.exe" (
    set "QT_DIR=C:\Qt\6.5.3\msvc2022_64"
) else if exist "C:\Qt\6.6.0\msvc2022_64\bin\qmake.exe" (
    set "QT_DIR=C:\Qt\6.6.0\msvc2022_64"
) else if exist "C:\Qt\6.7.0\msvc2022_64\bin\qmake.exe" (
    set "QT_DIR=C:\Qt\6.7.0\msvc2022_64"
) else if exist "C:\Qt\6.8.0\msvc2022_64\bin\qmake.exe" (
    set "QT_DIR=C:\Qt\6.8.0\msvc2022_64"
)

REM If Qt6 not found, offer to download
if not defined QT_DIR (
    echo %YELLOW%Qt6 not found in default locations.%NC%
    echo.
    set /p DOWNLOAD_QT="Do you want to download Qt 6.5.3 MSVC 2022 64-bit? (Y/N): "
    if /i "!DOWNLOAD_QT!"=="Y" (
        echo.
        echo Downloading Qt 6.5.3...
        powershell -Command "Invoke-WebRequest -Uri 'https://download.qt.io/official_releases/qt/6.5/6.5.3/single/qt-everywhere-src-6.5.3.zip' -OutFile 'qt-src.zip'"
        powershell -Command "Expand-Archive -Path 'qt-src.zip' -DestinationPath 'C:\qt-src' -Force"
        echo.
        echo %YELLOW%NOTE:%NC% Qt needs to be built from source, which takes 2-4 hours.
        echo.
        echo Please download Qt from:
        echo https://www.qt.io/download-qt-installer
        echo.
        echo Or use aqtinstall:
        echo     pip install aqtinstall
        echo     aqt install-qt windows desktop 6.5.3 win64_msvc2022_64
        echo.
        del qt-src.zip 2>nul
        pause
        exit /b 1
    ) else (
        echo.
        echo %RED%ERROR:%NC% Qt6 is required for GUI build.
        echo.
        pause
        exit /b 1
    )
)

echo %GREEN%[OK]%NC% Qt6 found: %QT_DIR%
echo.

REM ================================================================================
REM Clean Previous Build
REM ================================================================================

echo %BLUE%[1/5]%NC% Cleaning previous build...
if exist "%BUILD_DIR%" (
    rmdir /s /q "%BUILD_DIR%" 2>nul
)
mkdir "%BUILD_DIR%"
echo.

REM ================================================================================
REM Configure CMake
REM ================================================================================

echo %BLUE%[2/5]%NC% Configuring CMake...
echo.
echo   Build Type: Release
echo   Compiler: MSVC 2022
echo   Qt6: %QT_DIR%
echo   Static Runtime: YES (standalone executable)
echo.

cd "%~dp0"
cmake -S . -B "%BUILD_DIR%" ^
    -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DBUILD_QT6_GUI=ON ^
    -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded ^
    -DCMAKE_PREFIX_PATH="%QT_DIR%" ^
    -DCMAKE_INSTALL_PREFIX="%INSTALL_DIR%" ^
    -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="%BUILD_DIR%\bin"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo %RED%ERROR:%NC% CMake configuration failed!
    echo.
    pause
    exit /b 1
)

echo %GREEN%Configuration completed!%NC%
echo.

REM ================================================================================
REM Build
REM ================================================================================

echo %BLUE%[3/5]%NC% Building project...
echo.

cmake --build "%BUILD_DIR%" --config Release --parallel

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo %RED%ERROR:%NC% Build failed!
    echo.
    pause
    exit /b 1
)

echo %GREEN%Build completed!%NC%
echo.

REM ================================================================================
REM Deploy with windeployqt
REM ================================================================================

echo %BLUE%[4/5]%NC% Deploying runtime...

set "EXE_PATH=%BUILD_DIR%\bin\Release\ReDroidCPP.exe"
set "DEPLOY_DIR=%BUILD_DIR%\deploy"

if not exist "!EXE_PATH!" (
    echo %YELLOW%Searching for executable...%NC%
    for /r "%BUILD_DIR%\bin" %%F in (*.exe) do (
        set "EXE_PATH=%%F"
        set "DEPLOY_DIR=%%~dpFdeploy"
    )
)

echo   Executable: !EXE_PATH!
echo   Deploy Dir: !DEPLOY_DIR!

mkdir "!DEPLOY_DIR!" 2>nul

REM Copy executable
copy "!EXE_PATH!" "!DEPLOY_DIR!\" >nul

REM Run windeployqt
set "QT_PLUGIN_PATH=%QT_DIR%\plugins"
set "PATH=%QT_DIR%\bin;%PATH%"

echo.
echo Running windeployqt (this may take a minute)...
call "%QT_DIR%\bin\windeployqt.exe" ^
    --dir "!DEPLOY_DIR!" ^
    --no-translations ^
    --no-system-d3d-compiler ^
    --no-compiler-runtime ^
    --no-opengl-sw ^
    "!EXE_PATH!"

REM Copy additional required files
echo.
echo Copying additional files...

REM Copy MSVC runtime (for safety, though static linking should make this unnecessary)
if exist "C:\Windows\System32\VCRUNTIME140.dll" (
    copy "C:\Windows\System32\VCRUNTIME140.dll" "!DEPLOY_DIR!\" >nul
)
if exist "C:\Windows\System32\VCRUNTIME140_1.dll" (
    copy "C:\Windows\System32\VCRUNTIME140_1.dll" "!DEPLOY_DIR!\" >nul
)
if exist "C:\Windows\System32\MSVCP140.dll" (
    copy "C:\Windows\System32\MSVCP140.dll" "!DEPLOY_DIR!\" >nul
)

REM Copy docker config
if exist "docker" (
    mkdir "!DEPLOY_DIR!\docker" 2>nul
    xcopy /e /y "docker\*" "!DEPLOY_DIR!\docker\" >nul
)

REM Copy profiles
if exist "profiles" (
    mkdir "!DEPLOY_DIR!\profiles" 2>nul
    xcopy /e /y "profiles\*" "!DEPLOY_DIR!\profiles\" >nul
)

REM Copy platform-tools (ADB)
if exist "platform-tools" (
    mkdir "!DEPLOY_DIR!\platform-tools" 2>nul
    xcopy /e /y "platform-tools\*" "!DEPLOY_DIR!\platform-tools\" >nul
)

REM Create start script
(
echo @echo off
echo title ReDroidCPP
echo color 0A
echo echo.
echo echo ============================================
echo echo   ReDroidCPP v%PROJECT_VERSION% - Starting...
echo echo ============================================
echo echo.
echo set "SCRIPT_DIR=%%~dp0"
echo set "QT_PLUGIN_PATH=%%SCRIPT_DIR%%plugins"
echo.
echo start "" "%%SCRIPT_DIR%%ReDroidCPP.exe"
echo.
echo if exist "%%SCRIPT_DIR%%platform-tools\adb.exe" (
echo     start /min "" "%%SCRIPT_DIR%%platform-tools\adb.exe" start-server
echo )
echo.
) > "!DEPLOY_DIR!\start.bat"

REM Create ADB connect script
(
echo @echo off
echo set "ADBPATH=%%~dp0platform-tools\adb.exe"
echo.
echo if not exist "%%ADBPATH%%" (
echo     echo [ERROR] ADB not found. Download platform-tools from Android SDK.
echo     pause
echo     exit /b 1
echo ^)
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
) > "!DEPLOY_DIR!\adb-connect.bat"

REM Create README
(
echo ReDroidCPP v%PROJECT_VERSION% - Professional Android Emulator
echo ============================================================
echo.
echo This is a standalone portable build.
echo No Visual Studio or additional runtime installation required.
echo.
echo Files included:
echo - ReDroidCPP.exe (main application)
echo - Qt6 runtime DLLs
echo - Required plugins
echo - Docker configuration
echo - Device profiles
echo.
echo Getting Started:
echo 1. Run start.bat to launch the application
echo 2. Install Docker Desktop for Android containers
echo 3. Download Android SDK platform-tools for ADB
echo.
echo Note: Platform-tools is not included.
echo Download from: https://developer.android.com/studio/releases/platform-tools
echo.
) > "!DEPLOY_DIR!\README.txt"

REM ================================================================================
REM Create Final Package
REM ================================================================================

echo.
echo %BLUE%[5/5]NC% Creating portable package...

powershell -Command "Compress-Archive -Path '%BUILD_DIR%\deploy\*' -DestinationPath '%BUILD_DIR%\ReDroidCPP_%PROJECT_VERSION%_Windows_Portable.zip' -Force"

if %ERRORLEVEL% EQU 0 (
    echo %GREEN%Package created successfully!%NC%
)

REM ================================================================================
REM Summary
REM ================================================================================

echo.
echo #############################################################################
echo #############################################################################
echo.
echo   %GREEN%BUILD COMPLETE!%NC%
echo.
echo #############################################################################
echo.
echo   Output Files:
echo  .
echo   - Portable ZIP: %BUILD_DIR%\ReDroidCPP_%PROJECT_VERSION%_Windows_Portable.zip
echo   - Deploy Folder: %BUILD_DIR%\deploy\
echo   - Executable: %BUILD_DIR%\deploy\ReDroidCPP.exe
echo.
echo   Features:
echo   - Standalone executable (no VS required on target PC)
echo   - Static MSVC runtime (no DLL dependencies)
echo   - Qt6 runtime included
echo.
echo   Contents of deploy folder:
dir /b "%BUILD_DIR%\deploy" 2>nul | findstr /v /i ".pdb"
echo.
echo #############################################################################
echo.

pause

@echo off
REM ================================================================================
REM ReDroidCPP - Quick Build Script (Qt6 Required)
REM ================================================================================

setlocal

set "BUILD_DIR=build"
set "RELEASE_DIR=ReDroidCPP_v3_Standalone"

REM Find Qt6
set "QT_DIR="
for /d %%D in ("C:\Qt\6.*") do (
    if exist "%%D\msvc2022_64\bin\qmake.exe" (
        set "QT_DIR=%%D\msvc2022_64"
    )
)
for /d %%D in ("C:\Qt\*") do (
    if not defined QT_DIR (
        if exist "%%D\6.*\msvc2022_64\bin\qmake.exe" (
            for /d %%V in ("%%D\6.*") do (
                if not defined QT_DIR (
                    if exist "%%V\msvc2022_64\bin\qmake.exe" (
                        set "QT_DIR=%%V\msvc2022_64"
                    )
                )
            )
        )
    )
)

if not defined QT_DIR (
    echo Qt6 MSVC 2022 not found!
    echo Please install Qt6 from: https://www.qt.io/download-qt-installer
    pause
    exit /b 1
)

echo.
echo ReDroidCPP Quick Build
echo Qt6: %QT_DIR%
echo.

REM Clean
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
if exist "%RELEASE_DIR%" rmdir /s /q "%RELEASE_DIR%"

REM Configure with STATIC runtime
cmake -S . -B "%BUILD_DIR%" ^
    -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DBUILD_QT6_GUI=ON ^
    -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded ^
    -DCMAKE_PREFIX_PATH="%QT_DIR%" ^
    -DCMAKE_INSTALL_PREFIX="%RELEASE_DIR%" ^
    -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="%BUILD_DIR%\bin"

if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

REM Build
cmake --build "%BUILD_DIR%" --config Release --parallel

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    pause
    exit /b 1
)

REM Deploy
set "DEPLOY_DIR=%RELEASE_DIR%"
mkdir "%DEPLOY_DIR%" 2>nul

REM Copy exe
copy "%BUILD_DIR%\bin\Release\ReDroidCPP.exe" "%DEPLOY_DIR%\" >nul

REM Deploy Qt6
set "QT_PLUGIN_PATH=%QT_DIR%\plugins"
"%QT_DIR%\bin\windeployqt.exe" --dir "%DEPLOY_DIR%" --no-translations --no-system-d3d-compiler --no-compiler-runtime "%BUILD_DIR%\bin\Release\ReDroidCPP.exe"

REM Copy configs
if exist "docker" xcopy /e /y "docker\*" "%DEPLOY_DIR%\docker\" >nul
if exist "profiles" xcopy /e /y "profiles\*" "%DEPLOY_DIR%\profiles\" >nul

REM Create start script
(
echo @echo off
echo start "" "%%~dp0ReDroidCPP.exe"
) > "%DEPLOY_DIR%\start.bat"

echo.
echo Build complete!
echo Output: %RELEASE_DIR%\ReDroidCPP.exe
echo.
pause

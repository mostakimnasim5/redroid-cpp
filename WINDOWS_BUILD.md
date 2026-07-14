# Windows Build Instructions

## RedroidCPP v3.0.0 - Windows Build Guide

### Requirements

1. **Visual Studio 2022** (recommended) or Visual Studio 2019
   - Download from: https://visualstudio.microsoft.com/downloads/
   - Install with "Desktop development with C++" workload

2. **CMake 3.16 or higher**
   - Download from: https://cmake.org/download/
   - During installation, select "Add CMake to system PATH"

3. **Windows SDK** (for building)
   - Included with Visual Studio installation

4. **Optional: Qt6** (for GUI components)
   - Download from: https://www.qt.io/download-qt-installer

### Build Steps

#### Method 1: Using the build script (Recommended)

```batch
cd C:\path\to\redroid-cpp
build-win.bat Release
```

#### Method 2: Manual Build

```batch
cd C:\path\to\redroid-cpp
mkdir build-win
cd build-win
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release --parallel
```

### Running the Application

After successful build:

```batch
cd build-win\Release
redroid-cli.exe --help
```

### Command Examples

```batch
REM Create a Samsung device
redroid-cli.exe create -m Samsung -a 14

REM List all devices
redroid-cli.exe list

REM Generate a random profile
redroid-cli.exe profile -m Google

REM Validate an IMEI
redroid-cli.exe validate 358751090123450
```

### Troubleshooting

#### CMake not found
Add CMake to system PATH or use CMake GUI.

#### Visual Studio not found
Run from "Developer Command Prompt for VS 2022" or "x64 Native Tools Command Prompt".

#### Build errors
Ensure all dependencies are installed and paths are correct.

### Project Structure

```
redroid-cpp/
├── CMakeLists.txt           # Main build configuration
├── build-win.bat           # Windows build script
├── include/
│   ├── Core/              # Core classes (DeviceProfile, DeviceManager)
│   ├── Data/              # Data structures (TACDatabase)
│   └── VirtualPhonePro/   # Qt-based classes
├── src/
│   ├── main.cpp           # CLI entry point
│   ├── Core/             # Core implementations
│   └── ReDroidController/ # Qt-based implementations
└── profiles/             # Device profile templates
```

### License

Copyright (c) 2024. Licensed for authorized testing purposes only.

# ReDroidCPP - Build Guide

**Version:** 3.0.0  
**Last Updated:** 2026-07-14

---

## 📋 সূচিপত্র

1. [পূর্বশর্ত](#1-পূর্বশর্ত)
2. [ডিপেন্ডেন্সি ইনস্টলেশন](#2-ডিপেন্ডেন্সি-ইনস্টলেশন)
3. [Windows-এ বিল্ড](#3-windowsএ-বিল্ড)
4. [Linux-এ বিল্ড](#4-linuxএ-বিল্ড)
5. [CMake অপশন](#5-cmake-অপশন)
6. [বিল্ড ট্রাবলশুটিং](#6-বিল্ড-ট্রাবলশুটিং)
7. [আউটপুট](#7-আউটপুট)

---

## 1. পূর্বশর্ত

### সিস্টেম রিকোয়ারমেন্ট

| কম্পোনেন্ট | মিনিমাম | রিকোমেন্ডেড |
|------------|---------|-------------|
| OS | Windows 10 64-bit / Ubuntu 20.04 | Windows 11 64-bit / Ubuntu 22.04 |
| RAM | 8 GB | 16 GB |
| Storage | 10 GB | 20 GB |
| GPU | OpenGL 4.1 | OpenGL 4.6 |

### প্রয়োজনীয় সফটওয়্যার

| সফটওয়্যার | ভার্সন | উদ্দেশ্য |
|-----------|--------|---------|
| **C++ Compiler** | C++17 compatible | MSVC 2022 / GCC 11+ |
| **CMake** | 3.20+ | বিল্ড সিস্টেম |
| **Qt6** | 6.5+ | GUI ফ্রেমওয়ার্ক |
| **Docker Desktop** | 20.10+ | Android কন্টেইনার |
| **Git** | 2.30+ | ভার্সন কন্ট্রোল |

---

## 2. ডিপেন্ডেন্সি ইনস্টলেশন

### Windows (vcpkg ব্যবহার করে)

```powershell
# 1. vcpkg ইনস্টল করুন
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install

# 2. Qt6 এবং প্রয়োজনীয় প্যাকেজ ইনস্টল করুন
.\vcpkg install qt6[core,gui,widgets,network]:x64-windows
.\vcpkg install nlohmann-json:x64-windows
.\vcpkg install fmt:x64-windows

# 3. Qt Creator ইনস্টল করুন (ঐচ্ছিক)
# https://www.qt.io/download-qt-installer থেকে ডাউনলোড করুন
```

### Ubuntu/Debian

```bash
# 1. সিস্টেম আপডেট করুন
sudo apt update && sudo apt upgrade -y

# 2. প্রয়োজনীয় প্যাকেজ ইনস্টল করুন
sudo apt install -y \
    cmake \
    build-essential \
    qt6-base-dev \
    qt6-base-dev-tools \
    qt6-network-openssl-dev \
    libqt6core6 \
    libqt6gui6 \
    libqt6widgets6 \
    libnlohmann-json3-dev \
    libfmt-dev \
    git

# 3. Docker ইনস্টল করুন
sudo apt install -y docker.io docker-compose
sudo usermod -aG docker $USER
```

### macOS

```bash
# 1. Homebrew ইনস্টল করুন (যদি না থাকে)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 2. প্রয়োজনীয় প্যাকেজ ইনস্টল করুন
brew install cmake qt6 nlohmann-json fmt

# 3. Docker Desktop ইনস্টল করুন
brew install --cask docker
```

---

## 3. Windows-এ বিল্ড

### পদ্ধতি ১: CMake (কমান্ড লাইন)

```powershell
# 1. Repository ক্লোন করুন
git clone https://github.com/mostakimnasim5/redroid-cpp.git
cd redroid-cpp

# 2. Build directory তৈরি করুন
mkdir build
cd build

# 3. CMake কনফিগার করুন
cmake .. -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_BUILD_TYPE=Release `
    -DBUILD_QT6_GUI=ON `
    -DCMAKE_PREFIX_PATH="C:/vcpkg_installed/x64-windows"

# 4. বিল্ড করুন
cmake --build . --config Release

# 5. ইনস্টল করুন (ঐচ্ছিক)
cmake --install . --config Release
```

### পদ্ধতি ২: Qt Creator

```powershell
# 1. Qt Creator খুলুন
# File → Open File or Project

# 2. CMakeLists.txt বাছাই করুন

# 3. Kit সিলেক্ট করুন: Desktop Qt 6.5.x MSVC2022 64bit

# 4. Configure Project ক্লিক করুন

# 5. Build → Build All
```

### পদ্ধতি ৩: Build Script

```powershell
# স্ক্রিপ্ট চালান
.\build-windows.bat
```

### Visual Studio-তে

```powershell
# 1. CMake-সক্ষম টেমপ্লেট দিয়ে প্রজেক্ট খুলুন
devenv.exe redroid-cpp\CMakeLists.txt

# 2. প্রজেক্ট লোড হবে

# 3. Solution Explorer-এ প্রজেক্ট দেখা যাবে

# 4. Build → Build Solution
```

---

## 4. Linux-এ বিল্ড

### CMake ব্যবহার করে

```bash
# 1. Repository ক্লোন করুন
git clone https://github.com/mostakimnasim5/redroid-cpp.git
cd redroid-cpp

# 2. Build directory তৈরি করুন
mkdir build && cd build

# 3. CMake কনফিগার করুন
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_QT6_GUI=ON

# 4. বিল্ড করুন
make -j$(nproc)

# 5. ইনস্টল করুন (ঐচ্ছিক)
sudo make install
```

### Qt Creator-এ

```bash
# 1. Qt Creator খুলুন

# 2. File → Open File or Project

# 3. CMakeLists.txt বাছাই করুন

# 4. Configure Project

# 5. বিল্ড করুন
```

---

## 5. CMake অপশন

| অপশন | ডিফল্ট | বিবরণ |
|------|--------|-------|
| `BUILD_QT6_GUI` | ON | Qt6 GUI বিল্ড করুন |
| `CMAKE_BUILD_TYPE` | Release | বিল্ড টাইপ (Debug/Release) |
| `CMAKE_PREFIX_PATH` | - | Qt6 এবং অন্যান্য লাইব্রেরি পাথ |
| `ENABLE_TLS_SPOOFING` | ON | TLS ফিঙ্গারপ্রিন্টিং সক্ষম করুন |
| `ENABLE_SAFETYNET` | ON | SafetyNet spoofing সক্ষম করুন |

### উদাহরণ

```bash
# Debug বিল্ড
cmake .. -DCMAKE_BUILD_TYPE=Debug

# GUI ছাড়া বিল্ড
cmake .. -DBUILD_QT6_GUI=OFF

# কাস্টম Qt পাথ দিয়ে
cmake .. -DCMAKE_PREFIX_PATH=/path/to/qt6
```

---

## 6. বিল্ড ট্রাবলশুটিং

### সমস্যা ১: Qt6 পাওয়া যাচ্ছে না

```
CMake Error: Could not find Qt6 (requested version 6.5)
```

**সমাধান:**
```bash
# Qt6 ইনস্টল করুন
# Windows: vcpkg install qt6:x64-windows
# Linux: sudo apt install qt6-base-dev
```

### সমস্যা ২: MSVC Toolchain পাওয়া যাচ্ছে না

```
CMake Error: Could not find a package configuration file provided by "MSVC" with
any of the requested versions.
```

**সমাধান:**
```powershell
# Visual Studio 2022 Developer Command Prompt খুলুন
# অথবা CMake-এ সঠিক Generator সেট করুন
cmake .. -G "Visual Studio 17 2022" -A x64
```

### সমস্যা ৩: Docker সংযোগ ব্যর্থ

```
Error: Docker is not running
```

**সমাধান:**
```bash
# Windows: Docker Desktop চালু করুন
# Linux: sudo systemctl start docker
# macOS: Docker Desktop চালু করুন
```

### সমস্যা ৪: Permission Denied (Linux)

```
Error: cannot create directory '/usr/local/lib'
```

**সমাধান:**
```bash
# sudo দিয়ে ইনস্টল করুন
sudo make install

# অথবা ইউজার-লেভেল ইনস্টলেশন
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
make install
```

### সমস্যা ৫: Missing DLL (Windows)

```
The code execution cannot proceed because Qt6Core.dll was not found
```

**সমাধান:**
```powershell
# Qt DLL পাথ PATH-এ যোগ করুন
$env:PATH = "C:/vcpkg_installed/x64-windows/bin;$env:PATH"

# অথবা Qt Creator/Release ফোল্ডার থেকে চালান
```

---

## 7. আউটপুট

### বিল্ড সফল হলে

```
[100%] Built target virtualphonepro-qt
```

### আউটপুট ফাইল

| প্ল্যাটফর্ম | পাথ |
|-------------|-----|
| Windows | `build/Release/virtualphonepro-qt.exe` |
| Linux | `build/virtualphonepro-qt` |
| macOS | `build/virtualphonepro-qt.app` |

### অ্যাপ্লিকেশন চালানো

```bash
# Linux/macOS
./build/virtualphonepro-qt

# Windows
.\build\Release\virtualphonepro-qt.exe

# Docker ছাড়া চালানো (limited mode)
./build/virtualphonepro-qt --no-docker
```

---

## 📞 সাহায্য

সমস্যা হলে:

1. **GitHub Issues:** https://github.com/mostakimnasim5/redroid-cpp/issues
2. **Documentation:** https://github.com/mostakimnasim5/redroid-cpp#readme

---

**সংস্করণ:** 3.0.0  
**তারিখ:** 2026-07-14

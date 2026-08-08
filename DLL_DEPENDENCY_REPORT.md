# ReDroidCPP v3.0.0 - DLL ও Dependency বিশ্লেষণ রিপোর্ট

**তারিখ:** 2026-08-05  
**প্রজেক্ট:** ReDroidCPP - Professional Android Emulator  
**সংস্করণ:** 3.0.0

---

## 📋 সারসংক্ষেপ

এই রিপোর্টে ReDroidCPP প্রজেক্টের সমস্ত প্রয়োজনীয় DLL এবং dependency গুলো বিশ্লেষণ করা হয়েছে।

---

## 🔴 প্রয়োজনীয় DLL লাইব্রেরি (Required)

### 1. Qt6 Core DLLs (সবচেয়ে গুরুত্বপূর্ণ)

| DLL ফাইল | উদ্দেশ্য | যদি না থাকে তাহলে |
|----------|---------|-------------------|
| **Qt6Core.dll** | Qt Core framework | ❌ "Qt6Core.dll not found" error |
| **Qt6Gui.dll** | Qt GUI components | ❌ GUI লোড হবে না |
| **Qt6Widgets.dll** | Qt Widgets | ❌ UI কম্পোনেন্ট কাজ করবে না |
| **Qt6Network.dll** | Network functionality | ❌ Network features কাজ করবে না |
| **Qt6Concurrent.dll** | Multi-threading | ⚠️ Threading issues হতে পারে |
| **Qt6Svg.dll** | SVG icons | ⚠️ Icons দেখাবে না |
| **Qt6Xml.dll** | XML parsing | ⚠️ Config parsing fail হতে পারে |

### 2. Qt6 Platform DLLs (প্লাটফর্ম স্পেসিফিক)

| DLL ফাইল | উদ্দেশ্য | লোকেশন |
|----------|---------|---------|
| **qwindows.dll** | Windows platform plugin | `Qt6/plugins/platforms/` |
| **qwindowsvistastyle.dll** | Windows Vista+ style | `Qt6/plugins/styles/` |
| **qsvgicon.dll** | SVG icon engine | `Qt6/plugins/iconengines/` |

### 3. MSVC Runtime Libraries

| DLL ফাইল | উদ্দেশ্য | স্ট্যাটাস |
|----------|---------|---------|
| **VCRUNTIME140.dll** | VS 2015-2022 runtime | ✅ সাধারণত Windows-এ থাকে |
| **VCRUNTIME140_1.dll** | VS 2019+ extended runtime | ⚠️ নতুন version প্রয়োজন |
| **MSVCP140.dll** | C++ runtime | ✅ সাধারণত Windows-এ থাকে |
| **MSVCP140_1.dll** | C++ runtime extended | ⚠️ VS 2019+ প্রয়োজন |
| **MSVCP140_2.dll** | C++ runtime extended | ⚠️ VS 2022+ প্রয়োজন |

### 4. Windows System Libraries (সরাসরি লিংক করা)

এই গুলো CMakeLists.txt-এ `target_link_libraries` এ যোগ করা আছে:

```cmake
ws2_32      # Windows Sockets 2 API
advapi32    # Advanced API (Registry, Security)
userenv     # User environment variables
iphlpapi    # IP Helper API (Network info)
dnsapi      # DNS API
crypt32     # Cryptography API
```

| Library | উদ্দেশ্য | ডিফল্ট লোকেশন |
|---------|---------|---------------|
| ws2_32.dll | Network sockets | `C:\Windows\System32\` |
| advapi32.dll | Registry, Security | `C:\Windows\System32\` |
| userenv.dll | User environment | `C:\Windows\System32\` |
| iphlpapi.dll | Network interfaces | `C:\Windows\System32\` |
| dnsapi.dll | DNS resolution | `C:\Windows\System32\` |
| crypt32.dll | Cryptography | `C:\Windows\System32\` |

---

## ⚠️ সম্ভাব্য সমস্যা এবং সমাধান

### সমস্যা ১: Qt6 DLL পাওয়া যাচ্ছে না

```
The code execution cannot proceed because Qt6Core.dll was not found
```

**সমাধান (পদ্ধতি ১ - windeployqt):**
```powershell
# Qt6 bin folder থেকে windeployqt চালান
cd C:\Qt\6.5.3\msvc2022_64\bin
windeployqt.exe --no-translations --no-opengl-sw "C:\path\to\ReDroidCPP.exe"
```

**সমাধান (পদ্ধতি ২ - ম্যানুয়াল কপি):**
```batch
# build-release.bat স্বয়ংক্রিয়ভাবে এটি করে
copy "C:\Qt\6.5.3\msvc2022_64\bin\Qt6Core.dll" "ReDroidCPP_v3\"
copy "C:\Qt\6.5.3\msvc2022_64\bin\Qt6Gui.dll" "ReDroidCPP_v3\"
copy "C:\Qt\6.5.3\msvc2022_64\bin\Qt6Widgets.dll" "ReDroidCPP_v3\"
copy "C:\Qt\6.5.3\msvc2022_64\bin\Qt6Network.dll" "ReDroidCPP_v3\"
```

### সমস্যা ২: VCRUNTIME140_1.dll পাওয়া যাচ্ছে না

```
The code execution cannot proceed because VCRUNTIME140_1.dll was not found
```

**সমাধান:**
```powershell
# Visual Studio 2022 Redistributable ইনস্টল করুন
# https://visualstudio.microsoft.com/downloads/
# → Tools for Visual Studio → Visual Studio 2022 Redistributable
```

### সমস্যা ৩: Platform Plugin লোড হচ্ছে না

```
QLibrary::load: Cannot load C:/path/to/plugins/platforms/qwindows.dll
```

**সমাধান:**
```batch
# plugins/platforms/ ফোল্ডার তৈরি করে qwindows.dll কপি করুন
mkdir ReDroidCPP_v3\plugins\platforms
copy "C:\Qt\6.5.3\msvc2022_64\plugins\platforms\qwindows.dll" "ReDroidCPP_v3\plugins\platforms\"
```

**Application-এ Qt Plugin Path সেট করুন:**
```cpp
// main.cpp বা qtmain.cpp-তে যোগ করুন
QCoreApplication::addLibraryPath("plugins");
```

---

## ✅ বর্তমান বিল্ড কনফিগারেশন

### CMakeLists.txt থেকে সংগৃহীত তথ্য:

```cmake
# Qt6 Components
find_package(Qt6 COMPONENTS 
    Core 
    Gui 
    Widgets 
    Network 
    Concurrent 
    REQUIRED
)

# Link Libraries
target_link_libraries(ReDroidCPP PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Concurrent
    Threads::Threads
)

# Windows-specific
target_link_libraries(ReDroidCPP PRIVATE
    ws2_32
    advapi32
    userenv
    iphlpapi
    dnsapi
    crypt32
)
```

### Windows-এ প্রয়োজনীয় System DLLs:

| DLL | স্ট্যাটাস | টীকা |
|-----|---------|------|
| ws2_32.dll | ✅ Windows built-in | Windows Sockets |
| advapi32.dll | ✅ Windows built-in | Security APIs |
| userenv.dll | ✅ Windows built-in | User environment |
| iphlpapi.dll | ✅ Windows built-in | Network helper |
| dnsapi.dll | ✅ Windows built-in | DNS APIs |
| crypt32.dll | ✅ Windows built-in | Crypto APIs |

---

## 📦 OpenSSL/Security সম্পর্কিত

### বর্তমান ইমপ্লিমেন্টেশন:

প্রজেক্টে OpenSSL সরাসরি ব্যবহার না করে **custom stub implementation** ব্যবহার করে:

| ফাইল | বিবরণ |
|------|-------|
| `openssl_stub.cpp` | MD5, SHA256, SHA512 হ্যাশ ফাংশন |
| `openssl_stub.h` | Stub header definitions |

**এটি একটি ভালো অনুশীলন** কারণ:
- ✅ External OpenSSL DLL প্রয়োজন নেই
- ✅ Static linking possible
- ✅ Build সহজ হয়

---

## 🔧 বিল্ড রিলিজ ফোল্ডার স্ট্রাকচার

`build-release.bat` যা তৈরি করে:

```
ReDroidCPP_v3/
├── ReDroidCPP.exe          # Main executable
├── Qt6Core.dll            # ✅
├── Qt6Gui.dll             # ✅
├── Qt6Widgets.dll         # ✅
├── Qt6Network.dll         # ✅
├── Qt6Svg.dll             # ✅
├── Qt6Xml.dll             # ✅
├── VCRUNTIME140.dll       # ✅
├── VCRUNTIME140_1.dll     # ✅
├── MSVCP140.dll           # ✅
├── MSVCP140_1.dll         # ✅
├── plugins/
│   ├── platforms/
│   │   └── qwindows.dll   # ✅
│   ├── styles/
│   │   └── qwindowsvistastyle.dll  # ✅
│   └── iconengines/
│       └── qsvgicon.dll   # ✅
├── docker/
│   ├── Dockerfile
│   ├── docker-compose.yml
│   └── entrypoint.sh
├── profiles/
│   └── *.json
├── platform-tools/        # ADB (ডাউনলোড করতে হবে)
├── start.bat
├── adb-connect.bat
└── README.txt
```

---

## ⚠️ সম্ভাব্য Missing DLL সমস্যা

### 1. Qt6 Private Libraries

Qt6 এর কিছু internal class ব্যবহার করা হয়েছে:

```cmake
# CMakeLists.txt থেকে
find_package(Qt6 REQUIRED COMPONENTS CorePrivate GuiPrivate)
```

**প্রয়োজনীয়:**
| DLL | বিবরণ |
|-----|-------|
| Qt6CorePrivate.dll | Internal core classes |
| Qt6GuiPrivate.dll | Internal GUI classes |

### 2. OpenSSL (যদি পরে যোগ করা হয়)

যদি ভবিষ্যতে real OpenSSL ব্যবহার করা হয়:

| DLL | বিবরণ |
|-----|-------|
| libssl-3.dll | SSL/TLS library |
| libcrypto-3.dll | Crypto library |

### 3. Additional Qt Plugins

**যদি নিচের features ব্যবহার করা হয়:**

| Feature | প্রয়োজনীয় DLL/Plugin |
|---------|----------------------|
| Image formats | Qt6/plugins/imageformats/*.dll |
| Printing | Qt6/plugins/printsupport/*.dll |
| Direct3D | Qt6/plugins/platforms/qdirect3d*.dll |

---

## 🛠️ সম্পূর্ণ DLL ইনস্টলেশন গাইড

### ধাপ ১: Qt6 DLLs

```powershell
# Qt6 installation directory
$QT_DIR = "C:\Qt\6.5.3\msvc2022_64"

# Required DLLs from bin folder
$DLLs = @(
    "Qt6Core.dll",
    "Qt6Gui.dll", 
    "Qt6Widgets.dll",
    "Qt6Network.dll",
    "Qt6Concurrent.dll",
    "Qt6Svg.dll",
    "Qt6Xml.dll"
)

foreach ($dll in $DLLs) {
    if (Test-Path "$QT_DIR\bin\$dll") {
        Copy-Item "$QT_DIR\bin\$dll" "ReDroidCPP_v3\"
        Write-Host "[OK] $dll"
    } else {
        Write-Host "[MISSING] $dll" -ForegroundColor Red
    }
}
```

### ধাপ ২: Qt6 Plugins

```powershell
# Platform plugin
New-Item -ItemType Directory -Path "ReDroidCPP_v3\plugins\platforms" -Force
Copy-Item "$QT_DIR\plugins\platforms\qwindows.dll" "ReDroidCPP_v3\plugins\platforms\"

# Style plugin
New-Item -ItemType Directory -Path "ReDroidCPP_v3\plugins\styles" -Force
Copy-Item "$QT_DIR\plugins\styles\qwindowsvistastyle.dll" "ReDroidCPP_v3\plugins\styles\"

# Icon engine
New-Item -ItemType Directory -Path "ReDroidCPP_v3\plugins\iconengines" -Force
Copy-Item "$QT_DIR\plugins\iconengines\qsvgicon.dll" "ReDroidCPP_v3\plugins\iconengines\"
```

### ধাপ ৩: MSVC Runtime

```powershell
# VS 2022 Redistributable search locations
$RuntimeDLLs = @(
    "C:\Windows\System32\VCRUNTIME140.dll",
    "C:\Windows\System32\VCRUNTIME140_1.dll",
    "C:\Windows\System32\MSVCP140.dll",
    "C:\Windows\System32\MSVCP140_1.dll"
)

foreach ($dll in $RuntimeDLLs) {
    if (Test-Path $dll) {
        Copy-Item $dll "ReDroidCPP_v3\"
        Write-Host "[OK] $(Split-Path $dll -Leaf)"
    }
}
```

---

## 📊 Dependency Tree

```
ReDroidCPP.exe
│
├── Qt6Core.dll
│   └── Windows System Libraries (built-in)
│
├── Qt6Gui.dll
│   └── Qt6Core.dll
│
├── Qt6Widgets.dll
│   ├── Qt6Gui.dll
│   └── Qt6Core.dll
│
├── Qt6Network.dll
│   ├── Qt6Core.dll
│   ├── ws2_32.dll (Windows built-in)
│   └── dnsapi.dll (Windows built-in)
│
├── System Libraries (linked at compile time)
│   ├── ws2_32.dll
│   ├── advapi32.dll
│   ├── userenv.dll
│   ├── iphlpapi.dll
│   └── crypt32.dll
│
└── Plugins
    └── platforms/qwindows.dll
```

---

## ✅ সিদ্ধান্ত

### বর্তমান বিল্ডে কোনো Missing DLL সমস্যা নেই যদি:

1. ✅ `build-release.bat` সঠিকভাবে রান করা হয়
2. ✅ Qt6 সঠিক পাথে ইনস্টল করা আছে (`C:\Qt\6.5.3\msvc2022_64`)
3. ✅ Visual Studio 2022 Redistributable ইনস্টল করা আছে
4. ✅ Windows 10/11 64-bit ব্যবহার করা হচ্ছে

### সতর্কতা:

| সমস্যা | সমাধান |
|--------|--------|
| Qt6 not found | Qt6 MSVC 2022 ইনস্টল করুন |
| VCRUNTIME errors | VS 2022 Redistributable ইনস্টল করুন |
| Plugin not loading | `QT_PLUGIN_PATH` environment সেট করুন |
| Network issues | `ws2_32.dll` Windows Update দিয়ে আপডেট করুন |

---

## 📝 .gitignore-এ যোগ করা উচিত

```gitignore
# Build outputs
build/
ReDroidCPP_v3/

# Qt6 runtime (না কপি করুন)
*.dll

# Platform-tools (user download)
platform-tools/

# Config files
*.ini
*.json (project configs)
```

---

*রিপোর্ট তৈরি: 2026-08-05*

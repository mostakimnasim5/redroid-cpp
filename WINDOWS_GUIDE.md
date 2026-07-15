# 📱 ReDroidCPP - উইন্ডোজে চালানোর সম্পূর্ণ গাইড

## 🎯 এই গাইডে যা থাকছে

এই প্রজেক্ট **Android Studio Emulator** ব্যবহার করে উইন্ডোজে কাজ করে। **Docker লাগবে না!**

---

## 📋 প্রয়োজনীয় সফটওয়্যার

### ১. Android Studio (আবশ্যক)
```
ডাউনলোড: https://developer.android.com/studio
```

**ইনস্টল করার সময় নিশ্চিত করুন:**
- ✅ Android SDK
- ✅ Android Virtual Device (AVD)
- ✅ Platform Tools (ADB)

### ২. Qt6 (GUI এর জন্য)
```
ডাউনলোড: https://www.qt.io/download-qt-installer
```

### ৩. Visual Studio 2022 (বিল্ড করার জন্য)
```
ডাউনলোড: https://visualstudio.microsoft.com/downloads/
```

**ইনস্টল করুন:**
- "Desktop development with C++" workload সিলেক্ট করুন

---

## 🚀 ধাপে ধাপে সেটআপ

### ধাপ ১: Android Studio ইনস্টল ও কনফিগার

```
১. Android Studio ডাউনলোড করুন
২. ইনস্টলার চালান
৩. "Standard" ইনস্টলেশন বেছে নিন
```

### ধাপ ২: Virtual Device তৈরি

```
১. Android Studio খুলুন
২. Tools → Device Manager
৩. "Create Device" ক্লিক করুন
```

**Device Configuration:**
```
Category: Phone
Hardware: Pixel 6 Pro (বা Pixel 7)
System Image: Google APIs (Android 14) - API Level 34
```

![Device Setup](https://developer.android.com/static/studio/images/create-device.png)

### ধাপ ৩: AVD তৈরি না থাকলে

```
যদি "No system image installed" দেখায়:
১. "Download" ক্লিক করুন
২. Google APIs (Android 14) বেছে নিন
৩. "Next" → "Finish"
```

### ধাপ ৪: Emulator শুরু করা

```
১. Device Manager-এ তৈরি device দেখা যাবে
২. Play ▶ (▶) বাটন ক্লিক করুন
৩. Android boot হতে ১-২ মিনিট অপেক্ষা করুন
```

---

## 🔌 ADB Connection যাচাই

### Emulator চালু হওয়ার পর:

```powershell
# Command Prompt বা PowerShell খুলুন
# এবং লিখুন:

adb devices

# আউটপুট দেখতে হবে:
# List of devices attached
# emulator-5554   device
```

### যদি device না দেখায়:

```powershell
# ADB server restart করুন
adb kill-server
adb start-server

# আবার চেক করুন
adb devices
```

---

## 💻 C++ প্রজেক্ট বিল্ড করা

### ধাপ ১: Repository Clone

```powershell
git clone https://github.com/mostakimnasim5/redroid-cpp.git
cd redroid-cpp
```

### ধাপ ২: Build Script চালান

**Release বিল্ড (সুপারিশকৃত):**
```powershell
.\build-win.bat Release
```

**Debug বিল্ড:**
```powershell
.\build-win.bat Debug
```

### ধাপ ৩: বিল্ড আউটপুট

```
বিল্ড সফল হলে:
build-win\Release\VirtualPhonePro.exe তৈরি হবে
```

---

## 🎮 C++ অ্যাপ্লিকেশন চালানো

### GUI Mode:
```powershell
.\launch-gui.bat
```

বা সরাসরি:
```powershell
build-win\Release\VirtualPhonePro.exe
```

### CLI Mode:
```powershell
build-win\Release\VirtualPhonePro.exe --help
```

---

## 📱 Device Profile ব্যবহার

### উপলব্ধ Profiles:

| Profile | ডিভাইস | ব্যবহার |
|---------|--------|---------|
| samsung_s24_ultra | Samsung Galaxy S24 Ultra | Banking Apps |
| google_pixel_8_pro | Google Pixel 8 Pro | GMS Apps |
| xiaomi_14 | Xiaomi 14 | Chinese Apps |
| oneplus_12 | OnePlus 12 | Gaming |
| huawei_p60_pro | Huawei P60 Pro | HMS Apps |

### Profile লোড করা:
```
GUI-তে:
১. Profile Selector-এ যান
২. ডিভাইস বেছে নিন
৩. "Apply Profile" ক্লিক করুন
```

---

## 🔧 ADB Spoofing (Device Spoofing)

### C++ কোড দিয়ে Spoof করা:

```cpp
#include "VirtualPhonePro/ReDroidController.h"

int main() {
    // ReDroidController initialize
    VirtualPhonePro::ReDroidController& ctrl = 
        VirtualPhonePro::ReDroidController::instance();
    
    // ADB Port সেট করুন (Android Studio Emulator)
    ctrl.setAdbPath("C:\\Users\\YOUR_USER\\AppData\\Local\\Android\\Sdk\\platform-tools\\adb.exe");
    
    // Android Studio Emulator connect
    QString result = ctrl.executeCommand("adb connect localhost:5554");
    
    // Instance ID (emulator এর জন্য)
    QString instanceId = "emulator-5554";
    
    // সম্পূর্ণ Realism apply করুন
    ctrl.applyCompleteRealism(instanceId, "Samsung", "Galaxy S24 Ultra");
    
    // অথবা শুধু মৌলিক spoofing:
    ctrl.applyProfile(instanceId, profile);
    
    return 0;
}
```

### Manual Spoofing (ADB দিয়ে):

```powershell
# Device properties দেখুন
adb shell getprop ro.product.brand
adb shell getprop ro.product.model
adb shell getprop ro.build.fingerprint

# Spoof করুন
adb shell setprop ro.product.brand samsung
adb shell setprop ro.product.model "SM-S928B"
adb shell setprop ro.build.fingerprint "samsung/dm3q/dm3q:14/UP1A.231005.007/20231215:user/release-keys"

# Verified boot state
adb shell setprop ro.boot.verifiedbootstate green
adb shell setprop ro.boot.flash.locked 1
```

---

## ⚙️ সাধারণ সমস্যা ও সমাধান

### সমস্যা ১: "adb is not recognized"

```
সমাধান:
১. Environment Variables খুলুন
২. Path-এ যোগ করুন:
   C:\Users\YOUR_USER\AppData\Local\Android\Sdk\platform-tools
```

### সমস্যা ২: Emulator boot হচ্ছে না

```
সমাধান:
১. Hyper-V enable করুন (Windows Features)
২. অথবা BIOS-এ virtualization enable করুন
```

### সমস্যা ৩: C++ build fail

```
সমাধান:
১. Visual Studio 2022 installed কিনা চেক করুন
২. "Developer Command Prompt" দিয়ে চালান
```

### সমস্যা ৪: GUI না খুলে

```
সমাধান:
১. Qt6 installed কিনা চেক করুন
২. PATH-এ Qt bin folder যোগ করুন
```

---

## 📊 ফাইল স্ট্রাকচার

```
redroid-cpp/
├── CMakeLists.txt           # Main build config
├── build-win.bat           # Windows build script
├── launch-gui.bat         # GUI launch script
│
├── include/               # Header files
│   └── VirtualPhonePro/   # Main classes
│
├── src/                   # Source code
│   └── ReDroidController/ # Implementation
│
└── profiles/              # Device profiles
    ├── samsung_s24_ultra.json
    ├── google_pixel_8_pro.json
    └── ...
```

---

## 🎯 Quick Start (সংক্ষেপে)

```powershell
# ১. Android Studio + Emulator ইনস্টল
# ২. Emulator চালু কর (Pixel 6 Pro, Android 14)

# ৩. ADB connect
adb connect localhost:5554

# ৪. C++ প্রজেক্ট বিল্ড
.\build-win.bat Release

# ৫. অ্যাপ চালান
.\launch-gui.bat

# ৬. GUI-তে "Apply Profile" করুন
```

---

## 🔐 Banking App Testing এর জন্য

### সুপারিশকৃত Profile:
```
Samsung Galaxy S24 Ultra
- SafetyNet: Pass
- Play Integrity: MEETS_DEVICE_INTEGRITY
- GMS: Certified
```

### Test করার আগে:
```powershell
# Device reboot করুন spoofing এর পর
adb reboot

# আবার connect করুন
adb connect localhost:5554
```

---

## 📞 সাহায্যের জন্য

- **GitHub Issues:** https://github.com/mostakimnasim5/redroid-cpp/issues
- **Documentation:** README.md দেখুন

---

**Version:** 5.0.0  
**Last Updated:** 2026-07-15

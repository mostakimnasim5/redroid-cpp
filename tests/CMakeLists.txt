# Test Suite CMake Configuration
cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(WIN32)
    set(CMAKE_CXX_STANDARD 20)
endif()

# Required for Q_OBJECT in test classes
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6
find_package(Qt6 COMPONENTS Test Core Widgets Network Concurrent REQUIRED)
find_package(Threads REQUIRED)

# Enable testing
enable_testing()

# Windows: Define WIN32_LEAN_AND_MEAN to reduce header size and avoid conflicts
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
endif()

# Qt6 on Windows needs this for proper linking
if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
endif()

# ReDroidController sources for tests
set(REDROID_TEST_SOURCES
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/ReDroidController.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/DeviceProfile.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/EnhancedDeviceProfile.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/RealisticDeviceProfile.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/RealisticProfileGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/UniqueDeviceGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/DeviceIDGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/PersistentIdentityManager.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/DeviceBehaviorManager.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/AdvancedAntiDetection.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/AdvancedSpoofing.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/AdvancedRealisticSimulation.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/BankingAppSpoofer.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/GoogleFacebookSpoofer.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/DeepDeviceSpoofer.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/TLSFingerprint.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/HardwareFingerprintSpoofer.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/HardwareAttestation.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/HALSimulation.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/SafetyNetSpoofer.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/SafetyNetAdvancedBypass.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/CryptoEmulator.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/CryptoUtils.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/VirtualSecurityChip.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/DeviceIntegrityManager.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/PlayIntegrityManager.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/SSLCertificateManager.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/NetworkStackSpoofer.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/NetworkProfileManager.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/NetworkRealismEnhancer.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/NetworkConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/CarrierNetworkSimulator.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/OEMDeepSpoofing.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/EmulatorDetectionBypass.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/HypervisorBypass.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/RealPhoneHardening.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/FridaXposedDetector.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/MagiskPatcher.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/SELinuxManager.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/HyperRealisticTouchEmulator.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/SensorSimulator.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/BatteryPowerManager.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/TimingAttackPrevention.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/AndroidRealismEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/SystemAppSimulator.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/MultiInstanceManager.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/ADBManager.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/APIServer.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/AppCloner.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/ScreenMirror.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/ScreenStateManager.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/IPTimezoneConverter.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/HttpClient.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/WebhookManager.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/FindMyDeviceManager.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/Logger.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/TestingFramework.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/SecurityMitigationManager.cpp
    ${CMAKE_SOURCE_DIR}/src/Android/LocaleTimezoneManager.cpp
    ${CMAKE_SOURCE_DIR}/src/Data/TACDatabase.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/openssl_stub.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/FileLogger.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/ConfigManager.cpp
    ${CMAKE_SOURCE_DIR}/src/Database/FingerprintEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/Android/SensorInjector.cpp
    ${CMAKE_SOURCE_DIR}/src/Android/FileSystemRealism.cpp
    # Hardware-anchored identity engine: MultiInstanceManager/AppCloner (in
    # this list) reference generateUniqueHardwareAnchoredIdentity(),
    # applyIdentityToDeviceProfile() and registerIssuedIdentity() — without
    # these three sources the test executables fail to link (LNK2001).
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/DeviceProfileGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/HardwareId.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/ProfileGeneratorFactory.cpp
)

# =========================================================
# Test 1: Detection Bypass Test Suite (main tests)
# =========================================================
add_executable(Test_DetectionBypass
    Test_DetectionBypass.cpp
)

target_link_libraries(Test_DetectionBypass PRIVATE
    Qt6::Test
    Qt6::Core
    Qt6::Network
    Threads::Threads
)

target_include_directories(Test_DetectionBypass PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Set console subsystem for Windows (required for Qt Test)
if(WIN32)
    set_target_properties(Test_DetectionBypass PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
endif()

add_test(NAME DetectionBypass COMMAND Test_DetectionBypass)

# =========================================================
# Test 2: Unique Device Generator Tests
# =========================================================
add_executable(Test_UniqueDeviceGenerator
    Test_UniqueDeviceGenerator.cpp
    ${REDROID_TEST_SOURCES}
    # Add all Q_OBJECT headers so AUTOMOC can process them
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/ReDroidController.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/HardwareFingerprintSpoofer.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/PlayIntegrityManager.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/NetworkProfileManager.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/EmulatorDetectionBypass.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/MagiskPatcher.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/SELinuxManager.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/SensorSimulator.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/TimingAttackPrevention.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/WebhookManager.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/MultiInstanceManager.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/SecurityMitigationManager.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/APIServer.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/AppCloner.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/TestingFramework.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/ConfigManager.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/FileLogger.hpp
    ${CMAKE_SOURCE_DIR}/include/Android/SensorInjector.h
    ${CMAKE_SOURCE_DIR}/include/Android/FileSystemRealism.h
    ${CMAKE_SOURCE_DIR}/include/Android/LocaleTimezoneManager.h
)

target_link_libraries(Test_UniqueDeviceGenerator PRIVATE
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Concurrent
    Threads::Threads
)

target_include_directories(Test_UniqueDeviceGenerator PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/ReDroidController
    ${CMAKE_SOURCE_DIR}/src/Data
    ${CMAKE_SOURCE_DIR}/src/GUI
    ${Qt6_INCLUDE_DIRS}
)

# Force Qt to process all headers with Q_OBJECT
set_target_properties(Test_UniqueDeviceGenerator PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

# Set console subsystem for Windows (required for Qt Test)
if(WIN32)
    set_target_properties(Test_UniqueDeviceGenerator PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
endif()

add_test(NAME UniqueDeviceGenerator COMMAND Test_UniqueDeviceGenerator)

# =========================================================
# Test 3: Validation Tests (Login, Proxy, VPN, DNS)
# =========================================================
add_executable(Test_Validation
    Test_Validation.cpp
)

target_link_libraries(Test_Validation PRIVATE
    Qt6::Test
    Qt6::Core
    Qt6::Network
    Threads::Threads
)

target_include_directories(Test_Validation PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Set console subsystem for Windows (required for Qt Test)
if(WIN32)
    set_target_properties(Test_Validation PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
endif()

add_test(NAME Validation COMMAND Test_Validation)

# =========================================================
# Test 4: Device Profile Generator Tests
# =========================================================
add_executable(Test_ProfileGenerator
    Test_ProfileGenerator.cpp
    ${REDROID_TEST_SOURCES}
    # Add all Q_OBJECT headers so AUTOMOC can process them
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/ReDroidController.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/HardwareFingerprintSpoofer.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/PlayIntegrityManager.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/NetworkProfileManager.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/EmulatorDetectionBypass.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/MagiskPatcher.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/SELinuxManager.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/SensorSimulator.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/TimingAttackPrevention.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/WebhookManager.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/MultiInstanceManager.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/SecurityMitigationManager.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/APIServer.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/AppCloner.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/TestingFramework.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/ConfigManager.hpp
    ${CMAKE_SOURCE_DIR}/include/VirtualPhonePro/FileLogger.hpp
    ${CMAKE_SOURCE_DIR}/include/Android/SensorInjector.h
    ${CMAKE_SOURCE_DIR}/include/Android/FileSystemRealism.h
    ${CMAKE_SOURCE_DIR}/include/Android/LocaleTimezoneManager.h
)

target_link_libraries(Test_ProfileGenerator PRIVATE
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Concurrent
    Threads::Threads
)

target_include_directories(Test_ProfileGenerator PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/ReDroidController
    ${CMAKE_SOURCE_DIR}/src/Data
    ${CMAKE_SOURCE_DIR}/src/GUI
    ${Qt6_INCLUDE_DIRS}
)

# Force Qt to process all headers with Q_OBJECT
set_target_properties(Test_ProfileGenerator PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

# Set console subsystem for Windows (required for Qt Test)
if(WIN32)
    set_target_properties(Test_ProfileGenerator PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
endif()

add_test(NAME ProfileGenerator COMMAND Test_ProfileGenerator)

# =========================================================
# Test 5: Carrier Selection (multi-carrier, deterministic, MCC/MNC validity)
# Compiles ONLY LocaleTimezoneManager.cpp with LTM_NO_CONTROLLER to isolate
# from ReDroidController (executes ADB / device shell). No network, no proxy.
# =========================================================
add_executable(Test_CarrierSelection
    Test_CarrierSelection.cpp
    ${CMAKE_SOURCE_DIR}/src/Android/LocaleTimezoneManager.cpp
    # Q_OBJECT header must be listed so AUTOMOC generates moc_LocaleTimezoneManager.cpp
    ${CMAKE_SOURCE_DIR}/include/Android/LocaleTimezoneManager.h
)

target_compile_definitions(Test_CarrierSelection PRIVATE LTM_NO_CONTROLLER)

target_link_libraries(Test_CarrierSelection PRIVATE
    Qt6::Test
    Qt6::Core
    Qt6::Network
    Threads::Threads
)

target_include_directories(Test_CarrierSelection PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

if(WIN32)
    set_target_properties(Test_CarrierSelection PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
endif()

add_test(NAME CarrierSelection COMMAND Test_CarrierSelection)

# =========================================================
# Test 6: WebRTC/Cellular/Local IP consistency
# Compiles ONLY DeterministicIP.hpp (header-only — the exact code path used
# by ReDroidController::deterministicLocalIP) + DeviceProfileGenerator.cpp
# (self-contained, Qt-free HWID-anchored engine). No network, no proxy,
# no ADB, no Docker.
# =========================================================
add_executable(Test_WebRTCIPConsistency
    Test_WebRTCIPConsistency.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/DeviceProfileGenerator.cpp
)

target_link_libraries(Test_WebRTCIPConsistency PRIVATE
    Qt6::Test
    Qt6::Core
    Threads::Threads
)

target_include_directories(Test_WebRTCIPConsistency PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

if(WIN32)
    set_target_properties(Test_WebRTCIPConsistency PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
endif()

add_test(NAME WebRTCIPConsistency COMMAND Test_WebRTCIPConsistency)

# =========================================================
# Test 7: WiFi Network Identity (ISP/residential proxy mode)
# Same LTM_NO_CONTROLLER isolation as Test_CarrierSelection — only the pure
# generateWifiNetworkConfig() is exercised; no network, no proxy, no ADB.
# =========================================================
add_executable(Test_WifiNetworkIdentity
    Test_WifiNetworkIdentity.cpp
    ${CMAKE_SOURCE_DIR}/src/Android/LocaleTimezoneManager.cpp
    # Q_OBJECT header must be listed so AUTOMOC generates moc_LocaleTimezoneManager.cpp
    ${CMAKE_SOURCE_DIR}/include/Android/LocaleTimezoneManager.h
)

target_compile_definitions(Test_WifiNetworkIdentity PRIVATE LTM_NO_CONTROLLER)

target_link_libraries(Test_WifiNetworkIdentity PRIVATE
    Qt6::Test
    Qt6::Core
    Qt6::Network
    Threads::Threads
)

target_include_directories(Test_WifiNetworkIdentity PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

add_test(NAME WifiNetworkIdentity COMMAND Test_WifiNetworkIdentity)

# =========================================================
# Test 8: Proxy-type + network-kind propagation
# Same LTM_NO_CONTROLLER isolation: only LocaleTimezoneManager.cpp. Asserts
# the GUI combo's proxy-type string round-trips through setProxy/state-JSON
# (the value QNetworkProxy + redsocks branch on) and that the WiFi-vs-Cellular
# kind mapping mirrors the controller. No network, no proxy, no ADB.
# =========================================================
add_executable(Test_ProxyTypeKindPropagation
    Test_ProxyTypeKindPropagation.cpp
    ${CMAKE_SOURCE_DIR}/src/Android/LocaleTimezoneManager.cpp
    # Q_OBJECT header must be listed so AUTOMOC generates moc_LocaleTimezoneManager.cpp
    ${CMAKE_SOURCE_DIR}/include/Android/LocaleTimezoneManager.h
)

target_compile_definitions(Test_ProxyTypeKindPropagation PRIVATE LTM_NO_CONTROLLER)

target_link_libraries(Test_ProxyTypeKindPropagation PRIVATE
    Qt6::Test
    Qt6::Core
    Qt6::Network
    Threads::Threads
)

target_include_directories(Test_ProxyTypeKindPropagation PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

add_test(NAME ProxyTypeKindPropagation COMMAND Test_ProxyTypeKindPropagation)

# =========================================================
# Copy test profiles
# =========================================================
if(EXISTS "${CMAKE_SOURCE_DIR}/profiles")
    file(COPY ${CMAKE_SOURCE_DIR}/profiles/
         DESTINATION ${CMAKE_BINARY_DIR}/profiles/)
endif()

message(STATUS "Tests configured: DetectionBypass + UniqueDeviceGenerator + Validation + ProfileGenerator")

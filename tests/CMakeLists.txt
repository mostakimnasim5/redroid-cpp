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
    add_definitions(-DWIN32_LEAN_AND_MEAN -DNOGDI -DNOUSER)
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
    ${CMAKE_SOURCE_DIR}/src/Data/TACDatabase.cpp
    ${CMAKE_SOURCE_DIR}/src/ReDroidController/openssl_stub.cpp
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

add_test(NAME DetectionBypass COMMAND Test_DetectionBypass)

# =========================================================
# Test 2: Unique Device Generator Tests
# =========================================================
add_executable(Test_UniqueDeviceGenerator
    Test_UniqueDeviceGenerator.cpp
    ${REDROID_TEST_SOURCES}
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

add_test(NAME UniqueDeviceGenerator COMMAND Test_UniqueDeviceGenerator)

# =========================================================
# Copy test profiles
# =========================================================
if(EXISTS "${CMAKE_SOURCE_DIR}/profiles")
    file(COPY ${CMAKE_SOURCE_DIR}/profiles/
         DESTINATION ${CMAKE_BINARY_DIR}/profiles/)
endif()

message(STATUS "Tests configured: DetectionBypass + UniqueDeviceGenerator")

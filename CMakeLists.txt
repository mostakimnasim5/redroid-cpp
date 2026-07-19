cmake_minimum_required(VERSION 3.20)
project(RedroidCPP VERSION 3.0.0 LANGUAGES CXX)

# ==============================================================================
# Project Configuration
# ==============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==============================================================================
# Qt Auto-generation (MOC/UIC/RCC) - required for Q_OBJECT classes
# ==============================================================================
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Version info
set(PROJECT_VERSION_MAJOR 3)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ==============================================================================
# Platform Detection
# ==============================================================================
if(WIN32)
    message(STATUS "Building for Windows")
    set(WINDOWS 1)
    # Windows 10/11 API
    add_definitions(-DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00)
elseif(UNIX AND NOT APPLE)
    message(STATUS "Building for Linux")
    set(LINUX 1)
elseif(APPLE)
    message(STATUS "Building for macOS")
    set(APPLE 1)
endif()

# ==============================================================================
# Compiler Flags
# ==============================================================================
if(MSVC)
    add_compile_options(
        /W3           # Reasonable warning level
        /WX-          # Warnings NOT errors
        /permissive-  # Strict conformance
        /EHsc         # Exception handling
        /utf-8        # UTF-8 source
        /wd4100       # Unreferenced formal parameter
        /wd4189       # Local variable initialized but not referenced
        /wd4505       # Unreferenced local function
        /wd4702       # Unreachable code
        /wd4996       # Deprecated functions
        /wd4458       # Declaration hides class member
        /wd4457       # Declaration hides function parameter
        /wd4456       # Declaration hides previous local
    )
    # Lean and mean
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)                          # prevent Windows min/max macros conflicting with std::min/std::max
    add_definitions(-D_USE_MATH_DEFINES)                 # enable M_PI in <cmath> on MSVC
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
    
    # Release optimizations
    add_compile_options(
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Release>:/Oi>
        $<$<CONFIG:Release>:/Ot>
        $<$<CONFIG:Release>:/GL>
    )
    add_link_options(
        $<$<CONFIG:Release>:/LTCG>
        $<$<CONFIG:Release>:/OPT:REF>
        $<$<CONFIG:Release>:/OPT:ICF>
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
    )
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -march=native)
    endif()
endif()

# ==============================================================================
# Qt6 GUI Application
# ==============================================================================
option(BUILD_QT6_GUI "Build Qt6 GUI application" ON)
option(BUILD_CLI "Build CLI tool" OFF)
option(BUNDLE_QT "Bundle Qt DLLs with executable" OFF)

if(BUILD_QT6_GUI)
    # Find Qt6
    set(QT_MIN_VERSION "6.5.0")
    
    # Find Qt6 - works on both Linux and Windows CI (install-qt-action sets CMAKE_PREFIX_PATH)
    find_package(Qt6 COMPONENTS Core Gui Widgets Network Concurrent REQUIRED)
    
    message(STATUS "Found Qt6: ${Qt6_DIR}")
    
    # =========================================================================
    # ALL ReDroidController Source Files
    # =========================================================================
    set(REDROID_SOURCES
        # Core Controller
        src/ReDroidController/ReDroidController.cpp
        
        # Device Profiles
        src/ReDroidController/DeviceProfile.cpp
        src/ReDroidController/EnhancedDeviceProfile.cpp
        src/ReDroidController/RealisticDeviceProfile.cpp
        src/ReDroidController/RealisticProfileGenerator.cpp
        
        # Anti-Detection Modules
        src/ReDroidController/AdvancedAntiDetection.cpp
        src/ReDroidController/AdvancedSpoofing.cpp
        src/ReDroidController/AdvancedRealisticSimulation.cpp
        src/ReDroidController/BankingAppSpoofer.cpp
        src/ReDroidController/GoogleFacebookSpoofer.cpp
        src/ReDroidController/DeepDeviceSpoofer.cpp
        src/ReDroidController/TLSFingerprint.cpp
        
        # Hardware Spoofing
        src/ReDroidController/HardwareFingerprintSpoofer.cpp
        src/ReDroidController/HardwareAttestation.cpp
        src/ReDroidController/HALSimulation.cpp
        
        # Security Modules
        src/ReDroidController/SafetyNetSpoofer.cpp
        src/ReDroidController/SafetyNetAdvancedBypass.cpp
        src/ReDroidController/CryptoEmulator.cpp
        src/ReDroidController/CryptoUtils.cpp
        src/ReDroidController/VirtualSecurityChip.cpp
        src/ReDroidController/DeviceIntegrityManager.cpp
        src/ReDroidController/PlayIntegrityManager.cpp
        src/ReDroidController/SSLCertificateManager.cpp
        
        # Network & Spoofing
        src/ReDroidController/NetworkStackSpoofer.cpp
        src/ReDroidController/NetworkProfileManager.cpp
        src/ReDroidController/NetworkRealismEnhancer.cpp
        src/ReDroidController/NetworkConfig.cpp
        src/ReDroidController/CarrierNetworkSimulator.cpp
        src/ReDroidController/OEMDeepSpoofing.cpp
        
        # Emulator Bypass
        src/ReDroidController/EmulatorDetectionBypass.cpp
        src/ReDroidController/HypervisorBypass.cpp
        src/ReDroidController/RealPhoneHardening.cpp
        src/ReDroidController/FridaXposedDetector.cpp
        src/ReDroidController/MagiskPatcher.cpp
        src/ReDroidController/SELinuxManager.cpp
        
        # Identity & Device
        src/ReDroidController/UniqueDeviceGenerator.cpp
        src/ReDroidController/DeviceIDGenerator.cpp
        src/ReDroidController/PersistentIdentityManager.cpp
        src/ReDroidController/DeviceBehaviorManager.cpp
        
        # Simulation
        src/ReDroidController/HyperRealisticTouchEmulator.cpp
        src/ReDroidController/SensorSimulator.cpp
        src/ReDroidController/BatteryPowerManager.cpp
        src/ReDroidController/TimingAttackPrevention.cpp
        src/ReDroidController/AndroidRealismEngine.cpp
        src/ReDroidController/SystemAppSimulator.cpp
        
        # Utilities
        src/ReDroidController/MultiInstanceManager.cpp
        src/ReDroidController/ADBManager.cpp
        src/ReDroidController/APIServer.cpp
        src/ReDroidController/AppCloner.cpp
        src/ReDroidController/ScreenMirror.cpp
        src/ReDroidController/ScreenStateManager.cpp
        src/ReDroidController/IPTimezoneConverter.cpp
        src/ReDroidController/HttpClient.cpp
        src/ReDroidController/WebhookManager.cpp
        src/ReDroidController/FindMyDeviceManager.cpp
        src/ReDroidController/Logger.cpp
        src/ReDroidController/TestingFramework.cpp
        src/ReDroidController/SecurityMitigationManager.cpp
        
        # Data Layer
        src/Data/TACDatabase.cpp
        
        # OpenSSL Stub
        src/ReDroidController/openssl_stub.cpp
        
        # GUI Windows
        src/GUI/PhoneWindow.cpp
        src/GUI/DashboardWindow.cpp
        src/GUI/AntiDetectionPanel.cpp
        
        # Main App
        src/qtmain.cpp
    )
    
    # =========================================================================
    # ALL Headers
    # =========================================================================
    set(REDROID_HEADERS
        include/VirtualPhonePro/ReDroidController.h
        include/VirtualPhonePro/DeviceProfile.h
        include/VirtualPhonePro/EnhancedDeviceProfile.h
        include/VirtualPhonePro/BankingAppSpoofer.h
        include/VirtualPhonePro/GoogleFacebookSpoofer.h
        include/VirtualPhonePro/DeepDeviceSpoofer.h
        include/VirtualPhonePro/UniqueDeviceGenerator.h
        include/VirtualPhonePro/AdvancedAntiDetection.hpp
        include/VirtualPhonePro/TLSFingerprint.hpp
        include/VirtualPhonePro/HyperRealisticTouchEmulator.hpp
        include/VirtualPhonePro/NetworkProfileManager.h
        include/VirtualPhonePro/RealisticDeviceProfile.hpp
        include/VirtualPhonePro/MultiInstanceManager.h
        include/VirtualPhonePro/HardwareFingerprintSpoofer.h
        include/VirtualPhonePro/SafetyNetSpoofer.h
        include/VirtualPhonePro/SafetyNetAdvancedBypass.hpp
        include/VirtualPhonePro/HypervisorBypass.hpp
        include/VirtualPhonePro/RealPhoneHardening.hpp
        include/VirtualPhonePro/VirtualSecurityChip.hpp
        include/VirtualPhonePro/CryptoEmulator.hpp
        include/VirtualPhonePro/AdvancedSpoofing.hpp
        include/VirtualPhonePro/RealisticProfileGenerator.hpp
        include/VirtualPhonePro/ADBManager.hpp
        include/VirtualPhonePro/DeviceIDGenerator.hpp
        include/VirtualPhonePro/IPTimezoneConverter.hpp
        include/VirtualPhonePro/PlayIntegrityManager.hpp
        include/VirtualPhonePro/EmulatorDetectionBypass.hpp
        include/VirtualPhonePro/AndroidRealismEngine.h
        include/VirtualPhonePro/NetworkStackSpoofer.hpp
        include/VirtualPhonePro/TimingAttackPrevention.hpp
        include/VirtualPhonePro/CarrierNetworkSimulator.h
        include/VirtualPhonePro/OEMDeepSpoofing.h
        include/VirtualPhonePro/SensorSimulator.h
        include/VirtualPhonePro/BatteryPowerManager.h
        include/VirtualPhonePro/SSLCertificateManager.h
        include/VirtualPhonePro/PersistentIdentityManager.h
        include/VirtualPhonePro/DeviceBehaviorManager.h
        include/VirtualPhonePro/AppCloner.h
        include/VirtualPhonePro/ScreenMirror.hpp
        include/VirtualPhonePro/ScreenStateManager.h
        include/VirtualPhonePro/FindMyDeviceManager.h
        include/VirtualPhonePro/Logger.hpp
        include/VirtualPhonePro/HttpClient.hpp
        include/VirtualPhonePro/WebhookManager.h
        include/VirtualPhonePro/APIServer.h
        include/VirtualPhonePro/DeviceIntegrityManager.h
        include/VirtualPhonePro/HardwareAttestation.h
        include/VirtualPhonePro/HALSimulation.h
        include/VirtualPhonePro/TestingFramework.h
        include/VirtualPhonePro/NetworkRealismEnhancer.h
        include/VirtualPhonePro/CryptoUtils.hpp
        include/VirtualPhonePro/SecurityMitigationManager.h
        include/VirtualPhonePro/FridaXposedDetector.h
        include/VirtualPhonePro/MagiskPatcher.h
        include/VirtualPhonePro/SELinuxManager.hpp
        include/VirtualPhonePro/AdvancedRealisticSimulation.h
        include/VirtualPhonePro/NetworkConfig.h
        include/Data/TACDatabase.h
        src/GUI/PhoneWindow.h
        src/GUI/DashboardWindow.h
        src/GUI/AntiDetectionPanel.h
        src/ReDroidController/openssl_stub.h
    )
    
    # Threads
    find_package(Threads REQUIRED)

    # =========================================================================
    # Main Executable - WIN32 for no console
    # =========================================================================
    add_executable(ReDroidCPP WIN32
        ${REDROID_SOURCES}
        ${REDROID_HEADERS}
    )
    
    # Include directories
    target_include_directories(ReDroidCPP PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/GUI
        ${CMAKE_SOURCE_DIR}/src/Data
        ${CMAKE_SOURCE_DIR}/src/ReDroidController
        ${Qt6_INCLUDE_DIRS}
    )
    
    # Link Qt6 libraries
    target_link_libraries(ReDroidCPP PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        Qt6::Concurrent
        Threads::Threads
    )
    
    # Windows-specific libraries
    if(WIN32)
        target_link_libraries(ReDroidCPP PRIVATE
            ws2_32
            advapi32
            userenv
            iphlpapi
            dnsapi
            crypt32
        )
        
        # Set output name
        set_target_properties(ReDroidCPP PROPERTIES
            OUTPUT_NAME "ReDroidCPP"
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
            PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        )
        
        # Bundle Qt if enabled
        if(BUNDLE_QT)
            if(WIN32)
                find_program(WINDEPLOYQT_EXECUTABLE windeployqt
                    HINTS "${Qt6_DIR}/../../../bin"
                    "$ENV{QTDIR}/bin"
                )
                if(WINDEPLOYQT_EXECUTABLE)
                    add_custom_command(TARGET ReDroidCPP POST_BUILD
                        COMMAND ${WINDEPLOYQT_EXECUTABLE}
                            --no-translations
                            --no-opengl-sw
                            --release
                            "$<TARGET_FILE:ReDroidCPP>"
                        COMMENT "Bundling Qt DLLs with windeployqt"
                    )
                    message(STATUS "windeployqt found: ${WINDEPLOYQT_EXECUTABLE}")
                else()
                    message(WARNING "windeployqt not found!")
                endif()
            endif()
            if(WIN32) # original block below
                # Find Qt6 deployment plugin
                find_package(Qt6 REQUIRED COMPONENTS CorePrivate GuiPrivate)
                
                # Get Qt bin directory
                get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
                get_filename_component(QT_DIR "${QT_QMAKE_EXECUTABLE}" PATH)
                get_filename_component(QT_DIR "${QT_DIR}/.." PATH)
                
                set(QT_BIN_DIR "${QT_DIR}/bin")
                
                # Copy Qt DLLs (will be done post-build)
                message(STATUS "Qt binaries: ${QT_BIN_DIR}")
            endif()
        endif()
    endif()
    
    message(STATUS "Building ReDroidCPP v${PROJECT_VERSION} with Qt6 GUI")
endif()

# ==============================================================================
# CLI Tool (Optional)
# ==============================================================================
if(BUILD_CLI)
    set(CLI_SOURCES src/main.cpp)
    
    add_executable(redroid-cli ${CLI_SOURCES})
    
    target_include_directories(redroid-cli PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    
    find_package(Threads REQUIRED)
    target_link_libraries(redroid-cli PRIVATE Threads::Threads)
    
    if(WIN32)
        target_link_libraries(redroid-cli PRIVATE ws2_32 advapi32 userenv)
    endif()
endif()

# ==============================================================================
# Installation
# ==============================================================================
install(TARGETS ReDroidCPP DESTINATION bin)
install(DIRECTORY profiles/ DESTINATION share/redroid/profiles)
install(DIRECTORY docker/ DESTINATION share/redroid/docker)

# ==============================================================================
# Testing
# ==============================================================================
enable_testing()
add_subdirectory(tests)

# ==============================================================================
# Docker Build Target
# ==============================================================================
find_program(DOCKER_EXECUTABLE docker)
if(DOCKER_EXECUTABLE)
    add_custom_target(docker-build
        COMMAND ${DOCKER_EXECUTABLE} build 
            -t redroid-cpp/emulator:${PROJECT_VERSION} 
            -f ${CMAKE_SOURCE_DIR}/docker/Dockerfile 
            ${CMAKE_SOURCE_DIR}
        COMMENT "Building Docker images for emulator"
        VERBATIM
    )
    
    add_custom_target(docker-up
        COMMAND ${DOCKER_EXECUTABLE} compose 
            -f ${CMAKE_SOURCE_DIR}/docker/docker-compose.yml 
            up -d
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/docker
        COMMENT "Starting emulator with docker compose"
        VERBATIM
    )
    
    add_custom_target(docker-down
        COMMAND ${DOCKER_EXECUTABLE} compose 
            -f ${CMAKE_SOURCE_DIR}/docker/docker-compose.yml 
            down
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/docker
        COMMENT "Stopping emulator"
        VERBATIM
    )
    
    message(STATUS "Docker support enabled")
else()
    message(STATUS "Docker not found - Docker build targets disabled")
endif()

# ==============================================================================
# Build Summary
# ==============================================================================
message(STATUS "")
message(STATUS "==============================================")
message(STATUS "  ReDroidCPP v${PROJECT_VERSION} Build Configuration")
message(STATUS "==============================================")
message(STATUS "  Platform:     ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler:     ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "  Qt6 GUI:      ${BUILD_QT6_GUI}")
message(STATUS "  CLI Tool:     ${BUILD_CLI}")
if(WIN32)
message(STATUS "  Output:       ${CMAKE_BINARY_DIR}/bin/ReDroidCPP.exe")
else()
message(STATUS "  Output:       ${CMAKE_BINARY_DIR}/bin/ReDroidCPP")
endif()
message(STATUS "==============================================")
message(STATUS "")

# ==============================================================================
# CRITICAL: MSVC Static Runtime Library - MUST BE BEFORE project()
# This eliminates dependency on MSVCP140.dll, MSVCP140_1.dll, 
# VCRUNTIME140.dll, VCRUNTIME140_1.dll
# ==============================================================================
if(MSVC)
    # MultiThreaded = /MT (static runtime, no DLL dependency)
    # MultiThreadedDebug = /MTd (static debug runtime)
    # This MUST be set before project() for proper propagation
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded" CACHE STRING "MSVC Runtime Library")
    message(STATUS "MSVC Runtime Library: ${CMAKE_MSVC_RUNTIME_LIBRARY}")
endif()

cmake_minimum_required(VERSION 3.20)
project(RedroidCPP VERSION 3.0.0 LANGUAGES CXX)

# ==============================================================================
# Project Configuration
# ==============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Windows: Use C++20 for designated initializer support
if(WIN32)
    set(CMAKE_CXX_STANDARD 20)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==============================================================================
# Qt Auto-generation (MOC/UIC/RCC) - required for Q_OBJECT classes
# ==============================================================================
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Windows: Define WIN32_LEAN_AND_MEAN to reduce header size and avoid conflicts
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN -DNOGDI -DNOUSER -DWIN32)
endif()

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
    find_package(Qt6 COMPONENTS Core Gui Widgets Network Concurrent OpenGL OpenGLWidgets REQUIRED)

    # ── FFmpeg (libavcodec + libavutil + libswscale) ──────────────────────────
    # Strategy:
    #   1. pkg-config / system install (fastest — already on the machine)
    #   2. vcpkg / conda manual search
    #   3. FetchContent: download pre-built FFmpeg shared libs from
    #      BtbN/FFmpeg-Builds (Windows) or static build (Linux/macOS)
    #      so the user never has to install anything manually.

    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(FFMPEG IMPORTED_TARGET
            libavcodec libavutil libswscale)
    endif()

    if(NOT FFMPEG_FOUND)
        find_library(AVCODEC_LIB avcodec)
        find_library(AVUTIL_LIB  avutil)
        find_library(SWSCALE_LIB swscale)
        if(AVCODEC_LIB AND AVUTIL_LIB AND SWSCALE_LIB)
            set(FFMPEG_FOUND TRUE)
            message(STATUS "FFmpeg found via manual search")
        endif()
    endif()

    if(NOT FFMPEG_FOUND)
        message(STATUS "FFmpeg not found locally — downloading pre-built binaries...")
        include(FetchContent)

        if(WIN32)
            # BtbN Windows builds: shared libs + headers + MSVC import libs.
            # We pin to the "master-latest" track because the older
            # "n7.1-latest-win64-lgpl-shared-7.1.zip" asset was removed from
            # BtbN's rolling release (HTTP 404). URL_HASH is intentionally
            # omitted: BtbN's "latest" tag is a rolling daily release whose
            # file content (and therefore hash) changes every day, so a
            # pinned hash would break within 24h. CMake accepts a download
            # with no hash (it skips verification). An empty "SHA256=" value
            # is NOT valid and aborts configuration, hence the omission.
            #
            # Archive layout: ffmpeg-master-latest-win64-lgpl-shared/
            #                  ├─ include/libavcodec/avcodec.h
            #                  ├─ lib/avcodec.lib   (MSVC import lib)
            #                  └─ bin/avcodec-*.dll
            FetchContent_Declare(ffmpeg_prebuilt
                URL "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-lgpl-shared.zip"
                DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            )
        elseif(LINUX)
            FetchContent_Declare(ffmpeg_prebuilt
                URL "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-linux64-lgpl-shared.tar.xz"
                DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            )
        endif()

        if(WIN32 OR LINUX)
            FetchContent_MakeAvailable(ffmpeg_prebuilt)
            set(FFMPEG_PREBUILT_DIR "${ffmpeg_prebuilt_SOURCE_DIR}")
            message(STATUS "FFmpeg pre-built source dir: ${FFMPEG_PREBUILT_DIR}")

            # BtbN archives extract to a single top-level directory whose
            # name embeds the track (master-latest / n7.1-latest / ...).
            # FetchContent may also flatten the archive straight into
            # SOURCE_DIR (observed on Windows), so the glob can be empty.
            # Resolve the extracted root only when a subdir actually exists;
            # otherwise fall back to SOURCE_DIR itself (the find_* calls
            # below already list it, so detection still works).
            file(GLOB FFMPEG_TOP_DIRS LIST_DIRECTORIES TRUE
                "${FFMPEG_PREBUILT_DIR}/ffmpeg-master-latest-win64-lgpl-shared"
                "${FFMPEG_PREBUILT_DIR}/ffmpeg-master-latest-linux64-lgpl-shared"
                "${FFMPEG_PREBUILT_DIR}/ffmpeg-n*-win64-lgpl-shared-*"
                "${FFMPEG_PREBUILT_DIR}/ffmpeg-n*-linux64-lgpl-shared-*"
            )
            list(LENGTH FFMPEG_TOP_DIRS _FFMPEG_TOP_COUNT)
            if(_FFMPEG_TOP_COUNT GREATER 0)
                list(GET FFMPEG_TOP_DIRS 0 FFMPEG_EXTRACTED_ROOT)
            else()
                set(FFMPEG_EXTRACTED_ROOT "${FFMPEG_PREBUILT_DIR}")
            endif()

            # Locate the include and lib directories inside the downloaded archive
            find_path(FFMPEG_INCLUDE_DIR libavcodec/avcodec.h
                PATHS "${FFMPEG_EXTRACTED_ROOT}/include"
                      "${FFMPEG_PREBUILT_DIR}/include"
                      "${FFMPEG_PREBUILT_DIR}/ffmpeg-master-latest-win64-lgpl-shared/include"
                      "${FFMPEG_PREBUILT_DIR}/ffmpeg-master-latest-linux64-lgpl-shared/include"
                      "${FFMPEG_PREBUILT_DIR}/ffmpeg-n7.1-latest-win64-lgpl-shared-7.1/include"
                      "${FFMPEG_PREBUILT_DIR}/ffmpeg-n7.1-latest-linux64-lgpl-shared-7.1/include"
                NO_DEFAULT_PATH)

            find_library(AVCODEC_LIB avcodec
                PATHS "${FFMPEG_EXTRACTED_ROOT}/lib"
                      "${FFMPEG_PREBUILT_DIR}/lib"
                      "${FFMPEG_PREBUILT_DIR}/ffmpeg-master-latest-win64-lgpl-shared/lib"
                      "${FFMPEG_PREBUILT_DIR}/ffmpeg-master-latest-linux64-lgpl-shared/lib"
                      "${FFMPEG_PREBUILT_DIR}/ffmpeg-n7.1-latest-win64-lgpl-shared-7.1/lib"
                      "${FFMPEG_PREBUILT_DIR}/ffmpeg-n7.1-latest-linux64-lgpl-shared-7.1/lib"
                NO_DEFAULT_PATH)
            find_library(AVUTIL_LIB avutil
                PATHS "${FFMPEG_EXTRACTED_ROOT}/lib"
                      "${FFMPEG_PREBUILT_DIR}/lib"
                      "${FFMPEG_PREBUILT_DIR}/ffmpeg-master-latest-win64-lgpl-shared/lib"
                      "${FFMPEG_PREBUILT_DIR}/ffmpeg-master-latest-linux64-lgpl-shared/lib"
                      "${FFMPEG_PREBUILT_DIR}/ffmpeg-n7.1-latest-win64-lgpl-shared-7.1/lib"
                      "${FFMPEG_PREBUILT_DIR}/ffmpeg-n7.1-latest-linux64-lgpl-shared-7.1/lib"
                NO_DEFAULT_PATH)
            find_library(SWSCALE_LIB swscale
                PATHS "${FFMPEG_EXTRACTED_ROOT}/lib"
                      "${FFMPEG_PREBUILT_DIR}/lib"
                      "${FFMPEG_PREBUILT_DIR}/ffmpeg-master-latest-win64-lgpl-shared/lib"
                      "${FFMPEG_PREBUILT_DIR}/ffmpeg-master-latest-linux64-lgpl-shared/lib"
                      "${FFMPEG_PREBUILT_DIR}/ffmpeg-n7.1-latest-win64-lgpl-shared-7.1/lib"
                      "${FFMPEG_PREBUILT_DIR}/ffmpeg-n7.1-latest-linux64-lgpl-shared-7.1/lib"
                NO_DEFAULT_PATH)

            if(FFMPEG_INCLUDE_DIR AND AVCODEC_LIB AND AVUTIL_LIB AND SWSCALE_LIB)
                set(FFMPEG_FOUND TRUE)
                set(FFMPEG_AUTO_DOWNLOADED TRUE)
                message(STATUS "FFmpeg auto-downloaded successfully")
                message(STATUS "  include: ${FFMPEG_INCLUDE_DIR}")
                message(STATUS "  avcodec: ${AVCODEC_LIB}")
            else()
                message(WARNING
                    "FFmpeg auto-download succeeded but libs not located.\n"
                    "Run: sudo apt install libavcodec-dev libavutil-dev libswscale-dev")
            endif()
        endif()
    endif()

    # Linux requires OpenGL and EGL for Qt6Gui
    if(LINUX)
        find_package(OpenGL REQUIRED)
        find_package(EGL REQUIRED)
    endif()
    
    # Get Qt6 include directories
    get_target_property(QT6_CORE_INCLUDE_DIRS Qt6::Core INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(QT6_GUI_INCLUDE_DIRS Qt6::Gui INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(QT6_WIDGETS_INCLUDE_DIRS Qt6::Widgets INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(QT6_NETWORK_INCLUDE_DIRS Qt6::Network INTERFACE_INCLUDE_DIRECTORIES)
    
    message(STATUS "Found Qt6: ${Qt6_DIR}")
    message(STATUS "Qt6 Network include dirs: ${QT6_NETWORK_INCLUDE_DIRS}")
    
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
        src/ReDroidController/DeviceProfileGenerator.cpp
        
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

        # Android locale/proxy manager (used by ReDroidController)
        src/Android/LocaleTimezoneManager.cpp
        src/ReDroidController/SecurityMitigationManager.cpp

        # Data Layer
        src/Data/TACDatabase.cpp
        
        # OpenSSL Stub
        src/ReDroidController/openssl_stub.cpp

        # Requirements installer (WSL2 + Docker Desktop)
        src/ReDroidController/RequirementsManager.cpp

        # Native ADB + streaming pipeline (no external exe required)
        src/ReDroidController/AdbSocketClient.cpp
        src/ReDroidController/MediaStreamDecoder.cpp

        # GUI Windows
        src/GUI/PhoneWindow.cpp
        src/GUI/FrameRenderer.cpp
        src/GUI/DashboardWindow.cpp
        src/GUI/AntiDetectionPanel.cpp
        src/GUI/LoginWindow.cpp
        src/GUI/MainWindow.cpp
        src/GUI/PhoneSettingsDialog.cpp
        src/GUI/SettingsDialog.cpp
        
        # Utilities
        src/ReDroidController/FileLogger.cpp
        src/ReDroidController/ConfigManager.cpp
        
        # Main App
        src/qtmain.cpp
    )
    
    # =========================================================================
    # ALL Headers
    # =========================================================================
    set(REDROID_HEADERS
        include/VirtualPhonePro/ReDroidController.hpp
        include/VirtualPhonePro/DeviceProfile.h
        include/VirtualPhonePro/EnhancedDeviceProfile.h
        include/VirtualPhonePro/BankingAppSpoofer.h
        include/VirtualPhonePro/GoogleFacebookSpoofer.h
        include/VirtualPhonePro/DeepDeviceSpoofer.h
        include/VirtualPhonePro/UniqueDeviceGenerator.h
        include/VirtualPhonePro/AdvancedAntiDetection.hpp
        include/VirtualPhonePro/TLSFingerprint.hpp
        include/VirtualPhonePro/HyperRealisticTouchEmulator.hpp
        include/VirtualPhonePro/NetworkProfileManager.hpp
        include/VirtualPhonePro/RealisticDeviceProfile.hpp
        include/VirtualPhonePro/MultiInstanceManager.hpp
        include/VirtualPhonePro/HardwareFingerprintSpoofer.hpp
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
        include/VirtualPhonePro/RequirementsManager.hpp
        include/VirtualPhonePro/PlayIntegrityManager.hpp
        include/VirtualPhonePro/EmulatorDetectionBypass.hpp
        include/VirtualPhonePro/AndroidRealismEngine.h
        include/VirtualPhonePro/NetworkStackSpoofer.hpp
        include/VirtualPhonePro/TimingAttackPrevention.hpp
        include/VirtualPhonePro/CarrierNetworkSimulator.h
        include/VirtualPhonePro/OEMDeepSpoofing.h
        include/VirtualPhonePro/SensorSimulator.hpp
        include/VirtualPhonePro/BatteryPowerManager.h
        include/VirtualPhonePro/SSLCertificateManager.h
        include/VirtualPhonePro/PersistentIdentityManager.h
        include/VirtualPhonePro/DeviceBehaviorManager.h
        include/VirtualPhonePro/AppCloner.hpp
        include/VirtualPhonePro/ScreenMirror.hpp
        include/VirtualPhonePro/ScreenStateManager.h
        include/VirtualPhonePro/FindMyDeviceManager.h
        include/VirtualPhonePro/Logger.hpp
        include/VirtualPhonePro/HttpClient.hpp
        include/VirtualPhonePro/WebhookManager.hpp
        include/VirtualPhonePro/APIServer.hpp
        include/VirtualPhonePro/DeviceIntegrityManager.h
        include/VirtualPhonePro/HardwareAttestation.h
        include/VirtualPhonePro/HALSimulation.h
        include/VirtualPhonePro/TestingFramework.hpp
        include/VirtualPhonePro/NetworkRealismEnhancer.h
        include/VirtualPhonePro/CryptoUtils.hpp
        include/VirtualPhonePro/SecurityMitigationManager.hpp
        include/VirtualPhonePro/FridaXposedDetector.h
        include/VirtualPhonePro/MagiskPatcher.hpp
        include/VirtualPhonePro/SELinuxManager.hpp
        include/VirtualPhonePro/AdvancedRealisticSimulation.h
        include/VirtualPhonePro/NetworkConfig.h
        include/Data/TACDatabase.h
        # Q_OBJECT headers that AUTOMOC must process (declared in the target
        # source list so the moc_*.cpp outputs are generated and linked).
        include/VirtualPhonePro/AdbSocketClient.hpp
        include/VirtualPhonePro/MediaStreamDecoder.hpp
        include/VirtualPhonePro/FrameRenderer.hpp
        include/Android/LocaleTimezoneManager.h
        src/GUI/PhoneWindow.h
        src/GUI/PhoneWindow.cpp
        src/GUI/DashboardWindow.h
        src/GUI/DashboardWindow.cpp
        src/GUI/AntiDetectionPanel.h
        src/GUI/AntiDetectionPanel.cpp
        src/GUI/LoginWindow.h
        src/GUI/LoginWindow.cpp
        src/GUI/MainWindow.h
        src/GUI/MainWindow.cpp
        src/GUI/PhoneSettingsDialog.h
        src/GUI/PhoneSettingsDialog.cpp
        src/GUI/SettingsDialog.h
        src/GUI/SettingsDialog.cpp
        include/VirtualPhonePro/FileLogger.hpp
        include/VirtualPhonePro/ConfigManager.hpp
        src/ReDroidController/FileLogger.cpp
        src/ReDroidController/ConfigManager.cpp
        src/ReDroidController/openssl_stub.h
    )
    
    # Threads
    find_package(Threads REQUIRED)

    # =========================================================================
    # Main Executable - WIN32 for no console
    # =========================================================================
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)
    
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
        ${QT6_CORE_INCLUDE_DIRS}
        ${QT6_GUI_INCLUDE_DIRS}
        ${QT6_WIDGETS_INCLUDE_DIRS}
        ${QT6_NETWORK_INCLUDE_DIRS}
    )
    
    # Link Qt6 libraries
    target_link_libraries(ReDroidCPP PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        Qt6::Concurrent
        Qt6::OpenGL
        Qt6::OpenGLWidgets
        Threads::Threads
    )

    # FFmpeg libraries (native H.264 decoding, no external exe needed)
    if(FFMPEG_FOUND)
        if(TARGET PkgConfig::FFMPEG)
            target_link_libraries(ReDroidCPP PRIVATE PkgConfig::FFMPEG)
        else()
            target_link_libraries(ReDroidCPP PRIVATE
                ${AVCODEC_LIB} ${AVUTIL_LIB} ${SWSCALE_LIB})
            if(FFMPEG_INCLUDE_DIR)
                target_include_directories(ReDroidCPP PRIVATE ${FFMPEG_INCLUDE_DIR})
            endif()
        endif()
        target_compile_definitions(ReDroidCPP PRIVATE VPP_FFMPEG_AVAILABLE)

        # On Windows, copy FFmpeg DLLs next to the exe so it runs without PATH changes
        if(WIN32 AND FFMPEG_AUTO_DOWNLOADED AND FFMPEG_INCLUDE_DIR)
            get_filename_component(FFMPEG_LIB_DIR "${AVCODEC_LIB}" DIRECTORY)
            get_filename_component(FFMPEG_ROOT "${FFMPEG_LIB_DIR}/.." ABSOLUTE)
            set(FFMPEG_BIN_DIR "${FFMPEG_ROOT}/bin")
            if(EXISTS "${FFMPEG_BIN_DIR}")
                add_custom_command(TARGET ReDroidCPP POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                        "${FFMPEG_BIN_DIR}"
                        "$<TARGET_FILE_DIR:ReDroidCPP>"
                    COMMENT "Copying FFmpeg DLLs to output directory"
                )
            endif()
        endif()
    endif()

    # Linux-specific libraries
    if(LINUX)
        target_link_libraries(ReDroidCPP PRIVATE
            OpenGL::GL
            EGL::EGL
        )
    endif()

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

    # ── scrcpy-server auto-download (build-time, no user action needed) ───────
    # The scrcpy-server JAR must sit next to the ReDroidCPP executable so
    # PhoneWindow::pushScrcpyServer() can find it at runtime via
    # QCoreApplication::applicationDirPath().
    # We download it from the official scrcpy release during cmake --build
    # using a POST_BUILD custom command on the main target.
    # The file is only downloaded when missing or outdated (file size check).
    set(SCRCPY_VERSION "v4.1")
    set(SCRCPY_SERVER_URL
        "https://github.com/Genymobile/scrcpy/releases/download/${SCRCPY_VERSION}/scrcpy-server-${SCRCPY_VERSION}")    set(SCRCPY_SERVER_DEST
        "${CMAKE_BINARY_DIR}/bin/scrcpy-server")

    # CMake script that does the conditional download
    set(SCRCPY_DOWNLOAD_SCRIPT "${CMAKE_BINARY_DIR}/download_scrcpy_server.cmake")
    file(WRITE "${SCRCPY_DOWNLOAD_SCRIPT}" "
# Auto-generated by CMakeLists.txt — do not edit
set(DEST \"${SCRCPY_SERVER_DEST}\")
set(URL  \"${SCRCPY_SERVER_URL}\")

if(EXISTS \"\${DEST}\")
    file(SIZE \"\${DEST}\" FSIZE)
    if(FSIZE GREATER 10000)
        message(STATUS \"[scrcpy-server] Already present (\${FSIZE} bytes) — skip download\")
        return()
    endif()
endif()

message(STATUS \"[scrcpy-server] Downloading \${URL} ...\")
file(DOWNLOAD \"\${URL}\" \"\${DEST}\"
    SHOW_PROGRESS
    STATUS DOWNLOAD_STATUS
    TIMEOUT 60)
list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
if(NOT STATUS_CODE EQUAL 0)
    list(GET DOWNLOAD_STATUS 1 ERROR_MSG)
    message(WARNING \"[scrcpy-server] Download failed: \${ERROR_MSG}\")
    message(WARNING \"  Manual fix: place scrcpy-server-${SCRCPY_VERSION} next to ReDroidCPP\")
else()
    file(SIZE \"\${DEST}\" FSIZE)
    message(STATUS \"[scrcpy-server] Downloaded OK (\${FSIZE} bytes)\")
endif()
")

    add_custom_command(TARGET ReDroidCPP POST_BUILD
        COMMAND ${CMAKE_COMMAND} -P "${SCRCPY_DOWNLOAD_SCRIPT}"
        COMMENT "Downloading scrcpy-server ${SCRCPY_VERSION} to bin/"
        VERBATIM
    )

    # Also install scrcpy-server alongside the exe
    install(FILES "${SCRCPY_SERVER_DEST}"
        DESTINATION bin
        OPTIONAL   # Don't fail install if download didn't run yet
    )

endif() # BUILD_QT6_GUI

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

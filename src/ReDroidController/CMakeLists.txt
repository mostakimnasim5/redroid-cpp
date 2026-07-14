# ==============================================================================
# VirtualPhonePro - ReDroidController CMake Configuration
# Windows-compatible Qt6 C++ library for managing ReDroid containers
# ==============================================================================

cmake_minimum_required(VERSION 3.20)
project(ReDroidController VERSION 1.0.0 LANGUAGES CXX)

# ═══════════════════════════════════════════════════════════════════════════════
# C++ Standard
# ═══════════════════════════════════════════════════════════════════════════════
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ═══════════════════════════════════════════════════════════════════════════════
# Platform Detection
# ═══════════════════════════════════════════════════════════════════════════════
if(WIN32)
    message(STATUS "Configuring for Windows")
    set(WINDOWS 1)
elseif(UNIX)
    message(STATUS "Configuring for Unix/Linux")
    set(UNIX 1)
endif()

# ═══════════════════════════════════════════════════════════════════════════════
# Qt6 Configuration
# ═══════════════════════════════════════════════════════════════════════════════
set(QT_VERSION_MAJOR 6)

# Try to find Qt6 with CMake's find_package
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS 
    Core 
    Network 
    ProcessManagement 
    REQUIRED
)

if(Qt${QT_VERSION_MAJOR}_FOUND)
    message(STATUS "Found Qt${QT_VERSION_MAJOR}: ${Qt${QT_VERSION_MAJOR}_VERSION}")
else()
    message(FATAL_ERROR "Qt${QT_VERSION_MAJOR} not found. Please install Qt${QT_VERSION_MAJOR}.")
endif()

# ═══════════════════════════════════════════════════════════════════════════════
# Windows-Specific Configuration
# ═══════════════════════════════════════════════════════════════════════════════
if(WIN32)
    # Unicode support
    add_definitions(-DUNICODE -D_UNICODE)
    
    # Windows XP/Vista compatibility
    add_definitions(-DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00)
    
    # Lean and mean
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    
    # Disable CRT deprecation warnings
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
    
    # Enable SSE/SSE2 for better performance
    add_compile_options(/arch:AVX2)
    
    # Multi-threaded runtime
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# ═══════════════════════════════════════════════════════════════════════════════
# Output Directories
# ═══════════════════════════════════════════════════════════════════════════════
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ═══════════════════════════════════════════════════════════════════════════════
# Include Directories
# ═══════════════════════════════════════════════════════════════════════════════
# Project headers
set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(
    ${PROJECT_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}
)

# Qt6 includes
include_directories(${Qt${QT_VERSION_MAJOR}_INCLUDE_DIRS})

# ═══════════════════════════════════════════════════════════════════════════════
# Source Files
# ═══════════════════════════════════════════════════════════════════════════════
set(REDCONTROLLER_HEADERS
    ${PROJECT_INCLUDE_DIR}/VirtualPhonePro/DeviceProfile.h
    ${PROJECT_INCLUDE_DIR}/VirtualPhonePro/EnhancedDeviceProfile.h
    ${PROJECT_INCLUDE_DIR}/VirtualPhonePro/ReDroidController.h
    ${PROJECT_INCLUDE_DIR}/VirtualPhonePro/MultiInstanceManager.h
    ${PROJECT_INCLUDE_DIR}/VirtualPhonePro/SafetyNetSpoofer.h
    ${PROJECT_INCLUDE_DIR}/VirtualPhonePro/BankingAppSpoofer.h
    ${PROJECT_INCLUDE_DIR}/VirtualPhonePro/DeepDeviceSpoofer.h
    ${PROJECT_INCLUDE_DIR}/VirtualPhonePro/GoogleFacebookSpoofer.h
    ${PROJECT_INCLUDE_DIR}/VirtualPhonePro/APIServer.h
    ${PROJECT_INCLUDE_DIR}/VirtualPhonePro/AppCloner.h
    ${PROJECT_INCLUDE_DIR}/VirtualPhonePro/MagiskPatcher.h
    ${PROJECT_INCLUDE_DIR}/VirtualPhonePro/NetworkConfig.h
    ${PROJECT_INCLUDE_DIR}/VirtualPhonePro/SensorSimulator.h
    ${PROJECT_INCLUDE_DIR}/VirtualPhonePro/TestingFramework.h
    ${PROJECT_INCLUDE_DIR}/VirtualPhonePro/WebhookManager.h
)

set(REDCONTROLLER_SOURCES
    ${PROJECT_SOURCE_DIR}/DeviceProfile.cpp
    ${PROJECT_SOURCE_DIR}/EnhancedDeviceProfile.cpp
    ${PROJECT_SOURCE_DIR}/ReDroidController.cpp
    ${PROJECT_SOURCE_DIR}/MultiInstanceManager.cpp
    ${PROJECT_SOURCE_DIR}/SafetyNetSpoofer.cpp
    ${PROJECT_SOURCE_DIR}/BankingAppSpoofer.cpp
    ${PROJECT_SOURCE_DIR}/DeepDeviceSpoofer.cpp
    ${PROJECT_SOURCE_DIR}/GoogleFacebookSpoofer.cpp
)

# ═══════════════════════════════════════════════════════════════════════════════
# Build Library
# ═══════════════════════════════════════════════════════════════════════════════
add_library(ReDroidController STATIC
    ${REDCONTROLLER_HEADERS}
    ${REDCONTROLLER_SOURCES}
)

# ═══════════════════════════════════════════════════════════════════════════════
# Target Properties
# ═══════════════════════════════════════════════════════════════════════════════
target_compile_features(ReDroidController PRIVATE cxx_std_17)

target_include_directories(ReDroidController PUBLIC
    ${PROJECT_INCLUDE_DIR}
)

target_link_libraries(ReDroidController PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
)

# Platform-specific linking
if(WIN32)
    target_link_libraries(ReDroidController PRIVATE
        ws2_32
        advapi32
        userenv
    )
endif()

# ═══════════════════════════════════════════════════════════════════════════════
# Preprocessor Definitions
# ═══════════════════════════════════════════════════════════════════════════════
target_compile_definitions(ReDroidController PRIVATE
    VIRTUALPHONEPRO_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    VIRTUALPHONEPRO_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    VIRTUALPHONEPRO_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    VIRTUALPHONEPRO_VERSION="${PROJECT_VERSION}"
)

# ═══════════════════════════════════════════════════════════════════════════════
# Export
# ═══════════════════════════════════════════════════════════════════════════════
# Create package config for use by other projects
include(GNUInstallDirs)

# Install headers
install(
    DIRECTORY ${PROJECT_INCLUDE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

# Install library
install(
    TARGETS ReDroidController
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ═══════════════════════════════════════════════════════════════════════════════
# Testing (optional)
# ═══════════════════════════════════════════════════════════════════════════════
option(BUILD_TESTS "Build tests" OFF)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ═══════════════════════════════════════════════════════════════════════════════
# Build Options
# ═══════════════════════════════════════════════════════════════════════════════
option(BUILD_SHARED_LIBS "Build shared library" OFF)
option(ENABLE_LOGGING "Enable debug logging" ON)

if(ENABLE_LOGGING)
    target_compile_definitions(ReDroidController PRIVATE VPP_LOGGING_ENABLED)
endif()

# ═══════════════════════════════════════════════════════════════════════════════
# Compiler Warnings
# ═══════════════════════════════════════════════════════════════════════════════
if(MSVC)
    target_compile_options(ReDroidController PRIVATE
        /W4
        /WX-
        /permissive-
    )
else()
    target_compile_options(ReDroidController PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# ═══════════════════════════════════════════════════════════════════════════════
# Summary
# ═══════════════════════════════════════════════════════════════════════════════
message(STATUS "")
message(STATUS "ReDroidController Configuration Summary")
message(STATUS "======================================")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Qt Version: ${Qt${QT_VERSION_MAJOR}_VERSION}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Build Shared: ${BUILD_SHARED_LIBS}")
message(STATUS "  Build Tests: ${BUILD_TESTS}")
message(STATUS "======================================")
message(STATUS "")

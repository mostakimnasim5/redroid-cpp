# ==============================================================================
# VirtualPhonePro - Main CMake Configuration
# Windows-compatible Qt6 application for managing ReDroid Android emulators
# ==============================================================================

cmake_minimum_required(VERSION 3.20)
project(VirtualPhonePro VERSION 1.0.0 LANGUAGES CXX)

# ═══════════════════════════════════════════════════════════════════════════════
# C++ Standard
# ═══════════════════════════════════════════════════════════════════════════════
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ═══════════════════════════════════════════════════════════════════════════════
# Platform Detection
# ═══════════════════════════════════════════════════════════════════════════════
if(WIN32)
    message(STATUS "Configuring for Windows")
    set(WINDOWS 1)
elseif(UNIX AND NOT APPLE)
    message(STATUS "Configuring for Linux")
    set(LINUX 1)
elseif(APPLE)
    message(STATUS "Configuring for macOS")
    set(MACOS 1)
endif()

# ═══════════════════════════════════════════════════════════════════════════════
# Qt6 Configuration
# ═══════════════════════════════════════════════════════════════════════════════
set(QT_VERSION_MAJOR 6)

# Try to find Qt6
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS 
    Core 
    Widgets
    Gui 
    Network 
    ProcessManagement
    SerialPort
    REQUIRED
)

if(Qt${QT_VERSION_MAJOR}_FOUND)
    message(STATUS "Found Qt${QT_VERSION_MAJOR}: ${Qt${QT_VERSION_MAJOR}_VERSION}")
    message(STATUS "Qt Modules: Core, Widgets, Gui, Network, ProcessManagement")
else()
    message(FATAL_ERROR "Qt${QT_VERSION_MAJOR} not found. Please install Qt${QT_VERSION_MAJOR}.")
endif()

# ═══════════════════════════════════════════════════════════════════════════════
# Windows-Specific Configuration
# ═══════════════════════════════════════════════════════════════════════════════
if(WIN32)
    # Unicode support
    add_definitions(-DUNICODE -D_UNICODE)
    
    # Windows version
    add_definitions(-DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00)
    
    # Lean and mean
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    
    # Disable warnings
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    
    # Performance
    add_compile_options(/W4 /permissive-)
    add_compile_options(/arch:AVX2)
    
    # Runtime
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    
    # Executable extension
    set(CMAKE_EXECUTABLE_SUFFIX ".exe")
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
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
set(PROJECT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(PROJECT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)

include_directories(
    ${PROJECT_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}
)

# ═══════════════════════════════════════════════════════════════════════════════
# ReDroidController Library
# ═══════════════════════════════════════════════════════════════════════════════
add_subdirectory(ReDroidController)

# ═══════════════════════════════════════════════════════════════════════════════
# Application Sources
# ═══════════════════════════════════════════════════════════════════════════════
set(APP_HEADERS
    ${PROJECT_SOURCE_DIR}/mainwindow.h
)

set(APP_SOURCES
    ${PROJECT_SOURCE_DIR}/main.cpp
    ${PROJECT_SOURCE_DIR}/mainwindow.cpp
    ${PROJECT_SOURCE_DIR}/qtmain.cpp
)

# ═══════════════════════════════════════════════════════════════════════════════
# Build Executable
# ═══════════════════════════════════════════════════════════════════════════════
add_executable(${PROJECT_NAME}
    WIN32
    ${APP_SOURCES}
    ${APP_HEADERS}
)

# ═══════════════════════════════════════════════════════════════════════════════
# Target Properties
# ═══════════════════════════════════════════════════════════════════════════════
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_INCLUDE_DIR}
)

# ═══════════════════════════════════════════════════════════════════════════════
# Link Libraries
# ═══════════════════════════════════════════════════════════════════════════════
target_link_libraries(${PROJECT_NAME} PRIVATE
    ReDroidController
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Network
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ws2_32
        advapi32
        userenv
        iphlpapi
        netapi32
    )
endif()

# ═══════════════════════════════════════════════════════════════════════════════
# Preprocessor Definitions
# ═══════════════════════════════════════════════════════════════════════════════
target_compile_definitions(${PROJECT_NAME} PRIVATE
    APP_VERSION="${PROJECT_VERSION}"
    APP_NAME="${PROJECT_NAME}"
)

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        VPP_PLATFORM_WINDOWS=1
    )
elseif(LINUX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        VPP_PLATFORM_LINUX=1
    )
endif()

# ═══════════════════════════════════════════════════════════════════════════════
# Install Rules
# ═══════════════════════════════════════════════════════════════════════════════
include(GNUInstallDirs)

# Install executable
install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install ReDroidController library
install(
    TARGETS ReDroidController
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Install ADB tools (on Windows, copy from system or include)
if(WIN32)
    # Create tools directory
    install(
        DIRECTORY ${CMAKE_SOURCE_DIR}/tools/
        DESTINATION tools
        FILES_MATCHING PATTERN "*"
    )
endif()

# Install resources
install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/resources/
    DESTINATION resources
)

# ═══════════════════════════════════════════════════════════════════════════════
# Build Presets
# ═══════════════════════════════════════════════════════════════════════════════
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.21)
    message(STATUS "Creating build presets...")
    
    # Debug preset
    if(NOT DEFINED CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    endif()
endif()

# ═══════════════════════════════════════════════════════════════════════════════
# Summary
# ═══════════════════════════════════════════════════════════════════════════════
message(STATUS "")
message(STATUS "VirtualPhonePro Build Configuration")
message(STATUS "======================================")
message(STATUS "  Project: ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Qt Version: ${Qt${QT_VERSION_MAJOR}_VERSION}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Output Dir: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "======================================")
message(STATUS "")

# ═══════════════════════════════════════════════════════════════════════════════
# Custom Build Targets
# ═══════════════════════════════════════════════════════════════════════════════

# Target: Copy ADB tools
add_custom_target(copy-adb
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:Qt${QT_VERSION_MAJOR}::adb>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/adb.exe
    COMMENT "Copying ADB tools to output directory"
    DEPENDS ${PROJECT_NAME}
)

# Target: Validate Docker
add_custom_target(validate-docker
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ReDroidController
    COMMAND ${CMAKE_COMMAND} -E echo "Validating Docker installation..."
    COMMENT "Validating Docker configuration"
)

# ═══════════════════════════════════════════════════════════════════════════════
# CPack Configuration
# ═══════════════════════════════════════════════════════════════════════════════
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR "VirtualPhonePro")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Commercial-grade Anti-Detect Android Emulator Manager")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE)

if(WIN32)
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_ENABLE_UNINSTALLER ON)
    set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
endif()

include(CPack)

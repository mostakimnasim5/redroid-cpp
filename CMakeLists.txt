cmake_minimum_required(VERSION 3.16)
project(RedroidCPP VERSION 3.0.0 LANGUAGES CXX)

# ==============================================================================
# Project Configuration
# ==============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Version info
set(PROJECT_VERSION_MAJOR 3)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)

# ==============================================================================
# Platform Detection
# ==============================================================================
if(WIN32)
    message(STATUS "Building for Windows")
    set(WINDOWS 1)
elseif(UNIX)
    message(STATUS "Building for Unix/Linux")
    set(UNIX 1)
endif()

# ==============================================================================
# Compiler Flags
# ==============================================================================
if(MSVC)
    add_compile_options(
        /W4
        /WX-
        /permissive-
        /EHsc
    )
    # Windows XP/Vista compatibility
    add_definitions(-DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00)
    # Lean and mean
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    # Disable CRT deprecation warnings
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
    )
endif()

# ==============================================================================
# Source Files - Core Module
# ==============================================================================
set(CORE_SOURCES
    src/main.cpp
    src/Core/DeviceProfile.cpp
    src/Core/DeviceManager.cpp
)

set(CORE_HEADERS
    include/Core/DeviceProfile.h
    include/Core/DeviceManager.h
)

# ==============================================================================
# Executable - CLI Tool
# ==============================================================================
add_executable(redroid-cli ${CORE_SOURCES} ${CORE_HEADERS})

# Include directories
target_include_directories(redroid-cli PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Link libraries
find_package(Threads REQUIRED)
target_link_libraries(redroid-cli PRIVATE Threads::Threads)

# Windows-specific libraries
if(WIN32)
    target_link_libraries(redroid-cli PRIVATE
        ws2_32
        advapi32
        userenv
    )
endif()

# ==============================================================================
# Installation
# ==============================================================================
install(TARGETS redroid-cli DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)
install(DIRECTORY profiles/ DESTINATION share/redroid/profiles)

# ==============================================================================
# Qt6 GUI Application (Optional)
# ==============================================================================
option(BUILD_QT6_GUI "Build Qt6 GUI application" OFF)

if(BUILD_QT6_GUI)
    find_package(Qt6 COMPONENTS Core Gui Widgets Network REQUIRED)
    
    set(QT6_SOURCES
        src/mainwindow.cpp
        src/qtmain.cpp
        src/AutoStartDialog.cpp
        src/ReDroidController/ReDroidController.cpp
        src/ReDroidController/MultiInstanceManager.cpp
        src/ReDroidController/SafetyNetSpoofer.cpp
        src/ReDroidController/BankingAppSpoofer.cpp
        src/ReDroidController/DeepDeviceSpoofer.cpp
        src/ReDroidController/GoogleFacebookSpoofer.cpp
        src/ReDroidController/EnhancedDeviceProfile.cpp
    )
    
    set(QT6_HEADERS
        src/mainwindow.h
        src/AutoStartDialog.h
        include/VirtualPhonePro/ReDroidController.h
        include/VirtualPhonePro/DeviceProfile.h
        include/VirtualPhonePro/EnhancedDeviceProfile.h
        include/VirtualPhonePro/BankingAppSpoofer.h
        include/VirtualPhonePro/DeepDeviceSpoofer.h
        include/VirtualPhonePro/GoogleFacebookSpoofer.h
    )
    
    add_executable(virtualphonepro-qt WIN32 ${QT6_SOURCES} ${QT6_HEADERS})
    
    target_include_directories(virtualphonepro-qt PRIVATE
        include
        src
        ${Qt6_INCLUDE_DIRS}
    )
    
    target_link_libraries(virtualphonepro-qt PRIVATE
        Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network
        Threads::Threads
    )
    
    if(WIN32)
        target_link_libraries(virtualphonepro-qt PRIVATE ws2_32 advapi32 userenv)
    endif()
    
    install(TARGETS virtualphonepro-qt DESTINATION bin)
    
    message(STATUS "Qt6 GUI enabled - use -DBUILD_QT6_GUI=ON to build")
endif()

# ==============================================================================
# Banking App Testing Module
# ==============================================================================
option(BUILD_BANKING_MODULE "Build banking app testing module" ON)

if(BUILD_BANKING_MODULE)
    message(STATUS "Banking App Testing Module enabled")
    message(STATUS "  - BankingAppSpoofer: Root/Hook/Emulator detection bypass")
    message(STATUS "  - EnhancedDeviceProfile: 100% realistic device profiles")
endif()

# ==============================================================================
# Testing
# ==============================================================================
enable_testing()

# ==============================================================================
# Docker Build Target (requires Docker)
# ==============================================================================
find_program(DOCKER_EXECUTABLE docker)
if(DOCKER_EXECUTABLE)
    add_custom_target(docker-build
        COMMAND ${DOCKER_EXECUTABLE} build -t redroid-cpp/emulator:${PROJECT_VERSION} -f ${CMAKE_SOURCE_DIR}/docker/Dockerfile ${CMAKE_SOURCE_DIR}
        COMMENT "Building Docker images for emulator"
        VERBATIM
    )
    
    add_custom_target(docker-run
        COMMAND ${DOCKER_EXECUTABLE} run --rm -it --privileged --device /dev/kvm:/dev/kvm -v ${CMAKE_SOURCE_DIR}/profiles:/opt/profiles redroid-cpp/emulator:${PROJECT_VERSION}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running emulator in Docker"
        VERBATIM
    )
    
    add_custom_target(docker-up
        COMMAND ${DOCKER_EXECUTABLE} compose -f ${CMAKE_SOURCE_DIR}/docker/docker-compose.yml up -d
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/docker
        COMMENT "Starting emulator with docker compose"
        VERBATIM
    )
    
    add_custom_target(docker-down
        COMMAND ${DOCKER_EXECUTABLE} compose -f ${CMAKE_SOURCE_DIR}/docker/docker-compose.yml down
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/docker
        COMMENT "Stopping emulator"
        VERBATIM
    )
    
    message(STATUS "Docker support enabled")
else()
    message(STATUS "Docker not found - Docker build targets disabled")
endif()

# ==============================================================================
# Build Options
# ==============================================================================
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)

if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        target_compile_options(redroid-cli PRIVATE --coverage)
        target_link_libraries(redroid-cli PRIVATE --coverage)
    endif()
endif()

# ==============================================================================
# Summary
# ==============================================================================
message(STATUS "")
message(STATUS "===================================")
message(STATUS "  RedroidCPP Configuration Summary")
message(STATUS "===================================")
message(STATUS "  Version:    ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")

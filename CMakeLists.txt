cmake_minimum_required(VERSION 3.16)
project(RedroidCPP VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
    )
endif()

# Source files
set(SOURCES
    src/main.cpp
)

set(HEADERS
    include/Data/TACDatabase.h
    include/Core/DeviceProfile.h
    include/Core/DeviceManager.h
)

# Executable
add_executable(redroid-cli ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(redroid-cli PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Install
install(TARGETS redroid-cli DESTINATION bin)

# Testing
enable_testing()

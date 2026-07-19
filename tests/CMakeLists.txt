# Test Suite CMake Configuration
cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Required for Q_OBJECT in test classes
set(CMAKE_AUTOMOC ON)

# Find Qt6
find_package(Qt6 COMPONENTS Test Core Widgets Network Concurrent REQUIRED)
find_package(Threads REQUIRED)

# Enable testing
enable_testing()

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
)

target_link_libraries(Test_UniqueDeviceGenerator PRIVATE
    Qt6::Test
    Qt6::Core
    Qt6::Concurrent
    Threads::Threads
    ReDroidController
)

target_include_directories(Test_UniqueDeviceGenerator PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
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

# Test Suite CMake Configuration
cmake_minimum_required(VERSION 3.20)
project(ReDroidCPP_Tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 for testing
find_package(Qt6 COMPONENTS Test Core Widgets REQUIRED)

# Enable testing
enable_testing()

# Test sources
set(TEST_SOURCES
    Test_UniqueDeviceGenerator.cpp
    Test_TACDatabase.cpp
    Test_LuhnValidation.cpp
)

# Create test executable
add_executable(${PROJECT_NAME} ${TEST_SOURCES})

# Link Qt Test framework
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Test
    Qt6::Core
)

# Link the main library (if built)
find_library(REDROID_CONTROLLER_LIBRARY NAMES ReDroidController PATHS ${CMAKE_BINARY_DIR}/lib)
if(REDROID_CONTROLLER_LIBRARY)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${REDROID_CONTROLLER_LIBRARY})
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Copy test data
file(COPY ${CMAKE_SOURCE_DIR}/profiles/ DESTINATION ${CMAKE_BINARY_DIR}/profiles/)

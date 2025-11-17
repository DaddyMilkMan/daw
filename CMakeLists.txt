cmake_minimum_required(VERSION 3.22)

project(ZenithDAW VERSION 0.1.0 LANGUAGES C CXX)

# C++20 standard required
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Platform-specific settings
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build universal binary")
endif()

# Add zenith-core subdirectory (the native JUCE DAW)
add_subdirectory(zenith-core)

message(STATUS "")
message(STATUS "========================================")
message(STATUS "Zenith DAW - Pure JUCE Native Build")
message(STATUS "========================================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "")
message(STATUS "Building native JUCE application:")
message(STATUS "  - No Electron")
message(STATUS "  - No React/TypeScript")
message(STATUS "  - No Qt/QML")
message(STATUS "  - Pure C++ JUCE GUI")
message(STATUS "========================================")
message(STATUS "")

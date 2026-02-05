# ClockSyncAgent CMake configuration

cmake_minimum_required(VERSION 3.25)

project(ClockSyncAgent
    VERSION 0.1.0
    DESCRIPTION "Clock synchronization agent for Zenith DAW"
    LANGUAGES CXX
)

# Define the ClockSyncAgent library
add_library(ClockSyncAgent STATIC
    clock_sync.cpp
)

# Set C++ standard
target_compile_features(ClockSyncAgent PUBLIC cxx_std_20)

# Include directories
target_include_directories(ClockSyncAgent PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Installation rules (optional)
install(TARGETS ClockSyncAgent
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

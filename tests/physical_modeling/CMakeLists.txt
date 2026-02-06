cmake_minimum_required(VERSION 3.10)
project(ZenithPhysicalModelingTests)

# Find JUCE
find_package(JUCE REQUIRED)

# Add test executable
add_executable(TestRunner
    TestRunner.cpp
    PhysicalModelingTest.cpp
    PhysicalModelingTest.h
)

# Link JUCE libraries
target_link_libraries(TestRunner
    JUCE::juce_core
    JUCE::juce_audio_basics
    JUCE::juce_audio_devices
    JUCE::juce_audio_formats
    JUCE::juce_audio_processors
    JUCE::juce_audio_utils
    JUCE::juce_data_structures
    JUCE::juce_events
    JUCE::juce_graphics
    JUCE::juce_gui_basics
    JUCE::juce_gui_extra
)

# Include directories
target_include_directories(TestRunner PRIVATE
    ../../../modules/zenith_core/instruments/ZenithUltraSynth/physical_modeling
    ../../../modules/zenith_core
)

# Compile definitions
target_compile_definitions(TestRunner PRIVATE
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_DONT_AUTOSTART_PLUGINS_IN_CHILD_PROCESSES=1
)

# Set C++ standard
set_target_properties(TestRunner PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Run tests
add_test(NAME PhysicalModelingTests COMMAND TestRunner)

# Enable testing
enable_testing()

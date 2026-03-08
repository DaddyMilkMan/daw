cmake_minimum_required(VERSION 3.22)
project(ZenithSynthTests VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# Test configuration
enable_testing()
include(CTest)

# Sources
set(TEST_SOURCES
    ZenithTests.cpp
)

# Include paths
target_include_directories(ZenithTests PRIVATE
    ${CMAKE_SOURCE_DIR}/../../modules/zenith_core/instruments
)

# Create test executable
add_executable(run_zenith_tests ${TEST_SOURCES})

# Link JUCE
find_package(Juce REQUIRED COMPONENTS
    juce_audio_basics
    juce_audio_utils
    juce_audio_devices
    juce_core
    juce_events
    juce_graphics
    juce_gui_basics
    juce_gui_extra
    juce_data_structures
)

if(TARGET Juce::JUCE)
    target_link_libraries(run_zenith_tests PRIVATE Juce::JUCE)
endif()

# Add tests
add_test(NAME OscillatorTests COMMAND run_zenith_tests)
add_test(NAME FilterTests COMMAND run_zenith_tests)
add_test(NAME VoiceTests COMMAND run_zenith_tests)
add_test(NAME ModulationTests COMMAND run_zenith_tests)

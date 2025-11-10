cmake_minimum_required(VERSION 3.22)

project(JuceAudioEngine VERSION 1.0.0 LANGUAGES CXX)

# Find JUCE
# Option 1: JUCE as submodule (uncomment when added)
# add_subdirectory(${CMAKE_SOURCE_DIR}/JUCE JUCE)

# Option 2: System-installed JUCE
# find_package(JUCE CONFIG REQUIRED)

# For now, create a placeholder library
# When JUCE is added, this will be replaced with actual JUCE modules

# Source files
set(SOURCES
    Source/AudioEngine.cpp
    Source/AudioEngine.h
    Source/PluginHost.cpp
    Source/PluginHost.h
    Source/MidiProcessor.cpp
    Source/MidiProcessor.h
    Source/AudioProcessor.cpp
    Source/AudioProcessor.h
)

# Create audio engine library
add_library(JuceAudioEngine STATIC
    ${SOURCES}
)

# When JUCE is available, link these modules:
# target_link_libraries(JuceAudioEngine
#     PUBLIC
#         juce::juce_audio_basics
#         juce::juce_audio_devices
#         juce::juce_audio_formats
#         juce::juce_audio_processors
#         juce::juce_audio_utils
#         juce::juce_core
#         juce::juce_data_structures
#         juce::juce_events
#         juce::juce_graphics
#         juce::juce_gui_basics
#         juce::juce_gui_extra
# )

# For now, just link standard libraries
target_link_libraries(JuceAudioEngine
    PUBLIC
        ${CMAKE_DL_LIBS}
)

# Include directories
target_include_directories(JuceAudioEngine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
        # ${CMAKE_SOURCE_DIR}/JUCE/modules  # When JUCE is added
)

# Compiler settings
target_compile_features(JuceAudioEngine PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(JuceAudioEngine PRIVATE /W4)
    target_compile_definitions(JuceAudioEngine PUBLIC
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
    )
else()
    target_compile_options(JuceAudioEngine PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Platform-specific definitions
if(APPLE)
    target_compile_definitions(JuceAudioEngine PUBLIC
        JUCE_MAC=1
    )
    find_library(COREAUDIO_FRAMEWORK CoreAudio)
    find_library(COREMIDI_FRAMEWORK CoreMIDI)
    find_library(AUDIOUNIT_FRAMEWORK AudioUnit)
    find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)
    target_link_libraries(JuceAudioEngine PUBLIC
        ${COREAUDIO_FRAMEWORK}
        ${COREMIDI_FRAMEWORK}
        ${AUDIOUNIT_FRAMEWORK}
        ${AUDIOTOOLBOX_FRAMEWORK}
    )
elseif(WIN32)
    target_compile_definitions(JuceAudioEngine PUBLIC
        JUCE_WINDOWS=1
    )
    target_link_libraries(JuceAudioEngine PUBLIC
        winmm
        ole32
        oleaut32
        uuid
    )
elseif(UNIX)
    target_compile_definitions(JuceAudioEngine PUBLIC
        JUCE_LINUX=1
    )
    find_package(ALSA REQUIRED)
    target_link_libraries(JuceAudioEngine PUBLIC
        ${ALSA_LIBRARIES}
        pthread
        dl
    )
endif()

# =============================================================================
# DEPENDENCIES MANAGEMENT
# =============================================================================

# JUCE Setup - prefer local directory if available
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/CMakeLists.txt")
    message(STATUS "Using local JUCE from external/JUCE")
    add_subdirectory(external/JUCE)
else()
    message(STATUS "Fetching JUCE from GitHub...")
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.0
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# Core JUCE modules for Zenith DAW
set(ZENITH_JUCE_MODULES
    juce::juce_core
    juce::juce_cryptography
    juce::juce_events
    juce::juce_graphics
    juce::juce_data_structures
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_opengl
)

message(STATUS "Zenith DAW: JUCE 8.0.0 configured")

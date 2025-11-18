cmake_minimum_required(VERSION 3.22)

# Project definition
project(ZenithDAW VERSION 0.1.0 LANGUAGES C CXX)

enable_testing()

# C++20 standard required
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================================
# Build Options
# ============================================================================

# Phase control flags for incremental integration
option(ZENITH_ENABLE_PHASE1_AUDIO "Enable src/audio skeleton (Phase 1 - not ready)" OFF)
option(ZENITH_ENABLE_VEXEL_DONOR   "Enable donor compile checks from VexelDAW-Native" OFF)

# C3: Optional debug seed for testing track integration (disabled by default)
option(ZENITH_ENGINE_SEED_DEBUG_TRACKS "Create 8 demo tracks at startup in Debug builds" OFF)

# ============================================================================
# Fetch JUCE 8.0.9 using FetchContent
# ============================================================================
include(FetchContent)

message(STATUS "Fetching JUCE 8.0.9...")

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.9
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(JUCE)

message(STATUS "JUCE fetched successfully!")

# ============================================================================
# Build Configuration
# ============================================================================

# Set build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Platform-specific flags
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
    # Build universal binaries for Apple Silicon + Intel
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures")
endif()

# ============================================================================
# Zenith DAW Application
# ============================================================================

juce_add_gui_app(ZenithDAW
    PRODUCT_NAME "Zenith DAW"
    COMPANY_NAME "ZenithAudio"
    BUNDLE_ID "com.zenithaudio.zenith-daw"
    VERSION "0.1.0"

    # Icon file (will create later)
    # ICON_BIG "resources/icon.png"

    # Microphone/camera permissions (macOS)
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Zenith DAW needs access to your audio input devices for recording."
)

# ============================================================================
# Source Files
# ============================================================================

target_sources(ZenithDAW PRIVATE
    # Core application
    src/Main.cpp
    src/MainWindow.cpp
    src/Engine.cpp
    src/ProjectState.cpp

    # Phase 13: Automation system
    src/TrackAutomationSynchronizer.cpp

    # Integration: UI components and synchronizers
    src/ClipSynchronizer.cpp
    src/ArrangerView.cpp
    src/PianoRollEditor.cpp
    include/ClipSynchronizer.h
    include/ArrangerView.h
    include/PianoRollEditor.h

    # Engine primitives (ported from VexelDAW-Native, Phase 2)
    Source/engine/Track.h
    Source/engine/Track.cpp
    Source/engine/Clip.h
    Source/engine/Clip.cpp
    Source/engine/MixerChannel.h
    Source/engine/MixerChannel.cpp

    # Phase 1.2: Audio File Pool
    Source/engine/AudioFilePool.h
    Source/engine/AudioFilePool.cpp

    # Phase 3: Plugin hosting
    Source/engine/PluginHost.h
    Source/engine/PluginHost.cpp

    # Phase 3: Plugin UI
    Source/ui/PluginEditorWindow.h
    Source/ui/PluginEditorWindow.cpp

    # Phase 4: Timeline/Arranger UI
    Source/ui/ArrangerComponent.h
    Source/ui/ArrangerComponent.cpp
    Source/ui/PianoRollComponent.h
    Source/ui/PianoRollComponent.cpp

    # Phase 5: Wingman v0 (AI Command Console)
    Source/commands/CommandAPI.h
    Source/commands/CommandAPI.cpp
    Source/commands/SessionGraph.h
    Source/commands/SessionGraph.cpp
    Source/ui/WingmanPanel.h
    Source/ui/WingmanPanel.cpp

    # Phase 7: Wingman AI Integration
    Source/network/AIBridgeClient.h
    Source/network/AIBridgeClient.cpp

    # Instrument system
    Source/instruments/InstrumentMetadata.h
    Source/instruments/Instrument.h
    Source/instruments/Instrument.cpp
    Source/instruments/InstrumentRegistry.h
    Source/instruments/InstrumentRegistry.cpp
    Source/instruments/ContentPaths.h
    Source/instruments/ZenithPolySynth.h
    Source/instruments/ZenithPolySynth.cpp
    Source/instruments/ZenithSampler.h
    Source/instruments/ZenithSampler.cpp
    Source/instruments/ZenithSamplerEditor.h
    Source/instruments/ZenithSamplerEditor.cpp
    Source/instruments/RegisterBuiltInInstruments.h
    Source/instruments/RegisterBuiltInInstruments.cpp
    Source/instruments/ZenithPolySynthEditor.h
    Source/instruments/ZenithPolySynthEditor.cpp
    include/InstrumentCommandHelpers.h
)

# ============================================================================
# Include Directories
# ============================================================================

target_include_directories(ZenithDAW PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/commands
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/network
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/instruments
)

# ============================================================================
# JUCE Modules
# ============================================================================

target_link_libraries(ZenithDAW PRIVATE
    # Audio essentials
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils

    # DSP (for built-in instruments)
    juce::juce_dsp

    # Core functionality
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events

    # GUI
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra

    # Public link for GUI app config
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# ============================================================================
# Generate JuceHeader.h
# ============================================================================

juce_generate_juce_header(ZenithDAW)

# ============================================================================
# Preprocessor Definitions
# ============================================================================

target_compile_definitions(ZenithDAW PRIVATE
    # JUCE app config
    JUCE_APP_VERSION="0.1.0"
    JUCE_APP_VERSION_HEX=0x000100

    # Application name
    JucePlugin_Name="Zenith DAW"

    # Audio settings
    JUCE_USE_OGGVORBIS=1
    JUCE_USE_FLAC=1
    JUCE_USE_MP3AUDIOFORMAT=1

    # Disable JUCE splash screen (requires Pro license for commercial use)
    # JUCE_DISPLAY_SPLASH_SCREEN=0

    # Phase 3: VST3 hosting enabled
    JUCE_PLUGINHOST_VST3=1
    # Audio Unit hosting (macOS only, will enable in future)
    # JUCE_PLUGINHOST_AU=1

    # Don't auto-open plugin windows
    JUCE_PLUGINHOST_VST3_PLUGIN_EDITOR_SHOWS_BY_DEFAULT=0

    # Strict warnings
    JUCE_STRICT_REFCOUNTEDPOINTER=1

    # Disable analytics
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0

    # Debug builds
    $<$<CONFIG:Debug>:DEBUG=1>
    $<$<CONFIG:Debug>:_DEBUG=1>

    # Release builds
    $<$<CONFIG:Release>:NDEBUG=1>

    # C3: Optional debug seed for track testing
    $<$<BOOL:${ZENITH_ENGINE_SEED_DEBUG_TRACKS}>:ZENITH_ENGINE_SEED_DEBUG_TRACKS=1>
)

# ============================================================================
# ProjectState Tests
# ============================================================================

juce_add_console_app(ProjectStateTests
    PRODUCT_NAME "ProjectStateTests"
)

target_sources(ProjectStateTests PRIVATE
    tests/ProjectStateTests.cpp
    src/ProjectState.cpp
)

target_include_directories(ProjectStateTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(ProjectStateTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
)

target_compile_definitions(ProjectStateTests PRIVATE
    JUCE_USE_CURL=0
)

add_test(NAME ProjectStateTests COMMAND ProjectStateTests)

# ============================================================================
# Recording & Automation Tests
# ============================================================================

juce_add_console_app(RecordingAutomationTests
    PRODUCT_NAME "RecordingAutomationTests"
)

target_sources(RecordingAutomationTests PRIVATE
    tests/RecordingAutomationTests.cpp
    src/ProjectState.cpp
)

target_include_directories(RecordingAutomationTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/engine
)

target_link_libraries(RecordingAutomationTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_audio_basics
)

target_compile_definitions(RecordingAutomationTests PRIVATE
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
)

# Generate JuceHeader.h for this test target
juce_generate_juce_header(RecordingAutomationTests)

add_test(NAME RecordingAutomationTests COMMAND RecordingAutomationTests)

# ============================================================================
# Instrument Validation Tests
# ============================================================================

juce_add_console_app(InstrumentValidationTests
    PRODUCT_NAME "InstrumentValidationTests"
)

target_sources(InstrumentValidationTests PRIVATE
    tests/InstrumentValidationTests.cpp

    # Instrument system
    Source/instruments/Instrument.cpp
    Source/instruments/InstrumentRegistry.cpp
    Source/instruments/ZenithPolySynth.cpp
    Source/instruments/ZenithSampler.cpp
    Source/instruments/RegisterBuiltInInstruments.cpp
)

target_include_directories(InstrumentValidationTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/instruments
)

target_link_libraries(InstrumentValidationTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_audio_formats
    juce::juce_dsp
)

target_compile_definitions(InstrumentValidationTests PRIVATE
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_MODAL_LOOPS_PERMITTED=1
)

# Generate JuceHeader.h for this test target
juce_generate_juce_header(InstrumentValidationTests)

add_test(NAME InstrumentValidationTests COMMAND InstrumentValidationTests)

# ============================================================================
# Preset Generator Tool
# ============================================================================

juce_add_console_app(GeneratePolySynthPresets
    PRODUCT_NAME "GeneratePolySynthPresets"
)

target_sources(GeneratePolySynthPresets PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../tools/GeneratePolySynthPresets.cpp
)

target_include_directories(GeneratePolySynthPresets PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/instruments
)

target_link_libraries(GeneratePolySynthPresets PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
)

target_compile_definitions(GeneratePolySynthPresets PRIVATE
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_MODAL_LOOPS_PERMITTED=1
)

# Generate JuceHeader.h for this tool
juce_generate_juce_header(GeneratePolySynthPresets)

# ============================================================================
# Compiler Warnings
# ============================================================================

if(MSVC)
    target_compile_options(ZenithDAW PRIVATE /W4)
else()
    target_compile_options(ZenithDAW PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter  # JUCE callbacks often have unused params
    )
endif()

# ============================================================================
# Binary Data (Resources)
# ============================================================================

# Will add this when we have resources
# juce_add_binary_data(ZenithResources SOURCES
#     resources/fonts/Inter-Regular.ttf
#     resources/fonts/Inter-Bold.ttf
# )
#
# target_link_libraries(ZenithDAW PRIVATE ZenithResources)

# ============================================================================
# Post-Build Messages
# ============================================================================

message(STATUS "")
message(STATUS "========================================")
message(STATUS "Zenith DAW Configuration")
message(STATUS "========================================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "JUCE Version: 8.0.9")

if(APPLE)
    message(STATUS "macOS Deployment Target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
    message(STATUS "macOS Architectures: ${CMAKE_OSX_ARCHITECTURES}")
endif()

message(STATUS "========================================")
message(STATUS "")

# ============================================================================
# Installation (optional)
# ============================================================================

install(TARGETS ZenithDAW
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

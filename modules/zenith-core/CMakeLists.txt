cmake_minimum_required(VERSION 3.22)

# Set MSVC runtime to static BEFORE project() call to match Skia
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)

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
option(ZENITH_ENABLE_VEXEL_DONOR   "Enable donor compile checks from ZenithDAW-Native" OFF)
option(ZENITH_ENABLE_SKIA          "Enable Skia rendering backend" ON)

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
    src/TempoMap.cpp
    src/ProjectState.cpp
    src/MixerComponent.cpp

    # Phase 13: Automation system
    src/TrackAutomationSynchronizer.cpp

    # Integration: UI components and synchronizers
    src/ClipSynchronizer.cpp
    # src/ArrangerView.cpp - Moved to src/ui/
    # src/PianoRollEditor.cpp - Moved to src/ui/
    include/ClipSynchronizer.h
    include/ArrangerView.h
    include/PianoRollEditor.h

    # Additional UI Components
    src/PianoRollComponent.cpp
    src/AutomationLaneComponent.cpp
    # Temporarily disabled - incomplete implementation
    # src/MarkerLaneComponent.cpp
    # src/TempoLaneComponent.cpp
    # src/TempoMapSynchronizer.cpp
    # src/PluginBrowserComponent.cpp

    # Phase 11: Mixer → Engine wiring
    src/TrackStateSynchronizer.cpp

    # Engine primitives (all from src/engine/)
    src/engine/Track.h
    src/engine/Track.cpp
    src/engine/Clip.h
    src/engine/Clip.cpp
    src/engine/MixerChannel.h
    src/engine/MixerChannel.cpp
    src/engine/AudioFilePool.h
    src/engine/AudioFilePool.cpp
    src/engine/PluginHost.h
    src/engine/PluginHost.cpp
    src/engine/AuxBus.h
    src/engine/AuxBus.cpp
    src/engine/ExportEngine.h
    src/engine/ExportEngine.cpp

    # Phase 5: Wingman v0 (AI Command Console)
    src/commands/CommandAPI.h
    src/commands/CommandAPI.cpp
    src/commands/SessionGraph.h
    src/commands/SessionGraph.cpp

    # Phase 7: Wingman AI Integration
    src/network/AIBridgeClient.h
    src/network/AIBridgeClient.cpp

    # Instrument system (all from src/instruments/)
    src/instruments/Instrument.h
    src/instruments/Instrument.cpp
    src/instruments/InstrumentRegistry.h
    src/instruments/InstrumentRegistry.cpp
    src/instruments/ZenithPolySynth.h
    src/instruments/ZenithPolySynth.cpp
    src/instruments/ZenithPresetManager.h
    src/instruments/ZenithPresetManager.cpp
    src/instruments/ZenithSampler.h
    src/instruments/ZenithSampler.cpp
    src/instruments/ZenithSamplerEditor.h
    src/instruments/ZenithSamplerEditor.cpp
    src/instruments/RegisterBuiltInInstruments.h
    src/instruments/RegisterBuiltInInstruments.cpp
    src/instruments/ZenithPolySynthEditor.h
    src/instruments/ZenithPolySynthEditor.cpp
    include/InstrumentCommandHelpers.h

    # UI Components (from src/ui/)
    src/ui/ArrangerView.cpp
    src/ui/ClipComponent.cpp
    src/ui/MixerChannelComponent.cpp
    src/ui/MixerView.cpp
    src/ui/PianoRollEditor.cpp
    src/ui/TimelineRuler.cpp
    src/ui/TrackHeaderComponent.cpp
    src/ui/WingmanPanel.cpp
    src/ui/InstrumentBrowserPanel.cpp
    # Note: ZenithSlider.cpp is conditionally included based on ZENITH_ENABLE_SKIA

    # AI Components
    include/ai/OpenAIClient.h
    src/ai/OpenAIClient.cpp
)

# Conditionally add Skia UI components if Skia is enabled
if(ZENITH_ENABLE_SKIA)
    target_sources(ZenithDAW PRIVATE
        src/ui/skia/ZenithPolySynthUI.h
        src/ui/skia/ZenithPolySynthUI.cpp
        src/ui/skia/ZenithUIComponents.h
        src/ui/skia/SkiaMainWindowIntegration.h
        src/ui/skia/SkiaMainWindowIntegration.cpp
        src/ui/skia/TransportBar.h
        src/ui/skia/TransportBar.cpp
        src/ui/skia/BottomBar.h
        src/ui/skia/BottomBar.cpp
        src/ui/skia/RightSidePanel.h
        src/ui/skia/RightSidePanel.cpp
        src/ui/skia/views/PianoKeyboardViewSkia.h
        src/ui/skia/views/PianoKeyboardViewSkia.cpp
        src/ui/MainLayoutComponent.h
        src/ui/MainLayoutComponent.cpp
        src/ArrangementComponent.cpp
    )
else()
    # Include JUCE fallback slider only when not using Skia
    target_sources(ZenithDAW PRIVATE
        src/ui/ZenithSlider.cpp
    )
endif()

# ============================================================================
# Skia Rendering (conditional) - Manual Integration
# ============================================================================
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/SkiaManualIntegration.cmake)

# ============================================================================
# Include Directories
# ============================================================================

target_include_directories(ZenithDAW PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands
    ${CMAKE_CURRENT_SOURCE_DIR}/src/network
    ${CMAKE_CURRENT_SOURCE_DIR}/src/instruments
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering
    ${CMAKE_CURRENT_BINARY_DIR}/ZenithDAW_artefacts/JuceLibraryCode
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
    juce::juce_opengl

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

    # Skia rendering backend
    $<$<BOOL:${ZENITH_ENABLE_SKIA}>:ZENITH_USE_SKIA=1>
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
# Tempo/BPM Logic Tests
# ============================================================================

juce_add_console_app(TempoTests
    PRODUCT_NAME "TempoTests"
)

target_sources(TempoTests PRIVATE
    tests/TempoTests.cpp
    src/ProjectState.cpp
)

target_include_directories(TempoTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(TempoTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
)

target_compile_definitions(TempoTests PRIVATE
    JUCE_USE_CURL=0
)

add_test(NAME TempoTests COMMAND TempoTests)

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
    ${CMAKE_CURRENT_BINARY_DIR}/RecordingAutomationTests_artefacts/JuceLibraryCode
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

    # Audio file pool (needed by ZenithSampler)
    src/engine/AudioFilePool.cpp

    # Instrument system
    src/instruments/Instrument.cpp
    src/instruments/InstrumentRegistry.cpp
    src/instruments/ZenithPolySynth.cpp
    src/instruments/ZenithSampler.cpp
    src/instruments/RegisterBuiltInInstruments.cpp
)

target_include_directories(InstrumentValidationTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/src/instruments
    ${CMAKE_CURRENT_BINARY_DIR}/InstrumentValidationTests_artefacts/JuceLibraryCode
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
# Preset & CommandAPI Regression Tests
# ============================================================================

juce_add_console_app(PresetRegressionTests
    PRODUCT_NAME "PresetRegressionTests"
)

target_sources(PresetRegressionTests PRIVATE
    tests/PresetRegressionTests.cpp

    # Audio file pool (needed by ZenithSampler)
    src/engine/AudioFilePool.cpp

    # Instrument system
    src/instruments/Instrument.cpp
    src/instruments/InstrumentRegistry.cpp
    src/instruments/ZenithPolySynth.cpp
    src/instruments/ZenithSampler.cpp
    src/instruments/RegisterBuiltInInstruments.cpp
)

target_include_directories(PresetRegressionTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/src/instruments
    ${CMAKE_CURRENT_BINARY_DIR}/PresetRegressionTests_artefacts/JuceLibraryCode
)

target_link_libraries(PresetRegressionTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_audio_formats
    juce::juce_dsp
)

target_compile_definitions(PresetRegressionTests PRIVATE
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_MODAL_LOOPS_PERMITTED=1
)

# Generate JuceHeader.h for this test target
juce_generate_juce_header(PresetRegressionTests)

add_test(NAME PresetRegressionTests COMMAND PresetRegressionTests)

# ============================================================================
# Preset Generator Tool
# ============================================================================
# Disabled: tools directory doesn't exist
# juce_add_console_app(GeneratePolySynthPresets
#     PRODUCT_NAME "GeneratePolySynthPresets"
# )
#
# target_sources(GeneratePolySynthPresets PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/../tools/GeneratePolySynthPresets.cpp
# )
#
# target_include_directories(GeneratePolySynthPresets PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/include
#     ${CMAKE_CURRENT_SOURCE_DIR}/Source/instruments
#     ${CMAKE_CURRENT_BINARY_DIR}/GeneratePolySynthPresets_artefacts/JuceLibraryCode
# )
#
# target_link_libraries(GeneratePolySynthPresets PRIVATE
#     juce::juce_core
#     juce::juce_data_structures
#     juce::juce_events
# )
#
# target_compile_definitions(GeneratePolySynthPresets PRIVATE
#     JUCE_USE_CURL=0
#     JUCE_WEB_BROWSER=0
#     JUCE_MODAL_LOOPS_PERMITTED=1
# )
#
# # Generate JuceHeader.h for this tool
# juce_generate_juce_header(GeneratePolySynthPresets)

# ============================================================================
# Track + Instrument Integration Tests
# ============================================================================

juce_add_console_app(TrackInstrumentIntegrationTests
    PRODUCT_NAME "TrackInstrumentIntegrationTests"
)

target_sources(TrackInstrumentIntegrationTests PRIVATE
    tests/TrackInstrumentIntegrationTests.cpp

    # Track system (each test target needs its own sources)
    src/engine/Track.cpp
    src/engine/Clip.cpp
    src/engine/AudioFilePool.cpp
    src/engine/PluginHost.cpp

    # Instrument system
    src/instruments/Instrument.cpp
    src/instruments/InstrumentRegistry.cpp
    src/instruments/ZenithPolySynth.cpp
    src/instruments/ZenithSampler.cpp
    src/instruments/RegisterBuiltInInstruments.cpp
)

target_include_directories(TrackInstrumentIntegrationTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/src/instruments
    ${CMAKE_CURRENT_BINARY_DIR}/TrackInstrumentIntegrationTests_artefacts/JuceLibraryCode
)

target_link_libraries(TrackInstrumentIntegrationTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_audio_formats
    juce::juce_dsp
)

target_compile_definitions(TrackInstrumentIntegrationTests PRIVATE
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_MODAL_LOOPS_PERMITTED=1
)

# Generate JuceHeader.h for this test target
juce_generate_juce_header(TrackInstrumentIntegrationTests)

add_test(NAME TrackInstrumentIntegrationTests COMMAND TrackInstrumentIntegrationTests)

# ============================================================================
# Skia Integration Verification Test (only when Skia is enabled)
# ============================================================================

if(ZENITH_ENABLE_SKIA)
    # Simple linkage test (minimal dependencies)
    juce_add_console_app(SkiaSimpleTest
        PRODUCT_NAME "SkiaSimpleTest"
    )

    target_sources(SkiaSimpleTest PRIVATE
        tests/SkiaSimpleTest.cpp
    )

    target_link_libraries(SkiaSimpleTest PRIVATE
        juce::juce_core
        ${SKIA_LIBRARY}
    )

    target_include_directories(SkiaSimpleTest PRIVATE
        ${SKIA_INCLUDE_DIR}
        ${SKIA_DIR}/include
    )

    target_compile_definitions(SkiaSimpleTest PRIVATE
        JUCE_USE_CURL=0
        ZENITH_USE_SKIA=1
        SK_GL=1
    )

    juce_generate_juce_header(SkiaSimpleTest)
    add_test(NAME SkiaSimpleTest COMMAND SkiaSimpleTest)

    # TODO: Comprehensive integration test (with SkiaRenderer) - disabled due to compilation errors
    # This test file appears to be experimental/obsolete and needs refactoring
    # Re-enable when the test has been properly validated
    # juce_add_console_app(SkiaIntegrationVerification
    #     PRODUCT_NAME "SkiaIntegrationVerification"
    # )
    #
    # target_sources(SkiaIntegrationVerification PRIVATE
    #     tests/SkiaIntegrationVerification.cpp
    #
    #     # SkiaRenderer implementation
    #     Source/rendering/SkiaRenderer.cpp
    # )
    #
    # target_include_directories(SkiaIntegrationVerification PRIVATE
    #     ${CMAKE_CURRENT_SOURCE_DIR}/include
    #     ${CMAKE_CURRENT_SOURCE_DIR}/Source/rendering
    #     ${CMAKE_CURRENT_BINARY_DIR}/SkiaIntegrationVerification_artefacts/JuceLibraryCode
    # )
    #
    # target_link_libraries(SkiaIntegrationVerification PRIVATE
    #     juce::juce_core
    #     juce::juce_graphics
    #     juce::juce_gui_basics
    #     juce::juce_events
    #     unofficial::skia::skia
    # )
    #
    # target_compile_definitions(SkiaIntegrationVerification PRIVATE
    #     JUCE_USE_CURL=0
    #     JUCE_WEB_BROWSER=0
    #     ZENITH_USE_SKIA=1
    #     ZENITH_ENABLE_SKIA=1
    #     SK_GL=1
    # )
    #
    # # Generate JuceHeader.h for this test target
    # juce_generate_juce_header(SkiaIntegrationVerification)
    #
    # add_test(NAME SkiaIntegrationVerification COMMAND SkiaIntegrationVerification)
    #
    # message(STATUS "Skia Integration Verification tests ENABLED")
endif()

# ============================================================================
# Compiler Warnings and Runtime Library
# ============================================================================

if(MSVC)
    target_compile_options(ZenithDAW PRIVATE
        /W4
        /wd4244  # Suppress: conversion from 'int' to 'float', possible loss of data (Skia headers)
        /wd4267  # Suppress: conversion from 'size_t' to 'int', possible loss of data (Skia headers)
        /wd4201  # Suppress: nonstandard extension used: nameless struct/union (Skia headers)
        /wd4100  # Allow unreferenced parameters (JUCE callbacks often have these)
        /wd4189  # Allow unreferenced local variables (intentional in some cases)
        /wd4996  # Suppress: deprecated JUCE Font constructor (will fix when updating JUCE)
    )
else()
    target_compile_options(ZenithDAW PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
        -Wno-conversion
        -Wno-unused-variable
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

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

# ALEX'S FIX: Use short path to avoid Windows 260-char limit
set(FETCHCONTENT_BASE_DIR "C:/juce_deps_3" CACHE PATH "FetchContent dependencies")
set(CMAKE_OBJECT_PATH_MAX 240)

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
    src/ArrangerView.cpp
    src/PianoRollEditor.cpp
    include/ClipSynchronizer.h
    include/ArrangerView.h
    include/PianoRollEditor.h

    # Phase 11: Mixer → Engine wiring
    src/TrackStateSynchronizer.cpp
    src/MixerComponent.cpp

    # Engine primitives (ported from ZenithDAW-Native, Phase 2)
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
    # Source/ui/ArrangerComponent.h
    src/ArrangerComponent.cpp
    Source/ui/PianoRollComponent.h
    Source/ui/PianoRollComponent.cpp

    # Phase 5: Wingman v0 (AI Command Console)
    Source/commands/CommandAPI.h
    Source/commands/CommandAPI.cpp
    Source/commands/SessionGraph.h
    Source/commands/SessionGraph.cpp
    Source/ui/WingmanPanel.h
    Source/ui/WingmanPanel.cpp
    Source/ui/AudioFeedback.h
    Source/ui/AudioFeedback.cpp

    # Instrument Browser UI
    Source/ui/InstrumentBrowserPanel.h
    Source/ui/InstrumentBrowserPanel.cpp
    Source/ui/PresetBrowserComponent.h
    Source/ui/PresetBrowserComponent.cpp

    # Logic Pro Look & Feel System
    Source/ui/ZenithLookAndFeel.h
    Source/ui/ZenithLookAndFeel.cpp
    Source/ui/ZenithTypography.h

    # Phase 7: Wingman AI Integration
    Source/network/AIBridgeClient.h
    Source/network/AIBridgeClient.cpp
    Source/instruments/InstrumentRegistry.h
    Source/instruments/InstrumentRegistry.cpp
    Source/instruments/ContentPaths.h
    Source/instruments/ZenithPolySynth.h
    Source/instruments/ZenithPolySynth.cpp
    Source/instruments/ZenithPresetManager.h
    Source/instruments/ZenithPresetManager.cpp
    Source/instruments/ZenithSampler.h
    # Source/instruments/ZenithSampler.cpp  # TODO: Fix constructor definitions
    Source/instruments/ZenithSamplerEditor.h
    # Source/instruments/ZenithSamplerEditor.cpp  # Depends on ZenithSampler
    Source/instruments/RegisterBuiltInInstruments.h
    # Source/instruments/RegisterBuiltInInstruments.cpp  # Depends on ZenithSampler
    Source/instruments/ZenithPolySynthEditor.h
    Source/instruments/ZenithPolySynthEditor.cpp

    # Phase 8: AI DSP
    Source/dsp/DSPStemSeparator.h
    Source/dsp/DSPStemSeparator.cpp
    Source/dsp/DSPVoiceChanger.h
    Source/dsp/DSPVoiceChanger.cpp
    Source/dsp/ONNXStemSeparator.h
    Source/dsp/ONNXStemSeparator.cpp
    
    # AI Preset Generation (NEW - Phase 8) - TODO: Create these files
    # Source/instruments/PresetGenerator.h
    # Source/instruments/PresetGenerator.cpp
    
    include/InstrumentCommandHelpers.h
)

# ============================================================================
# Skia Rendering (vcpkg)
# ============================================================================
if(ZENITH_ENABLE_SKIA)
    message(STATUS "Locating Skia via vcpkg...")
    find_package(skia CONFIG REQUIRED)
    
    # Link Skia
    target_link_libraries(ZenithDAW PRIVATE skia)
    
    # Fix for Skia internal includes (e.g. SkPoint.h including SkPoint_impl.h)
    # We need to add the 'skia' directory to the include path so 'include/private/...' works.
    # Assuming standard vcpkg layout where skia_DIR is .../share/skia
    get_filename_component(SKIA_ROOT "${skia_DIR}/../../include/skia" ABSOLUTE)
    target_include_directories(ZenithDAW PRIVATE ${SKIA_ROOT})
    
    # Enable Skia usage in code
    target_compile_definitions(ZenithDAW PRIVATE 
        ZENITH_USE_SKIA=1 
        SK_GL=1
        NOMINMAX
    )

    # Add Skia UI sources
    target_sources(ZenithDAW PRIVATE
        Source/ui/skia/ZenithUIComponents.h
        Source/ui/skia/ZenithPolySynthUI.h
        Source/ui/skia/ZenithPolySynthUI.cpp
        Source/ui/skia/TestSkia.cpp
        Source/ui/skia/BottomBar.cpp
        Source/ui/skia/RightSidePanel.cpp
        Source/ui/skia/TransportBar.cpp
        Source/ui/skia/SkiaMainWindowIntegration.cpp
        Source/ui/skia/BrowserPanel.cpp
        Source/ui/views/PianoKeyboardViewSkia.cpp
        Source/ui/MainLayoutComponent.cpp
    )
    
    target_include_directories(ZenithDAW PRIVATE
        Source/ui/skia
        Source/ui/views
    )
    
    message(STATUS "Skia integrated successfully via vcpkg")
endif()

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
)

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

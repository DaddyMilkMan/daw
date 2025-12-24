cmake_minimum_required(VERSION 3.25)

# 1. Project Definition
project(ZenithDAW 
    VERSION 0.1.0
    LANGUAGES CXX C
)

# 2. Build Performance Optimizations
# Early-stage optimizations that must be set before target creation
include(apps/desktop/cmake/BuildOptimizations.cmake)

# 3. Default Build Type
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type selected, defaulting to Release")
    set(CMAKE_BUILD_TYPE "Release" CA STRING "Build type (Debug, Release, RelWithDebInfo, MinSizeRel)" FORCE)
endif()

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FS /bigobj" CACHE STRING "Flags used by the CXX compiler during all build types." FORCE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /FS /bigobj" CACHE STRING "Flags used by the C compiler during all build types." FORCE)
endif()

# 4. Options
option(ENABLE_IPO "Enable Interprocedural Optimization (LTO)" OFF)
option(ENABLE_HARDENING "Enable Security Hardening Flags" OFF)
option(ENABLE_SANITIZERS "Enable Address and UB Sanitizers" OFF)
option(ENABLE_SPECTRE "Enable Spectre Mitigations (MSVC)" OFF)

# Auto-enable sanitizers in Debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    option(ENABLE_SANITIZERS "Enable Address and UB Sanitizers" ON)
    option(ENABLE_HARDENING "Enable Security Hardening Flags" ON)
else()
    option(ENABLE_SANITIZERS "Enable Address and UB Sanitizers" OFF)
    option(ENABLE_HARDENING "Enable Security Hardening Flags" OFF)
endif()

if(ENABLE_SANITIZERS AND MSVC)
    add_compile_options(/fsanitize=address)
endif()

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

# Source files
set(ZENITH_SOURCES
    apps/desktop/Source/Main.cpp
    apps/desktop/Source/utils/SampleGenerator.cpp
)

# Core engine and command sources
set(ZENITH_CORE_SOURCES
    apps/desktop/Source/engine/Engine.cpp
    apps/desktop/Source/engine/EngineTransport.cpp
    apps/desktop/Source/engine/EngineTrackManagement.cpp
    apps/desktop/Source/engine/EngineMixing.cpp
    apps/desktop/Source/engine/EngineRecording.cpp
    apps/desktop/Source/engine/EngineExport.cpp
    apps/desktop/Source/engine/ProjectState.cpp
    apps/desktop/Source/engine/Track.cpp
    apps/desktop/Source/engine/Clip.cpp
    apps/desktop/Source/engine/MixerChannel.cpp
    apps/desktop/Source/engine/AudioFilePool.cpp
    apps/desktop/Source/engine/PluginHost.cpp
    apps/desktop/Source/engine/AuxBus.cpp
    apps/desktop/Source/engine/ZenithLogger.cpp
    apps/desktop/Source/engine/TrackPluginState.cpp
    apps/desktop/Source/engine/AudioRecorder.cpp
    apps/desktop/Source/engine/RoutingGraph.cpp
    apps/desktop/Source/engine/ClipSynchronizer.cpp
    apps/desktop/Source/engine/TrackStateSynchronizer.cpp
    apps/desktop/Source/engine/TrackAutomationSynchronizer.cpp
    apps/desktop/Source/engine/TempoMapSynchronizer.cpp
    apps/desktop/Source/engine/TempoMap.cpp
    apps/desktop/Source/engine/TrackFreeze.cpp
    apps/desktop/Source/engine/AudioRenderer.cpp
    apps/desktop/Source/engine/RecordingManager.cpp
    apps/desktop/Source/engine/TransportController.cpp
    apps/desktop/Source/engine/MeteringSystem.cpp
    apps/desktop/Source/engine/AutomationStateManager.cpp
    apps/desktop/Source/engine/Metronome.cpp
    apps/desktop/Source/engine/AudioTrack.cpp
    apps/desktop/Source/engine/MIDITrack.cpp
    apps/desktop/Source/engine/ClipTrack.cpp
    apps/desktop/Source/engine/AuxBusTrack.cpp
    apps/desktop/Source/engine/ClipStateManager.cpp
    apps/desktop/Source/engine/TrackStateManager.cpp
    apps/desktop/Source/engine/AutomationManager.cpp
    apps/desktop/Source/engine/PluginChain.cpp
    apps/desktop/Source/engine/RecentProjectManager.cpp
    apps/desktop/Source/engine/ProjectFileIO.cpp
    apps/desktop/Source/engine/ProjectEngineBridge.cpp
    apps/desktop/Source/ui/common/PluginEditorWindow.cpp
    
    # Missing dependencies for Engine
    apps/desktop/Source/commands/CommandAPI.cpp
    apps/desktop/Source/commands/TrackCommands.cpp
    apps/desktop/Source/commands/ClipCommands.cpp
    apps/desktop/Source/commands/TransportCommands.cpp
    apps/desktop/Source/commands/SessionGraph.cpp
    
    apps/desktop/Source/instruments/InstrumentRegistry.cpp
    apps/desktop/Source/instruments/ZenithPolySynth.cpp
    apps/desktop/Source/instruments/ZenithPolySynthVoice.cpp
    apps/desktop/Source/instruments/Instrument.cpp
    apps/desktop/Source/instruments/RegisterBuiltInInstruments.cpp
    apps/desktop/Source/instruments/ZenithPolySynthParameterManager.cpp
    apps/desktop/Source/instruments/ZenithOscillator.cpp
    apps/desktop/Source/instruments/ZenithFilter.cpp
    apps/desktop/Source/instruments/ZenithEffects.cpp
    apps/desktop/Source/instruments/ZenithSampler.cpp
    apps/desktop/Source/instruments/WavetableLoader.cpp
    apps/desktop/Source/instruments/PresetGenerator.cpp
    apps/desktop/Source/instruments/ZenithPresetManager.cpp
    apps/desktop/Source/ai/SessionDebuggerAgent.cpp
    apps/desktop/Source/network/GrokUtils.cpp
)

# Test sources
set(ZENITH_TEST_SOURCES
    apps/desktop/Source/tests/TestMain.cpp
    apps/desktop/Source/tests/AudioEngineTests.cpp
    apps/desktop/Source/tests/ProjectStateTests.cpp
    apps/desktop/Source/tests/EngineTests.cpp
    apps/desktop/Source/tests/ConcurrencyTests.cpp
    apps/desktop/Source/tests/AITests.cpp
    apps/desktop/Source/tests/UITests.cpp
    apps/desktop/Source/ui/design-system/ZenithLayout.cpp
    apps/desktop/Source/tests/BasicAudioTest.cpp
)

# Create main executable
juce_add_gui_app(ZenithDAW
    PRODUCT_NAME "Zenith DAW"
    COMPANY_NAME "Zenith Audio"
    VERSION ${PROJECT_VERSION}
    BUNDLE_ID com.zenithaudio.daw
)

# Generate JuceHeader.h
juce_generate_juce_header(ZenithDAW)

target_sources(ZenithDAW PRIVATE ${ZENITH_SOURCES} ${ZENITH_CORE_SOURCES})

# Skia Integration
option(ZENITH_ENABLE_SKIA "Enable Skia Rendering" ON)

# Modularized Source Groups
add_subdirectory(apps/desktop/Source/engine)
add_subdirectory(apps/desktop/Source/ui)
add_subdirectory(apps/desktop/Source/instruments)
add_subdirectory(apps/desktop/Source/network)
add_subdirectory(apps/desktop/Source/commands)
add_subdirectory(apps/desktop/Source/rendering)
add_subdirectory(apps/desktop/Source/dsp)
add_subdirectory(apps/desktop/Source/browser)
add_subdirectory(apps/desktop/Source/ai)

# Include directories
target_include_directories(ZenithDAW PUBLIC
    ${CMAKE_BINARY_DIR}/ZenithDAW_artefacts/JuceLibraryCode
    apps/desktop/Source
    apps/desktop/Source/engine
    apps/desktop/Source/ui
    apps/desktop/Source/ui/framework
    apps/desktop/Source/ui/design-system
    apps/desktop/Source/ui/arranger
    apps/desktop/Source/ui/mixer
    apps/desktop/Source/ui/piano-roll
    apps/desktop/Source/ui/browser
    apps/desktop/Source/ui/transport
    apps/desktop/Source/ui/common
    apps/desktop/Source/ui/widgets
    apps/desktop/Source/ui/session
    apps/desktop/Source/ui/instruments
    apps/desktop/Source/ui/dialogs
    apps/desktop/Source/ui/sample-editor
    apps/desktop/Source/commands
    apps/desktop/Source/instruments
    apps/desktop/Source/network
    apps/desktop/Source/dsp
    apps/desktop/Source/rendering
    apps/desktop/Source/ai
    apps/desktop/Source/ui/skia
    apps/desktop/Source/utils
    ${CMAKE_PREFIX_PATH}/include
    ${CMAKE_BINARY_DIR}/JuceLibraryCode
    "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/modules"
)

target_link_libraries(ZenithDAW PRIVATE
    juce::juce_core
    juce::juce_cryptography
    juce::juce_events
    juce::juce_graphics
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_opengl
)

# Graphics Backends
if(WIN32)
    target_link_libraries(ZenithDAW PRIVATE d3d12 dxgi dcomp uuid)
elseif(APPLE)
    target_link_libraries(ZenithDAW PRIVATE "-framework Metal" "-framework Foundation" "-framework QuartzCore")
elseif(UNIX AND NOT APPLE)
    find_package(Vulkan)
    if(Vulkan_FOUND)
        target_link_libraries(ZenithDAW PRIVATE Vulkan::Vulkan)
    endif()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FONTCONFIG REQUIRED fontconfig)
    pkg_check_modules(FREETYPE REQUIRED freetype2)
    pkg_check_modules(X11 REQUIRED x11)
    target_include_directories(ZenithDAW PRIVATE ${FONTCONFIG_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS} ${X11_INCLUDE_DIRS})
    target_link_libraries(ZenithDAW PRIVATE ${FONTCONFIG_LIBRARIES} ${FREETYPE_LIBRARIES} ${X11_LIBRARIES})
endif()

# Skia
if(ZENITH_ENABLE_SKIA)
    include(apps/desktop/cmake/SkiaManualIntegration.cmake)
    target_compile_definitions(ZenithDAW PRIVATE ZENITH_USE_SKIA=1)
endif()

enable_zenith_build_optimizations(ZenithDAW)
include(apps/desktop/cmake/PrecompiledHeaders.cmake)

target_compile_features(ZenithDAW PRIVATE cxx_std_20)

# Tests Target
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    juce_add_console_app(ZenithDAWTests PRODUCT_NAME "Zenith DAW Tests")
    juce_generate_juce_header(ZenithDAWTests)
    target_sources(ZenithDAWTests PRIVATE ${ZENITH_TEST_SOURCES})
    target_include_directories(ZenithDAWTests PRIVATE 
        apps/desktop/Source 
        apps/desktop/Source/engine
        apps/desktop/Source/instruments
        apps/desktop/Source/ui
        apps/desktop/Source/ui/arranger
        apps/desktop/Source/ui/browser
        apps/desktop/Source/ui/common
        apps/desktop/Source/ui/framework
        apps/desktop/Source/ui/instruments
        apps/desktop/Source/ui/design-system
        apps/desktop/Source/ui/widgets
        apps/desktop/Source/ai
        apps/desktop/Source/network
        apps/desktop/Source/dsp
        apps/desktop/Source/automation
        apps/desktop/Source/ui/dialogs
        apps/desktop/Source/ui/panels
        apps/desktop/Source/ui/visualization
        apps/desktop/Source/ui/controls
        apps/desktop/Source/ui/piano-roll
        apps/desktop/include
        ${CMAKE_BINARY_DIR}/ZenithDAWTests_artefacts/JuceLibraryCode
        "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/modules"
    )
    target_link_libraries(ZenithDAWTests PRIVATE
        juce::juce_core
        juce::juce_data_structures
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_opengl
    )
    target_compile_features(ZenithDAWTests PRIVATE cxx_std_20)
endif()
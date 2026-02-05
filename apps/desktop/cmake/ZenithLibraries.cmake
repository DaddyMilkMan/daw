# ==============================================================================
# ZenithLibraries.cmake
# Defines modular static libraries for the Zenith DAW project
# ==============================================================================
# This file separates the monolithic build into discrete libraries:
# - ZenithCore: Engine without UI (enables headless use cases)
# - Future: ZenithAudio (DSP, stem separation), ZenithUI (all UI components)
# ==============================================================================

# ==============================================================================
# ZenithCore - Engine without UI
# ==============================================================================
# Contains all core audio engine functionality without any UI dependencies.
# This enables:
# - Faster incremental builds (UI changes don't recompile engine)
# - Headless rendering and batch processing
# - Unit testing without UI framework
# ==============================================================================

add_library(ZenithCore STATIC
    # Core Engine Files
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/engine/Engine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/engine/Track.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/engine/Clip.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/engine/ProjectState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/engine/RoutingGraph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/engine/MixerChannel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/engine/AudioFilePool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/engine/PluginHost.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/engine/AuxBus.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/engine/TempoMap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/engine/AudioRecorder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/engine/AudioRenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/engine/RecordingManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/engine/TrackFreeze.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/engine/ZenithLogger.cpp
    
    # Additional Engine Components
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/engine/TrackPluginState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/engine/TransportController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/engine/ProjectEngineBridge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/engine/TrackStateManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/engine/ClipStateManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/engine/AutomationStateManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/engine/ProjectFileIO.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/engine/RecentProjectManager.cpp
    
    # Synchronizers (legacy, pending removal)
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/engine/ClipSynchronizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/engine/TrackStateSynchronizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/engine/TrackAutomationSynchronizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/engine/TempoMapSynchronizer.cpp
)

# Include directories for ZenithCore
target_include_directories(ZenithCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/modules
)

# JUCE dependencies required by the engine (no UI modules)
target_link_libraries(ZenithCore PUBLIC
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_data_structures
    juce::juce_events
)

# C++ Standard for ZenithCore
target_compile_features(ZenithCore PUBLIC cxx_std_20)
set_target_properties(ZenithCore PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
)

# Compile definitions for ZenithCore
target_compile_definitions(ZenithCore PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
)

# Windows-specific flags
if(MSVC)
    target_compile_options(ZenithCore PRIVATE /FS /bigobj)
endif()

message(STATUS "ZenithCore static library configured with ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/engine sources")

# ==============================================================================
# Future Libraries (placeholders for documentation)
# ==============================================================================
# ZenithAudio - DSP, stem separation, audio processing
# ZenithUI - All UI components (Skia-based rendering)
# ==============================================================================

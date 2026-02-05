# =============================================================================
# ZENITH CORE MODULE
# =============================================================================

# Core system modules
set(ZENITH_CORE_SOURCES
    modules/zenith_core/engine/Engine.cpp
    modules/zenith_core/engine/EngineConstants.h
    modules/zenith_core/engine/EngineEvent.h
    modules/zenith_core/engine/ExportCommon.h
    modules/zenith_core/engine/EngineExport.cpp
    modules/zenith_core/engine/EngineMIDI.cpp
    modules/zenith_core/engine/EngineMixing.cpp
    modules/zenith_core/engine/EnginePlugins.cpp
    modules/zenith_core/engine/EngineRecording.cpp
    modules/zenith_core/engine/EngineSync.cpp
    modules/zenith_core/engine/EngineTrackManagement.cpp
    modules/zenith_core/engine/EngineTransport.cpp
    modules/zenith_core/engine/GrokGodModeHelper.cpp
    modules/zenith_core/engine/GrokServiceRegistry.h
    modules/zenith_core/engine/IDService.h
    modules/zenith_core/engine/MemoryManager.h
    modules/zenith_core/engine/RTSafetyChecks.h
    modules/zenith_core/engine/RealTimeGarbageCollector.cpp
    modules/zenith_core/engine/RealTimeGarbageCollector.h
    modules/zenith_core/engine/RecentProjectManager.cpp
    modules/zenith_core/engine/RecordingManager.cpp
    modules/zenith_core/engine/RoutingGraph.cpp
    modules/zenith_core/engine/ScriptableProcessor.cpp
    modules/zenith_core/engine/TakeFolder.cpp
    modules/zenith_core/engine/TempoMap.cpp
    modules/zenith_core/engine/TempoMapSynchronizer.cpp
    modules/zenith_core/engine/ThreadSafeAudioProcessor.h
    modules/zenith_core/engine/Track.cpp
    modules/zenith_core/engine/Track.h
    modules/zenith_core/engine/TrackAutomationSynchronizer.cpp
    modules/zenith_core/engine/TrackManager.h
    modules/zenith_core/engine/TrackPluginManager.cpp
    modules/zenith_core/engine/TrackProcessor.cpp
    modules/zenith_core/engine/TrackSidechain.cpp
    modules/zenith_core/engine/TrackStateManager.cpp
    modules/zenith_core/engine/TrackStateSynchronizer.cpp
    modules/zenith_core/engine/TransportController.cpp
    modules/zenith_core/engine/WCETMonitor.cpp
    modules/zenith_core/engine/ZenithLogger.cpp
)

# Create core library
add_library(zenith_core STATIC
    ${ZENITH_CORE_SOURCES}
)

# Target properties
target_include_directories(zenith_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules/zenith_core/engine>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules/zenith_core/dsp>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules/zenith_core/effects>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules/zenith_core/audio>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules/zenith_core/analysis>
        $<INSTALL_INTERFACE:include>
    PRIVATE
)

target_compile_features(zenith_core
    PUBLIC
        cxx_std_14
)

target_link_libraries(zenith_core
    PUBLIC
        zenith_dsp
        zenith_audio_formats
        juce_core
        juce_events
        juce_data_structures
    PRIVATE
        zenith_audio_utils
        juce_audio_basics
        juce_audio_devices
)

# Installation
install(
    TARGETS zenith_core
    EXPORT ZenithTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
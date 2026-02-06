# =============================================================================
# ZENITH CORE MODULE
# =============================================================================

include_guard(GLOBAL)

# Core system modules
set(ZENITH_CORE_SOURCES
    modules/zenith_core/engine/AudioAsMidiBridge.cpp
    modules/zenith_core/engine/AudioExporter.cpp
    modules/zenith_core/engine/AudioFilePool.cpp
    modules/zenith_core/engine/AudioRecorder.cpp
    modules/zenith_core/engine/AudioRenderer.cpp
    modules/zenith_core/engine/AudioTrack.cpp
    modules/zenith_core/engine/AutomationManager.cpp
    modules/zenith_core/engine/AutomationStateManager.cpp
    modules/zenith_core/engine/AuxBus.cpp
    modules/zenith_core/engine/AuxBusTrack.cpp
    modules/zenith_core/engine/Clip.cpp
    modules/zenith_core/engine/ClipStateManager.cpp
    modules/zenith_core/engine/ClipSynchronizer.cpp
    modules/zenith_core/engine/ClipTrack.cpp
    modules/zenith_core/engine/Engine.cpp
    modules/zenith_core/engine/EngineExport.cpp
    modules/zenith_core/engine/EngineMIDI.cpp
    modules/zenith_core/engine/EngineMixing.cpp
    modules/zenith_core/engine/EnginePlugins.cpp
    modules/zenith_core/engine/EngineRecording.cpp
    modules/zenith_core/engine/EngineSync.cpp
    modules/zenith_core/engine/EngineTrackManagement.cpp
    modules/zenith_core/engine/EngineTransport.cpp
    modules/zenith_core/engine/ExportJob.cpp
    modules/zenith_core/engine/GrokGodModeHelper.cpp
    modules/zenith_core/engine/InstrumentTrack.cpp
    modules/zenith_core/engine/MasterLimiter.cpp
    modules/zenith_core/engine/MeteringSystem.cpp
    modules/zenith_core/engine/Metronome.cpp
    modules/zenith_core/engine/Midi2DiscoveryService.cpp
    modules/zenith_core/engine/MidiNoteStateManager.cpp
    modules/zenith_core/engine/MIDITrack.cpp
    modules/zenith_core/engine/MixerChannel.cpp
    modules/zenith_core/engine/MixerController.cpp
    modules/zenith_core/engine/PlatformAudioUtils.cpp
    modules/zenith_core/engine/PluginAutomationBinding.cpp
    modules/zenith_core/engine/PluginBlacklist.cpp
    modules/zenith_core/engine/PluginChain.cpp
    modules/zenith_core/engine/PluginHost.cpp
    modules/zenith_core/engine/ProjectEngineBridge.cpp
    modules/zenith_core/engine/ProjectFileIO.cpp
    modules/zenith_core/engine/ProjectState.cpp
    modules/zenith_core/engine/PropertyExchangeManager.cpp
    modules/zenith_core/engine/RealTimeGarbageCollector.cpp
    modules/zenith_core/engine/RecentProjectManager.cpp
    modules/zenith_core/engine/RecordingManager.cpp
    modules/zenith_core/engine/RoutingGraph.cpp
    modules/zenith_core/engine/Settings.cpp
    modules/zenith_core/engine/TakeFolder.cpp
    modules/zenith_core/engine/TempoMap.cpp
    modules/zenith_core/engine/TempoMapSynchronizer.cpp
    modules/zenith_core/engine/Track.cpp
    modules/zenith_core/engine/TrackAutomationSynchronizer.cpp
    modules/zenith_core/engine/TrackFreeze.cpp
    modules/zenith_core/engine/TrackPluginManager.cpp
    modules/zenith_core/engine/TrackPluginState.cpp
    modules/zenith_core/engine/TrackProcessor.cpp
    modules/zenith_core/engine/TrackSendManager.cpp
    modules/zenith_core/engine/TrackSidechain.cpp
    modules/zenith_core/engine/TrackStateManager.cpp
    modules/zenith_core/engine/TrackStateSynchronizer.cpp
    modules/zenith_core/engine/TransportController.cpp
    modules/zenith_core/engine/WCETMonitor.cpp
    modules/zenith_core/engine/ZenithLogger.cpp
)

# Optional (experimental): Lua scripting processor.
if(ZENITH_ENABLE_LUA_SCRIPTING)
    list(APPEND ZENITH_CORE_SOURCES
        modules/zenith_core/engine/ScriptableProcessor.cpp
    )
endif()

# Create core library
add_library(zenith_core STATIC
    ${ZENITH_CORE_SOURCES}
)

# Target properties
target_include_directories(zenith_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules/zenith_core/engine>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules/zenith_core/utils>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules/zenith_core/analysis>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules/zenith_core/dsp>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules/zenith_core/effects>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules/zenith_core/audio>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules/zenith_ui/ui/framework>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules/zenith_ui/ui/design-system>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/external/vcpkg/installed/x64-linux/include/skia>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/external/vcpkg/installed/x64-linux/include/skia/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/apps/desktop/Source>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules>
        $<INSTALL_INTERFACE:include>
    PRIVATE
)

target_compile_features(zenith_core
    PUBLIC
        cxx_std_20
)

target_link_libraries(zenith_core
    PUBLIC
        zenith_dsp
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

# Standalone Plugin Scanner executable
add_executable(PluginScanner
    modules/zenith_core/engine/PluginScanner.cpp
)

target_include_directories(PluginScanner
    PRIVATE
        modules/zenith_core/engine
        apps/desktop/Source
        modules
        external/JUCE/modules
)

# Find GTK3 for Linux build
if(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    target_include_directories(PluginScanner PRIVATE ${GTK3_INCLUDE_DIRS})
    target_link_libraries(PluginScanner PRIVATE ${GTK3_LIBRARIES})
    target_compile_options(PluginScanner PRIVATE ${GTK3_CFLAGS_OTHER})
endif()

target_compile_definitions(PluginScanner
    PRIVATE
        JUCE_PLUGINHOST_VST3=1
        JUCE_PLUGINHOST_VST=0
        JUCE_PLUGINHOST_LV2=1
        JUCE_PLUGINHOST_LADSPA=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
)

# Global definitions to fix headless build issues
add_definitions(
    -DJUCE_PLUGINHOST_VST=0
    -DJUCE_PLUGINHOST_LADSPA=0
    -DJUCE_WEB_BROWSER=0
    -DJUCE_USE_CURL=0
)

target_link_libraries(PluginScanner
    PRIVATE
        juce::juce_audio_processors
        juce::juce_audio_formats
        juce::juce_gui_extra
        juce::juce_gui_basics
        juce::juce_events
        juce::juce_core
)

install(TARGETS PluginScanner
    RUNTIME DESTINATION bin
)

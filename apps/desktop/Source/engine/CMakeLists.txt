target_sources(ZenithDAW PRIVATE
    Engine.cpp
    # Modular Engine split files
    EngineTransport.cpp
    EngineTrackManagement.cpp
    EngineMixing.cpp
    EngineRecording.cpp
    EngineExport.cpp
    EngineMIDI.cpp
    EngineSync.cpp
    EnginePlugins.cpp
    ProjectState.cpp
    Track.cpp
    Clip.cpp
    TakeFolder.cpp
    AudioTrack.cpp
    MIDITrack.cpp
    AuxBusTrack.cpp
    ClipTrack.cpp
    InstrumentTrack.cpp
    TrackProcessor.cpp
    MixerChannel.cpp
    MixerController.cpp
    AudioFilePool.cpp
    PluginHost.cpp
    SafePluginScanner.cpp
    AuxBus.cpp
    ZenithLogger.cpp
    TrackPluginState.cpp
    AudioRecorder.cpp
    RoutingGraph.cpp
    TempoMap.cpp
    TrackFreeze.cpp
    Metronome.cpp
    MeteringSystem.cpp
    PluginChain.cpp
    PluginAutomationBinding.cpp
    AutomationManager.cpp
    Midi2DiscoveryService.cpp
    AudioAsMidiBridge.cpp
    PropertyExchangeManager.cpp
    RealTimeGarbageCollector.cpp

    # Modular Engine Components (2025-12-11 Refactor)
    AudioRenderer.cpp
    AudioExporter.cpp
    RecordingManager.cpp
    TransportController.cpp
    ProjectEngineBridge.cpp
    TrackStateManager.cpp
    ClipStateManager.cpp
    AutomationStateManager.cpp
    ProjectFileIO.cpp
    RecentProjectManager.cpp
    
    # Deprecated synchronizers (kept for backward compatibility, will be removed)
    # Deprecated synchronizers - Migration Status:
    # - ClipSynchronizer.cpp: Functions moved to ClipStateManager.cpp
    # - TrackStateSynchronizer.cpp: Functions moved to TrackStateManager.cpp
    # - TrackAutomationSynchronizer.cpp: Functions moved to AutomationStateManager.cpp
    # - TempoMapSynchronizer.cpp: Functions moved to TempoMap.cpp
    # These files are retained for backward compatibility and will be removed in v2.0
    ClipSynchronizer.cpp
    TrackStateSynchronizer.cpp
    TrackAutomationSynchronizer.cpp
    TempoMapSynchronizer.cpp
    
)# Touch for rebuild

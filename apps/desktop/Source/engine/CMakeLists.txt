target_sources(ZenithDAW PRIVATE
    Engine.cpp
    EngineTransport.cpp
    EngineTrackManagement.cpp
    EngineMixing.cpp
    EngineRecording.cpp
    EngineExport.cpp
    ProjectState.cpp
    Track.cpp
    Clip.cpp
    AudioTrack.cpp
    MIDITrack.cpp
    AuxBusTrack.cpp

    ClipTrack.cpp
    MixerChannel.cpp
    AudioFilePool.cpp
    PluginHost.cpp
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
    AutomationManager.cpp
    
    # Modular Engine Components (2025-12-11 Refactor)
    AudioRenderer.cpp
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



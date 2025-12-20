target_sources(ZenithDAW PRIVATE
    Engine.cpp
    ProjectState.cpp
    Track.cpp
    Clip.cpp
    AudioTrack.cpp
    MIDITrack.cpp
    AuxBusTrack.cpp
    InstrumentTrack.cpp
    ClipTrack.cpp
    MixerChannel.cpp
    MixerController.cpp
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
    # TODO: Remove these after ProjectEngineBridge is fully integrated
    ClipSynchronizer.cpp
    TrackStateSynchronizer.cpp
    TrackAutomationSynchronizer.cpp
    TempoMapSynchronizer.cpp
)# Touch for rebuild



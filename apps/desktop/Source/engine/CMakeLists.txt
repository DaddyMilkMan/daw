target_sources(ZenithDAW PRIVATE
    Engine.cpp
    ProjectState.cpp
    Track.cpp
    Clip.cpp
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
    # TODO: Remove these after ProjectEngineBridge is fully integrated
    ClipSynchronizer.cpp
    TrackStateSynchronizer.cpp
    TrackAutomationSynchronizer.cpp
    TempoMapSynchronizer.cpp
)# Touch for rebuild

target_sources(ZenithDAW PRIVATE
    MainWindow.cpp
    MainLayoutComponent.cpp
    SampleEditorComponent.cpp
    ArrangerComponent.cpp
    MixerComponent.cpp
    PianoRollComponent.cpp
    AutomationLaneComponent.cpp
    MarkerLaneComponent.cpp
    TempoLaneComponent.cpp
    PluginBrowserComponent.cpp
    ClipComponent.cpp
    MixerView.cpp
    MixerChannelComponent.cpp
    TrackHeaderComponent.cpp
    TimelineRuler.cpp
    PluginEditorWindow.cpp
    PresetBrowserComponent.cpp
    InstrumentBrowserPanel.cpp
    WingmanPanel.cpp
    AudioFeedback.cpp
    ZenithLookAndFeel.cpp
    ZenithTheme.cpp
)

add_subdirectory(skia)
target_sources(ZenithDAW PRIVATE
    ZenithPlugin.cpp
    ZenithPluginEditor.cpp
    InternalPluginFormat.cpp

    # Effects
    ../effects/ConsoleEmulation.cpp
    ../effects/ZenithChannelStrip.cpp
    ../effects/ZenithDeEsser.cpp
    ../effects/ZenithTransientShaper.cpp
    ../effects/ZenithVoiceChanger.cpp

    # Modulation
    modulation/ZenithTremolo.cpp
)

target_sources(ZenithDAW PRIVATE
    ZenithPlugin.cpp
    ZenithPluginEditor.cpp
    InternalPluginFormat.cpp

    # Effects
    ../effects/ZenithChannelStrip.cpp
    ../effects/ZenithDeEsser.cpp
    ../effects/ZenithTransientShaper.cpp
    ../effects/ZenithVoiceChanger.cpp
    ../effects/ConsoleEmulation.cpp

    # Modulation
    modulation/ZenithTremolo.cpp
)

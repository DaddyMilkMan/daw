# ============================================================================
# Vexel Audio Engine - Core DSP and Audio Processing
# ============================================================================

add_library(VexelAudioEngine STATIC
    # Core Engine
    AudioEngine.cpp
    AudioEngine.h

    # MIDI System
    MidiRouter.cpp
    MidiRouter.h
    MidiEvent.h

    # DSP Graph
    DSPGraph.cpp
    DSPGraph.h
    DSPNode.cpp
    DSPNode.h

    # Audio I/O
    AudioDevice.cpp
    AudioDevice.h
    AudioDriver.h

    # Track System
    Track.cpp
    Track.h
    AudioTrack.cpp
    AudioTrack.h
    MidiTrack.cpp
    MidiTrack.h

    # Clip Management
    Clip.cpp
    Clip.h
    AudioClip.cpp
    AudioClip.h
    MidiClip.cpp
    MidiClip.h

    # Plugin Hosting
    PluginHost.cpp
    PluginHost.h
    PluginScanner.cpp
    PluginScanner.h

    # Mixer
    MixerChannel.cpp
    MixerChannel.h
    MixerBus.cpp
    MixerBus.h

    # DSP Nodes (built-in processors)
    nodes/OscillatorNode.cpp
    nodes/OscillatorNode.h
    nodes/FilterNode.cpp
    nodes/FilterNode.h
    nodes/GainNode.cpp
    nodes/GainNode.h
    nodes/DelayNode.cpp
    nodes/DelayNode.h
    nodes/ReverbNode.cpp
    nodes/ReverbNode.h

    # Utilities
    AudioBuffer.h
    RingBuffer.h
    SampleRateConverter.cpp
    SampleRateConverter.h
)

target_include_directories(VexelAudioEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link JUCE audio modules
vexel_add_audio_engine_library(VexelAudioEngine)

# Link platform audio drivers
if(VEXEL_AUDIO_ALSA)
    target_link_libraries(VexelAudioEngine PRIVATE alsa_driver)
endif()

if(VEXEL_AUDIO_JACK)
    target_link_libraries(VexelAudioEngine PRIVATE jack_driver)
endif()

if(VEXEL_AUDIO_COREAUDIO)
    target_link_libraries(VexelAudioEngine PRIVATE coreaudio_driver)
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(VexelAudioEngine PRIVATE /W4 /WX)
else()
    target_compile_options(VexelAudioEngine PRIVATE
        -Wall -Wextra -Wpedantic -Werror
        -Wno-unused-parameter  # JUCE callbacks
    )
endif()

# Export headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/vexel/audio
    FILES_MATCHING PATTERN "*.h"
)

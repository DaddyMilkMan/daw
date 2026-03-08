# Zenith Professional Synthesizer - Instruments Module
# Shipping-quality synthesizer matching Xfer Serum and Vital

set(ZENITH_INSTRUMENTS_SOURCES
    # Core Definitions
    ZenithPolySynthDefs.h

    # Wavetable System
    WavetableData.h
    WavetableData.cpp
    WavetableLoader.h
    WavetableLoader.cpp

    # Oscillators
    ZenithOscillator.h
    ZenithOscillator.cpp

    # Filters
    ZenithFilter.h
    ZenithFilter.cpp

    # Voice
    ZenithPolySynthVoice.h
    ZenithPolySynthVoice.cpp

    # Modulation
    ZenithModulationMatrix.h
    ZenithModulationMatrix.cpp

    # Effects
    ZenithEffects.h
    ZenithEffects.cpp
)

# Add to parent project
target_sources(Zenith PRIVATE ${ZENITH_INSTRUMENTS_SOURCES})

# Include directories
target_include_directories(Zenith PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
)

# Link required JUCE modules
target_link_libraries(Zenith PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_formats
    juce::juce_dsp
    juce::juce_core
)

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace zenith {

class UltraSynthParameterManager {
public:
    UltraSynthParameterManager() {}
    UltraSynthParameterManager(juce::AudioProcessorValueTreeState&) {}
    void prepareToPlay(double, int) {}
};

} // namespace zenith

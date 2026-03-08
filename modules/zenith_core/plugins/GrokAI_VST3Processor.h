/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include "../ai/GrokAPIClient.h"
#include "../audio/RealTimeAudioBuffer.h"
#include <memory>

namespace zenith {
namespace plugin {

// Plugin parameters
enum class ParameterID {
    MasteringMode = 0,
    Intensity,
    TargetLoudness,
    Character,
    Creativity,
    RealTimeAnalysis,
    AutoLearn,
    Bypass,
    NumParameters
};

    class GrokAI_VST3Processor : public GrokAIProcessor {
    public:
        GrokAI_VST3Processor() : GrokAIProcessor() {}

        const juce::String getName() const override { return GrokAIPluginFactory::getPluginName(); }
        bool acceptsMidi() const override { return GrokAIPluginFactory::isPluginMidiEffect(); }
        bool producesMidi() const override { return false; }
        bool isMidiEffect() const override { return GrokAIPluginFactory::isPluginMidiEffect(); }
        double getTailLengthSeconds() const override { return 0.0; }
    };

    juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
    {
        return new GrokAI_VST3Processor();
    }
#endif

#if JUCE_PLUGINHOST_AU
    #include <juce_audio_processors/juce_audio_processors.h>

} // namespace

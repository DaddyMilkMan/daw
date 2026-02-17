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

class GrokAIPluginFactory {
public:
    static juce::AudioProcessor* createPlugin();
    static const juce::String getPluginName();
    static const juce::String getPluginDescription();
    static bool isPluginMidiEffect();
    static bool isPluginSynth();
    static double getPluginVersion();
    static const juce::String getPluginIdentifier();

private:
    static constexpr double PLUGIN_VERSION = 1.0;
    static constexpr juce::String PLUGIN_NAME = "Grok AI Mastering";
    static constexpr juce::String PLUGIN_DESCRIPTION = "AI-powered mastering with Grok 4.1";
    static constexpr juce::String PLUGIN_IDENTIFIER = "zenith.grokai";
};

// Plugin entry points (for different formats)
#if JUCE_PLUGINHOST_VST3
    #define JUCE_VST3_CAN_REPLACE_VST2 0
    #include <juce_audio_processors/juce_audio_processors.h>

} // namespace

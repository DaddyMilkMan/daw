/*
  ==============================================================================

    ZenithSampler.cpp
    Created: 2025-11-18
    Author:  Zenith DAW

    Implementation of ZenithSampler.

  ==============================================================================
*/

#include "ZenithSampler.h"

namespace zenith {

//==============================================================================
// ZenithSamplerProcessor
//==============================================================================

ZenithSamplerProcessor::ZenithSamplerProcessor()
    : AudioProcessor(BusesProperties()
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    // Add sampler voices
    for (int i = 0; i < 8; ++i)
        addVoice(new juce::SamplerVoice());

    // For now, no samples loaded (would need sample loading system)
    // This is a stub for demonstration

    // Create parameters
    addParameter(new juce::AudioParameterFloat("attack", "Attack", 0.0f, 1.0f, 0.01f));
    addParameter(new juce::AudioParameterFloat("decay", "Decay", 0.0f, 1.0f, 0.1f));
    addParameter(new juce::AudioParameterFloat("sustain", "Sustain", 0.0f, 1.0f, 1.0f));
    addParameter(new juce::AudioParameterFloat("release", "Release", 0.0f, 1.0f, 0.1f));
    addParameter(new juce::AudioParameterFloat("filterCutoff", "Filter Cutoff", 0.0f, 1.0f, 1.0f));
    addParameter(new juce::AudioParameterFloat("filterResonance", "Filter Resonance", 0.0f, 1.0f, 0.0f));
}

void ZenithSamplerProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    setCurrentPlaybackSampleRate(sampleRate);
}

void ZenithSamplerProcessor::releaseResources()
{
}

void ZenithSamplerProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();

    // Use base Synthesiser rendering
    juce::Synthesiser::renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
}

//==============================================================================
// ZenithSampler
//==============================================================================

ZenithSampler::ZenithSampler()
    : InstrumentBase(std::make_unique<ZenithSamplerProcessor>(), createMetadata())
{
    // Map parameter IDs to JUCE indices
    mapParameter("attack", ZenithSamplerProcessor::Attack);
    mapParameter("decay", ZenithSamplerProcessor::Decay);
    mapParameter("sustain", ZenithSamplerProcessor::Sustain);
    mapParameter("release", ZenithSamplerProcessor::Release);
    mapParameter("filter_cutoff", ZenithSamplerProcessor::FilterCutoff);
    mapParameter("filter_resonance", ZenithSamplerProcessor::FilterResonance);

    // Register presets
    registerPresets();
}

InstrumentMetadata ZenithSampler::createMetadata()
{
    InstrumentMetadata metadata;
    metadata.instrumentId = "zenith_sampler";
    metadata.name = "Zenith Sampler";
    metadata.category = "Sampler";
    metadata.description = "Sample-based instrument with envelope and filter";

    // Parameters
    {
        ParameterMetadata param;
        param.id = "attack";
        param.name = "Attack";
        param.category = "Envelope";
        param.type = ParameterMetadata::Type::Float;
        param.defaultValue = 0.01f;
        param.minValue = 0.0f;
        param.maxValue = 1.0f;
        param.units = "s";
        metadata.parameters.push_back(param);
    }
    {
        ParameterMetadata param;
        param.id = "decay";
        param.name = "Decay";
        param.category = "Envelope";
        param.type = ParameterMetadata::Type::Float;
        param.defaultValue = 0.1f;
        param.minValue = 0.0f;
        param.maxValue = 1.0f;
        param.units = "s";
        metadata.parameters.push_back(param);
    }
    {
        ParameterMetadata param;
        param.id = "sustain";
        param.name = "Sustain";
        param.category = "Envelope";
        param.type = ParameterMetadata::Type::Float;
        param.defaultValue = 1.0f;
        param.minValue = 0.0f;
        param.maxValue = 1.0f;
        param.units = "%";
        metadata.parameters.push_back(param);
    }
    {
        ParameterMetadata param;
        param.id = "release";
        param.name = "Release";
        param.category = "Envelope";
        param.type = ParameterMetadata::Type::Float;
        param.defaultValue = 0.1f;
        param.minValue = 0.0f;
        param.maxValue = 1.0f;
        param.units = "s";
        metadata.parameters.push_back(param);
    }
    {
        ParameterMetadata param;
        param.id = "filter_cutoff";
        param.name = "Filter Cutoff";
        param.category = "Filter";
        param.type = ParameterMetadata::Type::Float;
        param.defaultValue = 1.0f;
        param.minValue = 0.0f;
        param.maxValue = 1.0f;
        param.units = "%";
        metadata.parameters.push_back(param);
    }
    {
        ParameterMetadata param;
        param.id = "filter_resonance";
        param.name = "Filter Resonance";
        param.category = "Filter";
        param.type = ParameterMetadata::Type::Float;
        param.defaultValue = 0.0f;
        param.minValue = 0.0f;
        param.maxValue = 1.0f;
        param.units = "%";
        metadata.parameters.push_back(param);
    }

    // Macros
    {
        MacroMetadata macro;
        macro.id = "macro_brightness";
        macro.name = "Brightness";
        macro.description = "Controls filter cutoff";

        MacroTarget target;
        target.parameterId = "filter_cutoff";
        target.amount = 1.0f;
        macro.targets.push_back(target);

        metadata.macros.push_back(macro);
    }
    {
        MacroMetadata macro;
        macro.id = "macro_envelope_speed";
        macro.name = "Envelope Speed";
        macro.description = "Controls attack and release times";

        MacroTarget target1;
        target1.parameterId = "attack";
        target1.amount = 0.7f;
        macro.targets.push_back(target1);

        MacroTarget target2;
        target2.parameterId = "release";
        target2.amount = 0.7f;
        macro.targets.push_back(target2);

        metadata.macros.push_back(macro);
    }

    return metadata;
}

void ZenithSampler::registerPresets()
{
    // Preset 1: Fast Attack
    {
        std::map<juce::String, float> params;
        params["attack"] = 0.001f;
        params["decay"] = 0.05f;
        params["sustain"] = 1.0f;
        params["release"] = 0.05f;
        params["filter_cutoff"] = 1.0f;
        params["filter_resonance"] = 0.0f;
        InstrumentBase::registerPreset("fast_attack", "Fast Attack", params);
    }

    // Preset 2: Slow Fade
    {
        std::map<juce::String, float> params;
        params["attack"] = 0.3f;
        params["decay"] = 0.2f;
        params["sustain"] = 0.8f;
        params["release"] = 0.5f;
        params["filter_cutoff"] = 0.7f;
        params["filter_resonance"] = 0.2f;
        InstrumentBase::registerPreset("slow_fade", "Slow Fade", params);
    }

    // Preset 3: Warm Sample
    {
        std::map<juce::String, float> params;
        params["attack"] = 0.01f;
        params["decay"] = 0.1f;
        params["sustain"] = 0.9f;
        params["release"] = 0.2f;
        params["filter_cutoff"] = 0.6f;
        params["filter_resonance"] = 0.3f;
        InstrumentBase::registerPreset("warm_sample", "Warm Sample", params);
    }
}

} // namespace zenith

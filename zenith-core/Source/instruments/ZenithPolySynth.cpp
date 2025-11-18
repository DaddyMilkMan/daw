/*
  ==============================================================================

    ZenithPolySynth.cpp
    Created: 2025-11-18
    Author:  Zenith DAW

    Implementation of ZenithPolySynth.

  ==============================================================================
*/

#include "ZenithPolySynth.h"

namespace zenith {

//==============================================================================
// ZenithPolySynthProcessor
//==============================================================================

ZenithPolySynthProcessor::ZenithPolySynthProcessor()
    : AudioProcessor(BusesProperties()
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    // Add voices
    for (int i = 0; i < 8; ++i)
        addVoice(new SineVoice());

    addSound(new SineSound());

    // Create parameters
    addParameter(new juce::AudioParameterChoice("oscType", "Oscillator Type",
                                               juce::StringArray{"Sine", "Saw", "Square"}, 0));
    addParameter(new juce::AudioParameterFloat("filterCutoff", "Filter Cutoff", 0.0f, 1.0f, 0.8f));
    addParameter(new juce::AudioParameterFloat("filterResonance", "Filter Resonance", 0.0f, 1.0f, 0.5f));
    addParameter(new juce::AudioParameterFloat("attack", "Attack", 0.0f, 1.0f, 0.1f));
    addParameter(new juce::AudioParameterFloat("decay", "Decay", 0.0f, 1.0f, 0.2f));
    addParameter(new juce::AudioParameterFloat("sustain", "Sustain", 0.0f, 1.0f, 0.7f));
    addParameter(new juce::AudioParameterFloat("release", "Release", 0.0f, 1.0f, 0.3f));
}

void ZenithPolySynthProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    setCurrentPlaybackSampleRate(sampleRate);
}

void ZenithPolySynthProcessor::releaseResources()
{
}

void ZenithPolySynthProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();

    // Use base Synthesiser rendering
    juce::Synthesiser::renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
}

//==============================================================================
// SineVoice implementation
//==============================================================================

void ZenithPolySynthProcessor::SineVoice::startNote(int midiNoteNumber, float velocity,
                                                     juce::SynthesiserSound*, int)
{
    currentAngle = 0.0;
    level = velocity * 0.15;
    tailOff = 0.0;

    auto cyclesPerSecond = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    auto cyclesPerSample = cyclesPerSecond / getSampleRate();
    angleDelta = cyclesPerSample * juce::MathConstants<double>::twoPi;
}

void ZenithPolySynthProcessor::SineVoice::stopNote(float, bool allowTailOff)
{
    if (allowTailOff)
    {
        if (tailOff == 0.0)
            tailOff = 1.0;
    }
    else
    {
        clearCurrentNote();
        angleDelta = 0.0;
    }
}

void ZenithPolySynthProcessor::SineVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                                           int startSample, int numSamples)
{
    if (angleDelta != 0.0)
    {
        if (tailOff > 0.0)
        {
            while (--numSamples >= 0)
            {
                auto currentSample = (float)(std::sin(currentAngle) * level * tailOff);

                for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                    outputBuffer.addSample(i, startSample, currentSample);

                currentAngle += angleDelta;
                ++startSample;

                tailOff *= 0.99;

                if (tailOff <= 0.005)
                {
                    clearCurrentNote();
                    angleDelta = 0.0;
                    break;
                }
            }
        }
        else
        {
            while (--numSamples >= 0)
            {
                auto currentSample = (float)(std::sin(currentAngle) * level);

                for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                    outputBuffer.addSample(i, startSample, currentSample);

                currentAngle += angleDelta;
                ++startSample;
            }
        }
    }
}

//==============================================================================
// ZenithPolySynth
//==============================================================================

ZenithPolySynth::ZenithPolySynth()
    : InstrumentBase(std::make_unique<ZenithPolySynthProcessor>(), createMetadata())
{
    // Map parameter IDs to JUCE indices
    mapParameter("osc_type", ZenithPolySynthProcessor::OscType);
    mapParameter("filter_cutoff", ZenithPolySynthProcessor::FilterCutoff);
    mapParameter("filter_resonance", ZenithPolySynthProcessor::FilterResonance);
    mapParameter("attack", ZenithPolySynthProcessor::Attack);
    mapParameter("decay", ZenithPolySynthProcessor::Decay);
    mapParameter("sustain", ZenithPolySynthProcessor::Sustain);
    mapParameter("release", ZenithPolySynthProcessor::Release);

    // Register presets
    registerPresets();
}

InstrumentMetadata ZenithPolySynth::createMetadata()
{
    InstrumentMetadata metadata;
    metadata.instrumentId = "zenith_poly_synth";
    metadata.name = "Zenith Poly Synth";
    metadata.category = "Synth";
    metadata.description = "Polyphonic synthesizer with oscillator, filter, and envelope";

    // Parameters
    {
        ParameterMetadata param;
        param.id = "osc_type";
        param.name = "Oscillator Type";
        param.category = "Oscillator";
        param.type = ParameterMetadata::Type::Choice;
        param.defaultValue = 0.0f;
        param.minValue = 0.0f;
        param.maxValue = 2.0f;
        param.choices = juce::StringArray{"Sine", "Saw", "Square"};
        metadata.parameters.push_back(param);
    }
    {
        ParameterMetadata param;
        param.id = "filter_cutoff";
        param.name = "Filter Cutoff";
        param.category = "Filter";
        param.type = ParameterMetadata::Type::Float;
        param.defaultValue = 0.8f;
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
        param.defaultValue = 0.5f;
        param.minValue = 0.0f;
        param.maxValue = 1.0f;
        param.units = "%";
        metadata.parameters.push_back(param);
    }
    {
        ParameterMetadata param;
        param.id = "attack";
        param.name = "Attack";
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
        param.id = "decay";
        param.name = "Decay";
        param.category = "Envelope";
        param.type = ParameterMetadata::Type::Float;
        param.defaultValue = 0.2f;
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
        param.defaultValue = 0.7f;
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
        param.defaultValue = 0.3f;
        param.minValue = 0.0f;
        param.maxValue = 1.0f;
        param.units = "s";
        metadata.parameters.push_back(param);
    }

    // Macros
    {
        MacroMetadata macro;
        macro.id = "macro_brightness";
        macro.name = "Brightness";
        macro.description = "Controls filter cutoff and resonance";

        MacroTarget target1;
        target1.parameterId = "filter_cutoff";
        target1.amount = 0.8f;
        macro.targets.push_back(target1);

        MacroTarget target2;
        target2.parameterId = "filter_resonance";
        target2.amount = 0.5f;
        macro.targets.push_back(target2);

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

void ZenithPolySynth::registerPresets()
{
    // Preset 1: Init Pad
    {
        std::map<juce::String, float> params;
        params["osc_type"] = 0.0f;        // Sine
        params["filter_cutoff"] = 0.6f;
        params["filter_resonance"] = 0.3f;
        params["attack"] = 0.4f;
        params["decay"] = 0.3f;
        params["sustain"] = 0.8f;
        params["release"] = 0.6f;
        InstrumentBase::registerPreset("init_basic_pad", "Init Basic Pad", params);
    }

    // Preset 2: Bright Pluck
    {
        std::map<juce::String, float> params;
        params["osc_type"] = 1.0f / 2.0f; // Saw
        params["filter_cutoff"] = 0.9f;
        params["filter_resonance"] = 0.4f;
        params["attack"] = 0.01f;
        params["decay"] = 0.2f;
        params["sustain"] = 0.3f;
        params["release"] = 0.1f;
        InstrumentBase::registerPreset("bright_pluck", "Bright Pluck", params);
    }

    // Preset 3: LoFi Keys
    {
        std::map<juce::String, float> params;
        params["osc_type"] = 2.0f / 2.0f; // Square
        params["filter_cutoff"] = 0.5f;
        params["filter_resonance"] = 0.2f;
        params["attack"] = 0.05f;
        params["decay"] = 0.1f;
        params["sustain"] = 0.6f;
        params["release"] = 0.2f;
        InstrumentBase::registerPreset("lofi_keys_01", "LoFi Keys 01", params);
    }
}

} // namespace zenith

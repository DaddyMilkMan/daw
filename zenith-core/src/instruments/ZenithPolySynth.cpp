/**
 * @file ZenithPolySynth.cpp
 * @brief Implementation of ZenithPolySynth
 */

#include "../../include/instruments/ZenithPolySynth.h"
#include "../../include/instruments/ZenithPolySynthEditor.h"

namespace zenith {

//==============================================================================
ZenithPolySynth::ZenithPolySynth()
    : ZenithInstrumentProcessor(),
      metadata_(createMetadata()),
      parameters_(*this, nullptr, "Parameters", createParameterLayout())
{
    // Initialize macro engine with metadata
    macroEngine_.initialize(metadata_);

    // Add voices to synthesizer
    for (int i = 0; i < 8; ++i)
        synth_.addVoice(new PolySynthVoice());

    // Add sound
    synth_.addSound(new PolySynthSound());
}

//==============================================================================
InstrumentMetadata ZenithPolySynth::createMetadata()
{
    InstrumentMetadata metadata(
        "zenith_poly_synth",
        "Zenith PolySynth",
        "Simple polyphonic synthesizer with macro controls",
        "1.0.0"
    );

    // Oscillator parameters
    metadata.addParameter(InstrumentParameterInfo(
        "osc_wave", "Waveform", ParameterType::Enum, ParameterCategory::Oscillator,
        0.0f, 3.0f, 0.0f, ""
    ));

    metadata.addParameter(InstrumentParameterInfo(
        "osc_detune", "Detune", ParameterType::Float, ParameterCategory::Oscillator,
        0.0f, 1.0f, 0.0f, "cents"
    ));

    // Filter parameters
    metadata.addParameter(InstrumentParameterInfo(
        "filter_cutoff", "Filter Cutoff", ParameterType::Float, ParameterCategory::Filter,
        0.0f, 1.0f, 0.8f, ""
    ));

    metadata.addParameter(InstrumentParameterInfo(
        "filter_resonance", "Filter Resonance", ParameterType::Float, ParameterCategory::Filter,
        0.0f, 1.0f, 0.3f, ""
    ));

    // Envelope parameters
    metadata.addParameter(InstrumentParameterInfo(
        "amp_attack", "Attack", ParameterType::Float, ParameterCategory::Envelope,
        0.001f, 5.0f, 0.01f, "s"
    ));

    metadata.addParameter(InstrumentParameterInfo(
        "amp_decay", "Decay", ParameterType::Float, ParameterCategory::Envelope,
        0.001f, 5.0f, 0.1f, "s"
    ));

    metadata.addParameter(InstrumentParameterInfo(
        "amp_sustain", "Sustain", ParameterType::Float, ParameterCategory::Envelope,
        0.0f, 1.0f, 0.7f, ""
    ));

    metadata.addParameter(InstrumentParameterInfo(
        "amp_release", "Release", ParameterType::Float, ParameterCategory::Envelope,
        0.001f, 10.0f, 0.5f, "s"
    ));

    // Master parameters
    metadata.addParameter(InstrumentParameterInfo(
        "master_volume", "Volume", ParameterType::Float, ParameterCategory::Global,
        0.0f, 1.0f, 0.7f, "dB"
    ));

    // Define macros with targets
    // Macro 1: Warmth - affects filter cutoff and resonance
    InstrumentMacroInfo warmthMacro("macro_warmth", "Warmth",
        "Controls overall tone warmth by adjusting filter cutoff and resonance");
    warmthMacro.addTarget("filter_cutoff", 0.5f);  // Increase cutoff = brighter
    warmthMacro.addTarget("filter_resonance", 0.3f);  // Slight resonance boost
    metadata.addMacro(warmthMacro);

    // Macro 2: Space - affects envelope release
    InstrumentMacroInfo spaceMacro("macro_space", "Space",
        "Controls spaciousness by adjusting envelope release time");
    spaceMacro.addTarget("amp_release", 0.8f);  // Longer release = more space
    metadata.addMacro(spaceMacro);

    // Macro 3: Bite - affects filter resonance
    InstrumentMacroInfo biteMacro("macro_bite", "Bite",
        "Adds edge and character by increasing filter resonance");
    biteMacro.addTarget("filter_resonance", 0.7f);  // Strong resonance boost
    metadata.addMacro(biteMacro);

    // Macro 4: Movement - affects detune and attack
    InstrumentMacroInfo movementMacro("macro_movement", "Movement",
        "Adds movement and animation via detune and attack time");
    movementMacro.addTarget("osc_detune", 0.6f);
    movementMacro.addTarget("amp_attack", 0.3f);
    metadata.addMacro(movementMacro);

    return metadata;
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout ZenithPolySynth::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Oscillator parameters
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "osc_wave", "Waveform",
        juce::StringArray{"Sine", "Saw", "Square", "Triangle"},
        0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "osc_detune", "Detune",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    // Filter parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "filter_cutoff", "Filter Cutoff",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.8f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "filter_resonance", "Filter Resonance",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

    // Envelope parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "amp_attack", "Attack",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.01f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "amp_decay", "Decay",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.1f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "amp_sustain", "Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "amp_release", "Release",
        juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.3f), 0.5f));

    // Master volume
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "master_volume", "Volume",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f));

    // Macro parameters
    for (size_t i = 0; i < 4; ++i)
    {
        auto macroId = "macro_" + std::to_string(i);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            macroId, "Macro " + juce::String(i + 1),
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    }

    return { params.begin(), params.end() };
}

//==============================================================================
void ZenithPolySynth::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    synth_.setCurrentPlaybackSampleRate(sampleRate);

    // Prepare voices
    for (int i = 0; i < synth_.getNumVoices(); ++i)
    {
        if (auto voice = dynamic_cast<PolySynthVoice*>(synth_.getVoice(i)))
        {
            voice->prepareToPlay(sampleRate);
        }
    }
}

void ZenithPolySynth::releaseResources()
{
}

void ZenithPolySynth::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Update macro values from parameters
    for (size_t i = 0; i < 4; ++i)
    {
        auto macroId = "macro_" + std::to_string(i);
        float macroValue = *parameters_.getRawParameterValue(macroId);
        macroEngine_.setMacroValue(i, macroValue);
    }

    // Update synth parameters with macro contributions
    updateParametersFromMacros();

    // Render synth
    buffer.clear();
    synth_.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // Apply master volume
    float masterVolume = *parameters_.getRawParameterValue("master_volume");
    buffer.applyGain(masterVolume);
}

//==============================================================================
void ZenithPolySynth::updateParametersFromMacros()
{
    // Get base parameter values
    int waveform = *parameters_.getRawParameterValue("osc_wave");
    float attack = *parameters_.getRawParameterValue("amp_attack");
    float decay = *parameters_.getRawParameterValue("amp_decay");
    float sustain = *parameters_.getRawParameterValue("amp_sustain");
    float release = *parameters_.getRawParameterValue("amp_release");

    // Apply macro contributions
    float attackContrib = macroEngine_.computeMacroContribution("amp_attack", attack, 5.0f - 0.001f);
    float releaseContrib = macroEngine_.computeMacroContribution("amp_release", release, 10.0f - 0.001f);

    attack = juce::jlimit(0.001f, 5.0f, attack + attackContrib);
    release = juce::jlimit(0.001f, 10.0f, release + releaseContrib);

    // Update voices
    for (int i = 0; i < synth_.getNumVoices(); ++i)
    {
        if (auto voice = dynamic_cast<PolySynthVoice*>(synth_.getVoice(i)))
        {
            voice->setWaveform(waveform);
            voice->setEnvelopeParameters(attack, decay, sustain, release);
        }
    }
}

//==============================================================================
bool ZenithPolySynth::loadPreset(const ZenithInstrumentPreset& preset)
{
    if (preset.instrumentId != metadata_.instrumentId)
        return false;

    // Load parameters
    for (const auto& [paramId, value] : preset.parameters)
    {
        if (auto param = parameters_.getParameter(paramId))
        {
            param->setValueNotifyingHost(value);
        }
    }

    // Load macros
    for (const auto& [macroId, value] : preset.macros)
    {
        macroEngine_.setMacroValue(macroId, value);

        // Also update the parameter if it exists
        if (auto param = parameters_.getParameter(macroId))
        {
            param->setValueNotifyingHost(value);
        }
    }

    return true;
}

ZenithInstrumentPreset ZenithPolySynth::getCurrentPreset() const
{
    ZenithInstrumentPreset preset("Current", metadata_.instrumentId);

    // Save all parameters
    for (const auto& paramInfo : metadata_.parameters)
    {
        if (auto param = parameters_.getParameter(paramInfo.id))
        {
            preset.setParameter(paramInfo.id, param->getValue());
        }
    }

    // Save macro values
    for (const auto& macroInfo : metadata_.macros)
    {
        float macroValue = macroEngine_.getMacroValue(macroInfo.id);
        preset.setMacro(macroInfo.id, macroValue);
    }

    return preset;
}

//==============================================================================
juce::AudioProcessorEditor* ZenithPolySynth::createEditor()
{
    return new ZenithPolySynthEditor(*this);
}

} // namespace zenith

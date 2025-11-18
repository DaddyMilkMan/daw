/**
 * @file ZenithSampler.cpp
 * @brief Implementation of ZenithSampler
 */

#include "../../include/instruments/ZenithSampler.h"
#include "../../include/instruments/ZenithSamplerEditor.h"

namespace zenith {

//==============================================================================
ZenithSampler::ZenithSampler()
    : ZenithInstrumentProcessor(),
      metadata_(createMetadata()),
      parameters_(*this, nullptr, "Parameters", createParameterLayout())
{
    // Initialize macro engine with metadata
    macroEngine_.initialize(metadata_);

    // Register audio formats
    formatManager_.registerBasicFormats();

    // Add sampler voices
    for (int i = 0; i < 8; ++i)
        sampler_.addVoice(new ZenithSamplerVoice());
}

//==============================================================================
InstrumentMetadata ZenithSampler::createMetadata()
{
    InstrumentMetadata metadata(
        "zenith_sampler",
        "Zenith Sampler",
        "Simple sample playback instrument with macro controls",
        "1.0.0"
    );

    // Sample parameters
    metadata.addParameter(InstrumentParameterInfo(
        "sample_start", "Sample Start", ParameterType::Float, ParameterCategory::Sample,
        0.0f, 1.0f, 0.0f, ""
    ));

    metadata.addParameter(InstrumentParameterInfo(
        "sample_end", "Sample End", ParameterType::Float, ParameterCategory::Sample,
        0.0f, 1.0f, 1.0f, ""
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
        "env_attack", "Attack", ParameterType::Float, ParameterCategory::Envelope,
        0.001f, 2.0f, 0.001f, "s"
    ));

    metadata.addParameter(InstrumentParameterInfo(
        "env_decay", "Decay", ParameterType::Float, ParameterCategory::Envelope,
        0.001f, 2.0f, 0.1f, "s"
    ));

    metadata.addParameter(InstrumentParameterInfo(
        "env_sustain", "Sustain", ParameterType::Float, ParameterCategory::Envelope,
        0.0f, 1.0f, 1.0f, ""
    ));

    metadata.addParameter(InstrumentParameterInfo(
        "env_release", "Release", ParameterType::Float, ParameterCategory::Envelope,
        0.001f, 5.0f, 0.1f, "s"
    ));

    // Master parameters
    metadata.addParameter(InstrumentParameterInfo(
        "master_volume", "Volume", ParameterType::Float, ParameterCategory::Global,
        0.0f, 1.0f, 0.7f, "dB"
    ));

    // Define macros with targets
    // Macro 1: Body - affects filter cutoff and resonance
    InstrumentMacroInfo bodyMacro("macro_body", "Body",
        "Controls tone body and fullness via filter");
    bodyMacro.addTarget("filter_cutoff", 0.4f);
    bodyMacro.addTarget("filter_resonance", 0.2f);
    metadata.addMacro(bodyMacro);

    // Macro 2: Snap - affects attack and transient emphasis
    InstrumentMacroInfo snapMacro("macro_snap", "Snap",
        "Controls attack snap and transient response");
    snapMacro.addTarget("env_attack", -0.5f);  // Negative = faster attack
    snapMacro.addTarget("filter_cutoff", 0.3f);  // Brighter for more snap
    metadata.addMacro(snapMacro);

    // Macro 3: LoFi - affects bit reduction simulation
    InstrumentMacroInfo lofiMacro("macro_lofi", "LoFi",
        "Simulates lo-fi character via filter and sample reduction");
    lofiMacro.addTarget("filter_cutoff", -0.5f);  // Darker tone
    metadata.addMacro(lofiMacro);

    // Macro 4: Tone - affects overall brightness
    InstrumentMacroInfo toneMacro("macro_tone", "Tone",
        "Controls overall tonal balance from dark to bright");
    toneMacro.addTarget("filter_cutoff", 0.8f);
    metadata.addMacro(toneMacro);

    return metadata;
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout ZenithSampler::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Sample parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "sample_start", "Sample Start",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "sample_end", "Sample End",
        juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));

    // Filter parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "filter_cutoff", "Filter Cutoff",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.8f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "filter_resonance", "Filter Resonance",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

    // Envelope parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "env_attack", "Attack",
        juce::NormalisableRange<float>(0.001f, 2.0f, 0.001f, 0.3f), 0.001f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "env_decay", "Decay",
        juce::NormalisableRange<float>(0.001f, 2.0f, 0.001f, 0.3f), 0.1f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "env_sustain", "Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "env_release", "Release",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.1f));

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
void ZenithSampler::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    sampler_.setCurrentPlaybackSampleRate(sampleRate);

    // Prepare voices
    for (int i = 0; i < sampler_.getNumVoices(); ++i)
    {
        if (auto voice = dynamic_cast<ZenithSamplerVoice*>(sampler_.getVoice(i)))
        {
            voice->prepareToPlay(sampleRate);
        }
    }
}

void ZenithSampler::releaseResources()
{
}

void ZenithSampler::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Update macro values from parameters
    for (size_t i = 0; i < 4; ++i)
    {
        auto macroId = "macro_" + std::to_string(i);
        float macroValue = *parameters_.getRawParameterValue(macroId);
        macroEngine_.setMacroValue(i, macroValue);
    }

    // Update sampler parameters with macro contributions
    updateParametersFromMacros();

    // Render sampler
    buffer.clear();
    sampler_.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // Apply master volume
    float masterVolume = *parameters_.getRawParameterValue("master_volume");
    buffer.applyGain(masterVolume);
}

//==============================================================================
void ZenithSampler::updateParametersFromMacros()
{
    // Get base parameter values
    float attack = *parameters_.getRawParameterValue("env_attack");
    float decay = *parameters_.getRawParameterValue("env_decay");
    float sustain = *parameters_.getRawParameterValue("env_sustain");
    float release = *parameters_.getRawParameterValue("env_release");

    // Apply macro contributions
    float attackContrib = macroEngine_.computeMacroContribution("env_attack", attack, 2.0f - 0.001f);
    float releaseContrib = macroEngine_.computeMacroContribution("env_release", release, 5.0f - 0.001f);

    attack = juce::jlimit(0.001f, 2.0f, attack + attackContrib);
    release = juce::jlimit(0.001f, 5.0f, release + releaseContrib);

    // Update voices
    for (int i = 0; i < sampler_.getNumVoices(); ++i)
    {
        if (auto voice = dynamic_cast<ZenithSamplerVoice*>(sampler_.getVoice(i)))
        {
            voice->setEnvelopeParameters(attack, decay, sustain, release);
        }
    }
}

//==============================================================================
void ZenithSampler::loadSample(const juce::File& file)
{
    sampler_.clearSounds();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager_.createReaderFor(file));

    if (reader != nullptr)
    {
        juce::BigInteger allNotes;
        allNotes.setRange(0, 128, true);

        sampler_.addSound(new juce::SamplerSound(
            file.getFileNameWithoutExtension(),
            *reader,
            allNotes,
            60,   // MIDI root note
            0.0,  // attack time
            0.1,  // release time
            10.0  // max sample length
        ));
    }
}

//==============================================================================
bool ZenithSampler::loadPreset(const ZenithInstrumentPreset& preset)
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

ZenithInstrumentPreset ZenithSampler::getCurrentPreset() const
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
juce::AudioProcessorEditor* ZenithSampler::createEditor()
{
    return new ZenithSamplerEditor(*this);
}

} // namespace zenith

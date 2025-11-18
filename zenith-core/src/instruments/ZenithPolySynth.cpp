#include "instruments/ZenithPolySynth.h"
#include "instruments/ZenithPolySynthEditor.h"

namespace zenith {
namespace instruments {

ZenithPolySynth::ZenithPolySynth()
    : AudioProcessor(BusesProperties()
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters_(*this, nullptr, "ZenithPolySynth", createParameterLayout())
{
    // Add voices to the synthesiser
    for (int i = 0; i < kNumVoices; ++i)
        synth_.addVoice(new ZenithPolySynthVoice());

    // Add a sound (required by JUCE Synthesiser, even though we don't use it for much)
    synth_.addSound(new ZenithPolySynthSound());
}

ZenithPolySynth::~ZenithPolySynth()
{
}

//==============================================================================
// Parameter layout

juce::AudioProcessorValueTreeState::ParameterLayout ZenithPolySynth::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Oscillator 1 waveform
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        kParamOsc1Wave,
        "Osc 1 Wave",
        juce::StringArray{"Sine", "Saw", "Square", "Triangle"},
        1)); // Default: Saw

    // Oscillator 2 waveform
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        kParamOsc2Wave,
        "Osc 2 Wave",
        juce::StringArray{"Sine", "Saw", "Square", "Triangle"},
        1)); // Default: Saw

    // Osc 2 detune (cents)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        kParamOsc2Detune,
        "Osc 2 Detune",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
        -7.0f, // Default: slightly detuned
        "cents"));

    // Oscillator mix (0 = all osc1, 1 = all osc2)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        kParamOscMix,
        "Osc Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f));

    // Noise level
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        kParamNoiseLevel,
        "Noise Level",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));

    // Filter type
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        kParamFilterType,
        "Filter Type",
        juce::StringArray{"Low Pass", "Band Pass", "High Pass"},
        0)); // Default: Low Pass

    // Filter cutoff (20 Hz to 20 kHz, logarithmic)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        kParamFilterCutoff,
        "Filter Cutoff",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), // skew for log scale
        12000.0f, // Default: wide open
        "Hz"));

    // Filter resonance
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        kParamFilterResonance,
        "Filter Resonance",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.1f));

    // Envelope - Attack (0.001 to 5 seconds)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        kParamEnvAttack,
        "Env Attack",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.5f), // skew for log scale
        0.01f,
        "s"));

    // Envelope - Decay (0.001 to 5 seconds)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        kParamEnvDecay,
        "Env Decay",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.5f),
        0.1f,
        "s"));

    // Envelope - Sustain (0 to 1)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        kParamEnvSustain,
        "Env Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.7f));

    // Envelope - Release (0.001 to 10 seconds)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        kParamEnvRelease,
        "Env Release",
        juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.5f),
        0.3f,
        "s"));

    // Master gain (-60 dB to +6 dB)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        kParamMasterGain,
        "Master Gain",
        juce::NormalisableRange<float>(-60.0f, 6.0f, 0.1f),
        -6.0f, // Default: -6 dB to prevent clipping
        "dB"));

    return {params.begin(), params.end()};
}

//==============================================================================
// Audio processing

void ZenithPolySynth::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    synth_.setCurrentPlaybackSampleRate(sampleRate);

    // Prepare each voice
    for (int i = 0; i < synth_.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<ZenithPolySynthVoice*>(synth_.getVoice(i)))
        {
            voice->prepareToPlay(sampleRate, samplesPerBlock);
        }
    }
}

void ZenithPolySynth::releaseResources()
{
    // Nothing to release in our case
}

bool ZenithPolySynth::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // We support stereo output only
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void ZenithPolySynth::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear output buffer
    buffer.clear();

    // Update voice parameters from parameter state
    updateVoiceParameters();

    // Let the synthesiser render the audio
    synth_.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // Apply master gain
    float masterGainDb = parameters_.getRawParameterValue(kParamMasterGain)->load();
    float masterGainLinear = juce::Decibels::decibelsToGain(masterGainDb);

    buffer.applyGain(masterGainLinear);
}

void ZenithPolySynth::updateVoiceParameters()
{
    // Read parameters (these are atomic, so safe to read from audio thread)
    auto osc1Wave = static_cast<ZenithPolySynthVoice::OscWaveType>(
        static_cast<int>(parameters_.getRawParameterValue(kParamOsc1Wave)->load()));
    auto osc2Wave = static_cast<ZenithPolySynthVoice::OscWaveType>(
        static_cast<int>(parameters_.getRawParameterValue(kParamOsc2Wave)->load()));
    float osc2Detune = parameters_.getRawParameterValue(kParamOsc2Detune)->load();
    float oscMix = parameters_.getRawParameterValue(kParamOscMix)->load();
    float noiseLevel = parameters_.getRawParameterValue(kParamNoiseLevel)->load();

    auto filterType = static_cast<ZenithPolySynthVoice::FilterType>(
        static_cast<int>(parameters_.getRawParameterValue(kParamFilterType)->load()));
    float filterCutoff = parameters_.getRawParameterValue(kParamFilterCutoff)->load();
    float filterResonance = parameters_.getRawParameterValue(kParamFilterResonance)->load();

    float envAttack = parameters_.getRawParameterValue(kParamEnvAttack)->load();
    float envDecay = parameters_.getRawParameterValue(kParamEnvDecay)->load();
    float envSustain = parameters_.getRawParameterValue(kParamEnvSustain)->load();
    float envRelease = parameters_.getRawParameterValue(kParamEnvRelease)->load();

    float masterGainDb = parameters_.getRawParameterValue(kParamMasterGain)->load();
    float masterGainLinear = juce::Decibels::decibelsToGain(masterGainDb);

    // Update all voices
    for (int i = 0; i < synth_.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<ZenithPolySynthVoice*>(synth_.getVoice(i)))
        {
            voice->setOsc1Wave(osc1Wave);
            voice->setOsc2Wave(osc2Wave);
            voice->setOsc2Detune(osc2Detune);
            voice->setOscMix(oscMix);
            voice->setNoiseLevel(noiseLevel);

            voice->setFilterType(filterType);
            voice->setFilterCutoff(filterCutoff);
            voice->setFilterResonance(filterResonance);

            voice->setAmpEnvAttack(envAttack);
            voice->setAmpEnvDecay(envDecay);
            voice->setAmpEnvSustain(envSustain);
            voice->setAmpEnvRelease(envRelease);

            voice->setGain(masterGainLinear);
        }
    }
}

//==============================================================================
// Editor

juce::AudioProcessorEditor* ZenithPolySynth::createEditor()
{
    return new ZenithPolySynthEditor(*this);
}

//==============================================================================
// State save/load

void ZenithPolySynth::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters_.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ZenithPolySynth::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr)
    {
        if (xmlState->hasTagName(parameters_.state.getType()))
        {
            parameters_.replaceState(juce::ValueTree::fromXml(*xmlState));
        }
    }
}

//==============================================================================
// Factory function

std::unique_ptr<juce::AudioProcessor> createZenithPolySynth()
{
    return std::make_unique<ZenithPolySynth>();
}

} // namespace instruments
} // namespace zenith

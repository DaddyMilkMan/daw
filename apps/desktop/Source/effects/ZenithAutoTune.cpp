/*
  ==============================================================================

    ZenithAutoTune.cpp
    Created: 2026-01-29
    Author:  Zenith DAW

    Built-in professional pitch correction effect - INCLUDED FREE.

  ==============================================================================
*/

#include "ZenithAutoTune.h"

namespace zenith {
namespace effects {

//==============================================================================
// Preset definitions
static const char* kPresetNatural = "Natural";
static const char* kPresetTransparent = "Transparent";
static const char* kPresetTight = "Tight";
static const char* kPresetRobot = "Robot (T-Pain)";
static const char* kPresetSubtle = "Subtle";

//==============================================================================
ZenithAutoTune::ZenithAutoTune()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    parameters = std::make_unique<juce::AudioProcessorValueTreeState>(*this, nullptr, 
                                                                      juce::Identifier("ZenithAutoTune"),
                                                                      createParameterLayout());
    
    // Initialize with default scale
    pitchCorrector_.setScale(dsp::Note::C, dsp::MusicalScale::Chromatic);
    
    // Load default preset
    loadPreset(kPresetNatural);
}

ZenithAutoTune::~ZenithAutoTune()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout ZenithAutoTune::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    // Enable/disable
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "enabled", "Enabled", true));
    
    // Retune Speed (0-800ms)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "retuneSpeed", "Retune Speed",
        juce::NormalisableRange<float>(0.0f, 800.0f, 0.1f, 0.3f),
        50.0f,  // Default: natural
        juce::AudioParameterFloatAttributes()
            .withLabel("ms")));
    
    // Humanize (0-1)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "humanize", "Humanize",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f,
        juce::AudioParameterFloatAttributes()
            .withLabel("%")));
    
    // Correction Amount (0-1)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "correctionAmount", "Correction Amount",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        1.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel("%")));
    
    // Formant Preservation (0-1)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "formantPreserve", "Formant Preservation",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.8f,
        juce::AudioParameterFloatAttributes()
            .withLabel("%")));
    
    // Key (0-11, C to B)
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        "key", "Key",
        0, 11, 0,
        juce::AudioParameterIntAttributes()
            .withStringFromValueFunction([](int value, int) {
                const char* notes[] = {"C", "C#", "D", "D#", "E", "F", 
                                       "F#", "G", "G#", "A", "A#", "B"};
                return notes[value];
            })));
    
    // Scale (0-11, various scales)
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        "scale", "Scale",
        0, 11, 0,  // Default: Chromatic
        juce::AudioParameterIntAttributes()
            .withStringFromValueFunction([](int value, int) {
                const char* scales[] = {"Chromatic", "Major", "Minor", "Minor Harmonic", 
                                       "Minor Melodic", "Pentatonic Major", "Pentatonic Minor",
                                       "Blues", "Dorian", "Phrygian", "Lydian", "Mixolydian"};
                return scales[value];
            })));
    
    return { params.begin(), params.end() };
}

//==============================================================================
void ZenithAutoTune::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    pitchDetector_.prepare(sampleRate, 2048);
    pitchCorrector_.prepare(sampleRate, samplesPerBlock);
    
    monoBuffer_.setSize(1, samplesPerBlock);
    correctedBuffer_.setSize(1, samplesPerBlock);
}

void ZenithAutoTune::releaseResources()
{
    pitchDetector_.reset();
    pitchCorrector_.reset();
}

void ZenithAutoTune::processBlock(juce::AudioBuffer<float>& buffer, 
                                  juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    if (numChannels == 0 || numSamples == 0)
        return;
    
    // Get parameters
    bool enabled = *parameters->getRawParameterValue("enabled") > 0.5f;
    if (!enabled)
    {
        isCorrecting_.store(false);
        return;
    }
    
    // Update corrector parameters
    pitchCorrector_.setRetuneSpeed(*parameters->getRawParameterValue("retuneSpeed"));
    pitchCorrector_.setHumanize(*parameters->getRawParameterValue("humanize"));
    pitchCorrector_.setCorrectionAmount(*parameters->getRawParameterValue("correctionAmount"));
    pitchCorrector_.setFormantPreservation(*parameters->getRawParameterValue("formantPreserve"));
    
    // Update scale
    int key = static_cast<int>(*parameters->getRawParameterValue("key"));
    int scale = static_cast<int>(*parameters->getRawParameterValue("scale"));
    pitchCorrector_.setScale(static_cast<dsp::Note>(key), static_cast<dsp::MusicalScale>(scale));
    
    // Ensure mono buffer is large enough
    if (monoBuffer_.getNumSamples() < numSamples)
    {
        monoBuffer_.setSize(1, numSamples, false, false, true);
        correctedBuffer_.setSize(1, numSamples, false, false, true);
    }
    
    // Convert to mono for pitch detection (average channels)
    monoBuffer_.clear();
    for (int ch = 0; ch < numChannels; ++ch)
    {
        monoBuffer_.addFrom(0, 0, buffer, ch, 0, numSamples, 1.0f / numChannels);
    }
    
    // Detect pitch
    float detectedPitch = pitchDetector_.processBlock(monoBuffer_);
    detectedPitch_.store(detectedPitch);
    isVoiced_.store(pitchDetector_.isVoiced());
    
    // Apply pitch correction
    pitchCorrector_.process(monoBuffer_, detectedPitch, correctedBuffer_);
    
    // Update UI state
    targetPitch_.store(pitchCorrector_.getTargetPitch());
    isCorrecting_.store(pitchCorrector_.isPitchCorrecting());
    
    // Mix corrected signal back to stereo
    // For simplicity, we apply the same correction to all channels
    // In a full implementation, you'd calculate the difference and apply per-channel
    
    // Calculate gain compensation
    float inputLevel = monoBuffer_.getRMSLevel(0, 0, numSamples);
    float outputLevel = correctedBuffer_.getRMSLevel(0, 0, numSamples);
    float gainCompensation = (outputLevel > 0.001f) ? inputLevel / outputLevel : 1.0f;
    gainCompensation = juce::jlimit(0.5f, 2.0f, gainCompensation);
    
    // Apply to output
    for (int ch = 0; ch < numChannels; ++ch)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            // Blend original and corrected based on correction amount
            float correctionAmt = *parameters->getRawParameterValue("correctionAmount");
            float originalSample = buffer.getSample(ch, i);
            float correctedSample = correctedBuffer_.getSample(0, i) * gainCompensation;
            
            float outputSample = originalSample * (1.0f - correctionAmt) + 
                                correctedSample * correctionAmt;
            
            buffer.setSample(ch, i, outputSample);
        }
    }
}

//==============================================================================
// Direct parameter control
void ZenithAutoTune::setRetuneSpeed(float ms)
{
    *parameters->getParameter("retuneSpeed") = ms;
}

float ZenithAutoTune::getRetuneSpeed() const
{
    return parameters->getRawParameterValue("retuneSpeed")->load();
}

void ZenithAutoTune::setHumanize(float amount)
{
    *parameters->getParameter("humanize") = juce::jlimit(0.0f, 1.0f, amount);
}

float ZenithAutoTune::getHumanize() const
{
    return parameters->getRawParameterValue("humanize")->load();
}

void ZenithAutoTune::setCorrectionAmount(float amount)
{
    *parameters->getParameter("correctionAmount") = juce::jlimit(0.0f, 1.0f, amount);
}

float ZenithAutoTune::getCorrectionAmount() const
{
    return parameters->getRawParameterValue("correctionAmount")->load();
}

void ZenithAutoTune::setFormantPreservation(float amount)
{
    *parameters->getParameter("formantPreserve") = juce::jlimit(0.0f, 1.0f, amount);
}

float ZenithAutoTune::getFormantPreservation() const
{
    return parameters->getRawParameterValue("formantPreserve")->load();
}

void ZenithAutoTune::setKey(dsp::Note rootNote)
{
    *parameters->getParameter("key") = static_cast<float>(rootNote);
}

dsp::Note ZenithAutoTune::getKey() const
{
    return static_cast<dsp::Note>(static_cast<int>(parameters->getRawParameterValue("key")->load()));
}

void ZenithAutoTune::setScale(dsp::MusicalScale scale)
{
    *parameters->getParameter("scale") = static_cast<float>(scale);
}

dsp::MusicalScale ZenithAutoTune::getScale() const
{
    return static_cast<dsp::MusicalScale>(static_cast<int>(parameters->getRawParameterValue("scale")->load()));
}

//==============================================================================
// State management
void ZenithAutoTune::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters->copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ZenithAutoTune::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName(parameters->state.getType()))
        {
            parameters->replaceState(juce::ValueTree::fromXml(*xmlState));
        }
    }
}

//==============================================================================
// Presets
juce::StringArray ZenithAutoTune::getPresetNames()
{
    return { kPresetNatural, kPresetTransparent, kPresetTight, kPresetRobot, kPresetSubtle };
}

void ZenithAutoTune::loadPreset(const juce::String& presetName)
{
    if (presetName == kPresetNatural)
    {
        setRetuneSpeed(50.0f);     // Medium speed
        setHumanize(0.6f);          // Fairly natural
        setCorrectionAmount(0.8f);  // Strong correction
        setFormantPreservation(0.8f);
    }
    else if (presetName == kPresetTransparent)
    {
        setRetuneSpeed(150.0f);     // Slow
        setHumanize(0.9f);          // Very natural
        setCorrectionAmount(0.4f);  // Subtle correction
        setFormantPreservation(0.9f);
    }
    else if (presetName == kPresetTight)
    {
        setRetuneSpeed(20.0f);      // Fast
        setHumanize(0.3f);          // Less natural
        setCorrectionAmount(1.0f);  // Full correction
        setFormantPreservation(0.7f);
    }
    else if (presetName == kPresetRobot)
    {
        setRetuneSpeed(0.0f);       // Instant (T-Pain effect)
        setHumanize(0.0f);          // Robotic
        setCorrectionAmount(1.0f);  // Full
        setFormantPreservation(0.5f);
    }
    else if (presetName == kPresetSubtle)
    {
        setRetuneSpeed(100.0f);     // Very slow
        setHumanize(0.8f);          // Natural
        setCorrectionAmount(0.3f);  // Light correction
        setFormantPreservation(0.9f);
    }
}

} // namespace effects
} // namespace zenith

//==============================================================================
// Plugin wrapper
#include <juce_audio_processors/juce_audio_processors.h>

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new zenith::effects::ZenithAutoTune();
}

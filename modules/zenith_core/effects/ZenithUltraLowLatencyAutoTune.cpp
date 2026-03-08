/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#include "ZenithUltraLowLatencyAutoTune.h"

namespace zenith {
namespace effects {

//==============================================================================
// Preset definitions
static const char* kPresetNatural = "Natural";
static const char* kPresetTransparent = "Transparent";
static const char* kPresetTight = "Tight";
static const char* kPresetRobot = "Robot (T-Pain)";
static const char* kPresetSubtle = "Subtle";

// Latency mode strings
static const char* kLatencyTurbo = "Turbo";
static const char* kLatencyExtreme = "Extreme";
static const char* kLatencyUltraLow = "UltraLow";
static const char* kLatencyLow = "Low";
static const char* kLatencyStandard = "Standard";
static const char* kLatencyHighQuality = "HighQuality";

//==============================================================================
ZenithUltraLowLatencyAutoTune::ZenithUltraLowLatencyAutoTune()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    parameters = std::make_unique<juce::AudioProcessorValueTreeState>(*this, nullptr,
                                                                      juce::Identifier("ZenithUltraLowLatencyAutoTune"),
                                                                      createParameterLayout());

    // Initialize with default scale
    pitchCorrector_.setScale(dsp::Note::C, dsp::MusicalScale::Chromatic);

    // Load default preset
    loadPreset(kPresetNatural);
}

ZenithUltraLowLatencyAutoTune::~ZenithUltraLowLatencyAutoTune()
{
    if (auto* ptr = algorithmUsed_.exchange(nullptr))
        delete ptr;
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout ZenithUltraLowLatencyAutoTune::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Enable/disable
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "enabled", "Enabled", true));

    // Latency Mode
    juce::StringArray latencyModes;
    latencyModes.add(kLatencyTurbo);
    latencyModes.add(kLatencyExtreme);
    latencyModes.add(kLatencyUltraLow);
    latencyModes.add(kLatencyLow);
    latencyModes.add(kLatencyStandard);
    latencyModes.add(kLatencyHighQuality);

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "latencyMode", "Latency Mode",
        latencyModes,
        3));  // Default: Low (index 2, which is 128 samples)

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
                return juce::String(notes[value]);
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
                return juce::String(scales[value]);
            })));

    // Advanced: Enable downsampling
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "downsampling", "Downsampling (Faster)", true));

    // Advanced: Enable pitch prediction
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "pitchPrediction", "Pitch Prediction", true));

    return { params.begin(), params.end() };
}

//==============================================================================
void ZenithUltraLowLatencyAutoTune::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    sampleRate_ = sampleRate;

    // Prepare pitch detector with current latency mode
    pitchDetector_.prepare(sampleRate, latencyMode_);
    pitchCorrector_.prepare(sampleRate, samplesPerBlock);
    pitchShifter_.prepare(sampleRate);
    scaleAutoDetector_.reset();
    throatModel_.prepare(sampleRate, samplesPerBlock);

    // Prepare pitch corrector
    pitchCorrector_.prepare(sampleRate, samplesPerBlock);

    // Prepare pitch shifter for formant preservation
    pitchShifter_.prepare(sampleRate, 2, samplesPerBlock);

    // Allocate buffers
    monoBuffer_.setSize(1, samplesPerBlock);
    correctedBuffer_.setSize(1, samplesPerBlock);
}

void ZenithUltraLowLatencyAutoTune::releaseResources()
{
    pitchDetector_.reset();
    pitchCorrector_.reset();
    pitchShifter_.reset();
}

//==============================================================================
void ZenithUltraLowLatencyAutoTune::processBlock(juce::AudioBuffer<float>& buffer,
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
        detectedPitch_.store(0.0f);
        confidence_.store(0.0f);
        isVoiced_.store(false);
        if (auto* ptr = algorithmUsed_.load())
            ptr->value = "Disabled";
        return;
    }

    // Check for latency mode changes
    int latencyModeIndex = static_cast<int>(*parameters->getRawParameterValue("latencyMode"));
    LatencyMode newMode = static_cast<LatencyMode>(latencyModeIndex);
    if (newMode != latencyMode_)
    {
        setLatencyMode(newMode);
    }

    // Update advanced features
    bool downsampling = *parameters->getRawParameterValue("downsampling") > 0.5f;
    bool pitchPrediction = *parameters->getRawParameterValue("pitchPrediction") > 0.5f;
    pitchDetector_.setDownsamplingEnabled(downsampling);
    pitchDetector_.setPitchPredictionEnabled(pitchPrediction);

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

    // Detect pitch using ultra-low latency detector
    float detectedPitch = pitchDetector_.processBlock(monoBuffer_);
    detectedPitch_.store(detectedPitch);
    isVoiced_.store(pitchDetector_.isVoiced());
    confidence_.store(pitchDetector_.getConfidence());
    if (auto* ptr = algorithmUsed_.load())
        ptr->value = pitchDetector_.getLastAlgorithmUsed();

    //==========================================================================
    // GRAPH MODE: Record pitch history for visualization
    //==========================================================================
    if (graphModeEnabled_.load() && detectedPitch > 0.0f)
    {
        double currentTime = pitchHistorySampleCount_ / sampleRate_;
        pitchHistory_.push_back({currentTime, detectedPitch});

        // Keep history limited to reasonable size (e.g., 5 minutes)
        constexpr size_t kMaxHistorySize = 5 * 60 * 48;  // 5 min @ 48kHz
        if (pitchHistory_.size() > kMaxHistorySize)
        {
            pitchHistory_.erase(pitchHistory_.begin());
        }
    }
    pitchHistorySampleCount_ += numSamples;

    //==========================================================================
    // AUTO-KEY: Accumulate pitch samples for key detection
    //==========================================================================
    if (autoKeyEnabled_.load() && detectedPitch > 0.0f && pitchDetector_.getConfidence() > 0.6f)
    {
        scaleAutoDetector_.addPitchSample(detectedPitch, pitchDetector_.getConfidence());

        // Periodically check for key update (every ~1 second at 48kHz)
        lastAutoKeyUpdate_ += numSamples;
        if (lastAutoKeyUpdate_ > 48000)
        {
            lastAutoKeyUpdate_ = 0;
            applyAutoKey();
        }
    }

    // Apply pitch correction
    pitchCorrector_.process(monoBuffer_, detectedPitch, correctedBuffer_);

    // Update UI state
    targetPitch_.store(pitchCorrector_.getTargetPitch());
    isCorrecting_.store(pitchCorrector_.isPitchCorrecting());
    currentCorrection_.store(pitchCorrector_.getCurrentCorrection());

    //==========================================================================
    // THROAT MODELING: Apply vocal tract modeling after pitch correction
    //==========================================================================
    if (throatModelEnabled_.load())
    {
        throatModel_.process(correctedBuffer_);
    }

    // Mix corrected signal back to stereo
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
juce::AudioProcessorEditor* ZenithUltraLowLatencyAutoTune::createEditor()
{
    // Return generic editor for now - Skia-based editor is a future enhancement
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void ZenithUltraLowLatencyAutoTune::setLatencyMode(LatencyMode mode)
{
    latencyMode_ = mode;
    pitchDetector_.prepare(sampleRate_, mode);
}

//==============================================================================
// Direct parameter control
void ZenithUltraLowLatencyAutoTune::setRetuneSpeed(float ms)
{
    if (auto* p = parameters->getParameter("retuneSpeed"))
        p->setValueNotifyingHost(p->getNormalisableRange().convertTo0to1(ms));
}

float ZenithUltraLowLatencyAutoTune::getRetuneSpeed() const
{
    return parameters->getRawParameterValue("retuneSpeed")->load();
}

void ZenithUltraLowLatencyAutoTune::setHumanize(float amount)
{
    if (auto* p = parameters->getParameter("humanize"))
        p->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, amount));
}

float ZenithUltraLowLatencyAutoTune::getHumanize() const
{
    return parameters->getRawParameterValue("humanize")->load();
}

void ZenithUltraLowLatencyAutoTune::setCorrectionAmount(float amount)
{
    if (auto* p = parameters->getParameter("correctionAmount"))
        p->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, amount));
}

float ZenithUltraLowLatencyAutoTune::getCorrectionAmount() const
{
    return parameters->getRawParameterValue("correctionAmount")->load();
}

void ZenithUltraLowLatencyAutoTune::setFormantPreservation(float amount)
{
    if (auto* p = parameters->getParameter("formantPreserve"))
        p->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, amount));
}

float ZenithUltraLowLatencyAutoTune::getFormantPreservation() const
{
    return parameters->getRawParameterValue("formantPreserve")->load();
}

void ZenithUltraLowLatencyAutoTune::setKey(int rootNote)
{
    if (auto* p = parameters->getParameter("key"))
        p->setValueNotifyingHost(p->getNormalisableRange().convertTo0to1(static_cast<float>(rootNote)));
}

int ZenithUltraLowLatencyAutoTune::getKey() const
{
    return static_cast<int>(parameters->getRawParameterValue("key")->load());
}

void ZenithUltraLowLatencyAutoTune::setScale(int scaleType)
{
    if (auto* p = parameters->getParameter("scale"))
        p->setValueNotifyingHost(p->getNormalisableRange().convertTo0to1(static_cast<float>(scaleType)));
}

int ZenithUltraLowLatencyAutoTune::getScale() const
{
    return static_cast<int>(parameters->getRawParameterValue("scale")->load());
}

void ZenithUltraLowLatencyAutoTune::setDownsamplingEnabled(bool enable)
{
    if (auto* p = parameters->getParameter("downsampling"))
        p->setValueNotifyingHost(enable ? 1.0f : 0.0f);
}

void ZenithUltraLowLatencyAutoTune::setPitchPredictionEnabled(bool enable)
{
    if (auto* p = parameters->getParameter("pitchPrediction"))
        p->setValueNotifyingHost(enable ? 1.0f : 0.0f);
}

//==============================================================================
// State management
void ZenithUltraLowLatencyAutoTune::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters->copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ZenithUltraLowLatencyAutoTune::setStateInformation(const void* data, int sizeInBytes)
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
juce::StringArray ZenithUltraLowLatencyAutoTune::getPresetNames()
{
    return { kPresetNatural, kPresetTransparent, kPresetTight, kPresetRobot, kPresetSubtle };
}

void ZenithUltraLowLatencyAutoTune::loadPreset(PresetType type)
{
    switch (type)
    {
        case PresetType::Natural:
            loadPreset(kPresetNatural);
            break;
        case PresetType::Transparent:
            loadPreset(kPresetTransparent);
            break;
        case PresetType::Tight:
            loadPreset(kPresetTight);
            break;
        case PresetType::Robot:
            loadPreset(kPresetRobot);
            break;
        case PresetType::Subtle:
            loadPreset(kPresetSubtle);
            break;
    }
}

void ZenithUltraLowLatencyAutoTune::loadPreset(const juce::String& presetName)
{
    if (presetName == kPresetNatural)
    {
        setRetuneSpeed(50.0f);     // Medium speed
        setHumanize(0.6f);          // Fairly natural
        setCorrectionAmount(0.8f);  // Strong correction
        setFormantPreservation(0.8f);
        setCurrentProgram(0);
    }
    else if (presetName == kPresetTransparent)
    {
        setRetuneSpeed(150.0f);     // Slow
        setHumanize(0.9f);          // Very natural
        setCorrectionAmount(0.4f);  // Subtle correction
        setFormantPreservation(0.9f);
        setCurrentProgram(1);
    }
    else if (presetName == kPresetTight)
    {
        setRetuneSpeed(20.0f);      // Fast
        setHumanize(0.3f);          // Less natural
        setCorrectionAmount(1.0f);  // Full correction
        setFormantPreservation(0.7f);
        setCurrentProgram(2);
    }
    else if (presetName == kPresetRobot)
    {
        setRetuneSpeed(0.0f);       // Instant (T-Pain effect)
        setHumanize(0.0f);          // Robotic
        setCorrectionAmount(1.0f);  // Full
        setFormantPreservation(0.5f);
        setCurrentProgram(3);
    }
    else if (presetName == kPresetSubtle)
    {
        setRetuneSpeed(100.0f);     // Very slow
        setHumanize(0.8f);          // Natural
        setCorrectionAmount(0.3f);  // Light correction
        setFormantPreservation(0.9f);
        setCurrentProgram(4);
    }
}

void ZenithUltraLowLatencyAutoTune::setCurrentProgram(int index)
{
    currentProgram_ = juce::jlimit(0, getNumPrograms(), index);
}

const juce::String ZenithUltraLowLatencyAutoTune::getProgramName(int index)
{
    switch (index)
    {
        case 0: return kPresetNatural;
        case 1: return kPresetTransparent;
        case 2: return kPresetTight;
        case 3: return kPresetRobot;
        case 4: return kPresetSubtle;
        default: return {};
    }
}

//==============================================================================
// Auto-Key Integration
//==============================================================================
void ZenithUltraLowLatencyAutoTune::setAutoKeyEnabled(bool enabled)
{
    autoKeyEnabled_ = enabled;
    if (enabled)
    {
        scaleAutoDetector_.reset();
    }
}

dsp::ScaleDetectionResult ZenithUltraLowLatencyAutoTune::getAutoKeyResult() const
{
    return lastAutoKeyResult_;
}

void ZenithUltraLowLatencyAutoTune::setAutoKeySensitivity(float sensitivity)
{
    autoKeySensitivity_ = juce::jlimit(0.0f, 1.0f, sensitivity);
    scaleAutoDetector_.setMinConfidence(sensitivity);
}

void ZenithUltraLowLatencyAutoTune::applyAutoKey()
{
    // Get current detection result
    lastAutoKeyResult_ = scaleAutoDetector_.getResult();

    // Only apply if confidence is high enough
    float sensitivity = autoKeySensitivity_.load();
    if (lastAutoKeyResult_.confidence >= sensitivity)
    {
        // Convert detected key to our scale format
        int rootNote = static_cast<int>(lastAutoKeyResult_.rootNote);
        int scaleType = 0;  // Default to Major

        // Convert MusicalScale enum
        switch (lastAutoKeyResult_.scaleType)
        {
            case dsp::MusicalScale::Major:          scaleType = 1; break;
            case dsp::MusicalScale::Minor:          scaleType = 2; break;
            case dsp::MusicalScale::MinorHarmonic:  scaleType = 3; break;
            case dsp::MusicalScale::MinorMelodic:   scaleType = 4; break;
            case dsp::MusicalScale::Dorian:         scaleType = 8; break;
            case dsp::MusicalScale::Mixolydian:     scaleType = 10; break;
            default: scaleType = 0;  // Chromatic
        }

        // Apply to pitch corrector
        pitchCorrector_.setScale(static_cast<dsp::Note>(rootNote),
                                 static_cast<dsp::MusicalScale>(scaleType));
    }
}

juce::String ZenithUltraLowLatencyAutoTune::getAutoKeyName() const
{
    if (lastAutoKeyResult_.confidence < autoKeySensitivity_.load())
        return "Analyzing...";

    const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    const char* scaleNames[] = {
        "Chromatic", "Major", "Minor", "Harmonic Minor",
        "Melodic Minor", "Pentatonic Major", "Pentatonic Minor",
        "Blues", "Dorian", "Phrygian", "Lydian", "Mixolydian"
    };

    int rootIndex = static_cast<int>(lastAutoKeyResult_.rootNote);
    int scaleIndex = static_cast<int>(lastAutoKeyResult_.scaleType);

    return juce::String(noteNames[rootIndex]) + " " + scaleNames[scaleIndex];
}

//==============================================================================
// Throat Model Integration
//==============================================================================
void ZenithUltraLowLatencyAutoTune::setThroatModelEnabled(bool enabled)
{
    throatModelEnabled_ = enabled;
    throatModel_.setEnabled(enabled);
}

void ZenithUltraLowLatencyAutoTune::setThroatLength(float length)
{
    throatLength_ = juce::jlimit(0.0f, 1.0f, length);
    throatModel_.setLength(throatLength_.load());
}

void ZenithUltraLowLatencyAutoTune::setThroatWidth(float width)
{
    throatWidth_ = juce::jlimit(0.0f, 1.0f, width);
    throatModel_.setWidth(throatWidth_.load());
}

void ZenithUltraLowLatencyAutoTune::setThroatBreathiness(float breathiness)
{
    throatBreathiness_ = juce::jlimit(0.0f, 1.0f, breathiness);
    throatModel_.setBreathiness(throatBreathiness_.load());
}

void ZenithUltraLowLatencyAutoTune::setThroatCharacter(float character)
{
    throatCharacter_ = juce::jlimit(0.0f, 1.0f, character);
    throatModel_.setCharacter(throatCharacter_.load());
}

void ZenithUltraLowLatencyAutoTune::setThroatFormantShift(float semitones)
{
    throatFormantShift_ = juce::jlimit(-12.0f, 12.0f, semitones);
    throatModel_.setFormantShift(throatFormantShift_.load());
}

void ZenithUltraLowLatencyAutoTune::loadThroatPreset(dsp::ThroatModel::Preset preset)
{
    throatModel_.loadPreset(preset);

    // Update our cached values
    throatLength_ = throatModel_.getLength();
    throatWidth_ = throatModel_.getWidth();
    throatBreathiness_ = throatModel_.getBreathiness();
    throatCharacter_ = throatModel_.getCharacter();
    throatFormantShift_ = throatModel_.getFormantShift();
}

//==============================================================================
// Graph Mode Integration
//==============================================================================
void ZenithUltraLowLatencyAutoTune::setNoteCorrections(const std::vector<NoteCorrection>& corrections)
{
    noteCorrections_ = corrections;
}

void ZenithUltraLowLatencyAutoTune::setGraphModeEnabled(bool enabled)
{
    graphModeEnabled_ = enabled;
    if (enabled)
    {
        // When enabling graph mode, capture current pitch history for editing
    }
}

void ZenithUltraLowLatencyAutoTune::updateNoteCorrection(int noteIndex, const NoteCorrection& correction)
{
    if (noteIndex >= 0 && noteIndex < static_cast<int>(noteCorrections_.size()))
    {
        noteCorrections_[noteIndex] = correction;
    }
}

std::vector<std::pair<double, float>> ZenithUltraLowLatencyAutoTune::getPitchHistory() const
{
    return pitchHistory_;
}

void ZenithUltraLowLatencyAutoTune::loadAudioForGraph(const juce::AudioBuffer<float>& audio)
{
    graphAudioBuffer_.makeCopyOf(audio);
    pitchHistory_.clear();
    pitchHistorySampleCount_ = 0;
}

//==============================================================================
// MIDI Control / MIDI Learn
//==============================================================================
void ZenithUltraLowLatencyAutoTune::mapMidiCC(int ccNumber, const juce::String& parameterID)
{
    // Check if mapping already exists for this CC
    for (auto& mapping : midiMappings_)
    {
        if (mapping.ccNumber == ccNumber)
        {
            mapping.parameterID = parameterID;
            return;
        }
    }

    // Create new mapping
    MidiMapping mapping;
    mapping.ccNumber = ccNumber;
    mapping.parameterID = parameterID;
    mapping.channel = midiChannel_.load();
    midiMappings_.push_back(mapping);
}

void ZenithUltraLowLatencyAutoTune::processMidiControl(const juce::MidiBuffer& midiMessages)
{
    if (midiMappings_.empty())
        return;

    int currentChannel = midiChannel_.load();

    for (const auto& message : midiMessages)
    {
        auto midiMessage = message.getMessage();

        // Only process CC messages
        if (!midiMessage.isController())
            continue;

        // Check channel filter
        if (currentChannel >= 0 && midiMessage.getChannel() != currentChannel)
            continue;

        int ccNumber = midiMessage.getControllerNumber();
        float ccValue = midiMessage.getControllerValue() / 127.0f;

        // Apply to mapped parameters
        for (const auto& mapping : midiMappings_)
        {
            if (mapping.ccNumber == ccNumber)
            {
                // Apply mapping
                float mappedValue = juce::jlimit(mapping.minValue, mapping.maxValue,
                    mapping.invert ? (1.0f - ccValue) : ccValue);

                // Set parameter value
                if (auto* param = parameters->getParameter(mapping.parameterID))
                {
                    param->setValueNotifyingHost(mappedValue);
                }
            }
        }

        // MIDI learn: capture CC for current learn parameter
        if (midiLearnEnabled_.load() && !currentLearnParameter_.isEmpty())
        {
            MidiMapping mapping;
            mapping.ccNumber = ccNumber;
            mapping.parameterID = currentLearnParameter_;
            mapping.channel = currentChannel;

            // Remove existing mapping for this parameter if any
            midiMappings_.erase(
                std::remove_if(midiMappings_.begin(), midiMappings_.end(),
                    [this](const MidiMapping& m) { return m.parameterID == currentLearnParameter_; }),
                midiMappings_.end());

            midiMappings_.push_back(mapping);
            midiLearnEnabled_ = false;
            currentLearnParameter_ = "";
        }
    }
}

//==============================================================================
// Automation Smoothing
//==============================================================================
void ZenithUltraLowLatencyAutoTune::setAutomationSmoothing(const juce::String& parameterID, float ms)
{
    SmoothedParameter& param = smoothedParameters_[parameterID];
    param.smoothingTime = juce::jmax(0.0f, ms);
}

void ZenithUltraLowLatencyAutoTune::updateSmoothedParameters(int numSamples)
{
    if (!automationSmoothingEnabled_.load())
        return;

    float sampleRate = static_cast<float>(sampleRate_);

    for (auto& [paramID, param] : smoothedParameters_)
    {
        if (!param.isSmoothing)
            continue;

        // Calculate smoothing coefficient
        // T = smoothing time in samples
        // alpha = 1 - exp(-1/T)
        float T = (param.smoothingTime / 1000.0f) * sampleRate;
        float alpha = (T > 0.0f) ? (1.0f - std::exp(-1.0f / T)) : 1.0f;

        // Smooth towards target
        float diff = param.targetValue - param.currentValue;
        if (std::abs(diff) < 0.0001f)
        {
            param.currentValue = param.targetValue;
            param.isSmoothing = false;
        }
        else
        {
            param.currentValue += alpha * diff;
        }

        // Apply to parameter
        if (auto* p = parameters->getParameter(paramID))
        {
            p->setValueNotifyingHost(param.currentValue);
        }
    }
}

float ZenithUltraLowLatencyAutoTune::getSmoothedParameterValue(const juce::String& parameterID) const
{
    auto it = smoothedParameters_.find(parameterID);
    if (it != smoothedParameters_.end())
        return it->second.currentValue;
    return 0.0f;
}

} // namespace effects
} // namespace zenith

//==============================================================================
// Plugin wrapper
#include <juce_audio_processors/juce_audio_processors.h>

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new zenith::effects::ZenithUltraLowLatencyAutoTune();
}

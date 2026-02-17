/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#include "NativeAutoTuneEffect.h"
#include <algorithm>

namespace zenith {
namespace engine {

//==============================================================================
// NativeAutoTuneEffect Implementation
//==============================================================================
NativeAutoTuneEffect::NativeAutoTuneEffect()
{
    // Register parameters
    addParameter({
        "retuneSpeed", "Retune Speed", 0.0f, 800.0f, 50.0f, &retuneSpeed_, true, 20.0f
    });
    addParameter({
        "correctionAmount", "Correction Amount", 0.0f, 1.0f, 1.0f, &correctionAmount_, true, 10.0f
    });
    addParameter({
        "humanize", "Humanize", 0.0f, 1.0f, 0.5f, &humanize_, true, 10.0f
    });
    addParameter({
        "formantPreserve", "Formant Preservation", 0.0f, 1.0f, 0.8f, &formantPreservation_, true, 5.0f
    });
    addParameter({
        "key", "Key", 0.0f, 11.0f, 0.0f, reinterpret_cast<std::atomic<float>*>(&key_), true, 0.0f
    });
    addParameter({
        "scale", "Scale", 0.0f, 11.0f, 0.0f, reinterpret_cast<std::atomic<float>*>(&scale_), true, 0.0f
    });
}

NativeAutoTuneEffect::~NativeAutoTuneEffect()
{
}

void NativeAutoTuneEffect::prepare(double sampleRate, int maxSamplesPerBlock)
{
    sampleRate_ = sampleRate;
    maxSamplesPerBlock_ = maxSamplesPerBlock;

    // Prepare DSP modules
    pitchDetector_.prepare(sampleRate, static_cast<int>(latencyMode_.load()));
    pitchCorrector_.prepare(sampleRate, maxSamplesPerBlock);
    pitchShifter_.prepare(sampleRate);
    scaleAutoDetector_.reset();
    throatModel_.prepare(sampleRate, maxSamplesPerBlock);

    // Prepare buffers
    monoBuffer_.setSize(1, maxSamplesPerBlock);
    correctedBuffer_.setSize(1, maxSamplesPerBlock);
}

void NativeAutoTuneEffect::reset()
{
    pitchDetector_.reset();
    pitchCorrector_.reset();
    pitchShifter_.reset();
    monoBuffer_.clear();
    correctedBuffer_.clear();
}

void NativeAutoTuneEffect::process(juce::AudioBuffer<float>& buffer,
                                   const juce::AudioBuffer<float>* sidechain)
{
    (void)sidechain;  // Unused for now

    if (isBypassed())
        return;

    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    // Ensure buffers are large enough
    if (monoBuffer_.getNumSamples() < numSamples)
    {
        monoBuffer_.setSize(1, numSamples, true, true, true);
        correctedBuffer_.setSize(1, numSamples, true, true, true);
    }

    // Convert to mono for pitch detection
    monoBuffer_.clear();
    for (int ch = 0; ch < numChannels; ++ch)
    {
        monoBuffer_.addFrom(0, 0, buffer, ch, 0, numSamples, 1.0f / numChannels);
    }

    // Detect pitch with ultra-low latency detector
    float detectedPitch = pitchDetector_.processBlock(monoBuffer_);
    detectedPitch_.store(detectedPitch);
    confidence_.store(pitchDetector_.getConfidence());

    //==========================================================================
    // AUTO-KEY: Accumulate pitch samples
    //==========================================================================
    if (autoKeyEnabled_.load() && detectedPitch > 0.0f && pitchDetector_.getConfidence() > 0.6f)
    {
        scaleAutoDetector_.addPitchSample(detectedPitch, pitchDetector_.getConfidence());

        // Periodically check for key update (every ~1 second)
        autoKeySampleCount_ += numSamples;
        if (autoKeySampleCount_ > 48000)
        {
            autoKeySampleCount_ = 0;
            lastAutoKeyResult_ = scaleAutoDetector_.getResult();

            // Apply detected key if confidence is high
            if (lastAutoKeyResult_.confidence >= 0.7f)
            {
                pitchCorrector_.scale_.rootNote = lastAutoKeyResult_.rootNote;
                pitchCorrector_.scale_.scaleType = static_cast<dsp::MusicalScale>(lastAutoKeyResult_.scaleType);
            }
        }
    }

    //==========================================================================
    // Update pitch corrector parameters from atomic values
    //==========================================================================
    pitchCorrector_.setRetuneSpeed(retuneSpeed_.load());
    pitchCorrector_.setHumanize(humanize_.load());
    pitchCorrector_.setCorrectionAmount(correctionAmount_.load());
    pitchCorrector_.setFormantPreservation(formantPreservation_.load());
    pitchCorrector_.scale_.rootNote = static_cast<dsp::Note>(key_.load());
    pitchCorrector_.scale_.scaleType = static_cast<dsp::MusicalScale>(scale_.load());

    // Flex-Tune
    pitchCorrector_.flexTuneEnabled_ = flexTuneEnabled_.load();
    pitchCorrector_.flexTuneThreshold_ = flexTuneThreshold_.load();
    pitchCorrector_.flexTuneAmount_ = flexTuneAmount_.load();

    //==========================================================================
    // Apply pitch correction
    //==========================================================================
    pitchCorrector_.process(monoBuffer_, detectedPitch, correctedBuffer_);

    // Update UI feedback
    targetPitch_.store(pitchCorrector_.getTargetPitch());
    isCorrecting_.store(pitchCorrector_.isPitchCorrecting());

    //==========================================================================
    // Throat modeling
    //==========================================================================
    if (throatModelEnabled_.load())
    {
        throatModel_.process(correctedBuffer_);
    }

    //==========================================================================
    // Mix corrected signal back to output
    //==========================================================================
    float correctionAmt = correctionAmount_.load();

    // Calculate gain compensation
    float inputLevel = monoBuffer_.getRMSLevel(0, 0, numSamples);
    float outputLevel = correctedBuffer_.getRMSLevel(0, 0, numSamples);
    float gainComp = (outputLevel > 0.001f) ? inputLevel / outputLevel : 1.0f;
    gainComp = juce::jlimit(0.5f, 2.0f, gainComp);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float originalSample = buffer.getSample(ch, i);
            float correctedSample = correctedBuffer_.getSample(0, i) * gainComp;
            buffer.setSample(ch, i, originalSample * (1.0f - correctionAmt) + correctedSample * correctionAmt);
        }
    }
}

int NativeAutoTuneEffect::getLatencySamples() const
{
    return pitchDetector_.getLatencySamples() + pitchCorrector_.getLatencySamples();
}

//==============================================================================
juce::ValueTree NativeAutoTuneEffect::getState() const
{
    juce::ValueTree state("AutoTuneState");

    state.setProperty("latencyMode", static_cast<int>(latencyMode_.load()), nullptr);
    state.setProperty("retuneSpeed", retuneSpeed_.load(), nullptr);
    state.setProperty("correctionAmount", correctionAmount_.load(), nullptr);
    state.setProperty("humanize", humanize_.load(), nullptr);
    state.setProperty("formantPreserve", formantPreservation_.load(), nullptr);
    state.setProperty("key", key_.load(), nullptr);
    state.setProperty("scale", scale_.load(), nullptr);
    state.setProperty("flexTuneEnabled", flexTuneEnabled_.load(), nullptr);
    state.setProperty("flexTuneThreshold", flexTuneThreshold_.load(), nullptr);
    state.setProperty("autoKeyEnabled", autoKeyEnabled_.load(), nullptr);
    state.setProperty("throatModelEnabled", throatModelEnabled_.load(), nullptr);
    state.setProperty("throatLength", throatLength_.load(), nullptr);
    state.setProperty("throatWidth", throatWidth_.load(), nullptr);

    return state;
}

void NativeAutoTuneEffect::setState(const juce::ValueTree& state)
{
    if (!state.isValid())
        return;

    latencyMode_ = static_cast<LatencyMode>(state.getProperty("latencyMode", 3));
    retuneSpeed_ = state.getProperty("retuneSpeed", 50.0f);
    correctionAmount_ = state.getProperty("correctionAmount", 1.0f);
    humanize_ = state.getProperty("humanize", 0.5f);
    formantPreservation_ = state.getProperty("formantPreserve", 0.8f);
    key_ = state.getProperty("key", 0);
    scale_ = state.getProperty("scale", 0);
    flexTuneEnabled_ = state.getProperty("flexTuneEnabled", false);
    flexTuneThreshold_ = state.getProperty("flexTuneThreshold", 15.0f);
    autoKeyEnabled_ = state.getProperty("autoKeyEnabled", false);
    throatModelEnabled_ = state.getProperty("throatModelEnabled", false);
    throatLength_ = state.getProperty("throatLength", 0.5f);
    throatWidth_ = state.getProperty("throatWidth", 0.5f);

    // Re-prepare pitch detector with new latency mode
    if (sampleRate_ > 0)
        pitchDetector_.prepare(sampleRate_, static_cast<int>(latencyMode_.load()));
}

//==============================================================================
// Parameter setters
//==============================================================================
void NativeAutoTuneEffect::setLatencyMode(LatencyMode mode)
{
    latencyMode_ = mode;
    pitchDetector_.prepare(sampleRate_, static_cast<int>(mode));
}

void NativeAutoTuneEffect::setRetuneSpeed(float ms)
{
    retuneSpeed_ = juce::jlimit(0.0f, 800.0f, ms);
}

void NativeAutoTuneEffect::setCorrectionAmount(float amount)
{
    correctionAmount_ = juce::jlimit(0.0f, 1.0f, amount);
}

void NativeAutoTuneEffect::setHumanize(float amount)
{
    humanize_ = juce::jlimit(0.0f, 1.0f, amount);
}

void NativeAutoTuneEffect::setFormantPreservation(float amount)
{
    formantPreservation_ = juce::jlimit(0.0f, 1.0f, amount);
}

void NativeAutoTuneEffect::setKey(int rootNote)
{
    key_ = juce::jlimit(0, 11, rootNote);
}

void NativeAutoTuneEffect::setScale(int scaleType)
{
    scale_ = juce::jlimit(0, 11, scaleType);
}

//==============================================================================
// Flex-Tune
//==============================================================================
void NativeAutoTuneEffect::setFlexTuneEnabled(bool enabled)
{
    flexTuneEnabled_ = enabled;
}

void NativeAutoTuneEffect::setFlexTuneThreshold(float cents)
{
    flexTuneThreshold_ = juce::jlimit(0.0f, 50.0f, cents);
}

void NativeAutoTuneEffect::setFlexTuneAmount(float amount)
{
    flexTuneAmount_ = juce::jlimit(0.0f, 1.0f, amount);
}

//==============================================================================
// Auto-Key
//==============================================================================
void NativeAutoTuneEffect::setAutoKeyEnabled(bool enabled)
{
    autoKeyEnabled_ = enabled;
    if (enabled)
        scaleAutoDetector_.reset();
}

dsp::ScaleDetectionResult NativeAutoTuneEffect::getAutoKeyResult() const
{
    return lastAutoKeyResult_;
}

juce::String NativeAutoTuneEffect::getAutoKeyName() const
{
    if (lastAutoKeyResult_.confidence < 0.5f)
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
// Throat Modeling
//==============================================================================
void NativeAutoTuneEffect::setThroatModelEnabled(bool enabled)
{
    throatModelEnabled_ = enabled;
    throatModel_.setEnabled(enabled);
}

void NativeAutoTuneEffect::setThroatLength(float length)
{
    throatLength_ = juce::jlimit(0.0f, 1.0f, length);
    throatModel_.setLength(throatLength_.load());
}

void NativeAutoTuneEffect::setThroatWidth(float width)
{
    throatWidth_ = juce::jlimit(0.0f, 1.0f, width);
    throatModel_.setWidth(throatWidth_.load());
}

void NativeAutoTuneEffect::setThroatBreathiness(float amount)
{
    throatBreathiness_ = juce::jlimit(0.0f, 1.0f, amount);
    throatModel_.setBreathiness(throatBreathiness_.load());
}

//==============================================================================
// Graph Mode
//==============================================================================
void NativeAutoTuneEffect::setGraphModeEnabled(bool enabled)
{
    graphModeEnabled_ = enabled;
}

void NativeAutoTuneEffect::setNoteCorrections(const std::vector<NoteCorrection>& corrections)
{
    noteCorrections_ = corrections;
}

//==============================================================================
// Presets
//==============================================================================
void NativeAutoTuneEffect::loadPreset(Preset preset)
{
    switch (preset)
    {
        case Preset::Natural:
            setRetuneSpeed(50.0f);
            setHumanize(0.6f);
            setCorrectionAmount(0.8f);
            setFormantPreservation(0.8f);
            break;
        case Preset::Transparent:
            setRetuneSpeed(150.0f);
            setHumanize(0.9f);
            setCorrectionAmount(0.4f);
            setFormantPreservation(0.9f);
            break;
        case Preset::Tight:
            setRetuneSpeed(20.0f);
            setHumanize(0.3f);
            setCorrectionAmount(1.0f);
            setFormantPreservation(0.7f);
            break;
        case Preset::Robot:
            setRetuneSpeed(0.0f);
            setHumanize(0.0f);
            setCorrectionAmount(1.0f);
            setFormantPreservation(0.5f);
            break;
        case Preset::Subtle:
            setRetuneSpeed(100.0f);
            setHumanize(0.8f);
            setCorrectionAmount(0.3f);
            setFormantPreservation(0.9f);
            break;
    }
}

//==============================================================================
// Factory
//==============================================================================
std::unique_ptr<NativeAutoTuneEffect> AutoTuneEffectFactory::create()
{
    return std::make_unique<NativeAutoTuneEffect>();
}

} // namespace engine
} // namespace zenith

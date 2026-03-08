/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#include "DynamicEQ.h"
#include <algorithm>
#include <cmath>

namespace zenith {
namespace effects {

//==============================================================================
// DynamicEQ Implementation
//==============================================================================

DynamicEQ::DynamicEQ()
{
    setupDefaultBands();

    // Register parameters
    addParameter({"wetDryMix", "Mix", 0.0f, 1.0f, 1.0f, true, 0.0f});
    addParameter({"outputGain", "Output", -20.0f, 20.0f, 0.0f, true, 0.0f});
    addParameter({"bandsLinked", "Link Bands", 0.0f, 1.0f, 0.0f, false, 0.0f});
}

DynamicEQ::~DynamicEQ()
{
}

//==============================================================================
void DynamicEQ::setDefaultBands()
{
    numBands_ = 4;

    // Band 0: Low-end control (sub-bass)
    bands_[0].frequency = 80.0f;
    bands_[0].q = 1.0f;
    bands_[0].threshold = -20.0f;
    bands_[0].ratio = 4.0f;
    bands_[0].attack = 20.0f;
    bands_[0].release = 200.0f;
    bands_[0].knee = 6.0f;
    bands_[0].makeup = 0.0f;

    // Band 1: Low-mids control (mud, boxiness)
    bands_[1].frequency = 250.0f;
    bands_[1].q = 2.0f;
    bands_[1].threshold = -15.0f;
    bands_[1].ratio = 3.0f;
    bands_[1].attack = 10.0f;
    bands_[1].release = 100.0f;
    bands_[1].knee = 6.0f;
    bands_[1].makeup = 0.0f;

    // Band 2: High-mids control (presence, harshness)
    bands_[2].frequency = 2500.0f;
    bands_[2].q = 2.0f;
    bands_[2].threshold = -15.0f;
    bands_[2].ratio = 3.0f;
    bands_[2].attack = 10.0f;
    bands_[2].release = 100.0f;
    bands_[2].knee = 6.0f;
    bands_[2].makeup = 0.0f;

    // Band 3: Air control (high end harshness)
    bands_[3].frequency = 8000.0f;
    bands_[3].q = 1.5f;
    bands_[3].threshold = -18.0f;
    bands_[3].ratio = 2.0f;
    bands_[3].attack = 5.0f;
    bands_[3].release = 50.0f;
    bands_[3].knee = 6.0f;
    bands_[3].makeup = 0.0f;
}

void DynamicEQ::prepare(double sampleRate, int maxSamplesPerBlock)
{
    sampleRate_ = sampleRate;
    maxSamplesPerBlock_ = maxSamplesPerBlock;

    // Prepare filters for each band
    juce::dsp::ProcessSpec spec{sampleRate, static_cast<juce::uint32>(maxSamplesPerBlock), 2};

    for (int i = 0; i < maxBands; ++i)
    {
        bandFilters_[i].bandpass.prepare(spec);
        bandFilters_[i].sidechainFilter.prepare(spec);
        bandCompressors_[i].compressor.prepare(spec);
        bandCompressors_[i].envelopeFollower.prepare(spec);

        // Prepare band buffers
        bandBuffers_[i].setSize(2, maxSamplesPerBlock);
    }

    updateAllFilters();
}

void DynamicEQ::reset()
{
    for (int i = 0; i < maxBands; ++i)
    {
        bandFilters_[i].bandpass.reset();
        bandFilters_[i].sidechainFilter.reset();
        bandCompressors_[i].compressor.reset();
        bandCompressors_[i].envelopeFollower.reset();
        bandBuffers_[i].clear();

        bands_[i].gainReduction = 0.0f;
        bands_[i].inputLevel = -100.0f;
        bands_[i].outputLevel = -100.0f;
    }
}

void DynamicEQ::process(juce::AudioBuffer<float>& buffer,
                        const juce::AudioBuffer<float>* sidechain)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Store dry signal for wet/dry mix
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    // Check if any bands are soloed
    bool anySolo = false;
    for (int i = 0; i < numBands_; ++i)
    {
        if (bands_[i].solo)
        {
            anySolo = true;
            break;
        }
    }

    // Process each band
    for (int band = 0; band < numBands_; ++band)
    {
        // Skip if disabled, muted (unless soloed), or not in use
        if (!bands_[band].enabled || (!bands_[band].solo && anySolo))
            continue;

        // Process this band
        processBand(band, buffer, sidechain);
    }

    // Apply wet/dry mix
    float wet = wetDryMix_.load();
    float dry = 1.0f - wet;
    float wetGain = std::sqrt(wet);
    float dryGain = std::sqrt(dry);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* out = buffer.getWritePointer(ch);
        auto* wetChan = buffer.getReadPointer(ch);
        auto* dryChan = dryBuffer.getReadPointer(ch);

        for (int i = 0; i < numSamples; ++i)
        {
            out[i] = wetChan[i] * wetGain + dryChan[i] * dryGain;
        }
    }

    // Apply output gain
    float outputGain = juce::Decibels::decibelsToGain(outputGain_.load());
    buffer.applyGain(outputGain);
}

void DynamicEQ::processBand(int bandIndex, juce::AudioBuffer<float>& buffer,
                             const juce::AudioBuffer<float>* sidechain)
{
    auto& bandData = bands_[bandIndex];
    auto& filters = bandFilters_[bandIndex];
    auto& compressor = bandCompressors_[bandIndex];

    // Copy input to band buffer
    bandBuffers_[bandIndex].makeCopyOf(buffer);

    // Apply bandpass filter to isolate frequency range
    juce::dsp::AudioBlock<float> bandBlock(bandBuffers_[bandIndex]);
    juce::dsp::ProcessContextReplacing<float> bandContext(bandBlock);
    filters.bandpass.process(bandContext);

    // Get sidechain signal (either external or internal)
    juce::AudioBuffer<float> sidechainBuffer;
    if (sidechain && bandData.externalSidechain)
    {
        // Use external sidechain (specified channel)
        sidechainBuffer = const_cast<juce::AudioBuffer<float>*>(sidechain)->getSubsetChannel(0, bandData.sidechainChannel);
    }
    else
    {
        // Use internal band signal
        sidechainBuffer = bandBuffers_[bandIndex];
    }

    // Apply sidechain filter if external
    if (bandData.externalSidechain)
    {
        juce::dsp::AudioBlock<float> scBlock(sidechainBuffer);
        juce::dsp::ProcessContextReplacing<float> scContext(scBlock);
        filters.sidechainFilter.process(scContext);
    }

    // Calculate band level for metering
    float maxLevel = 0.0f;
    const float* scData = sidechainBuffer.getReadPointer(0);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        maxLevel = juce::jmax(maxLevel, std::abs(scData[i]));
    }
    bandData.inputLevel = juce::Decibels::gainToDecibels(maxLevel + 1e-6f);

    // Auto-threshold learning
    if (bandData.autoThreshold)
    {
        learnThresholdForBand(bandIndex);
    }

    // Configure compressor
    compressor.compressor.setThreshold(bandData.threshold);
    compressor.compressor.setRatio(bandData.ratio);
    compressor.compressor.setAttack(bandData.attack / 1000.0);  // ms to seconds
    compressor.compressor.setRelease(bandData.release / 1000.0);
    compressor.compressor.setKnee(bandData.knee / 2.0f);  // Full knee width

    // Process compression on band signal
    juce::dsp::AudioBlock<float> processBlock(bandBuffers_[bandIndex]);
    juce::dsp::ProcessContextReplacing<float> processContext(processBlock);
    compressor.compressor.process(processContext);

    // Get gain reduction
    bandData.gainReduction = compressor.compressor.getAverageGainReduction();

    // Apply makeup gain
    float makeup = juce::Decibels::decibelsToGain(bandData.makeup);
    bandBuffers_[bandIndex].applyGain(makeup);

    // Calculate output level
    maxLevel = 0.0f;
    const float* outData = bandBuffers_[bandIndex].getReadPointer(0);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        maxLevel = juce::jmax(maxLevel, std::abs(outData[i]));
    }
    bandData.outputLevel = juce::Decibels::gainToDecibels(maxLevel + 1e-6f);

    // Mix back into main buffer (sum with other bands)
    // In a full implementation, would use more sophisticated mixing
    // For now, simple additive approach
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* main = buffer.getWritePointer(ch);
        auto* bandSignal = bandBuffers_[bandIndex].getReadPointer(ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            // Add band signal to main (simple approach - would subtract dry signal in full implementation)
            main[i] += bandSignal[i] * 0.25f;  // Divide by numBands to prevent clipping
        }
    }
}

//==============================================================================
void DynamicEQ::setNumBands(int num)
{
    numBands_ = juce::jlimit(1, maxBands, num);
    updateAllFilters();
}

void DynamicEQ::setBand(int index, const DynamicBand& band)
{
    if (index >= 0 && index < maxBands)
    {
        bands_[index] = band;
        updateFilters(index);
    }
}

DynamicEQ::DynamicBand DynamicEQ::getBand(int index) const
{
    if (index >= 0 && index < maxBands)
        return bands_[index];
    return DynamicBand{};
}

void DynamicEQ::setBandFrequency(int band, float frequency)
{
    if (band >= 0 && band < maxBands)
    {
        bands_[band].frequency = juce::jlimit(20.0f, 20000.0f, frequency);
        updateFilters(band);
    }
}

void DynamicEQ::setBandQ(int band, float q)
{
    if (band >= 0 && band < maxBands)
    {
        bands_[band].q = juce::jlimit(0.1f, 100.0f, q);
        updateFilters(band);
    }
}

void DynamicEQ::setBandEnabled(int band, bool enabled)
{
    if (band >= 0 && band < maxBands)
        bands_[band].enabled = enabled;
}

void DynamicEQ::setBandSolo(int band, bool solo)
{
    if (band >= 0 && band < maxBands)
        bands_[band].solo = solo;
}

void DynamicEQ::setThreshold(int band, float dB)
{
    if (band >= 0 && band < maxBands)
        bands_[band].threshold = juce::jlimit(-60.0f, 0.0f, dB);
}

void DynamicEQ::setRatio(int band, float ratio)
{
    if (band >= 0 && band < maxBands)
        bands_[band].ratio = juce::jmax(1.0f, ratio);
}

void DynamicEQ::setAttack(int band, float ms)
{
    if (band >= 0 && band < maxBands)
        bands_[band].attack = juce::jlimit(0.1f, 100.0f, ms);
}

void DynamicEQ::setRelease(int band, float ms)
{
    if (band >= 0 && band < maxBands)
        bands_[band].release = juce::jlimit(10.0f, 1000.0f, ms);
}

void DynamicEQ::setKnee(int band, float dB)
{
    if (band >= 0 && band < maxBands)
        bands_[band].knee = juce::jlimit(0.0f, 24.0f, dB);
}

void DynamicEQ::setMakeupGain(int band, float dB)
{
    if (band >= 0 && band < maxBands)
        bands_[band].makeup = juce::jlimit(-20.0f, 20.0f, dB);
}

void DynamicEQ::setExternalSidechain(int band, bool enabled)
{
    if (band >= 0 && band < maxBands)
        bands_[band].externalSidechain = enabled;
}

void DynamicEQ::setSidechainChannel(int band, int channel)
{
    if (band >= 0 && band < maxBands)
        bands_[band].sidechainChannel = juce::jlimit(0, 127, channel);
}

void DynamicEQ::setAutoThresholdEnabled(int band, bool enabled)
{
    if (band >= 0 && band < maxBands)
    {
        bands_[band].autoThreshold = enabled;
        if (enabled)
            learningThreshold_[band] = true;
    }
}

void DynamicEQ::learnThreshold(int band)
{
    if (band >= 0 && band < maxBands)
        learningThreshold_[band] = true;
}

bool DynamicEQ::isLearningThreshold(int band) const
{
    if (band >= 0 && band < maxBands)
        return learningThreshold_[band];
    return false;
}

void DynamicEQ::setBandsLinked(bool linked)
{
    bandsLinked_ = linked;
}

void DynamicEQ::setLinkGroup(int band, int group)
{
    if (band >= 0 && band < maxBands)
        linkGroups_[band] = juce::jlimit(0, 3, group);
}

void DynamicEQ::setWetDryMix(float mix)
{
    wetDryMix_.store(juce::jlimit(0.0f, 1.0f, mix));
}

void DynamicEQ::setOutputGain(float dB)
{
    outputGain_.store(juce::jlimit(-20.0f, 20.0f, dB));
}

float DynamicEQ::getGainReduction(int band) const
{
    if (band >= 0 && band < maxBands)
        return bands_[band].gainReduction;
    return 0.0f;
}

float DynamicEQ::getInputLevel(int band) const
{
    if (band >= 0 && band < maxBands)
        return bands_[band].inputLevel;
    return -100.0f;
}

float DynamicEQ::getOutputLevel(int band) const
{
    if (band >= 0 && band < maxBands)
        return bands_[band].outputLevel;
    return -100.0f;
}

float DynamicEQ::getBandLevel(int band, float frequency, float q) const
{
    // Calculate level in a specific frequency band
    // This would be used for visualization
    // Simplified implementation - returns current level
    if (band >= 0 && band < numBands_)
        return bands_[band].inputLevel;
    return -100.0f;
}

void DynamicEQ::learnThresholdForBand(int bandIndex)
{
    if (bandIndex < 0 || bandIndex >= maxBands)
        return;

    auto& band = bands_[bandIndex];

    // Learn threshold from current input level
    float level = band.inputLevel;

    // Set threshold slightly below current level (with hysteresis)
    if (band.learnedThreshold < -100.0f)
    {
        // First learning
        band.learnedThreshold = level - 3.0f;  // 3dB below
    }
    else
    {
        // Continuous learning (slow adaptation)
        float adaptationRate = 0.01f;
        band.learnedThreshold = band.learnedThreshold * (1.0f - adaptationRate) + level * adaptationRate;
    }

    band.threshold = band.learnedThreshold;

    // Stop learning after a while
    static int learnCount = 0;
    learnCount++;
    if (learnCount > 100)  // Learn for ~1 second at 48kHz
    {
        learningThreshold_[bandIndex] = false;
        learnCount = 0;
    }
}

void DynamicEQ::updateFilters(int bandIndex)
{
    if (bandIndex < 0 || bandIndex >= maxBands)
        return;

    const auto& band = bands_[bandIndex];

    // Design bandpass filter for this band
    *bandFilters_[bandIndex].bandpass.state =
        *juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate_, band.frequency, band.q);
}

void DynamicEQ::updateAllFilters()
{
    for (int i = 0; i < maxBands; ++i)
    {
        updateFilters(i);
    }
}

//==============================================================================
juce::ValueTree DynamicEQ::getState() const
{
    juce::ValueTree state("DynamicEQ");

    state.setProperty("numBands", numBands_, nullptr);
    state.setProperty("wetDryMix", wetDryMix_.load(), nullptr);
    state.setProperty("outputGain", outputGain_.load(), nullptr);
    state.setProperty("bandsLinked", bandsLinked_, nullptr);

    // Save each band
    for (int i = 0; i < numBands_; ++i)
    {
        juce::ValueTree bandState("Band");
        bandState.setProperty("index", i, nullptr);
        bandState.setProperty("frequency", bands_[i].frequency, nullptr);
        bandState.setProperty("q", bands_[i].q, nullptr);
        bandState.setProperty("enabled", bands_[i].enabled, nullptr);
        bandState.setProperty("threshold", bands_[i].threshold, nullptr);
        bandState.setProperty("ratio", bands_[i].ratio, nullptr);
        bandState.setProperty("attack", bands_[i].attack, nullptr);
        bandState.setProperty("release", bands_[i].release, nullptr);
        bandState.setProperty("knee", bands_[i].knee, nullptr);
        bandState.setProperty("makeup", bands_[i].makeup, nullptr);
        bandState.setProperty("externalSidechain", bands_[i].externalSidechain, nullptr);
        bandState.setProperty("sidechainChannel", bands_[i].sidechainChannel, nullptr);
        bandState.setProperty("autoThreshold", bands_[i].autoThreshold, nullptr);
        bandState.setProperty("linkGroup", linkGroups_[i], nullptr);
        state.addChild(bandState, -1, nullptr);
    }

    return state;
}

void DynamicEQ::setState(const juce::ValueTree& state)
{
    if (!state.isValid() || state.getType() != juce::String("DynamicEQ"))
        return;

    numBands_ = state.getProperty("numBands", defaultBands);
    wetDryMix_.store(state.getProperty("wetDryMix", 1.0f));
    outputGain_.store(state.getProperty("outputGain", 0.0f));
    bandsLinked_ = state.getProperty("bandsLinked", false);

    // Load bands
    for (const auto& child : state)
    {
        if (child.getType() == juce::String("Band"))
        {
            int index = child.getProperty("index", 0);
            if (index >= 0 && index < maxBands)
            {
                bands_[index].frequency = child.getProperty("frequency", 1000.0f);
                bands_[index].q = child.getProperty("q", 2.0f);
                bands_[index].enabled = child.getProperty("enabled", true);
                bands_[index].threshold = child.getProperty("threshold", -20.0f);
                bands_[index].ratio = child.getProperty("ratio", 4.0f);
                bands_[index].attack = child.getProperty("attack", 10.0f);
                bands_[index].release = child.getProperty("release", 100.0f);
                bands_[index].knee = child.getProperty("knee", 6.0f);
                bands_[index].makeup = child.getProperty("makeup", 0.0f);
                bands_[index].externalSidechain = child.getProperty("externalSidechain", false);
                bands_[index].sidechainChannel = child.getProperty("sidechainChannel", 0);
                bands_[index].autoThreshold = child.getProperty("autoThreshold", false);
                linkGroups_[index] = child.getProperty("linkGroup", 0);
            }
        }
    }

    updateAllFilters();
}

//==============================================================================
// Presets
void DynamicEQ::loadPreset(const juce::::String& presetName)
{
    if (presetName == "De-Essing")
    {
        numBands_ = 1;

        bands_[0].frequency = 6000.0f;
        bands_[0].q = 4.0f;
        bands_[0].threshold = -30.0f;
        bands_[0].ratio = 5.0f;
        bands_[0].attack = 1.0f;
        bands_[0].release = 50.0f;
        bands_[0].knee = 4.0f;
        bands_[0].makeup = 0.0f;
        bands_[0].enabled = true;
    }
    else if (presetName == "Bass Tamer")
    {
        numBands_ = 2;

        // Sub control
        bands_[0].frequency = 60.0f;
        bands_[0].q = 2.0f;
        bands_[0].threshold = -12.0f;
        bands_[0].ratio = 4.0f;
        bands_[0].attack = 20.0f;
        bands_[0].release = 200.0f;
        bands_[0].knee = 6.0f;
        bands_[0].makeup = 0.0f;

        // Low-mud control
        bands_[1].frequency = 200.0f;
        bands_[1].q = 1.5f;
        bands_[1].threshold = -10.0f;
        bands_[1].ratio = 3.0f;
        bands_[1].attack = 10.0f;
        bands_[1].release = 100.0f;
        bands_[1].knee = 6.0f;
        bands_[1].makeup = 0.0f;
    }
    else if (presetName == "Vocal Sibilance")
    {
        numBands_ = 2;

        // Sibilance control
        bands_[0].frequency = 5000.0f;
        bands_[0].q = 5.0f;
        bands_[0].threshold = -25.0f;
        bands_[0].ratio = 4.0f;
        bands_[0].attack = 1.0f;
        bands_[0].release = 50.0f;
        bands_[0].knee = 4.0f;
        bands_[0].makeup = 0.0f;

        // Air control
        bands_[1].frequency = 10000.0f;
        bands_[1].q = 2.0f;
        bands_[1].threshold = -20.0f;
        bands_[1].ratio = 2.0f;
        bands_[1].attack = 5.0f;
        bands_[1].release = 50.0f;
        bands_[1].knee = 6.0f;
        bands_[1].makeup = 0.0f;
    }
    else if (presetName == "Master Bus")
    {
        numBands_ = 4;

        // Sub control
        bands_[0].frequency = 60.0f;
        bands_[0].q = 2.0f;
        bands_[0].threshold = -3.0f;
        bands_[0].ratio = 2.0f;
        bands_[0].attack = 50.0f;
        bands_[0].release = 500.0f;
        bands_[0].knee = 6.0f;
        bands_[0].makeup = 0.0f;

        // Low-mids
        bands_[1].frequency = 250.0f;
        bands_[1].q = 1.5f;
        bands_[1].threshold = -6.0f;
        bands_[1].ratio = 2.0f;
        bands_[1].attack = 30.0f;
        bands_[1].release = 200.0f;
        bands_[1].knee = 6.0f;
        bands_[1].makeup = 0.0f;

        // High-mids
        bands_[2].frequency = 2500.0f;
        bands_[2].q = 1.5f;
        bands_[2].threshold = -8.0f;
        bands_[2].ratio = 2.0f;
        bands_[2].attack = 30.0f;
        bands_[2].release = 200.0f;
        bands_[2].knee = 6.0f;
        bands_[2].makeup = 0.0f;

        // Air
        bands_[3].frequency = 8000.0f;
        bands_[3].q = 1.0f;
        bands_[3].threshold = -10.0f;
        bands_[3].ratio = 1.5f;
        bands_[3].attack = 20.0f;
        bands_[3].release = 100.0f;
        bands_[3].knee = 6.0f;
        bands_[3].makeup = 0.0f;

        setBandsLinked(true);
    }

    updateAllFilters();
}

juce::StringArray DynamicEQ::getPresetNames()
{
    return {
        "De-Essing",
        "Bass Tamer",
        "Vocal Sibilance",
        "Master Bus"
    };
}

} // namespace effects
} // namespace zenith

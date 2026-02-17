/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#include "MultibandCompressor.h"
#include <algorithm>

namespace zenith {
namespace effects {

//==============================================================================
// MultibandCompressor Implementation
//==============================================================================

MultibandCompressor::MultibandCompressor()
{
    // Set default crossover frequencies
    // Band 0: 0-80 Hz (Sub)
    // Band 1: 80-2000 Hz (Low-mids)
    // Band 2: 2000-8000 Hz (High-mids)
    // Band 3: 8000+ Hz (Highs)
    crossovers_[0].frequency.store(80.0f);
    crossovers_[1].frequency.store(2000.0f);
    crossovers_[2].frequency.store(8000.0f);

    // Register parameters
    // Crossover frequencies
    addParameter({"crossover1", "Crossover 1", 20.0f, 20000.0f, 80.0f, true, 0.0f});
    addParameter({"crossover2", "Crossover 2", 20.0f, 20000.0f, 2000.0f, true, 0.0f});
    addParameter({"crossover3", "Crossover 3", 20.0f, 20000.0f, 8000.0f, true, 0.0f});

    // Per-band parameters
    for (int band = 0; band < numBands; ++band)
    {
        juce::String bandStr = juce::String(band + 1);
        addParameter({bandStr + "_threshold", "Band " + bandStr + " Threshold", -60.0f, 0.0f, -20.0f, true, 0.0f});
        addParameter({bandStr + "_ratio", "Band " + bandStr + " Ratio", 1.0f, 100.0f, 4.0f, true, 0.0f});
        addParameter({bandStr + "_attack", "Band " + bandStr + " Attack", 0.1f, 100.0f, 10.0f, true, 0.0f});
        addParameter({bandStr + "_release", "Band " + bandStr + " Release", 10.0f, 1000.0f, 100.0f, true, 0.0f});
        addParameter({bandStr + "_knee", "Band " + bandStr + " Knee", 0.0f, 24.0f, 6.0f, true, 0.0f});
        addParameter({bandStr + "_makeup", "Band " + bandStr + " Makeup", -20.0f, 20.0f, 0.0f, true, 0.0f});
    }

    // Global parameters
    addParameter({"wetDryMix", "Mix", 0.0f, 1.0f, 1.0f, true, 0.0f});
    addParameter({"outputGain", "Output", -20.0f, 20.0f, 0.0f, true, 0.0f});
    addParameter({"bandsLinked", "Link Bands", 0.0f, 1.0f, 0.0f, true, 0.0f});
}

MultibandCompressor::~MultibandCompressor()
{
}

//==============================================================================
void MultibandCompressor::prepare(double sampleRate, int maxSamplesPerBlock)
{
    sampleRate_ = sampleRate;
    maxSamplesPerBlock_ = maxSamplesPerBlock;

    // Prepare crossovers
    juce::dsp::ProcessSpec spec{sampleRate, static_cast<juce::uint32>(maxSamplesPerBlock), 2};

    for (int i = 0; i < numCrossovers; ++i)
    {
        crossovers_[i].lowpass.prepare(spec);
        crossovers_[i].highpass.prepare(spec);
        updateCrossover(i);
    }

    // Prepare compressors
    for (int band = 0; band < numBands; ++band)
    {
        bands_[band].compressor.prepare(spec);
        bands_[band].compressor.setThreshold(bands_[band].threshold.load());
        bands_[band].compressor.setRatio(bands_[band].ratio.load());
        updateCompressor(band);
    }

    // Prepare band buffers
    for (int band = 0; band < numBands; ++band)
    {
        bandBuffers_[band].setSize(2, maxSamplesPerBlock);
    }

    // Prepare dry buffer
    dryBuffer_.setSize(2, maxSamplesPerBlock);
}

void MultibandCompressor::reset()
{
    for (int i = 0; i < numCrossovers; ++i)
    {
        crossovers_[i].lowpass.reset();
        crossovers_[i].highpass.reset();
    }

    for (int band = 0; band < numBands; ++band)
    {
        bands_[band].compressor.reset();
        bands_[band].gainReduction = 0.0f;
        bands_[band].inputLevel = -100.0f;
        bands_[band].outputLevel = -100.0f;
    }
}

void MultibandCompressor::process(juce::AudioBuffer<float>& buffer,
                                  const juce::AudioBuffer<float>* sidechain)
{
    (void)sidechain;  // Not implemented yet

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Store dry signal
    dryBuffer_.makeCopyOf(buffer);

    // Clear band buffers
    for (int band = 0; band < numBands; ++band)
    {
        bandBuffers_[band].clear();
    }

    // Split into bands using crossover network
    // Band 0: Low (below crossover 0)
    // Band 1: Low-mid (crossover 0 to crossover 1)
    // Band 2: High-mid (crossover 1 to crossover 2)
    // Band 3: High (above crossover 2)

    // Start with input in band 0
    bandBuffers_[0].makeCopyOf(buffer);

    // Apply crossovers
    for (int crossover = 0; crossover < numCrossovers; ++crossover)
    {
        auto& lowpass = crossovers_[crossover].lowpass;
        auto& highpass = crossovers_[crossover].highpass;

        // Lowpass the lower band
        juce::dsp::AudioBlock<float> block(bandBuffers_[crossover]);
        juce::dsp::ProcessContextReplacing<float> context(block);
        lowpass.process(context);

        // Highpass to create next band
        bandBuffers_[crossover + 1].makeCopyOf(bandBuffers_[crossover]);
        juce::dsp::AudioBlock<float> blockNext(bandBuffers_[crossover + 1]);
        juce::dsp::ProcessContextReplacing<float> contextNext(blockNext);
        highpass.process(contextNext);
    }

    // Check if any bands are soloed
    bool anySolo = false;
    for (int band = 0; band < numBands; ++band)
    {
        if (bands_[band].solo.load())
        {
            anySolo = true;
            break;
        }
    }

    // Process each band
    juce::AudioBuffer<float> outputBuffer;
    outputBuffer.setSize(numChannels, numSamples);
    outputBuffer.clear();

    for (int band = 0; band < numBands; ++band)
    {
        auto& bandData = bands_[band];

        // Skip if muted (unless soloed)
        if (bandData.mute.load() && !anySolo)
            continue;

        // Skip if not soloed and another band is soloed
        if (anySolo && !bandData.solo.load())
            continue;

        // Skip if disabled
        if (!bandData.enabled.load())
            continue;

        auto& bandBuffer = bandBuffers_[band];

        // Calculate input level
        {
            float maxLevel = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* channelData = bandBuffer.getReadPointer(ch);
                for (int i = 0; i < numSamples; ++i)
                {
                    maxLevel = juce::jmax(maxLevel, std::abs(channelData[i]));
                }
            }
            bandData.inputLevel = levelToDecibels(maxLevel);
        }

        // Apply compression
        juce::dsp::AudioBlock<float> block(bandBuffer);
        juce::dsp::ProcessContextReplacing<float> context(block);

        if (bandsLinked_.load() && band > 0)
        {
            // Use band 0's gain reduction for all bands
            // (simplified implementation - could be more sophisticated)
            bandData.compressors[0].process(context);
        }
        else
        {
            bandData.compressor.process(context);
        }

        // Get gain reduction
        bandData.gainReduction = bandData.compressor.getAverageGainReduction();

        // Apply makeup gain
        float makeup = juce::Decibels::decibelsToGain(bandData.makeupGain.load());
        bandBuffer.applyGain(makeup);

        // Calculate output level
        {
            float maxLevel = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* channelData = bandBuffer.getReadPointer(ch);
                for (int i = 0; i < numSamples; ++i)
                {
                    maxLevel = juce::jmax(maxLevel, std::abs(channelData[i]));
                }
            }
            bandData.outputLevel = levelToDecibels(maxLevel);
        }

        // Add to output
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* out = outputBuffer.getWritePointer(ch);
            auto* bandChan = bandBuffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                out[i] += bandChan[i];
            }
        }
    }

    // Mix wet/dry
    float wet = wetDryMix_.load();
    float dry = 1.0f - wet;
    float wetGain = std::sqrt(wet);
    float dryGain = std::sqrt(dry);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* out = buffer.getWritePointer(ch);
        auto* wetChan = outputBuffer.getReadPointer(ch);
        auto* dryChan = dryBuffer_.getReadPointer(ch);

        for (int i = 0; i < numSamples; ++i)
        {
            out[i] = wetChan[i] * wetGain + dryChan[i] * dryGain;
        }
    }

    // Apply output gain
    float outputGain = juce::Decibels::decibelsToGain(outputGain_.load());
    buffer.applyGain(outputGain);
}

//==============================================================================
void MultibandCompressor::updateCrossover(int crossoverIndex)
{
    if (crossoverIndex < 0 || crossoverIndex >= numCrossovers)
        return;

    float freq = crossovers_[crossoverIndex].frequency.load();

    // Linkwitz-Riley 4th order = two cascaded 2nd order Butterworth filters
    *crossovers_[crossoverIndex].lowpass.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
        sampleRate_, freq).get();

    *crossovers_[crossoverIndex].highpass.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
        sampleRate_, freq).get();
}

void MultibandCompressor::updateCompressor(int bandIndex)
{
    if (bandIndex < 0 || bandIndex >= numBands)
        return;

    auto& band = bands_[bandIndex];

    band.compressor.setThreshold(band.threshold.load());
    band.compressor.setRatio(band.ratio.load());
    band.compressor.setAttack(band.attack.load() / 1000.0);  // ms to seconds
    band.compressor.setRelease(band.release.load() / 1000.0);
}

//==============================================================================
juce::ValueTree MultibandCompressor::getState() const
{
    juce::ValueTree state("MultibandCompressor");

    // Save crossovers
    for (int i = 0; i < numCrossovers; ++i)
    {
        state.setProperty("crossover" + juce::String(i + 1),
                         crossovers_[i].frequency.load(), nullptr);
    }

    // Save per-band settings
    for (int band = 0; band < numBands; ++band)
    {
        juce::String bandStr = "band" + juce::String(band);
        const auto& b = bands_[band];

        state.setProperty(bandStr + "_enabled", b.enabled.load(), nullptr);
        state.setProperty(bandStr + "_threshold", b.threshold.load(), nullptr);
        state.setProperty(bandStr + "_ratio", b.ratio.load(), nullptr);
        state.setProperty(bandStr + "_attack", b.attack.load(), nullptr);
        state.setProperty(bandStr + "_release", b.release.load(), nullptr);
        state.setProperty(bandStr + "_knee", b.knee.load(), nullptr);
        state.setProperty(bandStr + "_makeup", b.makeupGain.load(), nullptr);
    }

    // Save global settings
    state.setProperty("wetDryMix", wetDryMix_.load(), nullptr);
    state.setProperty("outputGain", outputGain_.load(), nullptr);
    state.setProperty("bandsLinked", bandsLinked_.load(), nullptr);

    return state;
}

void MultibandCompressor::setState(const juce::ValueTree& state)
{
    if (!state.isValid() || state.getType() != juce::String("MultibandCompressor"))
        return;

    // Load crossovers
    for (int i = 0; i < numCrossovers; ++i)
    {
        float freq = state.getProperty("crossover" + juce::String(i + 1),
                                       crossovers_[i].frequency.load());
        crossovers_[i].frequency.store(freq);
        updateCrossover(i);
    }

    // Load per-band settings
    for (int band = 0; band < numBands; ++band)
    {
        juce::String bandStr = "band" + juce::String(band);
        auto& b = bands_[band];

        b.enabled.store(state.getProperty(bandStr + "_enabled", true));
        b.threshold.store(state.getProperty(bandStr + "_threshold", -20.0f));
        b.ratio.store(state.getProperty(bandStr + "_ratio", 4.0f));
        b.attack.store(state.getProperty(bandStr + "_attack", 10.0f));
        b.release.store(state.getProperty(bandStr + "_release", 100.0f));
        b.knee.store(state.getProperty(bandStr + "_knee", 6.0f));
        b.makeupGain.store(state.getProperty(bandStr + "_makeup", 0.0f));

        updateCompressor(band);
    }

    // Load global settings
    wetDryMix_.store(state.getProperty("wetDryMix", 1.0f));
    outputGain_.store(state.getProperty("outputGain", 0.0f));
    bandsLinked_.store(state.getProperty("bandsLinked", false));
}

//==============================================================================
// Band controls
void MultibandCompressor::setCrossoverFrequency(int crossoverIndex, float frequencyHz)
{
    if (crossoverIndex >= 0 && crossoverIndex < numCrossovers)
    {
        crossovers_[crossoverIndex].frequency.store(
            juce::jlimit(20.0f, 20000.0f, frequencyHz));
        updateCrossover(crossoverIndex);
    }
}

float MultibandCompressor::getCrossoverFrequency(int crossoverIndex) const
{
    if (crossoverIndex >= 0 && crossoverIndex < numCrossovers)
        return crossovers_[crossoverIndex].frequency.load();
    return 0.0f;
}

void MultibandCompressor::setBandEnabled(int bandIndex, bool enabled)
{
    if (bandIndex >= 0 && bandIndex < numBands)
        bands_[bandIndex].enabled.store(enabled);
}

bool MultibandCompressor::isBandEnabled(int bandIndex) const
{
    if (bandIndex >= 0 && bandIndex < numBands)
        return bands_[bandIndex].enabled.load();
    return false;
}

void MultibandCompressor::setBandSolo(int bandIndex, bool solo)
{
    if (bandIndex >= 0 && bandIndex < numBands)
        bands_[bandIndex].solo.store(solo);
}

bool MultibandCompressor::isBandSolo(int bandIndex) const
{
    if (bandIndex >= 0 && bandIndex < numBands)
        return bands_[bandIndex].solo.load();
    return false;
}

void MultibandCompressor::setBandMute(int bandIndex, bool mute)
{
    if (bandIndex >= 0 && bandIndex < numBands)
        bands_[bandIndex].mute.store(mute);
}

bool MultibandCompressor::isBandMute(int bandIndex) const
{
    if (bandIndex >= 0 && bandIndex < numBands)
        return bands_[bandIndex].mute.load();
    return false;
}

void MultibandCompressor::setThreshold(int bandIndex, float dB)
{
    if (bandIndex >= 0 && bandIndex < numBands)
    {
        bands_[bandIndex].threshold.store(juce::jlimit(-60.0f, 0.0f, dB));
        updateCompressor(bandIndex);
    }
}

float MultibandCompressor::getThreshold(int bandIndex) const
{
    if (bandIndex >= 0 && bandIndex < numBands)
        return bands_[bandIndex].threshold.load();
    return 0.0f;
}

void MultibandCompressor::setRatio(int bandIndex, float ratio)
{
    if (bandIndex >= 0 && bandIndex < numBands)
    {
        bands_[bandIndex].ratio.store(juce::jmax(1.0f, ratio));
        updateCompressor(bandIndex);
    }
}

float MultibandCompressor::getRatio(int bandIndex) const
{
    if (bandIndex >= 0 && bandIndex < numBands)
        return bands_[bandIndex].ratio.load();
    return 1.0f;
}

void MultibandCompressor::setAttack(int bandIndex, float milliseconds)
{
    if (bandIndex >= 0 && bandIndex < numBands)
    {
        bands_[bandIndex].attack.store(juce::jlimit(0.1f, 100.0f, milliseconds));
        updateCompressor(bandIndex);
    }
}

float MultibandCompressor::getAttack(int bandIndex) const
{
    if (bandIndex >= 0 && bandIndex < numBands)
        return bands_[bandIndex].attack.load();
    return 10.0f;
}

void MultibandCompressor::setRelease(int bandIndex, float milliseconds)
{
    if (bandIndex >= 0 && bandIndex < numBands)
    {
        bands_[bandIndex].release.store(juce::jlimit(10.0f, 1000.0f, milliseconds));
        updateCompressor(bandIndex);
    }
}

float MultibandCompressor::getRelease(int bandIndex) const
{
    if (bandIndex >= 0 && bandIndex < numBands)
        return bands_[bandIndex].release.load();
    return 100.0f;
}

void MultibandCompressor::setKnee(int bandIndex, float dB)
{
    if (bandIndex >= 0 && bandIndex < numBands)
        bands_[bandIndex].knee.store(juce::jlimit(0.0f, 24.0f, dB));
}

float MultibandCompressor::getKnee(int bandIndex) const
{
    if (bandIndex >= 0 && bandIndex < numBands)
        return bands_[bandIndex].knee.load();
    return 0.0f;
}

void MultibandCompressor::setMakeupGain(int bandIndex, float dB)
{
    if (bandIndex >= 0 && bandIndex < numBands)
        bands_[bandIndex].makeupGain.store(juce::jlimit(-20.0f, 20.0f, dB));
}

float MultibandCompressor::getMakeupGain(int bandIndex) const
{
    if (bandIndex >= 0 && bandIndex < numBands)
        return bands_[bandIndex].makeupGain.load();
    return 0.0f;
}

void MultibandCompressor::setWetDryMix(float mix)
{
    wetDryMix_.store(juce::jlimit(0.0f, 1.0f, mix));
}

void MultibandCompressor::setOutputGain(float dB)
{
    outputGain_.store(juce::jlimit(-20.0f, 20.0f, dB));
}

void MultibandCompressor::setBandsLinked(bool linked)
{
    bandsLinked_.store(linked);
}

float MultibandCompressor::getGainReduction(int bandIndex) const
{
    if (bandIndex >= 0 && bandIndex < numBands)
        return bands_[bandIndex].gainReduction;
    return 0.0f;
}

float MultibandCompressor::getInputLevel(int bandIndex) const
{
    if (bandIndex >= 0 && bandIndex < numBands)
        return bands_[bandIndex].inputLevel;
    return -100.0f;
}

float MultibandCompressor::getOutputLevel(int bandIndex) const
{
    if (bandIndex >= 0 && bandIndex < numBands)
        return bands_[bandIndex].outputLevel;
    return -100.0f;
}

//==============================================================================
// Presets
void MultibandCompressor::loadPreset(const juce::String& presetName)
{
    if (presetName == "Vocal Gentle")
    {
        setCrossoverFrequency(0, 250.0f);
        setCrossoverFrequency(1, 2000.0f);
        setCrossoverFrequency(2, 8000.0f);

        setThreshold(0, -20.0f);  // Sub
        setRatio(0, 2.0f);

        setThreshold(1, -15.0f);  // Low-mids
        setRatio(1, 3.0f);

        setThreshold(2, -20.0f);  // High-mids
        setRatio(2, 2.5f);

        setThreshold(3, -25.0f);  // Highs
        setRatio(3, 2.0f);
    }
    else if (presetName == "Vocal Aggressive")
    {
        setCrossoverFrequency(0, 250.0f);
        setCrossoverFrequency(1, 3000.0f);
        setCrossoverFrequency(2, 8000.0f);

        setThreshold(0, -12.0f);
        setRatio(0, 4.0f);

        setThreshold(1, -10.0f);
        setRatio(1, 6.0f);

        setThreshold(2, -12.0f);
        setRatio(2, 5.0f);

        setThreshold(3, -15.0f);
        setRatio(3, 4.0f);
    }
    else if (presetName == "Drum Bus")
    {
        setCrossoverFrequency(0, 100.0f);
        setCrossoverFrequency(1, 1000.0f);
        setCrossoverFrequency(2, 5000.0f);

        setThreshold(0, -15.0f);  // Kick
        setRatio(0, 4.0f);

        setThreshold(1, -12.0f);  // Snare body
        setRatio(1, 5.0f);

        setThreshold(2, -15.0f);  // Snare crack
        setRatio(2, 4.0f);

        setThreshold(3, -18.0f);  // Cymbals
        setRatio(3, 3.0f);
    }
    else if (presetName == "Bass Control")
    {
        setCrossoverFrequency(0, 200.0f);
        setCrossoverFrequency(1, 2000.0f);
        setCrossoverFrequency(2, 8000.0f);

        setThreshold(0, -12.0f);  // Sub
        setRatio(0, 4.0f);

        setThreshold(1, -15.0f);  // Low-mids
        setRatio(1, 3.0f);

        setThreshold(2, -20.0f);
        setRatio(2, 2.0f);

        setThreshold(3, -20.0f);
        setRatio(3, 2.0f);
    }
    else if (presetName == "Master Bus")
    {
        setCrossoverFrequency(0, 80.0f);
        setCrossoverFrequency(1, 2000.0f);
        setCrossoverFrequency(2, 8000.0f);

        setThreshold(0, -6.0f);   // Subtle glue
        setRatio(0, 1.5f);
        setAttack(0, 30.0f);
        setRelease(0, 200.0f);

        setThreshold(1, -8.0f);
        setRatio(1, 1.5f);
        setAttack(1, 30.0f);
        setRelease(1, 200.0f);

        setThreshold(2, -10.0f);
        setRatio(2, 1.5f);
        setAttack(2, 30.0f);
        setRelease(2, 200.0f);

        setThreshold(3, -12.0f);
        setRatio(3, 1.5f);
        setAttack(3, 30.0f);
        setRelease(3, 200.0f);

        setBandsLinked(true);
        setWetDryMix(0.5f);  // Parallel compression
    }
}

juce::StringArray MultibandCompressor::getPresetNames()
{
    return {
        "Vocal Gentle",
        "Vocal Aggressive",
        "Drum Bus",
        "Bass Control",
        "Master Bus"
    };
}

} // namespace effects
} // namespace zenith

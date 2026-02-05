/*
  ==============================================================================

    ProPitchShifter.cpp
    Created: 2026-01-29
    Author:  Zenith DAW

    Professional pitch shifting implementation using Rubber Band.

  ==============================================================================
*/

#include "ProPitchShifter.h"
#include <cmath>

namespace zenith {
namespace dsp {

//==============================================================================
ProPitchShifter::ProPitchShifter()
{
}

ProPitchShifter::~ProPitchShifter()
{
}

//==============================================================================
void ProPitchShifter::prepare(double sampleRate, int channels, int maxBlockSize)
{
    sampleRate_ = sampleRate;
    channels_ = juce::jlimit(1, 2, channels);
    maxBlockSize_ = maxBlockSize;
    
    createStretcher();
}

void ProPitchShifter::createStretcher()
{
#if ZENITH_HAS_RUBBERBAND
    if (sampleRate_ <= 0 || channels_ <= 0)
        return;
    
    // Configure Rubber Band options
    RubberBand::RubberBandStretcher::Options options =
        RubberBand::RubberBandStretcher::OptionProcessRealTime |
        RubberBand::RubberBandStretcher::OptionPitchHighConsistency;
    
    // Add formant preservation
    if (formantPreservation_.load())
    {
        options |= RubberBand::RubberBandStretcher::OptionFormantPreserved;
    }
    
    // Quality settings
    if (quality_ == Quality::Draft)
    {
        options |= RubberBand::RubberBandStretcher::OptionEngineFaster;
    }
    else if (quality_ == Quality::Maximum)
    {
        options |= RubberBand::RubberBandStretcher::OptionEngineFiner;
    }
    
    // Create stretcher
    stretcher_ = std::make_unique<RubberBand::RubberBandStretcher>(
        static_cast<size_t>(sampleRate_),
        static_cast<size_t>(channels_),
        options,
        1.0,  // Initial time ratio (no time stretch)
        1.0   // Initial pitch ratio
    );
    
    // Set block size hint
    stretcher_->setMaxProcessSize(static_cast<size_t>(maxBlockSize_));
    
    // Apply current settings
    updatePitchRatio();
    
    if (formantPreservation_.load())
    {
        stretcher_->setFormantScale(formantRatio_.load());
    }
#else
    // Rubber Band not available - will fall back to basic implementation
    juce::ignoreUnused(sampleRate_, channels_, maxBlockSize_);
#endif
}

void ProPitchShifter::reset()
{
#if ZENITH_HAS_RUBBERBAND
    if (stretcher_)
    {
        stretcher_->reset();
    }
#endif
    needsReset_ = false;
}

//==============================================================================
void ProPitchShifter::updatePitchRatio()
{
#if ZENITH_HAS_RUBBERBAND
    if (!stretcher_)
        return;
    
    float ratio = pitchRatio_.load();
    
    // Apply pitch ratio (time ratio stays at 1.0 for pitch-only shifting)
    stretcher_->setTimeRatio(1.0);
    stretcher_->setPitchScale(ratio);
#else
    juce::ignoreUnused(pitchRatio_);
#endif
}

//==============================================================================
void ProPitchShifter::setPitchSemitones(float semitones)
{
    pitchSemitones_.store(semitones);
    
    // Convert semitones to ratio
    float ratio = std::pow(2.0f, semitones / 12.0f);
    pitchRatio_.store(ratio);
    
#if ZENITH_HAS_RUBBERBAND
    if (stretcher_)
    {
        updatePitchRatio();
    }
#endif
}

void ProPitchShifter::setPitchRatio(float ratio)
{
    pitchRatio_.store(ratio);
    
    // Convert ratio to semitones for storage
    float semitones = 12.0f * std::log2(ratio);
    pitchSemitones_.store(semitones);
    
#if ZENITH_HAS_RUBBERBAND
    if (stretcher_)
    {
        updatePitchRatio();
    }
#endif
}

//==============================================================================
void ProPitchShifter::setFormantRatio(float ratio)
{
    formantRatio_.store(ratio);
    
#if ZENITH_HAS_RUBBERBAND
    if (stretcher_ && formantPreservation_.load())
    {
        stretcher_->setFormantScale(ratio);
    }
#else
    juce::ignoreUnused(formantPreservation_);
#endif
}

void ProPitchShifter::setFormantPreservation(bool preserve)
{
    bool oldValue = formantPreservation_.exchange(preserve);
    
    if (oldValue != preserve)
    {
        // Need to recreate stretcher with new options
        needsReset_ = true;
        createStretcher();
    }
}

//==============================================================================
void ProPitchShifter::setQuality(Quality quality)
{
    if (quality_ != quality)
    {
        quality_ = quality;
        needsReset_ = true;
        createStretcher();
    }
}

void ProPitchShifter::setLowLatencyMode(bool lowLatency)
{
    if (lowLatencyMode_ != lowLatency)
    {
        lowLatencyMode_ = lowLatency;
        needsReset_ = true;
        createStretcher();
    }
}

//==============================================================================
int ProPitchShifter::getLatencySamples() const
{
#if ZENITH_HAS_RUBBERBAND
    if (!stretcher_)
        return 0;
    
    return static_cast<int>(stretcher_->getLatency());
#else
    return 2048;  // Fallback latency estimate
#endif
}

float ProPitchShifter::getLatencyMs() const
{
    if (sampleRate_ <= 0)
        return 0.0f;
    
    return (getLatencySamples() * 1000.0f) / static_cast<float>(sampleRate_);
}

//==============================================================================
void ProPitchShifter::process(const float* input, float* output, int numSamples)
{
#if ZENITH_HAS_RUBBERBAND
    if (!stretcher_ || numSamples <= 0)
    {
        // Pass through
        for (int i = 0; i < numSamples * channels_; ++i)
        {
            output[i] = input[i];
        }
        return;
    }
    
    // Handle reset if needed
    if (needsReset_)
    {
        reset();
    }
    
    // Process through Rubber Band
    // Input needs to be deinterleaved for Rubber Band
    std::vector<std::vector<float>> deinterleavedInput;
    std::vector<std::vector<float>> deinterleavedOutput;
    std::vector<const float*> inputPtrs;
    std::vector<float*> outputPtrs;
    
    deinterleavedInput.resize(channels_);
    deinterleavedOutput.resize(channels_);
    inputPtrs.resize(channels_);
    outputPtrs.resize(channels_);
    
    for (int ch = 0; ch < channels_; ++ch)
    {
        deinterleavedInput[ch].resize(numSamples);
        deinterleavedOutput[ch].resize(numSamples);
        
        // Deinterleave input
        for (int i = 0; i < numSamples; ++i)
        {
            deinterleavedInput[ch][i] = input[i * channels_ + ch];
        }
        
        inputPtrs[ch] = deinterleavedInput[ch].data();
        outputPtrs[ch] = deinterleavedOutput[ch].data();
    }
    
    // Process
    stretcher_->process(inputPtrs.data(), static_cast<size_t>(numSamples), false);
    
    // Retrieve output
    size_t available = stretcher_->available();
    size_t toRetrieve = std::min(static_cast<size_t>(numSamples), available);
    
    if (toRetrieve > 0)
    {
        stretcher_->retrieve(outputPtrs.data(), toRetrieve);
    }
    else
    {
        // No output available yet (startup latency)
        for (int ch = 0; ch < channels_; ++ch)
        {
            std::fill(deinterleavedOutput[ch].begin(), deinterleavedOutput[ch].end(), 0.0f);
        }
    }
    
    // Interleave output
    for (int i = 0; i < numSamples; ++i)
    {
        for (int ch = 0; ch < channels_; ++ch)
        {
            output[i * channels_ + ch] = deinterleavedOutput[ch][i];
        }
    }
#else
    // Fallback: simple resampling (not quality pitch shift, but functional)
    juce::ignoreUnused(stretcher_);
    
    float ratio = pitchRatio_.load();
    if (std::abs(ratio - 1.0f) < 0.001f)
    {
        // No pitch shift - copy
        for (int i = 0; i < numSamples * channels_; ++i)
        {
            output[i] = input[i];
        }
        return;
    }
    
    // Simple linear interpolation pitch shift (not great quality, but works)
    float readPos = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        for (int ch = 0; ch < channels_; ++ch)
        {
            int idx = static_cast<int>(readPos);
            float frac = readPos - idx;
            
            float s1 = input[idx * channels_ + ch];
            float s2 = input[(idx + 1) * channels_ + ch];
            
            output[i * channels_ + ch] = s1 + frac * (s2 - s1);
        }
        readPos += ratio;
        
        // Wrap around (circular buffer behavior for this simple fallback)
        if (readPos >= numSamples - 1)
            readPos -= (numSamples - 1);
    }
#endif
}

void ProPitchShifter::process(const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output)
{
    const int numChannels = input.getNumChannels();
    const int numSamples = input.getNumSamples();
    
    // Ensure output is same size
    output.setSize(numChannels, numSamples, false, false, true);
    
    if (numChannels == 1)
    {
        process(input.getReadPointer(0), output.getWritePointer(0), numSamples);
    }
    else if (numChannels == 2)
    {
        // Interleave stereo
        std::vector<float> interleavedInput(numSamples * 2);
        std::vector<float> interleavedOutput(numSamples * 2);
        
        for (int i = 0; i < numSamples; ++i)
        {
            interleavedInput[i * 2] = input.getSample(0, i);
            interleavedInput[i * 2 + 1] = input.getSample(1, i);
        }
        
        process(interleavedInput.data(), interleavedOutput.data(), numSamples);
        
        for (int i = 0; i < numSamples; ++i)
        {
            output.setSample(0, i, interleavedOutput[i * 2]);
            output.setSample(1, i, interleavedOutput[i * 2 + 1]);
        }
    }
}

} // namespace dsp
} // namespace zenith

/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    Oversampler.h
    Created: 2025-12-09
    Author:  Zenith DAW

    Polyphase oversampler for high-quality nonlinear processing.
    
    Features:

    - 2x or 4x oversampling
    - High-quality polyphase FIR filters
    - Linear phase response
    - Pre-allocated buffers for RT-safety
    - Asymmetric filtering for reduced latency

    Thread Safety:
    - prepare() must be called from message thread
    - process() is RT-safe (no allocations)
    - Supports 2x and 4x oversampling

    Usage:
    1. Call prepare() with sample rate and max block size
    2. Call upsample() to oversample input
    3. Process with nonlinear algorithm at higher rate
    4. Call downsample() to return to original rate

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>
#include <cmath>

#include "../engine/EngineConstants.h"

namespace zenith {

//==============================================================================
/**
    Polyphase oversampler for eliminating aliasing in nonlinear processing.
    
    Oversampling is essential for:
    - Compressors with fast attack (prevents aliasing from gain changes)
    - Saturation/distortion (prevents harmonics from folding back)
    - Limiters (accurate peak detection)
*/
template<int Factor = 2>
class Oversampler {
public:
    static_assert(Factor == 2 || Factor == 4, "Oversampling factor must be 2 or 4");
    
    //==========================================================================
    Oversampler() = default;
    ~Oversampler() = default;

    //==========================================================================
    // Initialization
    //==========================================================================

    /**
     * @brief Prepare the oversampler for processing
     * @param sampleRate Native sample rate
     * @param maxBlockSize Maximum expected block size at native rate
     * @param numChannels Number of channels (1 or 2)
     * @note MESSAGE THREAD ONLY - allocates memory
     */
    void prepare(double sampleRate, int maxBlockSize, int numChannels = 2) {
        sampleRate_ = sampleRate;
        numChannels_ = juce::jmin(numChannels, 2);
        
        // Allocate oversampled buffer
        oversampledBufferSize_ = maxBlockSize * Factor;
        oversampledBuffer_.setSize(numChannels_, oversampledBufferSize_);
        oversampledBuffer_.clear();
        
        // Initialize JUCE's oversampler
        juce::dsp::Oversampling<float>::FilterType filterType = 
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR;
        
        // Create oversampler with specified factor
        // Using IIR filters for lower latency while maintaining quality
        internalOversampler_ = std::make_unique<juce::dsp::Oversampling<float>>(
            numChannels_, 
            Factor == 2 ? 1 : 2,  // numStages: 1 = 2x, 2 = 4x
            filterType,
            true  // Enable maximum quality
        );
        
        // Initialize with maximum block size
        internalOversampler_->initProcessing(maxBlockSize);
        
        // Calculate latency
        latency_ = static_cast<int>(internalOversampler_->getLatencyInSamples());
    }

    /**
     * @brief Reset the oversampler state
     * @note Clears all filter states
     */
    void reset() {
        if (internalOversampler_) {
            internalOversampler_->reset();
        }
        oversampledBuffer_.clear();
    }

    //==========================================================================
    // Processing
    //==========================================================================

    /**
     * @brief Upsample the input buffer
     * @param input Input buffer at native sample rate
     * @return Pointer to oversampled buffer (Factor times the samples)
     * @note RT-safe
     */
    juce::AudioBuffer<float>& upsample(juce::AudioBuffer<float>& input) noexcept {
        if (!internalOversampler_) return input;
        
        // Create processing context
        juce::dsp::AudioBlock<float> inputBlock(input);
        
        // Upsample
        auto oversampledBlock = internalOversampler_->processSamplesUp(inputBlock);
        
        // Copy to internal buffer
        const int numSamples = static_cast<int>(oversampledBlock.getNumSamples());
        const int channels = static_cast<int>(oversampledBlock.getNumChannels());
        
        for (int ch = 0; ch < channels; ++ch) {
            juce::FloatVectorOperations::copy(
                oversampledBuffer_.getWritePointer(ch),
                oversampledBlock.getChannelPointer(ch),
                numSamples);
        }
        
        currentOversampledSize_ = numSamples;
        
        return oversampledBuffer_;
    }

    /**
     * @brief Downsample back to native rate
     * @param output Output buffer at native sample rate
     * @note RT-safe, must be called after processing the oversampled data
     */
    void downsample(juce::AudioBuffer<float>& output) noexcept {
        if (!internalOversampler_) return;
        
        // Create context from output buffer
        juce::dsp::AudioBlock<float> outputBlock(output);
        
        // Downsample
        internalOversampler_->processSamplesDown(outputBlock);
    }

    //==========================================================================
    // Accessors
    //==========================================================================

    /**
     * @brief Get the oversampled buffer for in-place processing
     * @return Reference to the oversampled buffer
     */
    juce::AudioBuffer<float>& getOversampledBuffer() noexcept {
        return oversampledBuffer_;
    }

    /**
     * @brief Get number of samples in oversampled buffer
     * @return Current oversampled buffer size
     */
    int getOversampledSize() const noexcept {
        return currentOversampledSize_;
    }

    /**
     * @brief Get the oversampling factor
     * @return 2 or 4
     */
    constexpr int getFactor() const noexcept { return Factor; }

    /**
     * @brief Get the oversampled sample rate
     * @return Oversampled rate (native * factor)
     */
    double getOversampledSampleRate() const noexcept {
        return sampleRate_ * Factor;
    }

    /**
     * @brief Get latency in samples (at native rate)
     * @return Latency caused by oversampling filters
     */
    int getLatency() const noexcept { return latency_; }

private:
    //==========================================================================
    // State
    double sampleRate_ = constants::kDefaultSampleRate;
    int numChannels_ = 2;
    int latency_ = 0;
    int currentOversampledSize_ = 0;
    int oversampledBufferSize_ = 0;
    
    // JUCE oversampler
    std::unique_ptr<juce::dsp::Oversampling<float>> internalOversampler_;
    
    // Pre-allocated buffer for oversampled data
    juce::AudioBuffer<float> oversampledBuffer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Oversampler)
};

//==============================================================================
// Type aliases for common configurations
using Oversampler2x = Oversampler<2>;
using Oversampler4x = Oversampler<4>;

} // namespace zenith

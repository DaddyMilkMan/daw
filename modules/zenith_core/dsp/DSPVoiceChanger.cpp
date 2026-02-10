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

#include "DSPVoiceChanger.h"
#include <algorithm>
#include <cstring>

namespace zenith {

//==============================================================================
// Constructor / Destructor
//==============================================================================

DSPVoiceChanger::DSPVoiceChanger() {
    // Allocate maximum buffer sizes upfront to avoid runtime allocation
    inputBuffer_.resize(kMaxBufferSamples, 0.0f);
    transitionBuffer_.resize(8192, 0.0f); // Max transition buffer (~170ms at 48kHz)
    
    // Resize block processing buffers
    tempInputBuffer_.resize(kProcessBlockSize, 0.0f);
    tempShiftedBuffer_.resize(kProcessBlockSize, 0.0f);
    tempOutputBuffer_.resize(kProcessBlockSize, 0.0f);
    scratchBuffer_.resize(kProcessBlockSize, 0.0f);
    scratchBuffer2_.resize(kProcessBlockSize, 0.0f);

    // Default formant envelopes to zero
    formantEnvelopes_.fill(0.0f);
}

DSPVoiceChanger::~DSPVoiceChanger() = default;

//==============================================================================
// Prepare / Reset
//==============================================================================

void DSPVoiceChanger::prepare(const juce::dsp::ProcessSpec& spec) {
    sampleRate_ = spec.sampleRate;
    
    // Resize full block buffer
    fullBlockBuffer_.resize(spec.maximumBlockSize, 0.0f);

    // Recalculate all WSOLA parameters for new sample rate
    recalculateWsolaParameters();
    
    // Initialize formant filters
    initFormantFilters();
    
    // Calculate envelope follower coefficients
    // Attack: ~5ms, Release: ~50ms
    const float attackTimeMs = 5.0f;
    const float releaseTimeMs = 50.0f;
    envelopeAttack_ = 1.0f - std::exp(-1.0f / (static_cast<float>(sampleRate_) * attackTimeMs / 1000.0f));
    envelopeRelease_ = 1.0f - std::exp(-1.0f / (static_cast<float>(sampleRate_) * releaseTimeMs / 1000.0f));
    
    // Calculate block-rate envelope coefficients
    float blockSizeSeconds = static_cast<float>(kProcessBlockSize) / static_cast<float>(sampleRate_);
    envelopeAttackBlock_ = 1.0f - std::exp(-blockSizeSeconds / (attackTimeMs / 1000.0f));
    envelopeReleaseBlock_ = 1.0f - std::exp(-blockSizeSeconds / (releaseTimeMs / 1000.0f));

    // Calculate transition fade increment
    const int transitionSamples = static_cast<int>(sampleRate_ * kTransitionFadeMs / 1000.0f);
    fadeIncrement_ = 1.0f / static_cast<float>(std::max(1, transitionSamples));
    
    reset();
}

void DSPVoiceChanger::reset() {
    // Clear input buffer
    std::fill(inputBuffer_.begin(), inputBuffer_.end(), 0.0f);
    inputWritePos_ = 0;
    inputReadPos_ = 0.0f;
    
    // Clear grain buffers
    if (!currentGrain_.empty()) {
        std::fill(currentGrain_.begin(), currentGrain_.end(), 0.0f);
    }
    if (!prevGrain_.empty()) {
        std::fill(prevGrain_.begin(), prevGrain_.end(), 0.0f);
    }
    if (!overlapBuffer_.empty()) {
        std::fill(overlapBuffer_.begin(), overlapBuffer_.end(), 0.0f);
    }
    
    overlapOutputPos_ = 0;
    currentPitchRatio_ = 1.0f;
    
    // Reset transition state
    transitionFade_ = 1.0f;
    isTransitioning_ = false;
    std::fill(transitionBuffer_.begin(), transitionBuffer_.end(), 0.0f);
    
    // Reset ring modulator
    ringModPhase_ = 0.0;
    
    // Reset formant envelopes
    formantEnvelopes_.fill(0.0f);
    
    // Reset filters
    for (auto& filter : analysisFilters_) {
        filter.reset();
    }
    for (auto& filter : synthesisFilters_) {
        filter.reset();
    }
    
    firstBlock_ = true;
}

//==============================================================================
// WSOLA Parameter Calculation
//==============================================================================

void DSPVoiceChanger::recalculateWsolaParameters() {
    // Convert window size from ms to samples
    windowSizeSamples_ = static_cast<int>(sampleRate_ * windowSizeMs_ / 1000.0f);
    
    // Ensure minimum window size (at least 64 samples for stability)
    windowSizeSamples_ = std::max(64, windowSizeSamples_);
    
    // Calculate hop size (output advancement per grain)
    hopSizeSamples_ = static_cast<int>(windowSizeSamples_ * (1.0f - kOverlapRatio));
    hopSizeSamples_ = std::max(1, hopSizeSamples_);
    
    // Calculate tolerance for WSOLA search
    toleranceSamples_ = static_cast<int>(sampleRate_ * kWsolaToleranceMs / 1000.0f);
    toleranceSamples_ = std::max(1, toleranceSamples_);
    
    // Resize grain buffers
    currentGrain_.resize(windowSizeSamples_, 0.0f);
    prevGrain_.resize(windowSizeSamples_, 0.0f);
    
    // Overlap buffer = 2x window for double buffering
    overlapBuffer_.resize(windowSizeSamples_ * 2, 0.0f);
    
    // Generate Hann window
    generateHannWindow(windowSizeSamples_);
}

void DSPVoiceChanger::setWindowSize(float windowSizeMs) {
    // Clamp to valid range
    windowSizeMs_ = std::clamp(windowSizeMs, kMinWindowSizeMs, kMaxWindowSizeMs);
    recalculateWsolaParameters();
}

void DSPVoiceChanger::generateHannWindow(int windowSize) {
    hannWindow_.resize(windowSize);
    
    const float pi = juce::MathConstants<float>::pi;
    const float denominator = static_cast<float>(windowSize - 1);
    
    for (int i = 0; i < windowSize; ++i) {
        hannWindow_[i] = 0.5f * (1.0f - std::cos(2.0f * pi * static_cast<float>(i) / denominator));
    }
}

//==============================================================================
// Formant Filter Initialization
//==============================================================================

void DSPVoiceChanger::initFormantFilters() {
    for (int i = 0; i < kFormantBandCount; ++i) {
        // Create bandpass filter coefficients for each formant
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(
            sampleRate_,
            kFormantFrequencies[i],
            kFormantQValues[i]
        );
        
        *analysisFilters_[i].coefficients = *coeffs;
        *synthesisFilters_[i].coefficients = *coeffs;
        
        analysisFilters_[i].reset();
        synthesisFilters_[i].reset();
    }
}

//==============================================================================
// Pitch Ratio Calculation
//==============================================================================

float DSPVoiceChanger::getPitchRatioForCharacter(VoiceCharacter character) const {
    switch (character) {
        case VoiceCharacter::DeepMale:
            return semitonesToRatio(kDeepMaleSemitones);
        case VoiceCharacter::Chipmunk:
            return semitonesToRatio(kChipmunkSemitones);
        case VoiceCharacter::Robot:
        case VoiceCharacter::Ethereal:
        default:
            return 1.0f; // No pitch shift
    }
}

//==============================================================================
// Character Transition Handling
//==============================================================================

void DSPVoiceChanger::handleCharacterTransition(VoiceCharacter newCharacter) {
    if (firstBlock_) {
        prevCharacter_ = newCharacter;
        isTransitioning_ = false;
        firstBlock_ = false;
        return;
    }
    
    if (newCharacter != prevCharacter_) {
        // Store current state for crossfade (handled in process loop)
        isTransitioning_ = true;
        transitionFade_ = 0.0f;
        prevCharacter_ = newCharacter;
    }
}

//==============================================================================
// WSOLA Core Algorithm
//==============================================================================

void DSPVoiceChanger::extractGrain(float startPos) {
    const int bufferSize = static_cast<int>(inputBuffer_.size());
    
    for (int i = 0; i < windowSizeSamples_; ++i) {
        // Calculate read position with fractional interpolation
        float readPos = startPos + static_cast<float>(i);
        
        // Wrap position
        while (readPos >= bufferSize) readPos -= bufferSize;
        while (readPos < 0) readPos += bufferSize;
        
        // Linear interpolation
        int index0 = static_cast<int>(readPos);
        int index1 = (index0 + 1) % bufferSize;
        float frac = readPos - static_cast<float>(index0);
        
        float sample = inputBuffer_[index0] * (1.0f - frac) + inputBuffer_[index1] * frac;
        
        // Apply Hann window
        currentGrain_[i] = sample * hannWindow_[i];
    }
}

int DSPVoiceChanger::findBestMatchPosition(int targetPos) {
    const int bufferSize = static_cast<int>(inputBuffer_.size());
    
    // For pitch ratios close to 1.0, skip search
    if (std::abs(currentPitchRatio_ - 1.0f) < 0.01f) {
        return targetPos;
    }
    
    int bestPos = targetPos;
    float bestCorrelation = -2.0f;
    
    // Search window: ±tolerance samples around target
    const int searchStart = targetPos - toleranceSamples_;
    const int searchEnd = targetPos + toleranceSamples_;
    
    // Use correlation length of 1/8 window for efficiency
    const int correlationLength = windowSizeSamples_ / 8;
    
    for (int pos = searchStart; pos <= searchEnd; pos += 2) { // Step by 2 for efficiency
        int wrappedPos = pos;
        while (wrappedPos < 0) wrappedPos += bufferSize;
        while (wrappedPos >= bufferSize) wrappedPos -= bufferSize;
        
        float correlation = calculateCrossCorrelation(
            static_cast<int>(inputReadPos_) % bufferSize,
            wrappedPos,
            correlationLength
        );
        
        if (correlation > bestCorrelation) {
            bestCorrelation = correlation;
            bestPos = wrappedPos;
        }
    }
    
    return bestPos;
}

float DSPVoiceChanger::calculateCrossCorrelation(int pos1, int pos2, int length) {
    const int bufferSize = static_cast<int>(inputBuffer_.size());
    
    float sum = 0.0f;
    float norm1 = 0.0f;
    float norm2 = 0.0f;
    
    for (int i = 0; i < length; ++i) {
        int idx1 = (pos1 + i) % bufferSize;
        int idx2 = (pos2 + i) % bufferSize;
        
        float s1 = inputBuffer_[idx1];
        float s2 = inputBuffer_[idx2];
        
        sum += s1 * s2;
        norm1 += s1 * s1;
        norm2 += s2 * s2;
    }
    
    const float denominator = std::sqrt(norm1 * norm2);
    if (denominator < 1e-10f) {
        return 0.0f;
    }
    
    return sum / denominator;
}

float DSPVoiceChanger::processPitchShift(float input) {
    const int bufferSize = static_cast<int>(inputBuffer_.size());
    
    // Write input to circular buffer
    inputBuffer_[inputWritePos_] = input;
    inputWritePos_ = (inputWritePos_ + 1) % bufferSize;
    
    // Check if we need to extract a new grain
    if (overlapOutputPos_ >= hopSizeSamples_) {
        // Store previous grain
        std::copy(currentGrain_.begin(), currentGrain_.end(), prevGrain_.begin());
        
        // Calculate expected analysis position
        float analysisHop = static_cast<float>(hopSizeSamples_) / currentPitchRatio_;
        float targetReadPos = inputReadPos_ + analysisHop;
        
        // Wrap target position
        while (targetReadPos >= bufferSize) targetReadPos -= bufferSize;
        while (targetReadPos < 0) targetReadPos += bufferSize;
        
        // Find best match using WSOLA
        int bestPos = findBestMatchPosition(static_cast<int>(targetReadPos));
        inputReadPos_ = static_cast<float>(bestPos);
        
        // Extract new grain
        extractGrain(inputReadPos_);
        
        // Reset overlap position
        overlapOutputPos_ = 0;
        
        // Clear the portion of overlap buffer we're about to use
        for (int i = 0; i < hopSizeSamples_ && i < static_cast<int>(overlapBuffer_.size()); ++i) {
            overlapBuffer_[i] = 0.0f;
        }
    }
    
    // Overlap-add: combine current and previous grains
    float output = 0.0f;
    
    // Contribution from current grain
    if (overlapOutputPos_ < windowSizeSamples_) {
        output += currentGrain_[overlapOutputPos_];
    }
    
    // Contribution from previous grain (offset by hop size)
    int prevGrainIdx = overlapOutputPos_ + hopSizeSamples_;
    if (prevGrainIdx < windowSizeSamples_) {
        output += prevGrain_[prevGrainIdx];
    }
    
    overlapOutputPos_++;
    
    return output;
}

//==============================================================================
// Formant Preservation
//==============================================================================

float DSPVoiceChanger::processFormantPreservation(float input, float pitchShifted) {
    // Legacy single-sample implementation (kept for reference or fallback if needed)
    return input;
}

void DSPVoiceChanger::processFormantPreservationBlock(int numSamples) {
    if (numSamples <= 0) return;

    // Clear output buffer
    std::fill(tempOutputBuffer_.begin(), tempOutputBuffer_.begin() + numSamples, 0.0f);

    // Process input (Original Signal) Analysis
    for (int i = 0; i < kFormantBandCount; ++i) {
        // Copy input to scratch
        std::copy(tempInputBuffer_.begin(), tempInputBuffer_.begin() + numSamples, scratchBuffer_.begin());

        // Filter input block
        float* channelData[] = { scratchBuffer_.data() };
        juce::dsp::AudioBlock<float> block(channelData, 1, static_cast<size_t>(numSamples));
        juce::dsp::ProcessContextReplacing<float> context(block);
        analysisFilters_[i].process(context);
        
        // Find peak amplitude in block
        float maxAmplitude = 0.0f;
        auto range = juce::FloatVectorOperations::findMinAndMax(scratchBuffer_.data(), numSamples);
        maxAmplitude = std::max(std::abs(range.getStart()), std::abs(range.getEnd()));

        // Update envelope follower (once per block)
        if (maxAmplitude > formantEnvelopes_[i]) {
            formantEnvelopes_[i] += envelopeAttackBlock_ * (maxAmplitude - formantEnvelopes_[i]);
        } else {
            formantEnvelopes_[i] += envelopeReleaseBlock_ * (maxAmplitude - formantEnvelopes_[i]);
        }
        
        float currentEnvelope = formantEnvelopes_[i];

        // Process pitch-shifted (Synthesis Signal)
        
        // Copy shifted to scratch2
        std::copy(tempShiftedBuffer_.begin(), tempShiftedBuffer_.begin() + numSamples, scratchBuffer2_.begin());
        
        // Filter shifted block
        float* channelDataShifted[] = { scratchBuffer2_.data() };
        juce::dsp::AudioBlock<float> blockShifted(channelDataShifted, 1, static_cast<size_t>(numSamples));
        juce::dsp::ProcessContextReplacing<float> contextShifted(blockShifted);
        synthesisFilters_[i].process(contextShifted);

        // Find peak amplitude in shifted block
        float maxShiftedAmplitude = 0.0f;
        auto rangeShifted = juce::FloatVectorOperations::findMinAndMax(scratchBuffer2_.data(), numSamples);
        maxShiftedAmplitude = std::max(std::abs(rangeShifted.getStart()), std::abs(rangeShifted.getEnd()));
        maxShiftedAmplitude += 1e-10f; // Avoid division by zero

        // Calculate correction gain
        float correctionGain = currentEnvelope / maxShiftedAmplitude;
        correctionGain = std::clamp(correctionGain, 0.1f, 10.0f);

        // Apply gain to filtered shifted signal
        juce::FloatVectorOperations::multiply(scratchBuffer2_.data(), correctionGain, numSamples);

        // Accumulate to output
        juce::FloatVectorOperations::add(tempOutputBuffer_.data(), scratchBuffer2_.data(), numSamples);
    }
    
    // Mix
    const float formantMix = 0.5f;
    float invBandCount = 1.0f / static_cast<float>(kFormantBandCount);

    for (int n = 0; n < numSamples; ++n) {
        float corrected = tempOutputBuffer_[n] * invBandCount;
        float original = tempShiftedBuffer_[n];
        tempOutputBuffer_[n] = original * (1.0f - formantMix) + corrected * formantMix;
    }
}

//==============================================================================
// Main Processing
//==============================================================================

void DSPVoiceChanger::process(const juce::dsp::AudioBlock<const float>& inputBlock,
                              juce::dsp::AudioBlock<float>& outputBlock,
                              VoiceCharacter character) {
    const auto numSamples = static_cast<size_t>(inputBlock.getNumSamples());
    const auto numChannels = std::min(inputBlock.getNumChannels(), outputBlock.getNumChannels());
    
    if (numSamples == 0 || numChannels == 0) {
        return;
    }
    
    // Resize internal buffer if needed (safety check, should be done in prepare)
    if (fullBlockBuffer_.size() < numSamples) {
        fullBlockBuffer_.resize(numSamples);
    }

    // Handle character transition
    handleCharacterTransition(character);
    
    // Get pitch ratio for character
    currentPitchRatio_ = getPitchRatioForCharacter(character);
    
    // Get ring mod frequency (only for Robot)
    const bool isRobot = (character == VoiceCharacter::Robot);
    const double ringModIncrement = isRobot ? (kRobotRingModFreqHz / sampleRate_) : 0.0;
    
    // Process mono (left channel) then copy to right
    const float* inL = inputBlock.getChannelPointer(0);
    float* outL = outputBlock.getChannelPointer(0);
    
    // Phase 1: Generate Raw Pitch Shifted / Effect Signal
    for (size_t i = 0; i < numSamples; ++i) {
        float input = inL[i];
        float output = 0.0f;
        
        if (isRobot) {
            // Robot mode: Ring modulation
            float modulator = static_cast<float>(std::sin(ringModPhase_ * juce::MathConstants<double>::twoPi));
            output = input * modulator;
            
            ringModPhase_ += ringModIncrement;
            if (ringModPhase_ >= 1.0) {
                ringModPhase_ -= 1.0;
            }
        } else if (character == VoiceCharacter::Ethereal) {
            // Ethereal: Pass through (reverb handled by separate effect)
            output = input;
        } else {
            // Pitch shifting (DeepMale, Chipmunk)
            output = processPitchShift(input);
        }

        fullBlockBuffer_[i] = output;
    }

    // Phase 2: Formant Preservation (Block Processing)
    if (formantPreservation_ && character != VoiceCharacter::Ethereal) {
        // Process in chunks of kProcessBlockSize
        size_t samplesProcessed = 0;

        while (samplesProcessed < numSamples) {
            const int chunkSize = std::min(kProcessBlockSize, static_cast<int>(numSamples - samplesProcessed));
            
            // Copy chunk from input
            for (int k = 0; k < chunkSize; ++k) {
                tempInputBuffer_[k] = inL[samplesProcessed + k];
            }

            // Copy chunk from shifted (fullBlockBuffer_)
            for (int k = 0; k < chunkSize; ++k) {
                tempShiftedBuffer_[k] = fullBlockBuffer_[samplesProcessed + k];
            }

            // Process chunk
            processFormantPreservationBlock(chunkSize);

            // Write chunk back to fullBlockBuffer_
            for (int k = 0; k < chunkSize; ++k) {
                fullBlockBuffer_[samplesProcessed + k] = tempOutputBuffer_[k];
            }

            samplesProcessed += chunkSize;
        }
    }

    // Phase 3: Transition Logic and Output
    for (size_t i = 0; i < numSamples; ++i) {
        float output = fullBlockBuffer_[i];
        
        // Handle transition crossfade
        if (isTransitioning_) {
            // Store output for next transition
            if (i < transitionBuffer_.size()) {
                float prevOutput = transitionBuffer_[i];
                output = prevOutput * (1.0f - transitionFade_) + output * transitionFade_;
                transitionBuffer_[i] = output;
            }
            
            transitionFade_ += fadeIncrement_;
            if (transitionFade_ >= 1.0f) {
                transitionFade_ = 1.0f;
                isTransitioning_ = false;
            }
        }
        
        outL[i] = output;
    }
    
    // Copy to additional channels (stereo)
    if (numChannels > 1) {
        float* outR = outputBlock.getChannelPointer(1);
        std::memcpy(outR, outL, numSamples * sizeof(float));
    }
    
    // Copy to any remaining channels
    for (size_t ch = 2; ch < numChannels; ++ch) {
        float* outCh = outputBlock.getChannelPointer(ch);
        std::memcpy(outCh, outL, numSamples * sizeof(float));
    }
}

} // namespace zenith

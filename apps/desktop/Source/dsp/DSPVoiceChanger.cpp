/*
  ==============================================================================

    DSPVoiceChanger.cpp
    Created: 2025-11-29
    Author:  Zenith DAW - Efficient C++ Team

  ==============================================================================
*/

#include "DSPVoiceChanger.h"

namespace zenith {

DSPVoiceChanger::DSPVoiceChanger() {
    delayBuffer_.resize(192000); // ~4 seconds buffer
}

DSPVoiceChanger::~DSPVoiceChanger() {}

void DSPVoiceChanger::prepare(const juce::dsp::ProcessSpec& spec) {
    sampleRate_ = spec.sampleRate;
    formantFilters_.prepare(spec);
    reset();
}

void DSPVoiceChanger::reset() {
    std::fill(delayBuffer_.begin(), delayBuffer_.end(), 0.0f);
    writePos_ = 0;
    readPos_ = 0.0f;
    ringModPhase_ = 0.0;
    formantFilters_.reset();
}

void DSPVoiceChanger::process(const juce::dsp::AudioBlock<const float>& inputBlock,
                              juce::dsp::AudioBlock<float>& outputBlock,
                              VoiceCharacter character) {
    auto numSamples = inputBlock.getNumSamples();
    auto* inL = inputBlock.getChannelPointer(0);
    auto* outL = outputBlock.getChannelPointer(0);
    
    // Simple Pitch Shift Factor
    float pitchRatio = 1.0f;
    float ringModFreq = 0.0f;
    
    switch (character) {
        case VoiceCharacter::DeepMale: pitchRatio = 0.75f; break; // -5 semitones
        case VoiceCharacter::Chipmunk: pitchRatio = 1.5f; break;  // +7 semitones
        case VoiceCharacter::Robot:    pitchRatio = 1.0f; ringModFreq = 50.0f; break;
        case VoiceCharacter::Ethereal: pitchRatio = 1.0f; break; // Reverb handled elsewhere
    }

    // Process loop
    for (int i = 0; i < numSamples; ++i) {
        float input = inL[i];
        
        // Write to circular buffer
        delayBuffer_[writePos_] = input;
        
        // Read from buffer (Pitch Shift)
        // Simple granular-style read pointer movement
        float output = 0.0f;
        
        if (character == VoiceCharacter::Robot) {
            // Ring Modulator
            float modulator = std::sin(ringModPhase_ * juce::MathConstants<double>::twoPi);
            output = input * modulator;
            ringModPhase_ += ringModFreq / sampleRate_;
            if (ringModPhase_ >= 1.0) ringModPhase_ -= 1.0;
        } else {
            // Pitch Shifting (Basic resampling)
            int readIndex = (int)readPos_;
            float frac = readPos_ - readIndex;
            
            // Linear interpolation
            float s1 = delayBuffer_[readIndex % delayBuffer_.size()];
            float s2 = delayBuffer_[(readIndex + 1) % delayBuffer_.size()];
            output = s1 + frac * (s2 - s1);
            
            readPos_ += pitchRatio;
            if (readPos_ >= delayBuffer_.size()) readPos_ -= delayBuffer_.size();
            
            // Sync read/write pointers to avoid drift
            // Optimization: If pitch ratio is 1.0 (no shift), lock the read pointer relative to write pointer
            // to avoid any drift or interpolation artifacts.
            if (std::abs(pitchRatio - 1.0f) < 0.0001f) {
                 float targetReadPos = (float)writePos_ - 2000.0f; // Maintain constant delay
                 if (targetReadPos < 0) targetReadPos += delayBuffer_.size();
                 readPos_ = targetReadPos;
            } else {
                // Pitch shifting active - existing drift correction
                float dist = writePos_ - readPos_;
                if (dist < 0) dist += delayBuffer_.size();
                if (dist < 100 || dist > delayBuffer_.size() - 100) {
                    readPos_ = writePos_ - 2000; // Jump back
                    if (readPos_ < 0) readPos_ += delayBuffer_.size();
                }
            }
        }

        outL[i] = output;
        
        writePos_++;
        if (writePos_ >= delayBuffer_.size()) writePos_ = 0;
    }
    
    // Copy to Right channel
    if (outputBlock.getNumChannels() > 1) {
        auto* outR = outputBlock.getChannelPointer(1);
        for (int i = 0; i < numSamples; ++i) {
            outR[i] = outL[i];
        }
    }
}

} // namespace zenith

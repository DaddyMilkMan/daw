/*
  ==============================================================================

    ZenithEffects.cpp
    Created: 2025-12-06
    Author:  Zenith DAW

    Implementation of ZenithEffects.

  ==============================================================================
*/

#include "ZenithEffects.h"
#include <cmath>

namespace zenith {

void ZenithEffects::initReverb() {
    if (reverbInit_) return;
    // Tunings for 44.1kHz (scaled by SR in setSampleRate assumed, or here dynamic)
    // Standard Schroeder/Moorer values
    float scale = static_cast<float>(sampleRate_) / 44100.0f;
    
    int tunings[] = { 1116, 1188, 1277, 1356 }; 
    for (int i=0; i<4; ++i) combs_[i].resize(static_cast<int>(tunings[i] * scale));
    
    int allpassTunings[] = { 225, 556 };
    for (int i=0; i<2; ++i) allpasses_[i].resize(static_cast<int>(allpassTunings[i] * scale));
    
    reverbInit_ = true;
}

void ZenithEffects::process(float &left, float &right) {
    // Distortion
    if (distortionAmount_ > 0.0f) {
        float drive = 1.0f + distortionAmount_ * 9.0f;
        left = std::tanh(left * drive) / drive;
        right = std::tanh(right * drive) / drive;
    }
    
    // Chorus
    if (chorusAmount_ > 0.0f) {
        float lfoValue = std::sin(chorusPhase_ * juce::MathConstants<float>::twoPi);
        int delayTime = static_cast<int>(5.0f + lfoValue * 3.0f);
        
        int readPos = (delayPos_ - delayTime + delayBufferL_.size()) % delayBufferL_.size();
        float chorusL = delayBufferL_[readPos];
        float chorusR = delayBufferR_[readPos];
        
        left = left * (1.0f - chorusAmount_) + chorusL * chorusAmount_;
        right = right * (1.0f - chorusAmount_) + chorusR * chorusAmount_;
        
        delayBufferL_[delayPos_] = left;
        delayBufferR_[delayPos_] = right;
        delayPos_ = (delayPos_ + 1) % delayBufferL_.size();
        
        chorusPhase_ += 2.0f / static_cast<float>(sampleRate_);
        if (chorusPhase_ >= 1.0f) chorusPhase_ -= 1.0f;
    }

    // Reverb
    if (reverbAmount_ > 0.0f) {
        initReverb();
        
        // Mix left and right for reverb input (mono in)
        float input = (left + right) * 0.5f * reverbAmount_;
        
        // Parallel comb filters
        float combOut = 0.0f;
        for (auto& comb : combs_) {
            combOut += comb.process(input);
        }
        
        // Serial allpass filters
        float allpassOut = combOut;
        for (auto& allpass : allpasses_) {
            allpassOut = allpass.process(allpassOut);
        }
        
        // Wet/Dry mix
        left += allpassOut * 0.2f; // Scale reverb tail
        right += allpassOut * 0.2f;
    }
}

} // namespace zenith

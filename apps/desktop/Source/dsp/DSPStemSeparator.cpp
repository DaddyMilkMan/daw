/*
  ==============================================================================

    DSPStemSeparator.cpp
    Created: 2025-11-29
    Author:  Zenith DAW - Efficient C++ Team

    Real-time stem separation using Mid-Side processing and Linkwitz-Riley filters.
    This is a lightweight, zero-latency alternative to heavy AI models.

  ==============================================================================
*/

#include "DSPStemSeparator.h"
#include <vector>
#include <cmath>
#include <algorithm>

namespace zenith {

DSPStemSeparator::DSPStemSeparator() {
    reset();
}

DSPStemSeparator::~DSPStemSeparator() {}

void DSPStemSeparator::prepare(const juce::dsp::ProcessSpec& spec) {
    sampleRate_ = spec.sampleRate;
    
    // Setup Linkwitz-Riley split filters
    auto lpCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate_, 200.0f);
    auto hpCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate_, 200.0f);

    // Apply coefficients to filter chains (4 biquads = 8th order, but we use them in pairs usually)
    lowPassFilter_.get<0>().coefficients = lpCoeffs;
    lowPassFilter_.get<1>().coefficients = lpCoeffs;
    lowPassFilter_.get<2>().coefficients = lpCoeffs;
    lowPassFilter_.get<3>().coefficients = lpCoeffs;
    
    highPassFilter_.get<0>().coefficients = hpCoeffs;
    highPassFilter_.get<1>().coefficients = hpCoeffs;
    highPassFilter_.get<2>().coefficients = hpCoeffs;
    highPassFilter_.get<3>().coefficients = hpCoeffs;

    juce::dsp::ProcessSpec monoSpec = spec;
    monoSpec.numChannels = 1;
    
    lowPassFilter_.prepare(monoSpec);
    highPassFilter_.prepare(monoSpec);
}

void DSPStemSeparator::reset() {
    lowPassFilter_.reset();
    highPassFilter_.reset();
}

void DSPStemSeparator::computeMidSide(const float* left, const float* right, int numSamples, 
                                     std::vector<float>& mid, std::vector<float>& side) {
    mid.resize(static_cast<size_t>(numSamples));
    side.resize(static_cast<size_t>(numSamples));

    for (int i = 0; i < numSamples; ++i) {
        // Mid = (L + R) / 2  -> Center content (usually vocals, bass, kick)
        mid[static_cast<size_t>(i)] = (left[i] + right[i]) * 0.5f;
        // Side = (L - R) / 2 -> Stereo width content (usually guitars, pads, reverb)
        side[static_cast<size_t>(i)] = (left[i] - right[i]) * 0.5f;
    }
}

void DSPStemSeparator::process(const juce::dsp::AudioBlock<const float>& inputBlock,
                               juce::dsp::AudioBlock<float>& outputBlock,
                               StemType stemType) {
    const int numSamples = static_cast<int>(inputBlock.getNumSamples());
    const size_t numChannels = inputBlock.getNumChannels();
    
    if (numSamples == 0 || numChannels < 2) {
        outputBlock.clear();
        return;
    }

    const float* leftIn = inputBlock.getChannelPointer(0);
    const float* rightIn = inputBlock.getChannelPointer(1);
    float* leftOut = outputBlock.getChannelPointer(0);
    float* rightOut = outputBlock.getChannelPointer(1);

    // Compute Mid-Side representation
    std::vector<float> mid, side;
    computeMidSide(leftIn, rightIn, numSamples, mid, side);

    // Create temporary buffers for filtering
    std::vector<float> lowFreq(static_cast<size_t>(numSamples));
    std::vector<float> highFreq(static_cast<size_t>(numSamples));

    switch (stemType) {
        case StemType::Bass: {
            // Bass: Low frequencies from the Mid channel (< 200Hz)
            // Bass is typically mono and centered
            
            // Copy mid to temp buffer for filtering
            std::copy(mid.begin(), mid.end(), lowFreq.begin());
            
            // Apply 4th order Linkwitz-Riley lowpass
            // Apply 4th order Linkwitz-Riley lowpass
            float* lowPtr = &lowFreq[0];
            float* channels[] = { lowPtr };
            juce::dsp::AudioBlock<float> lowBlock(channels, 1, static_cast<size_t>(numSamples));
            juce::dsp::ProcessContextReplacing<float> lowContext(lowBlock);
            lowPassFilter_.process(lowContext);
            
            // Output as stereo (bass is centered, so L=R)
            for (int i = 0; i < numSamples; ++i) {
                leftOut[i] = lowFreq[static_cast<size_t>(i)];
                rightOut[i] = lowFreq[static_cast<size_t>(i)];
            }
            break;
        }
        
        case StemType::Vocals: {
            // Vocals: Mid channel with bass removed (typically 200Hz - 8kHz centered)
            // Vocals are usually centered (mono) and in the mid-high frequency range
            
            // Copy mid to temp buffer for highpass filtering
            std::copy(mid.begin(), mid.end(), highFreq.begin());
            
            // Apply 4th order Linkwitz-Riley highpass (removes bass frequencies)
            // Apply 4th order Linkwitz-Riley highpass (removes bass frequencies)
            float* highPtr = &highFreq[0];
            float* channels[] = { highPtr };
            juce::dsp::AudioBlock<float> highBlock(channels, 1, static_cast<size_t>(numSamples));
            juce::dsp::ProcessContextReplacing<float> highContext(highBlock);
            highPassFilter_.process(highContext);
            
            // Apply a gentle bandpass to focus on vocal range (reduce very high frequencies)
            // Simple 6dB/octave rolloff above 8kHz simulated with a smoothing average
            float prevSample = 0.0f;
            const float smoothCoeff = 0.3f; // Gentle high-frequency attenuation
            for (int i = 0; i < numSamples; ++i) {
                float sample = highFreq[static_cast<size_t>(i)];
                sample = prevSample + smoothCoeff * (sample - prevSample);
                prevSample = sample;
                
                // Output as stereo (vocals are centered)
                leftOut[i] = sample;
                rightOut[i] = sample;
            }
            break;
        }
        
        case StemType::Drums: {
            // Drums: Combination of low frequencies (kick) + transient detection (snare, hats)
            // Drums have strong transients and occupy both low and high frequencies
            
            // Get low frequencies from mid (for kick drum)
            std::copy(mid.begin(), mid.end(), lowFreq.begin());
            float* lowPtr = &lowFreq[0];
            float* channels[] = { lowPtr };
            juce::dsp::AudioBlock<float> lowBlock(channels, 1, static_cast<size_t>(numSamples));
            juce::dsp::ProcessContextReplacing<float> lowContext(lowBlock);
            lowPassFilter_.process(lowContext);
            
            // For high-frequency drums (snare, hats), use side channel + high frequencies
            // This exploits the fact that overheads/room mics capture stereo drum content
            std::copy(side.begin(), side.end(), highFreq.begin());
            
            // Add high frequencies from mid (for centered snare attack)
            std::vector<float> midHigh(static_cast<size_t>(numSamples));
            std::copy(mid.begin(), mid.end(), midHigh.begin());
            float* midHighPtr = &midHigh[0];
            float* channelsHigh[] = { midHighPtr };
            juce::dsp::AudioBlock<float> midHighBlock(channelsHigh, 1, static_cast<size_t>(numSamples));
            juce::dsp::ProcessContextReplacing<float> midHighContext(midHighBlock);
            highPassFilter_.process(midHighContext);
            
            // Simple transient enhancement: emphasize attack by computing envelope derivative
            float envelope = 0.0f;
            const float attackCoeff = 0.1f;
            const float releaseCoeff = 0.001f;
            
            for (int i = 0; i < numSamples; ++i) {
                float sample = std::abs(midHigh[static_cast<size_t>(i)]);
                
                // Envelope follower
                if (sample > envelope)
                    envelope = envelope + attackCoeff * (sample - envelope);
                else
                    envelope = envelope + releaseCoeff * (sample - envelope);
                
                // Transient emphasis (boost when envelope is rising)
                float transientGain = 1.0f + envelope * 2.0f;
                transientGain = juce::jmin(transientGain, 3.0f); // Limit boost
                
                // Combine: kick (low) + snare/hat (mid-high with transient emphasis)
                float drumMix = lowFreq[static_cast<size_t>(i)] * 1.2f + 
                               midHigh[static_cast<size_t>(i)] * transientGain * 0.8f;
                
                // Add some of the side channel for stereo drum width
                float sideComponent = highFreq[static_cast<size_t>(i)] * 0.3f;
                
                leftOut[i] = drumMix + sideComponent;
                rightOut[i] = drumMix - sideComponent;
            }
            break;
        }
        
        case StemType::Other: {
            // Other: Side channel (stereo width) content
            // This captures guitars, synths, reverb tails - anything panned or stereo
            
            // Use side channel as the primary source
            // Side = (L - R) / 2, so we need to convert back: L = Mid + Side, R = Mid - Side
            // For "Other", we output primarily the side content
            
            // Apply highpass to remove any bass bleed
            std::copy(side.begin(), side.end(), highFreq.begin());
            float* sidePtr = &highFreq[0];
            float* channels[] = { sidePtr };
            juce::dsp::AudioBlock<float> sideBlock(channels, 1, static_cast<size_t>(numSamples));
            juce::dsp::ProcessContextReplacing<float> sideContext(sideBlock);
            highPassFilter_.process(sideContext);
            
            for (int i = 0; i < numSamples; ++i) {
                // Convert side back to L/R
                float sideContent = highFreq[static_cast<size_t>(i)];
                
                // Boost the side content slightly for better separation
                sideContent *= 1.5f;
                
                leftOut[i] = sideContent;
                rightOut[i] = -sideContent; // Inverted for stereo spread
            }
            break;
        }
    }
}

} // namespace zenith

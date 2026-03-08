/*
  ==============================================================================

    SIMDHelpers.h
    Created: 2025-12-09
    Author:  Zenith DAW

    SIMD-optimized DSP utilities for high-performance audio processing.
    
    Uses JUCE's FloatVectorOperations for cross-platform SIMD acceleration.
    Provides optimized versions of common DSP operations.

    Thread Safety:
    - All functions are RT-safe (no allocations, no locks)
    - Safe to call from audio thread

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <algorithm>

namespace zenith {
namespace simd {

//==============================================================================
// Gain Operations (SIMD Accelerated)
//==============================================================================

/**
 * @brief Apply gain to a buffer (SIMD accelerated)
 * @param buffer Buffer to process
 * @param gain Gain to apply
 * @note RT-safe
 */
inline void applyGain(juce::AudioBuffer<float>& buffer, float gain) noexcept {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        juce::FloatVectorOperations::multiply(
            buffer.getWritePointer(ch), gain, buffer.getNumSamples());
    }
}

/**
 * @brief Apply gain ramp to a buffer (SIMD accelerated)
 * @param buffer Buffer to process
 * @param startGain Starting gain value
 * @param endGain Ending gain value
 * @note RT-safe, provides smooth parameter changes
 */
inline void applyGainRamp(juce::AudioBuffer<float>& buffer, 
                          float startGain, float endGain) noexcept {
    const int numSamples = buffer.getNumSamples();
    if (numSamples == 0) return;
    
    const float increment = (endGain - startGain) / static_cast<float>(numSamples);
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* data = buffer.getWritePointer(ch);
        float gain = startGain;
        
        // Process in chunks of 4 for better cache utilization
        int i = 0;
        for (; i <= numSamples - 4; i += 4) {
            data[i]     *= gain;
            data[i + 1] *= gain + increment;
            data[i + 2] *= gain + 2.0f * increment;
            data[i + 3] *= gain + 3.0f * increment;
            gain += 4.0f * increment;
        }
        
        // Handle remaining samples
        for (; i < numSamples; ++i) {
            data[i] *= gain;
            gain += increment;
        }
    }
}

//==============================================================================
// Level Detection (SIMD Accelerated)
//==============================================================================

/**
 * @brief Find peak level across all channels (SIMD accelerated)
 * @param buffer Buffer to analyze
 * @return Peak absolute value
 * @note RT-safe
 */
inline float findPeak(const juce::AudioBuffer<float>& buffer) noexcept {
    float peak = 0.0f;
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        
        // JUCE's findMinAndMax is SIMD optimized
        // JUCE's findMinAndMax is SIMD optimized
        auto range = juce::FloatVectorOperations::findMinAndMax(
            data, buffer.getNumSamples());
        float min = range.getStart();
        float max = range.getEnd();
        
        peak = std::max(peak, std::max(std::abs(min), std::abs(max)));
    }
    
    return peak;
}

/**
 * @brief Calculate RMS level across all channels (SIMD accelerated)
 * @param buffer Buffer to analyze
 * @return RMS level
 * @note RT-safe
 */
inline float calculateRMS(const juce::AudioBuffer<float>& buffer) noexcept {
    if (buffer.getNumSamples() == 0) return 0.0f;
    
    float sumSquares = 0.0f;
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        const int numSamples = buffer.getNumSamples();
        
        // Sum of squares - process in chunks for cache efficiency
        float localSum = 0.0f;
        int i = 0;
        
        // Unrolled loop for better performance
        for (; i <= numSamples - 4; i += 4) {
            localSum += data[i] * data[i];
            localSum += data[i + 1] * data[i + 1];
            localSum += data[i + 2] * data[i + 2];
            localSum += data[i + 3] * data[i + 3];
        }
        
        // Handle remaining samples
        for (; i < numSamples; ++i) {
            localSum += data[i] * data[i];
        }
        
        sumSquares += localSum;
    }
    
    const float mean = sumSquares / static_cast<float>(
        buffer.getNumSamples() * buffer.getNumChannels());
    
    return std::sqrt(mean);
}

//==============================================================================
// Mixing Operations (SIMD Accelerated)
//==============================================================================

/**
 * @brief Add source buffer to destination with gain (SIMD accelerated)
 * @param dest Destination buffer
 * @param source Source buffer
 * @param gain Gain to apply to source
 * @note RT-safe, adds source * gain to dest
 */
inline void addWithGain(juce::AudioBuffer<float>& dest,
                        const juce::AudioBuffer<float>& source,
                        float gain) noexcept {
    const int numChannels = std::min(dest.getNumChannels(), source.getNumChannels());
    const int numSamples = std::min(dest.getNumSamples(), source.getNumSamples());
    
    for (int ch = 0; ch < numChannels; ++ch) {
        juce::FloatVectorOperations::addWithMultiply(
            dest.getWritePointer(ch),
            source.getReadPointer(ch),
            gain,
            numSamples);
    }
}

/**
 * @brief Mix source buffer into destination (SIMD accelerated)
 * @param dest Destination buffer
 * @param source Source buffer
 * @note RT-safe, equivalent to addWithGain(dest, source, 1.0f)
 */
inline void mix(juce::AudioBuffer<float>& dest,
                const juce::AudioBuffer<float>& source) noexcept {
    addWithGain(dest, source, 1.0f);
}

//==============================================================================
// Stereo Processing (SIMD Accelerated)
//==============================================================================

/**
 * @brief Apply constant power panning (SIMD accelerated)
 * @param buffer Stereo buffer to process
 * @param pan Pan position (-1.0 = left, 0.0 = center, 1.0 = right)
 * @note RT-safe, expects stereo buffer
 */
inline void applyPan(juce::AudioBuffer<float>& buffer, float pan) noexcept {
    if (buffer.getNumChannels() < 2) return;
    
    // Constant power pan coefficients
    constexpr float piOver4 = juce::MathConstants<float>::pi / 4.0f;
    const float angle = piOver4 * (1.0f + pan);
    const float leftGain = std::cos(angle);
    const float rightGain = std::sin(angle);
    
    float* left = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);
    
    juce::FloatVectorOperations::multiply(left, leftGain, buffer.getNumSamples());
    juce::FloatVectorOperations::multiply(right, rightGain, buffer.getNumSamples());
}

/**
 * @brief Convert mono to stereo (SIMD accelerated)
 * @param mono Mono input buffer
 * @param stereo Stereo output buffer
 * @note RT-safe, copies mono to both channels
 */
inline void monoToStereo(const juce::AudioBuffer<float>& mono,
                         juce::AudioBuffer<float>& stereo) noexcept {
    if (mono.getNumChannels() < 1 || stereo.getNumChannels() < 2) return;
    
    const int numSamples = std::min(mono.getNumSamples(), stereo.getNumSamples());
    const float* monoData = mono.getReadPointer(0);
    
    juce::FloatVectorOperations::copy(stereo.getWritePointer(0), monoData, numSamples);
    juce::FloatVectorOperations::copy(stereo.getWritePointer(1), monoData, numSamples);
}

/**
 * @brief Convert stereo to mono (SIMD accelerated)
 * @param stereo Stereo input buffer
 * @param mono Mono output buffer
 * @note RT-safe, averages left and right
 */
inline void stereoToMono(const juce::AudioBuffer<float>& stereo,
                         juce::AudioBuffer<float>& mono) noexcept {
    if (stereo.getNumChannels() < 2 || mono.getNumChannels() < 1) return;
    
    const int numSamples = std::min(stereo.getNumSamples(), mono.getNumSamples());
    const float* left = stereo.getReadPointer(0);
    const float* right = stereo.getReadPointer(1);
    float* monoData = mono.getWritePointer(0);
    
    // Copy left, then add right with 0.5 gain
    juce::FloatVectorOperations::copy(monoData, left, numSamples);
    juce::FloatVectorOperations::addWithMultiply(monoData, right, 0.5f, numSamples);
    juce::FloatVectorOperations::multiply(monoData, 0.5f, numSamples);
}

//==============================================================================
// Soft Clipping / Saturation (SIMD Accelerated)
//==============================================================================

/**
 * @brief Apply soft clipping saturation
 * @param buffer Buffer to process
 * @param drive Drive amount (1.0 = clean, higher = more saturation)
 * @note RT-safe, provides musical saturation without harsh clipping
 */
inline void applySoftClip(juce::AudioBuffer<float>& buffer, float drive = 1.0f) noexcept {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* data = buffer.getWritePointer(ch);
        const int numSamples = buffer.getNumSamples();
        
        for (int i = 0; i < numSamples; ++i) {
            float x = data[i] * drive;
            
            // Soft clipping using tanh approximation (fast)
            // tanh(x) ≈ x / (1 + abs(x))^0.5 for |x| < 3
            if (x > 3.0f) {
                data[i] = 1.0f / drive;
            } else if (x < -3.0f) {
                data[i] = -1.0f / drive;
            } else {
                data[i] = x / (1.0f + std::abs(x)) / drive;
            }
        }
    }
}

/**
 * @brief Apply hard clipping (brickwall)
 * @param buffer Buffer to process
 * @param threshold Maximum allowed level
 * @note RT-safe
 */
inline void applyHardClip(juce::AudioBuffer<float>& buffer, 
                          float threshold = 1.0f) noexcept {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* data = buffer.getWritePointer(ch);
        juce::FloatVectorOperations::clip(data, data, -threshold, threshold, 
                                          buffer.getNumSamples());
    }
}

//==============================================================================
// Normalization (SIMD Accelerated)
//==============================================================================

/**
 * @brief Normalize buffer to peak level
 * @param buffer Buffer to normalize
 * @param targetPeak Target peak level (default 1.0)
 * @return Gain applied (0.0 if buffer was silent)
 * @note RT-safe
 */
inline float normalize(juce::AudioBuffer<float>& buffer, 
                       float targetPeak = 1.0f) noexcept {
    float peak = findPeak(buffer);
    
    if (peak < 1e-6f) return 0.0f; // Buffer is silent
    
    float gain = targetPeak / peak;
    applyGain(buffer, gain);
    
    return gain;
}

//==============================================================================
// DC Offset Removal
//==============================================================================

/**
 * @brief DC offset filter state for removeDC function
 * @note Each channel needs its own state for proper filtering
 */
struct DCFilterState {
    std::array<float, 2> prevInput{0.0f, 0.0f};
    std::array<float, 2> prevOutput{0.0f, 0.0f};
    
    void reset() {
        prevInput.fill(0.0f);
        prevOutput.fill(0.0f);
    }
};

/**
 * @brief Remove DC offset using high-pass filter
 * @param buffer Buffer to process
 * @param state Per-channel filter state (MUST be provided, will be modified)
 * @param coeff Filter coefficient (default 0.995, lower = more aggressive)
 * @note RT-safe - no allocations
 */
inline void removeDC(juce::AudioBuffer<float>& buffer, 
                     DCFilterState& state,
                     float coeff = 0.995f) noexcept {
    const int numChannels = std::min(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();
    
    for (int ch = 0; ch < numChannels; ++ch) {
        float* data = buffer.getWritePointer(ch);
        float xPrev = state.prevInput[ch];
        float yPrev = state.prevOutput[ch];
        
        for (int i = 0; i < numSamples; ++i) {
            float x = data[i];
            // First-order high-pass: y[n] = x[n] - x[n-1] + coeff * y[n-1]
            float y = x - xPrev + coeff * yPrev;
            data[i] = y;
            xPrev = x;
            yPrev = y;
        }
        
        state.prevInput[ch] = xPrev;
        state.prevOutput[ch] = yPrev;
    }
}

//==============================================================================
// Fade Operations
//==============================================================================

/**
 * @brief Apply fade-in to buffer
 * @param buffer Buffer to process
 * @param fadeLength Length of fade in samples
 * @param curveType Curve type (0.0 = linear, 1.0 = exponential)
 * @note RT-safe
 */
inline void fadeIn(juce::AudioBuffer<float>& buffer, 
                   int fadeLength,
                   float curveType = 0.5f) noexcept {
    fadeLength = std::min(fadeLength, buffer.getNumSamples());
    if (fadeLength <= 0) return;
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* data = buffer.getWritePointer(ch);
        
        for (int i = 0; i < fadeLength; ++i) {
            float pos = static_cast<float>(i) / static_cast<float>(fadeLength);
            
            // Blend between linear and exponential based on curveType
            float linear = pos;
            float exponential = pos * pos;
            float gain = linear * (1.0f - curveType) + exponential * curveType;
            
            data[i] *= gain;
        }
    }
}

/**
 * @brief Apply fade-out to buffer
 * @param buffer Buffer to process
 * @param fadeLength Length of fade in samples
 * @param curveType Curve type (0.0 = linear, 1.0 = exponential)
 * @note RT-safe
 */
inline void fadeOut(juce::AudioBuffer<float>& buffer, 
                    int fadeLength,
                    float curveType = 0.5f) noexcept {
    fadeLength = std::min(fadeLength, buffer.getNumSamples());
    if (fadeLength <= 0) return;
    
    const int startSample = buffer.getNumSamples() - fadeLength;
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* data = buffer.getWritePointer(ch);
        
        for (int i = 0; i < fadeLength; ++i) {
            float pos = static_cast<float>(i) / static_cast<float>(fadeLength);
            
            // Inverted fade curve
            float linear = 1.0f - pos;
            float exponential = (1.0f - pos) * (1.0f - pos);
            float gain = linear * (1.0f - curveType) + exponential * curveType;
            
            data[startSample + i] *= gain;
        }
    }
}

//==============================================================================
// Utility Conversions
//==============================================================================

/**
 * @brief Convert dB to linear gain
 * @param db Value in decibels
 * @return Linear gain value
 */
inline float dbToGain(float db) noexcept {
    return juce::Decibels::decibelsToGain(db);
}

/**
 * @brief Convert linear gain to dB
 * @param gain Linear gain value
 * @return Value in decibels
 */
inline float gainToDb(float gain) noexcept {
    return juce::Decibels::gainToDecibels(gain);
}

} // namespace simd
} // namespace zenith

//==============================================================================
// Fast Math Approximations (SIMD/Scalar)
//==============================================================================

namespace zenith {
namespace simd {

/**
 * @brief Fast approximation of log10(x)
 * @param x Input value (must be > 0)
 * @return Approximation of log10(x)
 */
inline float fastLog10(float x) noexcept {
    //  Bit hack for integer part of log2
    union { float f; int i; } vx = { x };
    float y = (float)vx.i;
    y *= 1.1920928955078125e-7f; // 1/8388608
    y -= 126.942529f; // Bias adjustment
    
    // Linear approximation of log2 converted to log10
    // log10(x) = log2(x) * log10(2)
    // log10(2) ~= 0.30103
    return y * 0.30102999566f;
}

/**
 * @brief Fast approximation of pow(10, x)
 * @param x Input power
 * @return Approximation of 10^x
 */
inline float fastPow10(float x) noexcept {
    // 10^x = 2^(x * log2(10))
    // log2(10) ~= 3.32192809489
    float y = x * 3.32192809489f;
    
    // Fast 2^y approximation
    // val = (y + 126.942529) * 8388608
    union { float f; int i; } vx;
    vx.i = (int)((y + 126.942529f) * 8388608.0f);
    return vx.f;
}

/**
 * @brief Fast conversion from linear gain to decibels
 * @param gain Linear gain
 * @return Decibels (or -100.0 if gain <= 0)
 */
inline float fastGainToDb(float gain) noexcept {
    if (gain <= 0.0000001f) return -100.0f;
    return fastLog10(gain) * 20.0f;
}

/**
 * @brief Fast conversion from decibels to linear gain
 * @param db Decibels
 * @return Linear gain
 */
inline float fastDbToGain(float db) noexcept {
    if (db <= -100.0f) return 0.0f;
    return fastPow10(db * 0.05f); // db/20
}

} // namespace simd
} // namespace zenith

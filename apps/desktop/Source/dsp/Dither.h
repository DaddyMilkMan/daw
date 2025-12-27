/*
  ==============================================================================
    apps/desktop/Source/dsp/Dither.h
    High-quality TPDF Dithering with HP (High-Pass) Noise Shaping.
    
    CRITIC FIX: Previously lastErrors was allocated but NEVER USED.
    Now implements proper HP-TPDF noise shaping for improved perceived quality.
  ==============================================================================
*/

#pragma once
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <random>
#include <vector>
#include <cmath>

namespace zenith {
namespace dsp {

/**
 * @brief TPDF Dither with optional HP noise shaping
 * 
 * HP-TPDF (High-Pass Triangular Probability Density Function) is the 
 * industry standard for CD mastering. The noise shaping pushes 
 * quantization noise into higher frequencies where human hearing is 
 * less sensitive.
 */
class Dither
{
public:
    Dither() = default;

    void prepare(int numChannels)
    {
        lastErrors.resize(static_cast<size_t>(numChannels), 0.0f);
        // Seed the generator
        rng.seed(std::random_device{}());
    }

    /**
     * @brief Enable/disable high-pass noise shaping
     * @param enabled true for HP-TPDF (better quality), false for flat TPDF
     */
    void setNoiseShapingEnabled(bool enabled) { noiseShapingEnabled = enabled; }

    /**
     * @brief Apply TPDF dither with optional noise shaping
     * @param buffer Audio buffer to dither (modified in-place)
     * @param targetBitDepth Target bit depth (16, 24, etc.)
     */
    void process(juce::AudioBuffer<float>& buffer, int targetBitDepth)
    {
        // No dithering needed for 32-bit float or higher
        if (targetBitDepth >= 32) return;

        // Calculate scale for the target bit depth
        // 16-bit signed: 2^15 = 32768, so 1 LSB = 1/32768
        const float scale = std::pow(2.0f, static_cast<float>(targetBitDepth - 1));
        const float invScale = 1.0f / scale;
        
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            float* data = buffer.getWritePointer(ch);
            const int numSamples = buffer.getNumSamples();
            float& lastError = lastErrors[static_cast<size_t>(ch)];

            for (int i = 0; i < numSamples; ++i)
            {
                // TPDF: Sum of two uniform random variables gives triangular distribution
                float r1 = dist(rng);
                float r2 = dist(rng);
                float tpdf = (r1 + r2) * 0.5f; // Range -1 to 1, triangular distribution

                // Scale noise to 1 LSB magnitude
                float noise = tpdf * invScale;

                if (noiseShapingEnabled)
                {
                    // HP-TPDF: Subtract the previous sample's error
                    // This creates a first-order high-pass filter on the noise,
                    // pushing it into higher frequencies where hearing is less sensitive
                    float shapedNoise = noise - lastError;
                    
                    // Quantize to get the error term for next sample
                    float original = data[i];
                    float dithered = original + shapedNoise;
                    float quantized = std::round(dithered * scale) * invScale;
                    
                    // Calculate error for next iteration
                    lastError = quantized - original;
                    
                    data[i] = dithered;
                }
                else
                {
                    // Flat TPDF (simpler, less CPU)
                    data[i] += noise;
                }
            }
        }
    }

    /**
     * @brief Reset noise shaping state (call between songs/sessions)
     */
    void reset()
    {
        std::fill(lastErrors.begin(), lastErrors.end(), 0.0f);
    }

private:
    std::vector<float> lastErrors;
    std::mt19937 rng;
    bool noiseShapingEnabled = true; // Default to HP-TPDF for best quality
};

} // namespace dsp
} // namespace zenith


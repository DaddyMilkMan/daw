/*
  ==============================================================================
    apps/desktop/Source/dsp/Dither.h
    High-quality TPDF Dithering for bit-depth reduction.
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

class Dither
{
public:
    Dither() = default;

    void prepare(int numChannels)
    {
        lastErrors.resize(numChannels, 0.0f);
        // Seed the generator
        rng.seed(std::random_device{}());
    }

    /**
     * @brief Apply TPDF dither to a buffer for a specific target bit depth
     */
    void process(juce::AudioBuffer<float>& buffer, int targetBitDepth)
    {
        // No dithering needed for 32-bit float or higher
        if (targetBitDepth >= 32) return;

        // Calculate scale for the target bit depth
        // e.g., 16-bit signed: 2^15 - 1 = 32767
        // 8-bit unsigned: 2^8 = 256 (handled by writer, but we dither relative to amplitude steps)
        
        // Effectively, we want to add noise at the magnitude of 1 LSB.
        // 1 LSB = 1.0 / (2^(bits-1)) roughly for signed PCM.
        
        const float scale = std::pow(2.0f, (float)targetBitDepth - 1.0f);
        const float invScale = 1.0f / scale;
        
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            float* data = buffer.getWritePointer(ch);
            const int numSamples = buffer.getNumSamples();

            for (int i = 0; i < numSamples; ++i)
            {
                // TPDF: Sum of two uniform random variables
                float r1 = dist(rng);
                float r2 = dist(rng);
                float tpdf = (r1 + r2) * 0.5f; // Range -1 to 1, triangular distribution

                // Scale noise to 1 LSB magnitude
                float noise = tpdf * invScale;

                // Apply dither
                data[i] += noise;
                
                // Note: We don't quantize here (floor/round). 
                // The AudioFormatWriter does the truncation. 
                // Adding the noise beforehand linearizes the quantization error.
            }
        }
    }

private:
    std::vector<float> lastErrors;
    std::mt19937 rng;
};

} // namespace dsp
} // namespace zenith

/*
  ==============================================================================
    apps/desktop/Source/dsp/Dither.h
    High-quality TPDF Dithering for bit-depth reduction.
    
    Supports:
    - Flat TPDF (standard triangular probability density function)
    - Noise-shaped TPDF (first-order error feedback pushing noise to less 
      audible frequencies)
  ==============================================================================
*/

#pragma once
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <random>
#include <vector>
#include <cmath>
#include <array>

namespace zenith {
namespace dsp {

/**
 * @brief Dither algorithm type
 */
enum class DitherType {
    None,        ///< No dithering (not recommended for bit depth reduction)
    FlatTPDF,    ///< Standard triangular probability density function dither
    ShapedTPDF   ///< Noise-shaped TPDF with first-order error feedback
};

/**
 * @class Dither
 * @brief High-quality dithering for professional bit-depth reduction
 * 
 * Implements TPDF (Triangular Probability Density Function) dithering which 
 * is the industry standard for audio mastering. The shaped variant uses
 * first-order noise shaping to push quantization noise into less audible
 * frequency ranges.
 */
class Dither
{
public:
    static constexpr int kMaxChannels = 8;

    Dither() : ditherType_(DitherType::ShapedTPDF) {
        errorBuffer_.fill(0.0f);
    }

    /**
     * @brief Set the dither algorithm type
     * @param type DitherType to use
     */
    void setType(DitherType type) noexcept { ditherType_ = type; }
    
    /**
     * @brief Get current dither type
     */
    DitherType getType() const noexcept { return ditherType_; }

    /**
     * @brief Prepare the ditherer for processing
     * @param numChannels Number of audio channels (max 8)
     */
    void prepare(int numChannels)
    {
        numChannels_ = juce::jmin(numChannels, kMaxChannels);
        reset();
        // Seed the generator with high-quality random seed
        std::random_device rd;
        rng_.seed(rd());
    }

    /**
     * @brief Reset all internal state (error buffers, etc.)
     */
    void reset()
    {
        errorBuffer_.fill(0.0f);
    }

    /**
     * @brief Apply TPDF dither to a buffer for a specific target bit depth
     * @param buffer Audio buffer to process (modified in-place)
     * @param targetBitDepth Target bit depth (8, 16, 24). 32-bit is bypassed.
     */
    void process(juce::AudioBuffer<float>& buffer, int targetBitDepth)
    {
        // No dithering needed for 32-bit float or if disabled
        if (targetBitDepth >= 32 || ditherType_ == DitherType::None) 
            return;

        // Calculate quantization step size (1 LSB in float representation)
        // For signed PCM: range is -1.0 to +1.0, quantized to 2^(bits-1) levels per side
        // 16-bit: 2^15 = 32768 levels -> 1 LSB = 1/32768 ≈ 0.0000305
        // 24-bit: 2^23 = 8388608 levels -> 1 LSB = 1/8388608 ≈ 0.000000119
        const float quantizationLevels = std::pow(2.0f, static_cast<float>(targetBitDepth - 1));
        const float lsbSize = 1.0f / quantizationLevels;
        
        const int numChannels = juce::jmin(buffer.getNumChannels(), numChannels_);
        const int numSamples = buffer.getNumSamples();

        if (ditherType_ == DitherType::FlatTPDF) {
            processFlatTPDF(buffer, numChannels, numSamples, lsbSize);
        } else {
            processShapedTPDF(buffer, numChannels, numSamples, lsbSize, quantizationLevels);
        }
    }

private:
    /**
     * @brief Standard flat TPDF dithering
     * Adds triangular-distributed noise at 1 LSB amplitude.
     */
    void processFlatTPDF(juce::AudioBuffer<float>& buffer, int numChannels, 
                         int numSamples, float lsbSize)
    {
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = buffer.getWritePointer(ch);

            for (int i = 0; i < numSamples; ++i)
            {
                // TPDF: Sum of two independent uniform random variables
                // This creates a triangular distribution centered at 0
                const float r1 = dist(rng_);
                const float r2 = dist(rng_);
                const float tpdfNoise = (r1 + r2) * 0.5f; // Range [-1, 1], triangular PDF

                // Scale noise to 1 LSB magnitude and apply
                data[i] += tpdfNoise * lsbSize;
            }
        }
    }

    /**
     * @brief Noise-shaped TPDF dithering with first-order error feedback
     * 
     * Uses error feedback to shape the noise spectrum, pushing energy into
     * higher frequencies where human hearing is less sensitive. This provides
     * a perceptual improvement of approximately 3-4 dB in signal-to-noise ratio.
     * 
     * The algorithm:
     * 1. Add TPDF dither noise
     * 2. Quantize to target bit depth (for error calculation)
     * 3. Calculate quantization error
     * 4. Subtract previous error (first-order high-pass shaping)
     * 5. Store error for next sample
     */
    void processShapedTPDF(juce::AudioBuffer<float>& buffer, int numChannels,
                           int numSamples, float lsbSize, float quantLevels)
    {
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        
        // Noise shaping coefficient (first-order high-pass)
        // Higher values push more noise to high frequencies but risk instability
        constexpr float shapingCoeff = 0.5f;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = buffer.getWritePointer(ch);
            float& error = errorBuffer_[ch];

            for (int i = 0; i < numSamples; ++i)
            {
                // Generate TPDF noise
                const float r1 = dist(rng_);
                const float r2 = dist(rng_);
                const float tpdfNoise = (r1 + r2) * 0.5f * lsbSize;

                // Get input sample and subtract previous quantization error (noise shaping)
                float input = data[i] - (error * shapingCoeff);

                // Add dither noise
                float dithered = input + tpdfNoise;

                // Simulate quantization to calculate error
                // Quantize → round to nearest quantization level
                float scaled = dithered * quantLevels;
                float quantized = std::round(scaled) / quantLevels;

                // Calculate and store quantization error for next sample
                error = quantized - input;

                // Output the dithered (but not quantized) signal
                // The actual quantization happens in the AudioFormatWriter
                data[i] = dithered;
            }
        }
    }

    DitherType ditherType_;
    int numChannels_ = 2;
    std::mt19937 rng_;
    
    // Per-channel error feedback buffers for noise shaping
    std::array<float, kMaxChannels> errorBuffer_;
};

} // namespace dsp
} // namespace zenith

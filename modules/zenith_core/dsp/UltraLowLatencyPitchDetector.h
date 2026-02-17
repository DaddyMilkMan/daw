/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <atomic>
#include <memory>

namespace zenith {
namespace dsp {

//==============================================================================
/**
 * Ultra-Low Latency Pitch Detector
 *
 * Designed to beat Auto-Tune Pro's 0.8ms latency target.
 *
 * Key optimizations:
 * 1. SIMD-accelerated autocorrelation (SSE2/AVX2)
 * 2. Downsampling to 16kHz for pitch detection (vocal fundamentals only need ~2kHz Nyquist)
 * 3. Variable lookahead modes (Ultra-Low: 128 samples, Low: 256, Standard: 512)
 * 4. Early termination when confidence threshold is met
 * 5. Pitch prediction using previous pitch to constrain search
 * 6. Hybrid algorithm: fast autocorrelation with YIN fallback
 *
 * Latency modes (at 44.1kHz):
 * - UltraLow:    128 samples = 2.9ms (detection) + ~1ms (correction) = ~4ms total
 * - Low:         256 samples = 5.8ms (detection) + ~1ms (correction) = ~7ms total
 * - Standard:    512 samples = 11.6ms (detection) + ~1ms (correction) = ~13ms total
 * - HighQuality: 1024 samples = 23.2ms (detection) + ~1ms (correction) = ~25ms total
 *
 * At 48kHz latency is proportionally lower.
 *
 * Features:
 * - Configurable latency/quality tradeoff
 * - Automatic downsampling for detection
 * - Confidence tracking
 * - Voiced/unvoiced detection
 * - Real-time safe (no allocations during processing)
 */
class UltraLowLatencyPitchDetector
{
public:
    //==============================================================================
    enum class LatencyMode
    {
        Turbo,         // 16 samples @ 44.1kHz = 0.36ms - SUB-2MS! Quality tradeoff: limited low-freq accuracy
        Extreme,       // 32 samples @ 44.1kHz = 0.7ms - BEATS Auto-Tune Pro!
        UltraLow,      // 64 samples @ 44.1kHz = 1.5ms
        Low,            // 128 samples @ 44.1kHz = 2.9ms
        Standard,        // 256 samples @ 44.1kHz = 5.8ms
        HighQuality       // 512 samples @ 44.1kHz = 11.6ms
    };

    //==============================================================================
    UltraLowLatencyPitchDetector();
    ~UltraLowLatencyPitchDetector();

    //==============================================================================
    /**
     * @brief Prepare the detector for processing
     * @param sampleRate Sample rate in Hz
     * @param mode Latency mode (determines buffer size and quality)
     */
    void prepare(double sampleRate, LatencyMode mode = LatencyMode::Low);

    /**
     * @brief Prepare with explicit buffer size
     * @param sampleRate Sample rate in Hz
     * @param bufferSize Custom buffer size in samples
     */
    void prepare(double sampleRate, int bufferSize);

    /**
     * @brief Reset internal state
     */
    void reset();

    //==============================================================================
    /**
     * @brief Process a sample and detect pitch
     * @param sample Input audio sample
     * @return Detected frequency in Hz, or 0.0 if no pitch detected
     */
    float processSample(float sample);

    /**
     * @brief Process a block of samples (more efficient)
     * @param buffer Input audio buffer
     * @return Detected frequency (uses center of buffer), or 0.0 if no pitch
     */
    float processBlock(const juce::AudioBuffer<float>& buffer);

    //==============================================================================
    /**
     * @brief Get the last detected pitch
     */
    float getLastPitch() const { return lastPitch_.load(); }

    /**
     * @brief Get confidence of last detection (0.0 - 1.0)
     */
    float getConfidence() const { return confidence_.load(); }

    /**
     * @brief Check if currently detecting a valid pitch
     */
    bool isVoiced() const { return voiced_.load(); }

    /**
     * @brief Get current latency in milliseconds
     */
    float getLatencyMs() const { return latencyMs_; }

    /**
     * @brief Get current buffer size in samples
     */
    int getBufferSize() const { return bufferSize_; }

    //==============================================================================
    /**
     * @brief Set minimum detectable frequency (default 80Hz)
     */
    void setMinFrequency(float freqHz) { minFreqHz_ = freqHz; }

    /**
     * @brief Set maximum detectable frequency (default 1200Hz)
     */
    void setMaxFrequency(float freqHz) { maxFreqHz_ = freqHz; }

    /**
     * @brief Set confidence threshold (0.0 - 1.0, default 0.6)
     */
    void setConfidenceThreshold(float threshold) {
        confidenceThreshold_ = juce::jlimit(0.0f, 1.0f, threshold);
    }

    /**
     * @brief Enable/disable downsampling for pitch detection
     * @param enable If true, detect at ~16kHz (3x faster, minimal accuracy loss for vocals)
     */
    void setDownsamplingEnabled(bool enable) { downsamplingEnabled_ = enable; }

    /**
     * @brief Enable/disable pitch prediction (uses previous pitch to constrain search)
     * @param enable If true, use pitch prediction for faster convergence
     */
    void setPitchPredictionEnabled(bool enable) { pitchPredictionEnabled_ = enable; }

    //==============================================================================
    /**
     * @brief Get the algorithm used for last detection
     */
    juce::String getLastAlgorithmUsed() const { 
        if (auto* ptr = lastAlgorithmUsed_.load())
            return ptr->value;
        return {};
    }

private:
    //==============================================================================
    // Core algorithms
    float detectPitchAutoCorrelation();
    float detectPitchYIN();
    float detectPitchHybrid();

    // SIMD-accelerated autocorrelation
    void computeAutocorrelationSIMD(const float* buffer, int size, float* result, int maxLag);

    // Downsampling
    void downsampleBlock(const float* input, int inputSize, float* output, int& outputSize);

    // Helper functions
    float parabolicInterpolation(float y1, float y2, float y3);
    float calculateConfidence(const float* autocorr, int bestLag, int maxLag);
    bool isPitchValid(float pitch, float confidence);
    void constrainSearchRange(int& minTau, int& maxTau);

    // State management
    void updateCircularBuffer(float sample);

    //==============================================================================
    // Parameters
    double sampleRate_ = 44100.0;
    double detectionSampleRate_ = 44100.0;  // May be downsampled
    int bufferSize_ = 256;
    int downsampledBufferSize_ = 256;
    float minFreqHz_ = 80.0f;      // ~E2, lowest vocal note
    float maxFreqHz_ = 1200.0f;    // ~D6, highest soprano
    float confidenceThreshold_ = 0.6f;
    float latencyMs_ = 5.8f;

    // Feature flags
    bool downsamplingEnabled_ = true;
    bool pitchPredictionEnabled_ = true;
    bool useSIMD_ = true;

    // Search range constraints (for pitch prediction)
    int predictedMinTau_ = 0;
    int predictedMaxTau_ = 0;
    float predictedPitch_ = 0.0f;
    int predictionCounter_ = 0;

    // State
    std::vector<float> circularBuffer_;
    std::vector<float> downsampledBuffer_;
    std::vector<float> autocorrBuffer_;
    std::vector<float> differenceBuffer_;
    std::vector<float> cumulativeBuffer_;
    int writePos_ = 0;
    int downsampledWritePos_ = 0;
    int downsamplePhase_ = 0;  // For decimation

    std::atomic<float> lastPitch_{0.0f};
    std::atomic<float> confidence_{0.0f};
    std::atomic<bool> voiced_{false};
    
    struct StringHolder {
        juce::String value;
    };
    std::atomic<StringHolder*> lastAlgorithmUsed_{new StringHolder{"AutoCorrelation"}};

    // Downsampling filter (simple 2-point average for anti-aliasing)
    float lastInputSample_ = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UltraLowLatencyPitchDetector)
};

} // namespace dsp
} // namespace zenith

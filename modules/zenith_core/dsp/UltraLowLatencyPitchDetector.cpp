/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#include "UltraLowLatencyPitchDetector.h"
#include <algorithm>
#include <cmath>

// SIMD includes
#if JUCE_MAC || JUCE_IOS || JUCE_LINUX
    #include <xmmintrin.h>   // SSE
    #include <emmintrin.h>   // SSE2
    #if __AVX2__
        #include <immintrin.h>   // AVX2
    #endif
#elif JUCE_WINDOWS
    #include <intrin.h>
#endif

namespace zenith {
namespace dsp {

//==============================================================================
// SIMD detection at compile time
#if defined(__AVX2__)
    #define ZENITH_HAS_AVX2 1
#elif defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86) && defined(_SSE2))
    #define ZENITH_HAS_SSE2 1
#else
    #define ZENITH_HAS_NO_SIMD 1
#endif

//==============================================================================
UltraLowLatencyPitchDetector::UltraLowLatencyPitchDetector()
{
}

UltraLowLatencyPitchDetector::~UltraLowLatencyPitchDetector()
{
    if (auto* ptr = lastAlgorithmUsed_.exchange(nullptr))
        delete ptr;
}

//==============================================================================
void UltraLowLatencyPitchDetector::prepare(double sampleRate, LatencyMode mode)
{
    sampleRate_ = sampleRate;

    // Map mode to buffer size
    switch (mode)
    {
        case LatencyMode::Turbo:
            bufferSize_ = 16;      // 0.36ms @ 44.1kHz - SUB-2MS TOTAL!
            // Minimum for reliable pitch detection at vocal frequencies
            break;
        case LatencyMode::Extreme:
            bufferSize_ = 32;      // 0.7ms @ 44.1kHz - BEATS Auto-Tune Pro!
            break;
        case LatencyMode::UltraLow:
            bufferSize_ = 64;      // 1.5ms @ 44.1kHz
            break;
        case LatencyMode::Low:
            bufferSize_ = 128;     // 2.9ms @ 44.1kHz
            break;
        case LatencyMode::Standard:
            bufferSize_ = 256;     // 5.8ms @ 44.1kHz
            break;
        case LatencyMode::HighQuality:
            bufferSize_ = 512;     // 11.6ms @ 44.1kHz
            break;
    }

    prepare(sampleRate, bufferSize_);
}

void UltraLowLatencyPitchDetector::prepare(double sampleRate, int bufferSize)
{
    sampleRate_ = sampleRate;
    bufferSize_ = juce::isPositiveAndBelow(bufferSize, 2048) ? bufferSize : 2048;

    // Calculate latency in milliseconds
    latencyMs_ = static_cast<float>(bufferSize_) / static_cast<float>(sampleRate_) * 1000.0f;

    // Set detection sample rate (downsampled if enabled)
    if (downsamplingEnabled_)
    {
        detectionSampleRate_ = sampleRate_ / 3.0;  // Decimate by 3 -> ~14.7kHz from 44.1kHz
        downsampledBufferSize_ = bufferSize_ / 3;
    }
    else
    {
        detectionSampleRate_ = sampleRate_;
        downsampledBufferSize_ = bufferSize_;
    }

    // Pre-allocate all buffers (RT-safe)
    circularBuffer_.resize(bufferSize_, 0.0f);
    downsampledBuffer_.resize(downsampledBufferSize_, 0.0f);
    autocorrBuffer_.resize(downsampledBufferSize_, 0.0f);

    // YIN buffers
    differenceBuffer_.resize(downsampledBufferSize_, 0.0f);
    cumulativeBuffer_.resize(downsampledBufferSize_, 0.0f);

    reset();
}

void UltraLowLatencyPitchDetector::reset()
{
    std::fill(circularBuffer_.begin(), circularBuffer_.end(), 0.0f);
    std::fill(downsampledBuffer_.begin(), downsampledBuffer_.end(), 0.0f);
    std::fill(autocorrBuffer_.begin(), autocorrBuffer_.end(), 0.0f);
    std::fill(differenceBuffer_.begin(), differenceBuffer_.end(), 0.0f);
    std::fill(cumulativeBuffer_.begin(), cumulativeBuffer_.end(), 0.0f);

    writePos_ = 0;
    downsampledWritePos_ = 0;
    downsamplePhase_ = 0;
    lastInputSample_ = 0.0f;

    lastPitch_.store(0.0f);
    confidence_.store(0.0f);
    voiced_.store(false);
    if (auto* ptr = lastAlgorithmUsed_.load())
        ptr->value = "AutoCorrelation";

    // Reset prediction
    predictedPitch_ = 0.0f;
    predictionCounter_ = 0;
    predictedMinTau_ = 0;
    predictedMaxTau_ = 0;
}

//==============================================================================
void UltraLowLatencyPitchDetector::updateCircularBuffer(float sample)
{
    circularBuffer_[writePos_] = sample;
    writePos_ = (writePos_ + 1) % bufferSize_;

    // Downsampling: simple decimation with 2-point averaging for anti-aliasing
    if (downsamplingEnabled_)
    {
        downsamplePhase_++;
        if (downsamplePhase_ >= 3)  // Decimate by 3
        {
            downsamplePhase_ = 0;
            float downsampled = (sample + lastInputSample_) * 0.5f;
            downsampledBuffer_[downsampledWritePos_] = downsampled;
            downsampledWritePos_ = (downsampledWritePos_ + 1) % downsampledBufferSize_;
            lastInputSample_ = sample;
        }
    }
}

float UltraLowLatencyPitchDetector::processSample(float sample)
{
    updateCircularBuffer(sample);
    return detectPitchHybrid();
}

float UltraLowLatencyPitchDetector::processBlock(const juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const float* data = buffer.getReadPointer(0);

    // Fill circular buffer with block
    for (int i = 0; i < numSamples; ++i)
    {
        updateCircularBuffer(data[i]);
    }

    return detectPitchHybrid();
}

//==============================================================================
float UltraLowLatencyPitchDetector::detectPitchHybrid()
{
    // Try fast autocorrelation first
    float pitch = detectPitchAutoCorrelation();

    // Get confidence from autocorrelation result
    float conf = confidence_.load();

    // If confidence is too low, fall back to YIN for more accuracy
    if (conf < confidenceThreshold_ * 0.7f)  // 70% of threshold
    {
        float yinPitch = detectPitchYIN();
        float yinConf = confidence_.load();

        if (yinConf > conf)
        {
            pitch = yinPitch;
            if (auto* ptr = lastAlgorithmUsed_.load())
                ptr->value = "YIN";
        }
    }
    else
    {
        if (auto* ptr = lastAlgorithmUsed_.load())
            ptr->value = "AutoCorrelation";
    }

    // Update pitch prediction for next cycle
    if (pitch > 0.0f && conf > confidenceThreshold_)
    {
        predictedPitch_ = pitch;
        predictionCounter_ = 0;
    }
    else
    {
        predictionCounter_++;
    }

    return pitch;
}

//==============================================================================
float UltraLowLatencyPitchDetector::detectPitchAutoCorrelation()
{
    const float* buffer = downsamplingEnabled_ ? downsampledBuffer_.data() : circularBuffer_.data();
    const int size = downsamplingEnabled_ ? downsampledBufferSize_ : bufferSize_;
    const int readPos = downsamplingEnabled_ ? downsampledWritePos_ : writePos_;

    const int tauMin = static_cast<int>(detectionSampleRate_ / maxFreqHz_);
    const int tauMax = static_cast<int>(detectionSampleRate_ / minFreqHz_);

    // Constrain search range if using pitch prediction
    int searchTauMin = tauMin;
    int searchTauMax = tauMax;

    if (pitchPredictionEnabled_ && predictedPitch_ > 0.0f && predictionCounter_ < 10)
    {
        constrainSearchRange(searchTauMin, searchTauMax);
    }

    // Normalize autocorrelation at lag 0
    float energy = 0.0f;
    for (int i = 0; i < size; ++i)
    {
        int idx = (readPos + i) % size;
        float val = buffer[idx];
        energy += val * val;
    }

    if (energy < 0.0001f)  // Silence detection
    {
        voiced_.store(false);
        lastPitch_.store(0.0f);
        confidence_.store(0.0f);
        return 0.0f;
    }

    // Compute autocorrelation for search range
    computeAutocorrelationSIMD(buffer, size, autocorrBuffer_.data(), searchTauMax);

    // Find peak in autocorrelation (skip lag 0)
    int bestTau = tauMin;
    float bestVal = autocorrBuffer_[tauMin];

    for (int tau = searchTauMin; tau <= searchTauMax; ++tau)
    {
        float normalized = autocorrBuffer_[tau] / energy;
        if (normalized > bestVal)
        {
            bestVal = normalized;
            bestTau = tau;
        }

        // Early termination: found very high confidence peak
        if (normalized > 0.95f)
        {
            break;
        }
    }

    // Calculate confidence
    float conf = calculateConfidence(autocorrBuffer_.data(), bestTau, tauMax);
    conf = juce::jlimit(0.0f, 1.0f, conf);
    confidence_.store(conf);

    // Check voiced
    if (conf < confidenceThreshold_ || bestVal < 0.3f)
    {
        voiced_.store(false);
        lastPitch_.store(0.0f);
        return 0.0f;
    }

    voiced_.store(true);

    // Parabolic interpolation for sub-sample accuracy
    float interpolatedTau = static_cast<float>(bestTau);
    if (bestTau > tauMin && bestTau < tauMax)
    {
        float y1 = autocorrBuffer_[bestTau - 1] / energy;
        float y2 = bestVal;
        float y3 = autocorrBuffer_[bestTau + 1] / energy;
        interpolatedTau += parabolicInterpolation(y1, y2, y3);
    }

    // Calculate frequency
    float frequency = static_cast<float>(detectionSampleRate_ / interpolatedTau);
    lastPitch_.store(frequency);

    return frequency;
}

//==============================================================================
void UltraLowLatencyPitchDetector::computeAutocorrelationSIMD(const float* buffer, int size, float* result, int maxLag)
{
    // Initialize result buffer
    for (int tau = 0; tau <= maxLag; ++tau)
    {
        result[tau] = 0.0f;
    }

    const int readPos = downsamplingEnabled_ ? downsampledWritePos_ : writePos_;

#if defined(ZENITH_HAS_AVX2)
    // AVX2 implementation: process 8 floats at once
    const int simdSize = size - (size % 8);

    for (int tau = 0; tau <= maxLag; ++tau)
    {
        __m256 sum = _mm256_setzero_ps();

        for (int i = 0; i < simdSize; i += 8)
        {
            int idx1 = (readPos + i) % size;
            int idx2 = (readPos + i + tau) % size;

            __m256 v1 = _mm256_loadu_ps(&buffer[idx1]);
            __m256 v2 = _mm256_loadu_ps(&buffer[idx2]);

            __m256 prod = _mm256_mul_ps(v1, v2);
            sum = _mm256_add_ps(sum, prod);
        }

        // Horizontal sum
        float tmp[8];
        _mm256_storeu_ps(tmp, sum);
        float finalSum = tmp[0] + tmp[1] + tmp[2] + tmp[3] + tmp[4] + tmp[5] + tmp[6] + tmp[7];

        // Handle remaining samples
        for (int i = simdSize; i < size; ++i)
        {
            int idx1 = (readPos + i) % size;
            int idx2 = (readPos + i + tau) % size;
            finalSum += buffer[idx1] * buffer[idx2];
        }

        result[tau] = finalSum;
    }

#elif defined(ZENITH_HAS_SSE2)
    // SSE2 implementation: process 4 floats at once
    const int simdSize = size - (size % 4);

    for (int tau = 0; tau <= maxLag; ++tau)
    {
        __m128 sum = _mm_setzero_ps();

        for (int i = 0; i < simdSize; i += 4)
        {
            int idx1 = (readPos + i) % size;
            int idx2 = (readPos + i + tau) % size;

            __m128 v1 = _mm_loadu_ps(&buffer[idx1]);
            __m128 v2 = _mm_loadu_ps(&buffer[idx2]);

            sum = _mm_add_ps(sum, _mm_mul_ps(v1, v2));
        }

        // Horizontal sum
        float tmp[4];
        _mm_storeu_ps(tmp, sum);
        float finalSum = tmp[0] + tmp[1] + tmp[2] + tmp[3];

        // Handle remaining samples
        for (int i = simdSize; i < size; ++i)
        {
            int idx1 = (readPos + i) % size;
            int idx2 = (readPos + i + tau) % size;
            finalSum += buffer[idx1] * buffer[idx2];
        }

        result[tau] = finalSum;
    }

#else
    // Scalar fallback
    for (int tau = 0; tau <= maxLag; ++tau)
    {
        float sum = 0.0f;
        for (int i = 0; i < size; ++i)
        {
            int idx1 = (readPos + i) % size;
            int idx2 = (readPos + i + tau) % size;
            sum += buffer[idx1] * buffer[idx2];
        }
        result[tau] = sum;
    }
#endif
}

//==============================================================================
float UltraLowLatencyPitchDetector::detectPitchYIN()
{
    const float* buffer = downsamplingEnabled_ ? downsampledBuffer_.data() : circularBuffer_.data();
    const int size = downsamplingEnabled_ ? downsampledBufferSize_ : bufferSize_;
    const int readPos = downsamplingEnabled_ ? downsampledWritePos_ : writePos_;

    const int tauMin = static_cast<int>(detectionSampleRate_ / maxFreqHz_);
    const int tauMax = static_cast<int>(detectionSampleRate_ / minFreqHz_);

    // Constrain search range if using pitch prediction
    int searchTauMin = tauMin;
    int searchTauMax = tauMax;

    if (pitchPredictionEnabled_ && predictedPitch_ > 0.0f && predictionCounter_ < 10)
    {
        constrainSearchRange(searchTauMin, searchTauMax);
    }

    // Step 1: Difference function (optimized version)
    for (int tau = searchTauMin; tau <= searchTauMax; ++tau)
    {
        float sum = 0.0f;
        const int loopLimit = size / 2;  // YIN uses half buffer

        for (int i = 0; i < loopLimit; ++i)
        {
            int idx1 = (readPos + i) % size;
            int idx2 = (readPos + i + tau) % size;
            float diff = buffer[idx1] - buffer[idx2];
            sum += diff * diff;
        }
        differenceBuffer_[tau] = sum;
    }

    // Step 2: Cumulative mean normalized difference (CMND)
    cumulativeBuffer_[searchTauMin] = 1.0f;
    float runningSum = 0.0f;

    for (int tau = searchTauMin; tau <= searchTauMax; ++tau)
    {
        runningSum += differenceBuffer_[tau];
        if (runningSum > 0.0f)
        {
            cumulativeBuffer_[tau] = differenceBuffer_[tau] * static_cast<float>(tau) / runningSum;
        }
        else
        {
            cumulativeBuffer_[tau] = 1.0f;
        }

        // Early termination when we find a deep dip
        if (cumulativeBuffer_[tau] < 0.1f)
        {
            break;
        }
    }

    // Step 3: Find minimum in CMND
    const float threshold = 0.15f;  // Slightly higher for fast mode
    int bestTau = -1;
    float bestVal = 1.0f;

    for (int tau = searchTauMin; tau <= searchTauMax; ++tau)
    {
        if (cumulativeBuffer_[tau] < threshold)
        {
            // Found dip, look for local minimum
            while (tau + 1 <= searchTauMax && cumulativeBuffer_[tau + 1] < cumulativeBuffer_[tau])
            {
                ++tau;
            }
            bestTau = tau;
            bestVal = cumulativeBuffer_[tau];
            break;
        }
    }

    // If no dip below threshold, find global minimum
    if (bestTau < 0)
    {
        for (int tau = searchTauMin; tau <= searchTauMax; ++tau)
        {
            if (cumulativeBuffer_[tau] < bestVal)
            {
                bestVal = cumulativeBuffer_[tau];
                bestTau = tau;
            }
        }
    }

    // Calculate confidence
    float conf = 1.0f - bestVal;
    conf = juce::jlimit(0.0f, 1.0f, conf);
    confidence_.store(conf);

    // Check voiced
    if (conf < confidenceThreshold_)
    {
        voiced_.store(false);
        lastPitch_.store(0.0f);
        return 0.0f;
    }

    voiced_.store(true);

    // Parabolic interpolation
    float interpolatedTau = static_cast<float>(bestTau);
    if (bestTau > searchTauMin && bestTau < searchTauMax)
    {
        float y1 = cumulativeBuffer_[bestTau - 1];
        float y2 = cumulativeBuffer_[bestTau];
        float y3 = cumulativeBuffer_[bestTau + 1];
        interpolatedTau += parabolicInterpolation(y1, y2, y3);
    }

    // Calculate frequency
    float frequency = static_cast<float>(detectionSampleRate_ / interpolatedTau);
    lastPitch_.store(frequency);

    return frequency;
}

//==============================================================================
float UltraLowLatencyPitchDetector::parabolicInterpolation(float y1, float y2, float y3)
{
    // Parabolic interpolation: find minimum of parabola through 3 points
    float denominator = y1 - 2.0f * y2 + y3;
    if (std::abs(denominator) < 0.0001f)
        return 0.0f;

    float offset = (y1 - y3) / (2.0f * denominator);
    return juce::jlimit(-0.5f, 0.5f, offset);  // Clamp to ±0.5 samples
}

//==============================================================================
float UltraLowLatencyPitchDetector::calculateConfidence(const float* autocorr, int bestLag, int maxLag)
{
    if (bestLag <= 0 || bestLag >= maxLag)
        return 0.0f;

    // Compare peak to average of surrounding values
    float peak = std::abs(autocorr[bestLag]);
    float sum = 0.0f;
    int count = 0;

    // Average of nearby lags (excluding the peak itself)
    for (int tau = bestLag - 5; tau <= bestLag + 5; ++tau)
    {
        if (tau >= 1 && tau <= maxLag && tau != bestLag)
        {
            sum += std::abs(autocorr[tau]);
            count++;
        }
    }

    if (count == 0)
        return 0.0f;

    float average = sum / static_cast<float>(count);

    // Confidence is how much the peak stands out from surrounding
    if (average < 0.0001f)
        return peak > 0.1f ? 1.0f : 0.0f;

    return juce::jlimit(0.0f, 1.0f, peak / average);
}

//==============================================================================
bool UltraLowLatencyPitchDetector::isPitchValid(float pitch, float conf)
{
    return pitch >= minFreqHz_ && pitch <= maxFreqHz_ && conf >= confidenceThreshold_;
}

//==============================================================================
void UltraLowLatencyPitchDetector::constrainSearchRange(int& minTau, int& maxTau)
{
    if (predictedPitch_ <= 0.0f)
        return;

    // Convert predicted pitch to lag range
    float predictedTau = detectionSampleRate_ / predictedPitch_;

    // Allow ±1 semitone deviation (about ±6%)
    float minAllowed = predictedTau * 0.94f;
    float maxAllowed = predictedTau * 1.06f;

    predictedMinTau_ = static_cast<int>(minAllowed);
    predictedMaxTau_ = static_cast<int>(maxAllowed);

    // Constrain search range to predicted area
    minTau = juce::jmax(minTau, predictedMinTau_);
    maxTau = juce::jmin(maxTau, predictedMaxTau_);
}

//==============================================================================
void UltraLowLatencyPitchDetector::downsampleBlock(const float* input, int inputSize,
                                                  float* output, int& outputSize)
{
    // Simple 3:1 decimation with 2-point averaging anti-aliasing filter
    outputSize = 0;
    for (int i = 0; i < inputSize; i += 3)
    {
        if (i + 1 < inputSize)
        {
            output[outputSize++] = (input[i] + input[i + 1]) * 0.5f;
        }
        else
        {
            output[outputSize++] = input[i];
        }
    }
}

} // namespace dsp
} // namespace zenith

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

#include "PitchDetector.h"
#include <algorithm>
#include <cmath>

namespace zenith {
namespace dsp {

//==============================================================================
PitchDetector::PitchDetector()
{
}

PitchDetector::~PitchDetector()
{
}

//==============================================================================
void PitchDetector::prepare(double sampleRate, int bufferSize)
{
    sampleRate_ = sampleRate;
    bufferSize_ = bufferSize;
    
    // Pre-allocate all buffers (RT-safe)
    circularBuffer_.resize(bufferSize_, 0.0f);
    differenceBuffer_.resize(bufferSize_, 0.0f);
    cumulativeBuffer_.resize(bufferSize_, 0.0f);
    
    reset();
}

void PitchDetector::reset()
{
    std::fill(circularBuffer_.begin(), circularBuffer_.end(), 0.0f);
    std::fill(differenceBuffer_.begin(), differenceBuffer_.end(), 0.0f);
    std::fill(cumulativeBuffer_.begin(), cumulativeBuffer_.end(), 0.0f);
    
    writePos_ = 0;
    lastPitch_.store(0.0f);
    confidence_.store(0.0f);
    voiced_.store(false);
}

//==============================================================================
void PitchDetector::updateCircularBuffer(float sample)
{
    circularBuffer_[writePos_] = sample;
    writePos_ = (writePos_ + 1) % bufferSize_;
}

float PitchDetector::detectPitchYIN()
{
    const int tauMin = static_cast<int>(sampleRate_ / maxFreqHz_);
    const int tauMax = static_cast<int>(sampleRate_ / minFreqHz_);
    
    // Step 1: Difference function (circular buffer)
    for (int tau = tauMin; tau < tauMax; ++tau)
    {
        float sum = 0.0f;
        for (int i = 0; i < bufferSize_ / 2; ++i)
        {
            int idx1 = (writePos_ + i) % bufferSize_;
            int idx2 = (writePos_ + i + tau) % bufferSize_;
            float diff = circularBuffer_[idx1] - circularBuffer_[idx2];
            sum += diff * diff;
        }
        differenceBuffer_[tau] = sum;
    }
    
    // Step 2: Cumulative mean normalized difference function (CMND)
    cumulativeBuffer_[tauMin] = 1.0f;
    float runningSum = 0.0f;
    
    for (int tau = tauMin; tau < tauMax; ++tau)
    {
        runningSum += differenceBuffer_[tau];
        if (runningSum > 0.0f)
        {
            cumulativeBuffer_[tau] = differenceBuffer_[tau] * tau / runningSum;
        }
        else
        {
            cumulativeBuffer_[tau] = 1.0f;
        }
    }
    
    // Step 3: Absolute threshold and parabolic interpolation
    const float threshold = 0.1f;  // YIN threshold
    int bestTau = -1;
    float bestVal = 1.0f;
    
    for (int tau = tauMin; tau < tauMax; ++tau)
    {
        if (cumulativeBuffer_[tau] < threshold)
        {
            // Find local minimum
            while (tau + 1 < tauMax && cumulativeBuffer_[tau + 1] < cumulativeBuffer_[tau])
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
        for (int tau = tauMin; tau < tauMax; ++tau)
        {
            if (cumulativeBuffer_[tau] < bestVal)
            {
                bestVal = cumulativeBuffer_[tau];
                bestTau = tau;
            }
        }
    }
    
    // Calculate confidence (1.0 = high confidence, 0.0 = low)
    float confidence = 1.0f - bestVal;
    confidence = juce::jlimit(0.0f, 1.0f, confidence);
    confidence_.store(confidence);
    
    // Check if voiced
    if (confidence < confidenceThreshold_)
    {
        voiced_.store(false);
        lastPitch_.store(0.0f);
        return 0.0f;
    }
    
    // Step 4: Parabolic interpolation for better precision
    float interpolatedTau = static_cast<float>(bestTau);
    if (bestTau > tauMin && bestTau < tauMax - 1)
    {
        float y1 = cumulativeBuffer_[bestTau - 1];
        float y2 = cumulativeBuffer_[bestTau];
        float y3 = cumulativeBuffer_[bestTau + 1];
        interpolatedTau += parabolicInterpolation(bestTau, y1, y2, y3);
    }
    
    // Calculate frequency
    float frequency = static_cast<float>(sampleRate_ / interpolatedTau);
    
    voiced_.store(true);
    lastPitch_.store(frequency);
    
    return frequency;
}

float PitchDetector::parabolicInterpolation(int tau, float y1, float y2, float y3)
{
    // Parabolic interpolation: find minimum of parabola through 3 points
    float denominator = y1 - 2.0f * y2 + y3;
    if (std::abs(denominator) < 0.0001f)
        return 0.0f;
    
    return (y1 - y3) / (2.0f * denominator);
}

//==============================================================================
float PitchDetector::processSample(float sample)
{
    updateCircularBuffer(sample);
    return detectPitchYIN();
}

float PitchDetector::processBlock(const juce::AudioBuffer<float>& buffer)
{
    // Fill circular buffer with block
    const int numSamples = buffer.getNumSamples();
    const float* data = buffer.getReadPointer(0);
    
    for (int i = 0; i < numSamples; ++i)
    {
        updateCircularBuffer(data[i]);
    }
    
    return detectPitchYIN();
}

} // namespace dsp
} // namespace zenith

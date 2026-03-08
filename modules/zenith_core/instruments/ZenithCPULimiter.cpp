/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#include "ZenithCPULimiter.h"
#include <algorithm>

namespace zenith {

//==============================================================================
// CPU PERFORMANCE MONITOR
//==============================================================================

CpuPerformanceMonitor::CpuPerformanceMonitor() {
    cpuHistory_.fill(0.0f);
    updateTargetBlockTime();
}

void CpuPerformanceMonitor::startBlock() {
    blockStart_ = std::chrono::high_resolution_clock::now();
}

void CpuPerformanceMonitor::endBlock() {
    auto blockEnd = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = blockEnd - blockStart_;
    double actualTime = elapsed.count();

    updateCpuUsage(actualTime);
}

double CpuPerformanceMonitor::getTimeBudgetRemaining() const {
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = now - blockStart_;
    return targetBlockTime_ - elapsed.count();
}

bool CpuPerformanceMonitor::isOverBudget() const {
    return getTimeBudgetRemaining() < 0.0;
}

void CpuPerformanceMonitor::updateCpuUsage(double actualBlockTime) {
    // Calculate CPU usage ratio
    float usage = static_cast<float>(actualBlockTime / targetBlockTime_);
    usage = juce::jlimit(0.0f, 1.0f, usage);

    // Update history
    cpuHistory_[historyIndex_] = usage;
    historyIndex_ = (historyIndex_ + 1) % NUM_SAMPLES;

    // Calculate moving average
    float sum = 0.0f;
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        sum += cpuHistory_[i];
    }
    float average = sum / NUM_SAMPLES;

    cpuUsage_.store(average);

    // Update peak
    float currentPeak = peakCpuUsage_.load();
    if (usage > currentPeak) {
        peakCpuUsage_.store(usage);
    }

    updateTargetBlockTime();
}

void CpuPerformanceMonitor::updateTargetBlockTime() {
    targetBlockTime_ = expectedBlockSize_ / sampleRate_;
}

//==============================================================================
// CPU LIMITER
//==============================================================================

ZenithCpuLimiter::ZenithCpuLimiter() = default;

bool ZenithCpuLimiter::canStartVoice() {
    if (!voiceStealingEnabled_) {
        return activeVoiceCount_ < maxVoices_;
    }

    return activeVoiceCount_ < currentVoiceLimit_;
}

void ZenithCpuLimiter::updateQuality(float cpuUsage) {
    if (!qualityDropEnabled_) {
        qualityMultiplier_ = 1.0f;
        return;
    }

    float targetQuality = calculateQualityDrop(cpuUsage);

    // Smooth quality changes
    qualityMultiplier_ = qualitySmoothing_ * qualityMultiplier_ +
                       (1.0f - qualitySmoothing_) * targetQuality;
}

void ZenithCpuLimiter::process(float cpuUsage) {
    updateVoiceLimit(cpuUsage);
    updateQuality(cpuUsage);
}

void ZenithCpuLimiter::updateVoiceLimit(float cpuUsage) {
    if (!voiceStealingEnabled_) {
        currentVoiceLimit_ = maxVoices_;
        return;
    }

    // Gradually reduce voices when CPU is high
    float overThreshold = juce::jmax(0.0f, cpuUsage - cpuThreshold_);

    if (overThreshold > 0.0f) {
        // Reduce voice limit
        float reductionFactor = 1.0f - (overThreshold * 2.0f);
        int newLimit = static_cast<int>(maxVoices_ * reductionFactor);
        currentVoiceLimit_ = juce::jmax(4, newLimit);
    } else {
        // Restore voice limit gradually
        currentVoiceLimit_ = juce::jmin(maxVoices_, currentVoiceLimit_ + 1);
    }
}

float ZenithCpuLimiter::calculateQualityDrop(float cpuUsage) {
    // Quality drops when CPU exceeds threshold
    float overThreshold = juce::jmax(0.0f, cpuUsage - cpuThreshold_);

    if (overThreshold <= 0.0f) {
        return 1.0f;
    }

    // Drop quality gradually
    float qualityDrop = juce::jmin(0.5f, overThreshold * 2.0f);
    return 1.0f - qualityDrop;
}

} // namespace zenith

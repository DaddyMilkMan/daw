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

#pragma once

#include "ZenithPolySynthDefs.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <chrono>

namespace zenith {

//==============================================================================
// CPU LIMIT STRATEGY
//==============================================================================
enum class CPULimitStrategy {
    None,           ///< No limiting
    VoiceStealing,   ///< Steal voices when CPU high
    QualityDrop,     ///< Reduce quality when CPU high
    Both,           ///< Both voice stealing and quality drop
    Dynamic          ///< Adaptive based on profiling
};

//==============================================================================
// CPU PERFORMANCE MONITOR
//==============================================================================
/**
 * Real-time CPU load monitor for audio processing
 *
 * FEATURES:
 * - Block time measurement
 * - Moving average CPU percentage
 * - Peak detection
 * - RT-safe reporting
 */
class CpuPerformanceMonitor {
public:
    CpuPerformanceMonitor();
    ~CpuPerformanceMonitor() = default;

    //==========================================================================
    // Configuration
    //==========================================================================

    /**
     * @brief Set sample rate
     */
    void setSampleRate(double sr) { sampleRate_ = sr; }

    /**
     * @brief Set expected block size
     */
    void setExpectedBlockSize(int size) { expectedBlockSize_ = size; }

    //==========================================================================
    // Monitoring
    //==========================================================================

    /**
     * @brief Start CPU measurement for a block
     * Call this at the start of your audio callback
     */
    void startBlock();

    /**
     * @brief End CPU measurement for a block
     * Call this at the end of your audio callback
     */
    void endBlock();

    /**
     * @brief Get current CPU usage (0-1)
     */
    float getCpuUsage() const { return cpuUsage_.load(); }

    /**
     * @brief Get peak CPU usage since last reset
     */
    float getPeakCpuUsage() const { return peakCpuUsage_.load(); }

    /**
     * @brief Reset peak CPU usage
     */
    void resetPeak() { peakCpuUsage_.store(0.0f); }

    //==========================================================================
    // Time Budget
    //==========================================================================

    /**
     * @brief Get remaining time budget for current block (seconds)
     */
    double getTimeBudgetRemaining() const;

    /**
     * @brief Check if we're over budget
     */
    bool isOverBudget() const;

private:
    double sampleRate_ = 44100.0;
    int expectedBlockSize_ = 512;

    std::atomic<float> cpuUsage_{0.0f};
    std::atomic<float> peakCpuUsage_{0.0f};

    // Moving average filter
    static constexpr int NUM_SAMPLES = 32;
    float cpuHistory_[NUM_SAMPLES] = {0};
    int historyIndex_ = 0;

    // Timing
    std::chrono::high_resolution_clock::time_point blockStart_;
    double targetBlockTime_ = 0.0;

    //==========================================================================
    // Internal Helpers
    //==========================================================================

    void updateCpuUsage(double actualBlockTime);
};

//==============================================================================
// CPU LIMITER
//==============================================================================
/**
 * Adaptive CPU limiter for polyphonic synth
 *
 * FEATURES:
 * - Voice stealing when CPU high
 * - Quality degradation when CPU high
 * - Configurable thresholds
 * - Smooth transitions
 */
class ZenithCpuLimiter {
public:
    ZenithCpuLimiter();
    ~ZenithCpuLimiter() = default;

    //==========================================================================
    // Configuration
    //==========================================================================

    /**
     * @brief Set CPU limit threshold (0.0-1.0)
     * @param threshold Threshold (e.g., 0.80 = 80% CPU)
     */
    void setCpuThreshold(float threshold) {
        cpuThreshold_ = juce::jlimit(0.0f, 1.0f, threshold);
    }

    /**
     * @brief Set CPU limit strategy
     */
    void setLimitStrategy(CPULimitStrategy strategy) {
        strategy_ = strategy;
    }

    /**
     * @brief Enable/disable voice stealing
     */
    void setVoiceStealingEnabled(bool enabled) {
        voiceStealingEnabled_ = enabled;
    }

    /**
     * @brief Enable/disable quality drop
     */
    void setQualityDropEnabled(bool enabled) {
        qualityDropEnabled_ = enabled;
    }

    //==========================================================================
    // Voice Management
    //==========================================================================

    /**
     * @brief Check if a new voice should be started
     * @return True if voice can start
     */
    bool canStartVoice();

    /**
     * @brief Get current voice limit
     */
    int getVoiceLimit() const { return currentVoiceLimit_; }

    /**
     * @brief Set maximum voices
     */
    void setMaxVoices(int maxVoices) {
        maxVoices_ = juce::jlimit(1, 256, maxVoices);
    }

    //==========================================================================
    // Quality Management
    //==========================================================================

    /**
     * @brief Get current quality multiplier (0.5-1.0)
     */
    float getQualityMultiplier() const { return qualityMultiplier_; }

    /**
     * @brief Update quality based on CPU usage
     * @param cpuUsage Current CPU usage (0-1)
     */
    void updateQuality(float cpuUsage);

    //==========================================================================
    // Processing
    //==========================================================================

    /**
     * @brief Update CPU limiter state
     * @param cpuUsage Current CPU usage from monitor
     */
    void process(float cpuUsage);

private:
    // Configuration
    float cpuThreshold_ = 0.85f;
    CPULimitStrategy strategy_ = CPULimitStrategy::Both;
    bool voiceStealingEnabled_ = true;
    bool qualityDropEnabled_ = true;

    // Voice management
    int maxVoices_ = 16;
    int currentVoiceLimit_ = 16;
    int activeVoiceCount_ = 0;

    // Quality management
    float qualityMultiplier_ = 1.0f;
    float qualitySmoothing_ = 0.995f;

    //==========================================================================
    // Internal Helpers
    //==========================================================================

    void updateVoiceLimit(float cpuUsage);
    float calculateQualityDrop(float cpuUsage);
};

} // namespace zenith

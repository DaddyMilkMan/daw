/**
 * @file SkiaPerformanceDashboard.h
 * @brief GPU-rendered performance monitoring dashboard
 *
 * Features:
 * - Real-time FPS monitoring with history graph
 * - Frame time visualization
 * - Memory usage tracking
 * - Component count and rendering statistics
 * - Adaptive FPS indicator
 */

#pragma once

#include <JuceHeader.h>
#include <deque>

#ifdef ZENITH_USE_SKIA

#include "SkiaTheme.h"

namespace zenith {

/**
 * @class SkiaPerformanceDashboard
 * @brief Real-time performance monitoring and visualization
 */
class SkiaPerformanceDashboard : public juce::Component, private juce::Timer
{
public:
    SkiaPerformanceDashboard();
    ~SkiaPerformanceDashboard() override;

    /**
     * @brief Record a frame time measurement
     */
    void recordFrameTime(float milliseconds);

    /**
     * @brief Update memory usage (in MB)
     */
    void setMemoryUsage(float megabytes);

    /**
     * @brief Set rendered component count
     */
    void setComponentCount(int count);

    /**
     * @brief Get average frame time (ms)
     */
    float getAverageFrameTime() const;

    /**
     * @brief Get peak frame time (ms)
     */
    float getPeakFrameTime() const;

    /**
     * @brief Get current FPS
     */
    float getCurrentFPS() const;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    void renderMetrics(juce::Graphics& g);
    void renderFrameTimeGraph(juce::Graphics& g);
    void renderMemoryUsage(juce::Graphics& g);

    // Frame time history (stores last 120 frames)
    std::deque<float> frameTimeHistory_;
    static constexpr int MAX_HISTORY = 120;

    // Metrics
    float memoryUsage_ = 0.0f;
    int componentCount_ = 0;
    float averageFrameTime_ = 0.0f;
    float peakFrameTime_ = 0.0f;
    float currentFPS_ = 60.0f;

    // Smoothing
    float smoothedFrameTime_ = 16.67f; // ~60 FPS default

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaPerformanceDashboard)
};

}

#endif // ZENITH_USE_SKIA

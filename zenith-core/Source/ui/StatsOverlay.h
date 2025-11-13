/**
 * @file StatsOverlay.h
 * @brief DEBUG-only frame-time telemetry HUD (W6: Windows Performance Focus)
 *
 * W6 Features:
 * - Zero heap allocations in paint() (cached fonts, fixed-size ring buffers)
 * - Rolling 1s window for paints/sec and frame time stats (avg/min/max)
 * - TrackView-specific paint cost tracking
 * - Throttled updates (~15 Hz) to minimize overhead
 * - Toggle with Ctrl+F10 (default ON in Debug, OFF in Release)
 *
 * Performance Requirements:
 * - No dynamic allocations on hot path
 * - Minimal overhead (<1% of frame time)
 * - Translucent overlay in top-right corner
 */

#pragma once

#include <JuceHeader.h>
#include <array>
#include <cstddef>

// Forward declaration
class Engine;

//==============================================================================
/**
 * @class StatsOverlay
 * @brief Lightweight performance monitoring HUD for DEBUG builds
 *
 * W6 Design:
 * - Fixed-size circular buffer for frame timing (120 samples = ~2s @ 60fps)
 * - Dirty-checking to avoid unnecessary repaints
 * - Timer at ~15 Hz for stat updates (not paint-driven)
 */
class StatsOverlay : public juce::Component,
                     private juce::Timer
{
public:
    //==========================================================================
    explicit StatsOverlay(Engine& engine);
    ~StatsOverlay() override = default;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Public API (called by MainComponent or TrackView)
    //==========================================================================

    /**
     * @brief Record a paint event (call at end of paint())
     * @param componentName Name of component that painted (e.g., "TrackView")
     * @param paintTimeMs Time taken to paint in milliseconds
     */
    void recordPaint(const char* componentName, double paintTimeMs);

    /**
     * @brief Update TrackView-specific stats (tracks/clips painted)
     * @param visibleTracks Number of tracks painted in last frame
     * @param visibleClips Number of clips painted in last frame
     */
    void updateTrackViewStats(int visibleTracks, int visibleClips);

    /**
     * @brief Set visibility (Ctrl+F10 toggle)
     */
    void setOverlayVisible(bool shouldBeVisible);

private:
    //==========================================================================
    // Timer callback (~15 Hz)
    //==========================================================================

    void timerCallback() override;

    //==========================================================================
    // Stats calculation (zero allocations)
    //==========================================================================

    /**
     * @brief Calculate stats for last 1s window
     * @param outPaintsPerSec Number of paints in last second
     * @param outAvgMs Average frame time
     * @param outMinMs Minimum frame time
     * @param outMaxMs Maximum frame time
     */
    void calculateStats(int& outPaintsPerSec,
                       double& outAvgMs,
                       double& outMinMs,
                       double& outMaxMs) const;

    //==========================================================================
    // Ring buffer for frame timing (W6: fixed-size, zero allocations)
    //==========================================================================

    struct FrameSample
    {
        double timestampMs = 0.0;  // juce::Time::getMillisecondCounterHiRes()
        double paintTimeMs = 0.0;  // Duration of paint
    };

    static constexpr size_t RING_BUFFER_SIZE = 120;  // ~2s @ 60fps
    std::array<FrameSample, RING_BUFFER_SIZE> frameBuffer;
    size_t bufferHead = 0;  // Next write position
    size_t bufferCount = 0; // Number of valid samples

    //==========================================================================
    // TrackView-specific stats
    //==========================================================================

    double lastTrackViewPaintMs = 0.0;
    int lastVisibleTracks = 0;
    int lastVisibleClips = 0;

    //==========================================================================
    // Cached display values (dirty-check to avoid repaint churn)
    //==========================================================================

    int displayPaintsPerSec = 0;
    double displayAvgMs = 0.0;
    double displayMinMs = 0.0;
    double displayMaxMs = 0.0;

    //==========================================================================
    // W6: Cached resources (zero allocations in paint)
    //==========================================================================

    juce::Font labelFont {11.0f, juce::Font::bold};
    juce::Font valueFont {10.0f};

    // Pre-allocated strings (updated in timerCallback, not paint)
    juce::String cachedStatsText;

    //==========================================================================
    // W13.1 Debug HUD: Engine reference for audio stats
    //==========================================================================

    Engine& engine_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StatsOverlay)
};

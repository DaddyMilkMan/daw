/**
 * @file Phase1DebugOverlay.h
 * @brief Real-time debug HUD for Phase 1 audio engine
 *
 * Displays transport, tracks, voices, scheduler stats, and peak levels.
 * Toggle with F12 key. Semi-transparent, non-interactive overlay.
 *
 * Only available when ZENITH_ENABLE_PHASE1_AUDIO=ON and JUCE_DEBUG=1.
 */

#pragma once

#include <JuceHeader.h>

#if defined(ZENITH_ENABLE_PHASE1_AUDIO) && ZENITH_ENABLE_PHASE1_AUDIO && JUCE_DEBUG

#include "../../include/Engine.h"

//==============================================================================
/**
 * @class Phase1DebugOverlay
 * @brief Lightweight debug HUD showing Phase 1 engine metrics
 *
 * Features:
 * - Transport position (samples + seconds)
 * - Play/stop state
 * - Track count
 * - Active voice count
 * - Scheduler stats (queued events, dropped events)
 * - Output peak meters (L/R in dB)
 *
 * Updates at ~20 FPS via Timer.
 * Non-interactive (click-through).
 * Semi-transparent background.
 */
class Phase1DebugOverlay : public juce::Component,
                           private juce::Timer
{
public:
    //==========================================================================
    /**
     * @brief Construct debug overlay
     * @param engine Reference to Engine for metrics polling
     */
    explicit Phase1DebugOverlay(Engine& engine);

    ~Phase1DebugOverlay() override;

    //==========================================================================
    // Component overrides
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    //==========================================================================
    // Timer callback (polls metrics at ~20 FPS)
    //==========================================================================

    void timerCallback() override;

    //==========================================================================
    // Helper Methods
    //==========================================================================

    /** Convert linear peak to dB string. */
    static juce::String peakToDbString(float peak);

    /** Convert samples to time string (mm:ss.mmm). */
    static juce::String samplesToTimeString(int64_t samples, double sampleRate);

    //==========================================================================
    // Member Variables
    //==========================================================================

    Engine& engine_;
    Engine::Phase1DebugMetrics metrics_;

    // Font for rendering
    juce::Font monoFont_{juce::FontOptions(12.0f, juce::Font::plain)};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Phase1DebugOverlay)
};

#endif // ZENITH_ENABLE_PHASE1_AUDIO && JUCE_DEBUG

/**
 * @file MasterOutputComponent.h
 * @brief Master output fader with metering and Apple-like design
 *
 * Features:
 * - Vertical master fader (-∞ to +12 dB)
 * - Real-time output level meters (animated)
 * - Peak level indicator with hold time
 * - Headroom indicator
 * - Smooth JUCE 8 animations with easing
 * - Apple-inspired flat design with smooth colors
 *
 * @phase Phase 11: Master Output Control
 */

#pragma once

#include <JuceHeader.h>

class Engine;

namespace zenith {

/**
 * @class MasterOutputComponent
 * @brief Master output control with real-time metering and animations
 */
class MasterOutputComponent : public juce::Component,
                              public juce::Timer
{
public:
    //==========================================================================
    explicit MasterOutputComponent(Engine& eng);
    ~MasterOutputComponent() override;

    //==========================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    //==========================================================================
    // Methods
    //==========================================================================

    /**
     * @brief Get current master gain in dB
     */
    float getMasterGaindB() const { return masterGaindB_; }

    /**
     * @brief Set master gain in dB
     * @param gaindB Master gain (-∞ to +12 dB)
     */
    void setMasterGaindB(float gaindB) [[maybe_unused]];

    /**
     * @brief Reset peak meters
     */
    void resetPeaks();

private:
    //==========================================================================
    // Helpers
    //==========================================================================

    /**
     * @brief Convert dB to linear gain
     */
    static float dbToGain(float db);

    /**
     * @brief Convert linear gain to dB
     */
    static float gainToDb(float gain);

    /**
     * @brief Paint master fader
     */
    void paintMasterFader(juce::Graphics& g, const juce::Rectangle<int>& bounds);

    /**
     * @brief Paint level meters with animation
     */
    void paintLevelMeters(juce::Graphics& g, const juce::Rectangle<int>& bounds);

    /**
     * @brief Paint peak indicators
     */
    void paintPeakIndicators(juce::Graphics& g, const juce::Rectangle<int>& bounds);

    /**
     * @brief Update animated meter levels
     */
    void updateAnimatedLevels();

    /**
     * @brief Handle mouse interaction with fader
     */
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    //==========================================================================
    // Members
    //==========================================================================

    Engine& engine_;

    // Master gain in dB (-∞ to +12)
    float masterGaindB_ = 0.0f;

    // Animated meter levels (for smooth visual feedback)
    float animatedCurrentLevel_ = 0.0f;
    float animatedPeakLevel_ = 0.0f;

    // Peak meter hold time
    int peakHoldFrames_ = 0;
    static constexpr int PEAK_HOLD_TIME_FRAMES = 60;  // ~1 second at 60 Hz

    // Mouse interaction
    bool isDraggingFader_ = false;
    int faderStartY_ = 0;
    float faderStartValue_ = 0.0f;

    // Layout constants
    static constexpr int FADER_WIDTH = 40;
    static constexpr int METER_WIDTH = 16;
    static constexpr int SPACING = 12;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterOutputComponent)
};

}  // namespace zenith


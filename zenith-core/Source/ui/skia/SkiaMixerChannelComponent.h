/**
 * @file SkiaMixerChannelComponent.h
 * @brief GPU-rendered mixer channel with Skia
 *
 * Features:
 * - Fader slider with smooth spring physics
 * - Pan knob with velocity feedback
 * - Volume meter with peak detection
 * - Mute/Solo buttons with LED indicators
 * - Input meter visualization
 */

#pragma once

#include <JuceHeader.h>

#ifdef ZENITH_USE_SKIA
    #include <include/core/SkCanvas.h>
    #include <include/core/SkPaint.h>
#endif

#include "SkiaTheme.h"
#include "SkiaTextRenderer.h"

namespace zenith {

#ifdef ZENITH_USE_SKIA

/**
 * @class SkiaMixerChannelComponent
 * @brief GPU-rendered mixer channel
 */
class SkiaMixerChannelComponent : public juce::Component,
                                  private juce::Timer
{
public:
    SkiaMixerChannelComponent();
    ~SkiaMixerChannelComponent() override;

    //==========================================================================
    // Value accessors
    //==========================================================================

    /**
     * @brief Set the fader value (0-1)
     */
    void setFaderValue(float value);

    /**
     * @brief Get the fader value
     */
    float getFaderValue() const { return faderValue_; }

    /**
     * @brief Set the pan value (-1 to 1, 0 = center)
     */
    void setPanValue(float value);

    /**
     * @brief Get the pan value
     */
    float getPanValue() const { return panValue_; }

    /**
     * @brief Set mute state
     */
    void setMuted(bool muted);

    /**
     * @brief Get mute state
     */
    bool isMuted() const { return isMuted_; }

    /**
     * @brief Set solo state
     */
    void setSolo(bool solo);

    /**
     * @brief Get solo state
     */
    bool isSolo() const { return isSolo_; }

    /**
     * @brief Set the input level (0-1 for meter display)
     */
    void setInputLevel(float level);

    /**
     * @brief Set the channel name
     */
    void setChannelName(const juce::String& name);

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

private:
    //==========================================================================
    // Timer callback for animations
    //==========================================================================

    void timerCallback() override;

    //==========================================================================
    // Rendering
    //==========================================================================

    void renderFader();
    void renderPanKnob();
    void renderMeterDisplay();
    void renderButtons();
    void renderLabel();

    //==========================================================================
    // Hit testing
    //==========================================================================

    enum class HitTarget
    {
        None,
        Fader,
        Pan,
        Mute,
        Solo
    };

    HitTarget getHitTarget(const juce::Point<int>& pos) const;

    //==========================================================================
    // Member variables
    //==========================================================================

    float faderValue_ = 0.7f;
    float faderTarget_ = 0.7f;
    float panValue_ = 0.0f;
    float panTarget_ = 0.0f;
    bool isMuted_ = false;
    bool isSolo_ = false;
    float inputLevel_ = 0.0f;
    float inputLevelSmoothed_ = 0.0f;
    float peakLevel_ = 0.0f;
    juce::String channelName_ = "Channel";

    // Animation states
    bool isDraggingFader_ = false;
    bool isDraggingPan_ = false;
    HitTarget currentHitTarget_ = HitTarget::None;

    // Meters
    struct MeterState
    {
        float current = 0.0f;
        float peak = 0.0f;
        int peakHoldSamples = 0;
    } meter_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaMixerChannelComponent)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith

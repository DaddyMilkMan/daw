/**
 * @file SkiaToggleButton.h
 * @brief Beautiful GPU-accelerated toggle button/checkbox with Skia
 */

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>

#ifdef ZENITH_USE_SKIA
    #include "SkiaComponent.h"
    #include "SkiaTheme.h"
    #include <include/core/SkCanvas.h>
    #include <include/core/SkPaint.h>
    #include <include/core/SkRRect.h>
    #include <functional>
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

/**
 * @class SkiaToggleButton
 * @brief GPU-accelerated toggle button with smooth animations
 */
class SkiaToggleButton : public juce::Component, public SkiaComponent, private juce::Timer
{
public:
    SkiaToggleButton(const juce::String& labelText = {})
        : label_(labelText)
        , toggleState_(false)
        , animationProgress_(0.0f)
        , animationVelocity_(0.0f)
    {
        setOpaque(false);
        startTimer(16); // 60 FPS
    }

    ~SkiaToggleButton() override { stopTimer(); }

    void setToggleState(bool newState, bool sendNotification = true)
    {
        if (toggleState_ != newState)
        {
            toggleState_ = newState;
            if (sendNotification && onStateChange)
                onStateChange(toggleState_);
            repaint();
        }
    }

    bool getToggleState() const { return toggleState_; }

    void setButtonText(const juce::String& text)
    {
        label_ = text;
        repaint();
    }

    std::function<void(bool)> onStateChange;

    //==========================================================================
    // Component overrides
    //==========================================================================

    void paint(juce::Graphics& g) override { juce::ignoreUnused(g); }

    void mouseDown(const juce::MouseEvent&) override
    {
        setToggleState(!toggleState_, true);
    }

    //==========================================================================
    // SkiaComponent implementation
    //==========================================================================

    bool supportsSkiaRendering() const override { return true; }

    void paintToSkia(SkCanvas* canvas, SkRect bounds) override
    {
        const auto& theme = SkiaTheme::getInstance();
        const auto& colors = theme.getColors();

        // Checkbox area (left side)
        float checkboxSize = 18.0f;
        SkRect checkboxBounds = SkRect::MakeXYWH(
            bounds.x() + 4,
            bounds.centerY() - checkboxSize / 2.0f,
            checkboxSize,
            checkboxSize
        );

        // Checkbox background
        SkPaint bgPaint;
        bgPaint.setAntiAlias(true);
        bgPaint.setColor(toggleState_ ? colors.primary : colors.surfaceDefault);

        SkRRect checkboxRRect = SkRRect::MakeRectXY(checkboxBounds, 3.0f, 3.0f);
        canvas->drawRRect(checkboxRRect, bgPaint);

        // Border
        SkPaint borderPaint;
        borderPaint.setAntiAlias(true);
        borderPaint.setStyle(SkPaint::kStroke_Style);
        borderPaint.setStrokeWidth(1.5f);
        borderPaint.setColor(toggleState_ ? colors.primary : colors.border);
        canvas->drawRRect(checkboxRRect, borderPaint);

        // Checkmark (if toggled)
        if (toggleState_ && animationProgress_ > 0.1f)
        {
            SkPaint checkPaint;
            checkPaint.setAntiAlias(true);
            checkPaint.setStyle(SkPaint::kStroke_Style);
            checkPaint.setStrokeWidth(2.0f);
            checkPaint.setColor(colors.textOnAccent);
            checkPaint.setStrokeCap(SkPaint::kRound_Cap);

            float cx = checkboxBounds.centerX();
            float cy = checkboxBounds.centerY();
            float size = checkboxSize * 0.3f * animationProgress_;

            canvas->drawLine(cx - size, cy, cx - size * 0.3f, cy + size, checkPaint);
            canvas->drawLine(cx - size * 0.3f, cy + size, cx + size, cy - size, checkPaint);
        }

        // Label text
        if (!label_.isEmpty())
        {
            SkiaTextRenderer textRenderer;
            TextRenderOptions textOpts;
            textOpts.color = colors.textPrimary;
            textOpts.antiAlias = true;
            textOpts.effects = TextEffect::None;

            float textX = checkboxBounds.right() + 8.0f;
            float textY = bounds.centerY() + 4.0f;

            textRenderer.drawText(canvas, label_, textX, textY, TextStyle::Regular, textOpts);
        }
    }

private:
    void timerCallback() override
    {
        // Animate checkmark appearance
        const float dt = 0.016f;
        const float stiffness = 400.0f;
        const float damping = 25.0f;

        float target = toggleState_ ? 1.0f : 0.0f;

        if (std::abs(animationProgress_ - target) > 0.001f)
        {
            float force = -stiffness * (animationProgress_ - target) - damping * animationVelocity_;
            animationVelocity_ += force * dt;
            animationProgress_ += animationVelocity_ * dt;
            animationProgress_ = juce::jlimit(0.0f, 1.0f, animationProgress_);
            repaint();
        }
    }

    juce::String label_;
    bool toggleState_;
    float animationProgress_;
    float animationVelocity_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaToggleButton)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith

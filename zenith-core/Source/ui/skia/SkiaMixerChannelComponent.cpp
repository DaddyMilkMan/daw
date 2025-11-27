/**
 * @file SkiaMixerChannelComponent.cpp
<<<<<<< HEAD
 * @brief Logic Pro style mixer channel implementation
=======
 * @brief Implementation of Skia mixer channel component with full GPU rendering
>>>>>>> 9f38c723266f5a7d2dfa29ac0a1897af495d401c
 */

#include "SkiaMixerChannelComponent.h"
#include "SkiaTheme.h"

#ifdef ZENITH_USE_SKIA

#include <include/core/SkCanvas.h>
#include <include/core/SkPaint.h>

namespace zenith {

// Helper to create ARGB color
static constexpr SkColor ARGB(uint8_t a, uint8_t r, uint8_t g, uint8_t b)
{
    return (a << 24) | (r << 16) | (g << 8) | b;
}

SkiaMixerChannelComponent::SkiaMixerChannelComponent()
{
    setSize(80, 400);  // Logic Pro mixer strip width
    startTimer(50);    // 20fps for meter decay and peak hold
}

void SkiaMixerChannelComponent::setChannelName(const juce::String& name)
{
    channelName_ = name;
    repaint();
}

void SkiaMixerChannelComponent::setChannelColor(juce::Colour color)
{
    channelColor_ = color;
    repaint();
}

void SkiaMixerChannelComponent::setChannelType(bool isBus)
{
    isBus_ = isBus;
    repaint();
}

void SkiaMixerChannelComponent::setFaderValue(float db)
{
    faderDb_ = std::max(-60.0f, std::min(6.0f, db));
    repaint();
}

void SkiaMixerChannelComponent::setMeterLevel(float level)
{
    meterLevel_ = std::max(0.0f, std::min(1.0f, level));
    
    // Update peak if needed
    if (meterLevel_ > peakLevel_)
    {
        peakLevel_ = meterLevel_;
        peakHoldTime_ = 2.0f;  // Hold for 2 seconds
    }
    
    repaint();
}

void SkiaMixerChannelComponent::setPeakLevel(float peak)
{
    peakLevel_ = peak;
    peakHoldTime_ = 2.0f;
    repaint();
}

void SkiaMixerChannelComponent::setPan(float pan)
{
    pan_ = std::max(-1.0f, std::min(1.0f, pan));
    repaint();
}

void SkiaMixerChannelComponent::setMuted(bool muted)
{
    isMuted_ = muted;
    repaint();
}

void SkiaMixerChannelComponent::setSoloed(bool soloed)
{
    isSoloed_ = soloed;
    repaint();
}

void SkiaMixerChannelComponent::setRecordEnabled(bool enabled)
{
    isRecordEnabled_ = enabled;
    repaint();
}

void SkiaMixerChannelComponent::setSelected(bool selected)
{
    isSelected_ = selected;
    repaint();
}

void SkiaMixerChannelComponent::paint(juce::Graphics& g)
{
<<<<<<< HEAD
    auto& theme = SkiaTheme::getInstance();
    auto bounds = getLocalBounds();
    
    // Channel strip background (#282828 - Logic Pro mixer strip)
    if (isSelected_)
        g.fillAll(juce::Colour(0xff333333));
    else
        g.fillAll(juce::Colour(0xff282828));
    
    int currentY = 8;
    int centerX = bounds.getWidth() / 2;
    
    // === Channel Name (top) ===
    g.setColour(juce::Colour(0xffDFDFDF));
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText(channelName_, 4, currentY, bounds.getWidth() - 8, 16, 
              juce::Justification::centred);
    currentY += 20;
    
    // === Pan Knob ===
    panKnobBounds_ = juce::Rectangle<int>(centerX - 16, currentY, 32, 32);
    
    // Knob background
    g.setColour(juce::Colour(0xff2E2E2E));
    g.fillEllipse(panKnobBounds_.toFloat());
    
    // Green ring (Logic Pro pan indicator)
    g.setColour(juce::Colour(0xff00FF00));
    g.drawEllipse(panKnobBounds_.toFloat().reduced(2.0f), 3.0f);
    
    // Pan position indicator
    float panAngle = -2.356f + (pan_ + 1.0f) * 2.356f;  // -135° to +135°
    float indicatorLength = 12.0f;
    float endX = panKnobBounds_.getCentreX() + indicatorLength * std::cos(panAngle);
    float endY = panKnobBounds_.getCentreY() + indicatorLength * std::sin(panAngle);
    
    g.setColour(juce::Colour(0xffDFDFDF));
    g.drawLine(panKnobBounds_.getCentreX(), panKnobBounds_.getCentreY(), 
              endX, endY, 3.0f);
    
    currentY += 40;
    
    // === Audio Meter ===
    meterBounds_ = juce::Rectangle<int>(centerX - 8, currentY, 16, 200);
    
    // Meter background (#111111 - Logic Pro meter bg)
    g.setColour(juce::Colour(0xff111111));
    g.fillRect(meterBounds_);
    
    // Meter fill (bottom-up)
    if (meterLevel_ > 0.0f)
    {
        int fillHeight = (int)(meterBounds_.getHeight() * meterLevel_);
        juce::Rectangle<int> fillRect(meterBounds_.getX(), 
                                      meterBounds_.getBottom() - fillHeight,
                                      meterBounds_.getWidth(), fillHeight);
        
        // Color based on level (Logic Pro zones)
        juce::Colour meterColor;
        if (meterLevel_ < 0.75f)
            meterColor = juce::Colour(0xff00FF00);      // Green
        else if (meterLevel_ < 0.9f)
            meterColor = juce::Colour(0xffFFFF00);      // Yellow
        else if (meterLevel_ < 0.95f)
            meterColor = juce::Colour(0xffFF9900);      // Orange
        else
            meterColor = juce::Colour(0xffFF0000);      // Red
        
        g.setColour(meterColor);
        g.fillRect(fillRect);
    }
    
    // Peak indicator (2-second hold)
    if (peakLevel_ > 0.0f && peakHoldTime_ > 0.0f)
    {
        int peakY = meterBounds_.getBottom() - (int)(meterBounds_.getHeight() * peakLevel_);
        g.setColour(juce::Colour(0xffFFFFFF));
        g.fillRect(meterBounds_.getX(), peakY - 1, meterBounds_.getWidth(), 2);
    }
    
    // Meter border
    g.setColour(juce::Colour(0xff666666));
    g.drawRect(meterBounds_);
    
    currentY = meterBounds_.getBottom() + 12;
    
    // === Fader Track ===
    int faderTrackWidth = 6;
    faderTrackBounds_ = juce::Rectangle<int>(centerX - faderTrackWidth/2, currentY, 
                                             faderTrackWidth, 80);
    
    // Fader groove (with inset shadow effect)
    g.setColour(juce::Colour(0xff1A1A1A));
    g.fillRoundedRectangle(faderTrackBounds_.toFloat(), 3.0f);
    
    // Darker edge for groove effect
    g.setColour(juce::Colour(0xff000000));
    g.drawRoundedRectangle(faderTrackBounds_.toFloat(), 3.0f, 1.0f);
    
    // === Fader Cap (chrome/silver) ===
    float faderNormalized = (faderDb_ + 60.0f) / 66.0f;  // -60 to +6 dB
    int faderCapHeight = 20;
    int faderCapY = faderTrackBounds_.getBottom() - faderCapHeight - 
                    (int)(faderTrackBounds_.getHeight() * faderNormalized);
    
    faderCapBounds_ = juce::Rectangle<int>(centerX - 16, faderCapY, 32, faderCapHeight);
    
    // Chrome gradient (Logic Pro style) #DDDDDD → #888888
    juce::ColourGradient chromeGradient(
        juce::Colour(0xffDDDDDD), faderCapBounds_.getX(), faderCapBounds_.getY(),
        juce::Colour(0xff888888), faderCapBounds_.getX(), faderCapBounds_.getBottom(),
        false);
    
    g.setGradientFill(chromeGradient);
    g.fillRoundedRectangle(faderCapBounds_.toFloat(), 3.0f);
    
    // Concave center line
    g.setColour(juce::Colour(0xff646464));
    g.drawLine(faderCapBounds_.getX() + 4, faderCapBounds_.getCentreY(),
              faderCapBounds_.getRight() - 4, faderCapBounds_.getCentreY(), 1.5f);
    
    // Fader cap border
    g.setColour(juce::Colour(0xff555555));
    g.drawRoundedRectangle(faderCapBounds_.toFloat(), 3.0f, 1.0f);
    
    currentY = faderTrackBounds_.getBottom() + 12;
    
    // === Fader Value Text ===
    g.setColour(juce::Colour(0xff9A9A9A));
    g.setFont(juce::Font(9.0f));
    juce::String faderText;
    if (faderDb_ <= -59.9f)
        faderText = "-∞";
    else
        faderText = juce::String(faderDb_, 1);
    g.drawText(faderText, 0, currentY, bounds.getWidth(), 12, 
              juce::Justification::centred);
    
    currentY += 16;
    
    // === M/S/R Buttons (square, 20×20px) ===
    int buttonSize = 20;
    int buttonY = currentY;
    
    // Mute
    muteButtonBounds_ = juce::Rectangle<int>(centerX - 30, buttonY, buttonSize, buttonSize);
    if (isMuted_)
    {
        g.setColour(juce::Colour(0xff4A6CD6));  // Logic mute blue
        g.fillRoundedRectangle(muteButtonBounds_.toFloat(), 3.0f);
        g.setColour(juce::Colour(0xff000000));
    }
    else
    {
        g.setColour(juce::Colour(0xff383838));
        g.fillRoundedRectangle(muteButtonBounds_.toFloat(), 3.0f);
        g.setColour(juce::Colour(0xffDFDFDF));
    }
    g.setFont(juce::Font(9.0f, juce::Font::bold));
    g.drawText("M", muteButtonBounds_, juce::Justification::centred);
    
    // Solo
    soloButtonBounds_ = juce::Rectangle<int>(centerX - 10, buttonY, buttonSize, buttonSize);
    if (isSoloed_)
    {
        g.setColour(juce::Colour(0xffD6A200));  // Logic solo yellow
        g.fillRoundedRectangle(soloButtonBounds_.toFloat(), 3.0f);
        g.setColour(juce::Colour(0xff000000));
    }
    else
    {
        g.setColour(juce::Colour(0xff383838));
        g.fillRoundedRectangle(soloButtonBounds_.toFloat(), 3.0f);
        g.setColour(juce::Colour(0xffDFDFDF));
    }
    g.drawText("S", soloButtonBounds_, juce::Justification::centred);
    
    // Record (only for audio/MIDI channels, not buses)
    if (!isBus_)
    {
        recordButtonBounds_ = juce::Rectangle<int>(centerX + 10, buttonY, buttonSize, buttonSize);
        if (isRecordEnabled_)
        {
            g.setColour(juce::Colour(0xffD63030));  // Logic record red
            g.fillRoundedRectangle(recordButtonBounds_.toFloat(), 3.0f);
            g.setColour(juce::Colour(0xff000000));
        }
        else
        {
            g.setColour(juce::Colour(0xff383838));
            g.fillRoundedRectangle(recordButtonBounds_.toFloat(), 3.0f);
            g.setColour(juce::Colour(0xffDFDFDF));
        }
        g.drawText("R", recordButtonBounds_, juce::Justification::centred);
    }
    
    // === Channel color indicator (bottom strip, 4px) ===
    g.setColour(channelColor_);
    g.fillRect(0, bounds.getHeight() - 4, bounds.getWidth(), 4);
=======
    const auto& colors = SkiaTheme::getInstance().getColors();

    // Background with gradient
    g.fillAll(juce::Colour(SkColorGetR(colors.backgroundSecondary),
                           SkColorGetG(colors.backgroundSecondary),
                           SkColorGetB(colors.backgroundSecondary)));

    // Subtle gradient overlay for depth
    auto bounds = getLocalBounds();
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(SkColorGetR(colors.backgroundTertiary),
                     SkColorGetG(colors.backgroundTertiary),
                     SkColorGetB(colors.backgroundTertiary)).withAlpha(0.3f),
        0, 0,
        juce::Colour(SkColorGetR(colors.backgroundSecondary),
                     SkColorGetG(colors.backgroundSecondary),
                     SkColorGetB(colors.backgroundSecondary)),
        0, static_cast<float>(bounds.getHeight()),
        false));
    g.fillRect(bounds);

    // Border with slight glow
    g.setColour(juce::Colour(SkColorGetR(colors.border),
                             SkColorGetG(colors.border),
                             SkColorGetB(colors.border)));
    g.drawRect(bounds, 1);

    // Render all components
    renderLabel(g);
    renderMeterDisplay(g);
    renderFader(g);
    renderPanKnob(g);
    renderButtons(g);
>>>>>>> 9f38c723266f5a7d2dfa29ac0a1897af495d401c
}

void SkiaMixerChannelComponent::resized()
{
    // Bounds calculated in paint()
}

void SkiaMixerChannelComponent::mouseDown(const juce::MouseEvent& event)
{
    // Fader
    if (faderTrackBounds_.contains(event.getPosition()) || 
        faderCapBounds_.contains(event.getPosition()))
    {
        isDraggingFader_ = true;
        dragStartY_ = event.y;
        dragStartValue_ = faderDb_;
        return;
    }
    
    // Pan knob
    if (panKnobBounds_.contains(event.getPosition()))
    {
        isDraggingPan_ = true;
        dragStartY_ = event.y;
        dragStartValue_ = pan_;
        return;
    }
    
    // Mute button
    if (muteButtonBounds_.contains(event.getPosition()))
    {
        setMuted(!isMuted_);
        return;
    }
    
    // Solo button
    if (soloButtonBounds_.contains(event.getPosition()))
    {
        setSoloed(!isSoloed_);
        return;
    }
    
    // Record button
    if (!isBus_ && recordButtonBounds_.contains(event.getPosition()))
    {
        setRecordEnabled(!isRecordEnabled_);
        return;
    }
    
    // Click anywhere else to select
    setSelected(true);
}

void SkiaMixerChannelComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (isDraggingFader_)
    {
<<<<<<< HEAD
        int deltaY = dragStartY_ - event.y;  // Inverse (up = increase)
        float dbChange = (deltaY / 2.0f);     // 2 pixels per dB
        setFaderValue(dragStartValue_ + dbChange);
    }
    else if (isDraggingPan_)
    {
        int deltaY = event.y - dragStartY_;
        float panChange = deltaY / 50.0f;  // 50 pixels for full range
        setPan(dragStartValue_ + panChange);
=======
        float faderHeight = static_cast<float>(getHeight() - 140);
        float faderY = 30.0f;
        float newValue = 1.0f - ((event.y - faderY) / faderHeight);
        setFaderValue(newValue);
    }
    else if (isDraggingPan_)
    {
        int knobY = getHeight() - 90;
        int knobSize = 32;
        juce::Point<float> center(static_cast<float>(getWidth() / 2), static_cast<float>(knobY + knobSize / 2));
        float dx = event.x - center.x;
        float dy = event.y - center.y;
        float angle = std::atan2(dy, dx) * 180.0f / juce::MathConstants<float>::pi - 90.0f;

        // Map -135° to +135° range to -1 to +1
        float newValue = juce::jlimit(-1.0f, 1.0f, angle / 135.0f);
        setPanValue(newValue);
>>>>>>> 9f38c723266f5a7d2dfa29ac0a1897af495d401c
    }
}

void SkiaMixerChannelComponent::timerCallback()
{
<<<<<<< HEAD
    // Meter decay
    if (meterLevel_ > 0.0f)
=======
    // Spring physics for smooth animations
    const auto& physics = SkiaTheme::getInstance().getSliderPhysics();
    const float dt = 1.0f / physics.fps;
    bool needsRepaint = false;

    // Smooth fader animation
    if (std::abs(faderValue_ - faderTarget_) > 0.001f)
    {
        const float smoothFactor = 0.2f;
        faderValue_ += (faderTarget_ - faderValue_) * smoothFactor;
        needsRepaint = true;
    }

    // Smooth pan animation
    if (std::abs(panValue_ - panTarget_) > 0.001f)
    {
        const float smoothFactor = 0.2f;
        panValue_ += (panTarget_ - panValue_) * smoothFactor;
        needsRepaint = true;
    }

    // Decay peak hold
    if (meter_.peakHoldSamples > 0)
>>>>>>> 9f38c723266f5a7d2dfa29ac0a1897af495d401c
    {
        meterLevel_ *= 0.95f;  // Fast decay
        if (meterLevel_ < 0.01f)
            meterLevel_ = 0.0f;
    }
    
    // Peak hold decay
    if (peakHoldTime_ > 0.0f)
    {
<<<<<<< HEAD
        peakHoldTime_ -= 0.05f;  // 50ms timer
        if (peakHoldTime_ <= 0.0f)
        {
            peakLevel_ = 0.0f;
            peakHoldTime_ = 0.0f;
        }
    }
    
    repaint();
}

=======
        peakLevel_ *= 0.95f; // Decay peak
        needsRepaint = true;
    }

    if (needsRepaint)
    {
        repaint();
    }
}

//==============================================================================
// Rendering Methods
//==============================================================================

void SkiaMixerChannelComponent::renderLabel(juce::Graphics& g)
{
    const auto& colors = SkiaTheme::getInstance().getColors();
    const auto& typo = SkiaTheme::getInstance().getTypography();

    // Channel name at top with subtle background
    auto labelBounds = juce::Rectangle<int>(0, 0, getWidth(), 24);

    g.setColour(juce::Colour(SkColorGetR(colors.backgroundTertiary),
                             SkColorGetG(colors.backgroundTertiary),
                             SkColorGetB(colors.backgroundTertiary)));
    g.fillRect(labelBounds);

    // Text
    g.setColour(juce::Colour(SkColorGetR(colors.textPrimary),
                             SkColorGetG(colors.textPrimary),
                             SkColorGetB(colors.textPrimary)));
    g.setFont(juce::Font(typo.smallSize, juce::Font::bold));
    g.drawText(channelName_, labelBounds, juce::Justification::centred);
}

void SkiaMixerChannelComponent::renderFader(juce::Graphics& g)
{
    const auto& colors = SkiaTheme::getInstance().getColors();

    // Vertical fader from bottom to top
    const int faderWidth = 8;
    const int faderHeight = getHeight() - 140;
    const int faderX = 12;
    const int faderY = 30;

    // Track background with rounded corners
    auto trackBounds = juce::Rectangle<float>(static_cast<float>(faderX),
                                              static_cast<float>(faderY),
                                              static_cast<float>(faderWidth),
                                              static_cast<float>(faderHeight));

    g.setColour(juce::Colour(SkColorGetR(colors.surfaceDefault),
                             SkColorGetG(colors.surfaceDefault),
                             SkColorGetB(colors.surfaceDefault)));
    g.fillRoundedRectangle(trackBounds, 4.0f);

    // Filled portion (value)
    float fillHeight = faderValue_ * faderHeight;
    auto fillBounds = juce::Rectangle<float>(static_cast<float>(faderX),
                                             static_cast<float>(faderY + faderHeight - fillHeight),
                                             static_cast<float>(faderWidth),
                                             fillHeight);

    // Gradient fill
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(SkColorGetR(colors.primary),
                     SkColorGetG(colors.primary),
                     SkColorGetB(colors.primary)),
        fillBounds.getCentreX(), fillBounds.getBottom(),
        juce::Colour(SkColorGetR(colors.primaryHover),
                     SkColorGetG(colors.primaryHover),
                     SkColorGetB(colors.primaryHover)).brighter(0.2f),
        fillBounds.getCentreX(), fillBounds.getY(),
        false));
    g.fillRoundedRectangle(fillBounds, 4.0f);

    // Fader cap (thumb)
    float capY = faderY + (1.0f - faderValue_) * faderHeight;
    auto capBounds = juce::Rectangle<float>(static_cast<float>(faderX - 4),
                                           capY - 6.0f,
                                           static_cast<float>(faderWidth + 8),
                                           12.0f);

    // Shadow
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.fillRoundedRectangle(capBounds.translated(0, 2), 2.0f);

    // Cap body with gradient
    g.setGradientFill(juce::ColourGradient(
        juce::Colours::white,
        capBounds.getCentreX(), capBounds.getY(),
        juce::Colours::lightgrey,
        capBounds.getCentreX(), capBounds.getBottom(),
        false));
    g.fillRoundedRectangle(capBounds, 2.0f);

    // Cap border
    g.setColour(juce::Colours::darkgrey.withAlpha(0.5f));
    g.drawRoundedRectangle(capBounds, 2.0f, 1.0f);
}

void SkiaMixerChannelComponent::renderPanKnob(juce::Graphics& g)
{
    const auto& colors = SkiaTheme::getInstance().getColors();

    // Pan knob at bottom middle area
    const int knobSize = 32;
    const int knobX = 8;
    const int knobY = getHeight() - 90;

    auto knobBounds = juce::Rectangle<float>(static_cast<float>(knobX),
                                            static_cast<float>(knobY),
                                            static_cast<float>(knobSize),
                                            static_cast<float>(knobSize));

    // Knob shadow
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillEllipse(knobBounds.translated(0, 2));

    // Knob body with radial gradient
    juce::Point<float> center = knobBounds.getCentre();
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(80, 80, 85),
        center.x, center.y - knobSize * 0.3f,
        juce::Colour(40, 40, 45),
        center.x, center.y + knobSize * 0.3f,
        true));
    g.fillEllipse(knobBounds);

    // Pan indicator line
    float angle = panValue_ * 135.0f; // -135° to +135°
    float angleRad = (angle + 90.0f) * juce::MathConstants<float>::pi / 180.0f;
    float indicatorLen = knobSize / 2.5f;

    juce::Point<float> indicatorEnd(
        center.x + std::cos(angleRad) * indicatorLen,
        center.y + std::sin(angleRad) * indicatorLen
    );

    g.setColour(juce::Colours::white);
    g.drawLine(center.x, center.y, indicatorEnd.x, indicatorEnd.y, 2.0f);

    // Center dot
    g.fillEllipse(center.x - 2, center.y - 2, 4, 4);

    // Pan label
    g.setColour(juce::Colour(SkColorGetR(colors.textSecondary),
                             SkColorGetG(colors.textSecondary),
                             SkColorGetB(colors.textSecondary)));
    g.setFont(juce::FontOptions(9.0f));
    g.drawText("PAN", knobX, knobY + knobSize + 2, knobSize, 12,
               juce::Justification::centred);
}

void SkiaMixerChannelComponent::renderMeterDisplay(juce::Graphics& g)
{
    const auto& colors = SkiaTheme::getInstance().getColors();

    // VU meter on the right side
    const int meterWidth = 12;
    const int meterHeight = getHeight() - 140;
    const int meterX = getWidth() - meterWidth - 6;
    const int meterY = 30;

    // Meter background
    g.setColour(juce::Colour(30, 30, 30));
    g.fillRoundedRectangle(static_cast<float>(meterX),
                          static_cast<float>(meterY),
                          static_cast<float>(meterWidth),
                          static_cast<float>(meterHeight),
                          3.0f);

    // Level fill with color zones
    float levelHeight = inputLevelSmoothed_ * meterHeight;

    if (levelHeight > 0.0f)
    {
        // Green zone (0-70%)
        float greenHeight = std::min(levelHeight, meterHeight * 0.7f);
        if (greenHeight > 0)
        {
            auto greenBounds = juce::Rectangle<float>(
                static_cast<float>(meterX),
                static_cast<float>(meterY + meterHeight - greenHeight),
                static_cast<float>(meterWidth),
                greenHeight
            );

            g.setGradientFill(juce::ColourGradient(
                juce::Colour(SkColorGetR(colors.meterGreen),
                             SkColorGetG(colors.meterGreen),
                             SkColorGetB(colors.meterGreen)),
                greenBounds.getCentreX(), greenBounds.getBottom(),
                juce::Colour(SkColorGetR(colors.meterGreen),
                             SkColorGetG(colors.meterGreen),
                             SkColorGetB(colors.meterGreen)).darker(0.3f),
                greenBounds.getCentreX(), greenBounds.getY(),
                false));
            g.fillRoundedRectangle(greenBounds, 3.0f);
        }

        // Yellow zone (70-90%)
        if (levelHeight > meterHeight * 0.7f)
        {
            float yellowHeight = std::min(levelHeight - meterHeight * 0.7f,
                                         meterHeight * 0.2f);
            auto yellowBounds = juce::Rectangle<float>(
                static_cast<float>(meterX),
                static_cast<float>(meterY + meterHeight * 0.1f),
                static_cast<float>(meterWidth),
                yellowHeight
            );

            g.setColour(juce::Colour(SkColorGetR(colors.meterYellow),
                                     SkColorGetG(colors.meterYellow),
                                     SkColorGetB(colors.meterYellow)));
            g.fillRoundedRectangle(yellowBounds, 3.0f);
        }

        // Red zone (90-100%)
        if (levelHeight > meterHeight * 0.9f)
        {
            float redHeight = levelHeight - meterHeight * 0.9f;
            auto redBounds = juce::Rectangle<float>(
                static_cast<float>(meterX),
                static_cast<float>(meterY),
                static_cast<float>(meterWidth),
                redHeight
            );

            g.setColour(juce::Colour(SkColorGetR(colors.meterRed),
                                     SkColorGetG(colors.meterRed),
                                     SkColorGetB(colors.meterRed)));
            g.fillRoundedRectangle(redBounds, 3.0f);
        }
    }

    // Peak indicator
    if (peakLevel_ > 0.01f)
    {
        float peakY = meterY + (1.0f - peakLevel_) * meterHeight;
        g.setColour(juce::Colours::white);
        g.fillRect(static_cast<float>(meterX), peakY, static_cast<float>(meterWidth), 2.0f);
    }

    // Meter border
    g.setColour(juce::Colour(60, 60, 60));
    g.drawRoundedRectangle(static_cast<float>(meterX),
                          static_cast<float>(meterY),
                          static_cast<float>(meterWidth),
                          static_cast<float>(meterHeight),
                          3.0f, 1.0f);
}

void SkiaMixerChannelComponent::renderButtons(juce::Graphics& g)
{
    const auto& colors = SkiaTheme::getInstance().getColors();

    // Mute and Solo buttons at the bottom
    const int buttonWidth = 22;
    const int buttonHeight = 18;
    const int buttonY = getHeight() - 50;
    const int muteX = 4;
    const int soloX = 28;

    // Mute button
    auto muteBounds = juce::Rectangle<float>(static_cast<float>(muteX),
                                            static_cast<float>(buttonY),
                                            static_cast<float>(buttonWidth),
                                            static_cast<float>(buttonHeight));

    // Shadow
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillRoundedRectangle(muteBounds.translated(0, 1), 3.0f);

    // Button fill
    if (isMuted_)
    {
        g.setColour(juce::Colour(SkColorGetR(colors.warning),
                                 SkColorGetG(colors.warning),
                                 SkColorGetB(colors.warning)));
    }
    else
    {
        g.setColour(juce::Colour(SkColorGetR(colors.surfaceDefault),
                                 SkColorGetG(colors.surfaceDefault),
                                 SkColorGetB(colors.surfaceDefault)));
    }
    g.fillRoundedRectangle(muteBounds, 3.0f);

    // Border
    g.setColour(juce::Colour(SkColorGetR(colors.border),
                             SkColorGetG(colors.border),
                             SkColorGetB(colors.border)));
    g.drawRoundedRectangle(muteBounds, 3.0f, 1.0f);

    // Text
    g.setColour(isMuted_ ? juce::Colours::white :
                juce::Colour(SkColorGetR(colors.textPrimary),
                             SkColorGetG(colors.textPrimary),
                             SkColorGetB(colors.textPrimary)));
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText("M", muteBounds.toNearestInt(), juce::Justification::centred);

    // Solo button
    auto soloBounds = juce::Rectangle<float>(static_cast<float>(soloX),
                                            static_cast<float>(buttonY),
                                            static_cast<float>(buttonWidth),
                                            static_cast<float>(buttonHeight));

    // Shadow
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillRoundedRectangle(soloBounds.translated(0, 1), 3.0f);

    // Button fill
    if (isSolo_)
    {
        g.setColour(juce::Colour(SkColorGetR(colors.success),
                                 SkColorGetG(colors.success),
                                 SkColorGetB(colors.success)));
    }
    else
    {
        g.setColour(juce::Colour(SkColorGetR(colors.surfaceDefault),
                                 SkColorGetG(colors.surfaceDefault),
                                 SkColorGetB(colors.surfaceDefault)));
    }
    g.fillRoundedRectangle(soloBounds, 3.0f);

    // Border
    g.setColour(juce::Colour(SkColorGetR(colors.border),
                             SkColorGetG(colors.border),
                             SkColorGetB(colors.border)));
    g.drawRoundedRectangle(soloBounds, 3.0f, 1.0f);

    // Text
    g.setColour(isSolo_ ? juce::Colours::white :
                juce::Colour(SkColorGetR(colors.textPrimary),
                             SkColorGetG(colors.textPrimary),
                             SkColorGetB(colors.textPrimary)));
    g.drawText("S", soloBounds.toNearestInt(), juce::Justification::centred);
}

SkiaMixerChannelComponent::HitTarget SkiaMixerChannelComponent::getHitTarget(
    const juce::Point<int>& pos) const
{
    // Determine what was clicked
    const int height = getHeight();
    const int width = getWidth();

    // Button area at bottom (last 60 pixels)
    if (pos.y > height - 60)
    {
        if (pos.y > height - 50)
        {
            if (pos.x < width / 2)
                return HitTarget::Mute;
            else
                return HitTarget::Solo;
        }
        // Pan knob area
        return HitTarget::Pan;
    }

    // Meter is on the right side
    if (pos.x > width - 18)
    {
        return HitTarget::None; // Meter is not interactive
    }

    // Fader area (left/middle)
    return HitTarget::Fader;
}

>>>>>>> 9f38c723266f5a7d2dfa29ac0a1897af495d401c
} // namespace zenith

#endif // ZENITH_USE_SKIA

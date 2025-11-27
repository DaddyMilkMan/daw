/**
 * @file SkiaMixerChannelComponent.cpp
 * @brief Logic Pro style mixer channel implementation
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
        int deltaY = dragStartY_ - event.y;  // Inverse (up = increase)
        float dbChange = (deltaY / 2.0f);     // 2 pixels per dB
        setFaderValue(dragStartValue_ + dbChange);
    }
    else if (isDraggingPan_)
    {
        int deltaY = event.y - dragStartY_;
        float panChange = deltaY / 50.0f;  // 50 pixels for full range
        setPan(dragStartValue_ + panChange);
    }
}

void SkiaMixerChannelComponent::timerCallback()
{
    // Meter decay
    if (meterLevel_ > 0.0f)
    {
        meterLevel_ *= 0.95f;  // Fast decay
        if (meterLevel_ < 0.01f)
            meterLevel_ = 0.0f;
    }
    
    // Peak hold decay
    if (peakHoldTime_ > 0.0f)
    {
        peakHoldTime_ -= 0.05f;  // 50ms timer
        if (peakHoldTime_ <= 0.0f)
        {
            peakLevel_ = 0.0f;
            peakHoldTime_ = 0.0f;
        }
    }
    
    repaint();
}

} // namespace zenith

#endif // ZENITH_USE_SKIA

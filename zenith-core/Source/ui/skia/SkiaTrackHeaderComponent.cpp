/**
 * @file SkiaTrackHeaderComponent.cpp
 * @brief Logic Pro style track header implementation
 */

#include "SkiaTrackHeaderComponent.h"
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

SkiaTrackHeaderComponent::SkiaTrackHeaderComponent()
{
    setSize(250, 60);  // Logic Pro default track header size
}

void SkiaTrackHeaderComponent::setTrackNumber(int number)
{
    if (trackNumber_ != number)
    {
        trackNumber_ = number;
        repaint();
    }
}

void SkiaTrackHeaderComponent::setTrackName(const juce::String& name)
{
    if (trackName_ != name)
    {
        trackName_ = name;
        repaint();
    }
}

void SkiaTrackHeaderComponent::setTrackColor(juce::Colour color)
{
    if (trackColor_ != color)
    {
        trackColor_ = color;
        repaint();
    }
}

void SkiaTrackHeaderComponent::setMuted(bool muted)
{
    if (isMuted_ != muted)
    {
        isMuted_ = muted;
        repaint();
    }
}

void SkiaTrackHeaderComponent::setSoloed(bool soloed)
{
    if (isSoloed_ != soloed)
    {
        isSoloed_ = soloed;
        repaint();
    }
}

void SkiaTrackHeaderComponent::setRecordEnabled(bool enabled)
{
    if (isRecordEnabled_ != enabled)
    {
        isRecordEnabled_ = enabled;
        repaint();
    }
}

void SkiaTrackHeaderComponent::setInputMonitoring(bool enabled)
{
    if (isInputMonitoring_ != enabled)
    {
        isInputMonitoring_ = enabled;
        repaint();
    }
}

void SkiaTrackHeaderComponent::setVolume(float db)
{
    volumeDb_ = std::max(-60.0f, std::min(6.0f, db));
    repaint();
}

void SkiaTrackHeaderComponent::setPan(float pan)
{
    pan_ = std::max(-1.0f, std::min(1.0f, pan));
    repaint();
}

void SkiaTrackHeaderComponent::setSelected(bool selected)
{
    if (isSelected_ != selected)
    {
        isSelected_ = selected;
        repaint();
    }
}

void SkiaTrackHeaderComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    
    // Background (selected track is lighter)
    if (isSelected_)
        g.fillAll(juce::Colour(0xff444444));  // Logic Pro selected track
    else
        g.fillAll(juce::Colour(0xff292929));  // Logic Pro track header bg
    
    // === Track Color Bar (left edge, 4px) ===
    juce::Rectangle<int> colorBar(0, 0, 4, bounds.getHeight());
    g.setColour(trackColor_);
    g.fillRect(colorBar);
    
    int leftMargin = 8;
    int currentX = leftMargin;
    
    // === Track Number ===
    juce::Rectangle<int> numberBounds(currentX, 8, 24, 20);
    g.setColour(juce::Colour(0xff9A9A9A));  // Logic Pro secondary text
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText(juce::String(trackNumber_), numberBounds, juce::Justification::centred);
    currentX += 28;
    
    // === Track Icon (placeholder for now) ===
    juce::Rectangle<int> iconBounds(currentX, 8, 24, 24);
    g.setColour(juce::Colour(0xff006FFF));  // Logic Blue
    g.fillEllipse(iconBounds.toFloat());  // Simplified icon
    currentX += 28;
    
    // === Track Name ===
    juce::Rectangle<int> nameBounds(currentX, 8, 100, 24);
    g.setColour(juce::Colour(0xffDFDFDF));  // Logic Pro primary text
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText(trackName_, nameBounds, juce::Justification::centredLeft);
    
    // === M/S/R/I Buttons (bottom row) ===
    int buttonY = bounds.getHeight() - 28;
    int buttonSize = 24;
    int buttonSpacing = 28;
    currentX = leftMargin;
    
    // Mute Button (M) - Blue when active
    muteButtonBounds_ = juce::Rectangle<int>(currentX, buttonY, buttonSize, buttonSize);
    if (isMuted_)
    {
        g.setColour(juce::Colour(0xff4A6CD6));  // Logic Pro mute blue
        g.fillRoundedRectangle(muteButtonBounds_.toFloat(), 4.0f);
    }
    else
    {
        g.setColour(juce::Colour(0xff383838));  // Dark gray
        g.fillRoundedRectangle(muteButtonBounds_.toFloat(), 4.0f);
    }
    g.setColour(juce::Colour(0xff111111));
    g.drawRoundedRectangle(muteButtonBounds_.toFloat(), 4.0f, 1.0f);
    g.setColour(isMuted_ ? juce::Colour(0xff000000) : juce::Colour(0xffDFDFDF));
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.drawText("M", muteButtonBounds_, juce::Justification::centred);
    currentX += buttonSpacing;
    
    // Solo Button (S) - Yellow when active
    soloButtonBounds_ = juce::Rectangle<int>(currentX, buttonY, buttonSize, buttonSize);
    if (isSoloed_)
    {
        g.setColour(juce::Colour(0xffD6A200));  // Logic Pro solo yellow
        g.fillRoundedRectangle(soloButtonBounds_.toFloat(), 4.0f);
    }
    else
    {
        g.setColour(juce::Colour(0xff383838));
        g.fillRoundedRectangle(soloButtonBounds_.toFloat(), 4.0f);
    }
    g.setColour(juce::Colour(0xff111111));
    g.drawRoundedRectangle(soloButtonBounds_.toFloat(), 4.0f, 1.0f);
    g.setColour(isSoloed_ ? juce::Colour(0xff000000) : juce::Colour(0xffDFDFDF));
    g.drawText("S", soloButtonBounds_, juce::Justification::centred);
    currentX += buttonSpacing;
    
    // Record Enable Button (R) - Red when active
    recordButtonBounds_ = juce::Rectangle<int>(currentX, buttonY, buttonSize, buttonSize);
    if (isRecordEnabled_)
    {
        g.setColour(juce::Colour(0xffD63030));  // Logic Pro record red
        g.fillRoundedRectangle(recordButtonBounds_.toFloat(), 4.0f);
    }
    else
    {
        g.setColour(juce::Colour(0xff383838));
        g.fillRoundedRectangle(recordButtonBounds_.toFloat(), 4.0f);
    }
    g.setColour(juce::Colour(0xff111111));
    g.drawRoundedRectangle(recordButtonBounds_.toFloat(), 4.0f, 1.0f);
    g.setColour(isRecordEnabled_ ? juce::Colour(0xff000000) : juce::Colour(0xffDFDFDF));
    g.drawText("R", recordButtonBounds_, juce::Justification::centred);
    currentX += buttonSpacing;
    
    // Input Monitor Button (I) - Orange when active
    inputButtonBounds_ = juce::Rectangle<int>(currentX, buttonY, buttonSize, buttonSize);
    if (isInputMonitoring_)
    {
        g.setColour(juce::Colour(0xffD68020));  // Logic Pro input orange
        g.fillRoundedRectangle(inputButtonBounds_.toFloat(), 4.0f);
    }
    else
    {
        g.setColour(juce::Colour(0xff383838));
        g.fillRoundedRectangle(inputButtonBounds_.toFloat(), 4.0f);
    }
    g.setColour(juce::Colour(0xff111111));
    g.drawRoundedRectangle(inputButtonBounds_.toFloat(), 4.0f, 1.0f);
    g.setColour(isInputMonitoring_ ? juce::Colour(0xff000000) : juce::Colour(0xffDFDFDF));
    g.drawText("I", inputButtonBounds_, juce::Justification::centred);
    currentX += buttonSpacing + 8;
    
    // === Volume Slider (horizontal mini-slider) ===
    volumeSliderBounds_ = juce::Rectangle<int>(currentX, buttonY + 4, 40, 16);
    
    // Track
    g.setColour(juce::Colour(0xff1A1A1A));
    g.fillRoundedRectangle(volumeSliderBounds_.toFloat(), 2.0f);
    
    // Fill (based on volume)
    float volumeNormalized = (volumeDb_ + 60.0f) / 66.0f;  // -60 to +6 dB range
    int fillWidth = (int)(volumeSliderBounds_.getWidth() * volumeNormalized);
    juce::Rectangle<int> volumeFill(volumeSliderBounds_.getX(), volumeSliderBounds_.getY(), 
                                     fillWidth, volumeSliderBounds_.getHeight());
    g.setColour(juce::Colour(0xff006FFF));  // Logic Blue
    g.fillRoundedRectangle(volumeFill.toFloat(), 2.0f);
    
    currentX += 44;
    
    // === Pan Knob (simplified) ===
    panKnobBounds_ = juce::Rectangle<int>(currentX, buttonY, 24, 24);
    
    // Knob background
    g.setColour(juce::Colour(0xff2E2E2E));
    g.fillEllipse(panKnobBounds_.toFloat());
    
    // Pan indicator (green ring section)
    g.setColour(juce::Colour(0xff00FF00));
    g.drawEllipse(panKnobBounds_.toFloat().reduced(2.0f), 2.0f);
    
    // Pan position line
    float panAngle = -2.5f + (pan_ + 1.0f) * 2.5f;  // -2.5 to 2.5 radians
    float centerX = panKnobBounds_.getCentreX();
    float centerY = panKnobBounds_.getCentreY();
    float lineLength = 8.0f;
    float endX = centerX + lineLength * std::cos(panAngle);
    float endY = centerY + lineLength * std::sin(panAngle);
    
    g.setColour(juce::Colour(0xffDFDFDF));
    g.drawLine(centerX, centerY, endX, endY, 2.0f);
    
    // === Bottom separator line (deep black engraved) ===
    g.setColour(juce::Colour(0xff000000));
    g.fillRect(0, bounds.getHeight() - 1, bounds.getWidth(), 1);
}

void SkiaTrackHeaderComponent::resized()
{
    // Bounds are calculated dynamically in paint()
}

void SkiaTrackHeaderComponent::mouseDown(const juce::MouseEvent& event)
{
    // Check button clicks
    if (muteButtonBounds_.contains(event.getPosition()))
    {
        setMuted(!isMuted_);
        return;
    }
    
    if (soloButtonBounds_.contains(event.getPosition()))
    {
        setSoloed(!isSoloed_);
        return;
    }
    
    if (recordButtonBounds_.contains(event.getPosition()))
    {
        setRecordEnabled(!isRecordEnabled_);
        return;
    }
    
    if (inputButtonBounds_.contains(event.getPosition()))
    {
        setInputMonitoring(!isInputMonitoring_);
        return;
    }
    
    // Check volume slider
    if (volumeSliderBounds_.contains(event.getPosition()))
    {
        isDraggingVolume_ = true;
        float normalized = (float)(event.x - volumeSliderBounds_.getX()) / volumeSliderBounds_.getWidth();
        setVolume(normalized * 66.0f - 60.0f);  // Convert to dB
        return;
    }
    
    // Check pan knob
    if (panKnobBounds_.contains(event.getPosition()))
    {
        isDraggingPan_ = true;
        return;
    }
    
    // Clicking anywhere else selects the track
    setSelected(true);
}

void SkiaTrackHeaderComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (isDraggingVolume_)
    {
        float normalized = (float)(event.x - volumeSliderBounds_.getX()) / volumeSliderBounds_.getWidth();
        setVolume(normalized * 66.0f - 60.0f);
    }
    else if (isDraggingPan_)
    {
        float deltaY = event.getDistanceFromDragStartY();
        float newPan = pan_ - (deltaY / 50.0f);  // Drag up = pan right
        setPan(newPan);
    }
}

} // namespace zenith

#endif // ZENITH_USE_SKIA

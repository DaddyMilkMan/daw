/**
 * @file ZenithLookAndFeel.cpp
 * @brief Implementation of Zenith DAW's custom LookAndFeel
 */

#include "ZenithLookAndFeel.h"

namespace zenith {

//==============================================================================
// Constructor
//==============================================================================

ZenithLookAndFeel::ZenithLookAndFeel()
{
    // Set default colors using JUCE's color IDs
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(Colors::backgroundDark));
    
    // Text colors
    setColour(juce::Label::textColourId, juce::Colour(Colors::textPrimary));
    setColour(juce::TextEditor::textColourId, juce::Colour(Colors::textPrimary));
    setColour(juce::TextEditor::backgroundColourId, juce::Colour(Colors::backgroundPanel));
    setColour(juce::TextEditor::outlineColourId, juce::Colour(Colors::border));
    setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(Colors::accentPrimary));
    
    // Button colors
    setColour(juce::TextButton::buttonColourId, juce::Colour(Colors::backgroundLight));
    setColour(juce::TextButton::buttonOnColourId, juce::Colour(Colors::accentPrimary));
    setColour(juce::TextButton::textColourOffId, juce::Colour(Colors::textPrimary));
    setColour(juce::TextButton::textColourOnId, juce::Colour(0xff000000));
    
    // ComboBox colors
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(Colors::backgroundPanel));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(Colors::border));
    setColour(juce::ComboBox::textColourId, juce::Colour(Colors::textPrimary));
    setColour(juce::ComboBox::buttonColourId, juce::Colour(Colors::backgroundLight));
    
    // Slider colors
    setColour(juce::Slider::thumbColourId, juce::Colour(Colors::accentPrimary));
    setColour(juce::Slider::trackColourId, juce::Colour(Colors::accentPrimaryDark));
    setColour(juce::Slider::backgroundColourId, juce::Colour(Colors::backgroundPanel));
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(Colors::textPrimary));
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(Colors::backgroundPanel));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(Colors::border));
    
    // Scrollbar colors
    setColour(juce::ScrollBar::thumbColourId, juce::Colour(Colors::backgroundLight));
    setColour(juce::ScrollBar::trackColourId, juce::Colour(Colors::backgroundPanel));
    
    // ListBox colors
    setColour(juce::ListBox::backgroundColourId, juce::Colour(Colors::backgroundPanel));
    setColour(juce::ListBox::outlineColourId, juce::Colour(Colors::border));
    
    // Toggle button colors
    setColour(juce::ToggleButton::textColourId, juce::Colour(Colors::textPrimary));
    setColour(juce::ToggleButton::tickColourId, juce::Colour(Colors::accentPrimary));
    setColour(juce::ToggleButton::tickDisabledColourId, juce::Colour(Colors::textDisabled));
    
    // PopupMenu colors
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(Colors::backgroundMid));
    setColour(juce::PopupMenu::textColourId, juce::Colour(Colors::textPrimary));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(Colors::accentPrimary));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(0xff000000));
}

//==============================================================================
// Button Drawing
//==============================================================================

void ZenithLookAndFeel::drawButtonBackground(juce::Graphics& g,
                                            juce::Button& button,
                                            const juce::Colour& backgroundColour,
                                            bool shouldDrawButtonAsHighlighted,
                                            bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);
    
    auto baseColour = getButtonColour(button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    
    // Fill with flat color
    g.setColour(baseColour);
    g.fillRoundedRectangle(bounds, Metrics::radiusM);
    
    // Subtle border
    if (!shouldDrawButtonAsDown)
    {
        g.setColour(juce::Colour(Colors::border));
        g.drawRoundedRectangle(bounds, Metrics::radiusM, 1.0f);
    }
}

void ZenithLookAndFeel::drawButtonText(juce::Graphics& g,
                                      juce::TextButton& button,
                                      bool shouldDrawButtonAsHighlighted,
                                      bool shouldDrawButtonAsDown)
{
    auto font = getFontBody();
    g.setFont(font);
    
    // Determine text color
    juce::Colour textColour;
    if (!button.isEnabled())
        textColour = juce::Colour(Colors::textDisabled);
    else if (button.getToggleState())
        textColour = juce::Colour(0xff000000); // Black text on accent background
    else
        textColour = juce::Colour(Colors::textPrimary);
    
    g.setColour(textColour);
    
    auto textBounds = button.getLocalBounds();
    
    // Shift text down 1px when pressed for tactile feedback
    if (shouldDrawButtonAsDown)
        textBounds.translate(0, 1);
    
    g.drawText(button.getButtonText(),
              textBounds,
              juce::Justification::centred,
              true);
}

void ZenithLookAndFeel::drawToggleButton(juce::Graphics& g,
                                        juce::ToggleButton& button,
                                        bool shouldDrawButtonAsHighlighted,
                                        bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    
    // For compact toggles (like M/S/R buttons), draw as filled rect when active
    auto fontSize = juce::jmin(15.0f, (float)button.getHeight() * 0.75f);
    auto tickWidth = fontSize * 1.1f;
    
    // Draw checkbox/toggle area
    auto tickBounds = bounds.removeFromLeft(tickWidth).reduced(2.0f);
    
    auto baseColour = button.getToggleState() 
                    ? juce::Colour(Colors::accentPrimary)
                    : juce::Colour(Colors::backgroundPanel);
    
    if (shouldDrawButtonAsHighlighted)
        baseColour = baseColour.brighter(0.1f);
    if (shouldDrawButtonAsDown)
        baseColour = baseColour.darker(0.1f);
    
    g.setColour(baseColour);
    g.fillRoundedRectangle(tickBounds, Metrics::radiusS);
    
    // Border
    g.setColour(juce::Colour(Colors::border));
    g.drawRoundedRectangle(tickBounds, Metrics::radiusS, 1.0f);
    
    // Draw text
    g.setColour(button.findColour(juce::ToggleButton::textColourId));
    g.setFont(fontSize);
    
    g.drawFittedText(button.getButtonText(),
                    bounds.toNearestInt(),
                    juce::Justification::centredLeft,
                    1);
}

//==============================================================================
// Slider Drawing
//==============================================================================

void ZenithLookAndFeel::drawRotarySlider(juce::Graphics& g,
                                        int x, int y, int width, int height,
                                        float sliderPos,
                                        float rotaryStartAngle,
                                        float rotaryEndAngle,
                                        juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(10.0f);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = juce::jmin(8.0f, radius * 0.5f);
    auto arcRadius = radius - lineW * 0.5f;
    
    // Background arc (track)
    juce::Path backgroundArc;
    backgroundArc.addCentredArc(bounds.getCentreX(),
                               bounds.getCentreY(),
                               arcRadius,
                               arcRadius,
                               0.0f,
                               rotaryStartAngle,
                               rotaryEndAngle,
                               true);
    
    g.setColour(juce::Colour(Colors::backgroundPanel));
    g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    
    // Value arc (filled track)
    if (slider.isEnabled())
    {
        juce::Path valueArc;
        valueArc.addCentredArc(bounds.getCentreX(),
                              bounds.getCentreY(),
                              arcRadius,
                              arcRadius,
                              0.0f,
                              rotaryStartAngle,
                              toAngle,
                              true);
        
        g.setColour(juce::Colour(Colors::accentPrimary));
        g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    
    // Thumb (pointer line)
    juce::Point<float> thumbPoint(bounds.getCentreX() + arcRadius * std::cos(toAngle - juce::MathConstants<float>::halfPi),
                                 bounds.getCentreY() + arcRadius * std::sin(toAngle - juce::MathConstants<float>::halfPi));
    
    g.setColour(slider.isEnabled() ? juce::Colour(Colors::textPrimary) : juce::Colour(Colors::textDisabled));
    g.drawLine(bounds.getCentreX(), bounds.getCentreY(), thumbPoint.x, thumbPoint.y, 2.0f);
}

void ZenithLookAndFeel::drawLinearSlider(juce::Graphics& g,
                                        int x, int y, int width, int height,
                                        float sliderPos,
                                        float minSliderPos,
                                        float maxSliderPos,
                                        juce::Slider::SliderStyle style,
                                        juce::Slider& slider)
{
    auto isVertical = (style == juce::Slider::LinearVertical || style == juce::Slider::LinearBarVertical);
    auto trackWidth = isVertical ? width : height;
    trackWidth = juce::jmin(trackWidth, isVertical ? 40 : 40);
    
    auto trackBounds = isVertical
        ? juce::Rectangle<float>(x + (width - trackWidth) / 2.0f, (float)y, (float)trackWidth, (float)height)
        : juce::Rectangle<float>((float)x, y + (height - trackWidth) / 2.0f, (float)width, (float)trackWidth);
    
    // Background track
    g.setColour(juce::Colour(Colors::backgroundPanel));
    g.fillRoundedRectangle(trackBounds, Metrics::radiusS);
    
    // Filled portion
    auto filledBounds = trackBounds;
    if (isVertical)
    {
        filledBounds.removeFromTop(trackBounds.getHeight() - sliderPos);
    }
    else
    {
        filledBounds.removeFromRight(trackBounds.getWidth() - sliderPos);
    }
    
    g.setColour(slider.isEnabled() ? juce::Colour(Colors::accentPrimary) : juce::Colour(Colors::textDisabled));
    g.fillRoundedRectangle(filledBounds, Metrics::radiusS);
    
    // Border
    g.setColour(juce::Colour(Colors::border));
    g.drawRoundedRectangle(trackBounds, Metrics::radiusS, 1.0f);
}

//==============================================================================
// ComboBox Drawing
//==============================================================================

void ZenithLookAndFeel::drawComboBox(juce::Graphics& g,
                                    int width, int height,
                                    bool isButtonDown,
                                    int buttonX, int buttonY,
                                    int buttonW, int buttonH,
                                    juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(0.5f, 0.5f);
    
    // Background
    auto bgColour = box.findColour(juce::ComboBox::backgroundColourId);
    if (isButtonDown)
        bgColour = bgColour.brighter(0.1f);
    
    g.setColour(bgColour);
    g.fillRoundedRectangle(bounds, Metrics::radiusM);
    
    // Border
    g.setColour(box.findColour(juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(bounds, Metrics::radiusM, 1.0f);
    
    // Dropdown arrow
    juce::Path arrow;
    auto arrowBounds = juce::Rectangle<float>(buttonX, buttonY, buttonW, buttonH).reduced(4.0f);
    arrow.addTriangle(arrowBounds.getX(), arrowBounds.getY(),
                     arrowBounds.getRight(), arrowBounds.getY(),
                     arrowBounds.getCentreX(), arrowBounds.getBottom());
    
    g.setColour(box.findColour(juce::ComboBox::arrowColourId));
    g.fillPath(arrow);
}

//==============================================================================
// Label Drawing
//==============================================================================

void ZenithLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    g.fillAll(label.findColour(juce::Label::backgroundColourId));
    
    if (!label.isBeingEdited())
    {
        auto alpha = label.isEnabled() ? 1.0f : 0.5f;
        auto textColour = label.findColour(juce::Label::textColourId).withMultipliedAlpha(alpha);
        
        g.setColour(textColour);
        g.setFont(label.getFont());
        
        g.drawFittedText(label.getText(),
                        label.getLocalBounds(),
                        label.getJustificationType(),
                        juce::jmax(1, (int)((float)label.getHeight() / label.getFont().getHeight())),
                        label.getMinimumHorizontalScale());
        
        // Draw outline border if needed
        g.setColour(label.findColour(juce::Label::outlineColourId));
        g.drawRect(label.getLocalBounds());
    }
}

//==============================================================================
// Scrollbar Drawing
//==============================================================================

void ZenithLookAndFeel::drawScrollbar(juce::Graphics& g,
                                     juce::ScrollBar& scrollbar,
                                     int x, int y, int width, int height,
                                     bool isScrollbarVertical,
                                     int thumbStartPosition,
                                     int thumbSize,
                                     bool isMouseOver,
                                     bool isMouseDown)
{
    // Don't draw track background
    
    // Draw thumb
    auto thumbBounds = isScrollbarVertical
        ? juce::Rectangle<int>(x + 2, thumbStartPosition, width - 4, thumbSize)
        : juce::Rectangle<int>(thumbStartPosition, y + 2, thumbSize, height - 4);
    
    auto thumbColour = scrollbar.findColour(juce::ScrollBar::thumbColourId);
    if (isMouseOver)
        thumbColour = thumbColour.brighter(0.2f);
    if (isMouseDown)
        thumbColour = thumbColour.brighter(0.4f);
    
    g.setColour(thumbColour);
    g.fillRoundedRectangle(thumbBounds.toFloat(), Metrics::radiusS);
}

//==============================================================================
// Tab Button Drawing
//==============================================================================

void ZenithLookAndFeel::drawTabButton(juce::TabBarButton& button,
                                     juce::Graphics& g,
                                     bool isMouseOver,
                                     bool isMouseDown)
{
    auto activeArea = button.getActiveArea();
    auto isFrontTab = button.isFrontTab();
    
    // Background
    auto bgColour = isFrontTab 
                  ? juce::Colour(Colors::backgroundLight)
                  : juce::Colour(Colors::backgroundPanel);
    
    if (isMouseOver && !isFrontTab)
        bgColour = bgColour.brighter(0.1f);
    
    g.setColour(bgColour);
    g.fillRect(activeArea);
    
    // Bottom indicator for active tab
    if (isFrontTab)
    {
        g.setColour(juce::Colour(Colors::accentPrimary));
        g.fillRect(activeArea.removeFromBottom(2));
    }
    
    // Text
    g.setColour(isFrontTab ? juce::Colour(Colors::textPrimary) : juce::Colour(Colors::textSecondary));
    g.setFont(getFontBody());
    g.drawText(button.getButtonText(), activeArea, juce::Justification::centred, true);
}

//==============================================================================
// Helper Methods
//==============================================================================

void ZenithLookAndFeel::drawRoundedRect(juce::Graphics& g,
                                       const juce::Rectangle<float>& bounds,
                                       float cornerSize,
                                       const juce::Colour& fillColour,
                                       const juce::Colour& strokeColour,
                                       float strokeWidth)
{
    g.setColour(fillColour);
    g.fillRoundedRectangle(bounds, cornerSize);
    
    if (strokeWidth > 0.0f && strokeColour != juce::Colour())
    {
        g.setColour(strokeColour);
        g.drawRoundedRectangle(bounds, cornerSize, strokeWidth);
    }
}

juce::Colour ZenithLookAndFeel::getButtonColour(juce::Button& button,
                                               bool isHighlighted,
                                               bool isDown)
{
    juce::Colour baseColour;
    
    if (button.getToggleState())
    {
        baseColour = juce::Colour(Colors::accentPrimary);
    }
    else
    {
        baseColour = juce::Colour(Colors::backgroundLight);
    }
    
    if (!button.isEnabled())
        return juce::Colour(Colors::backgroundPanel);
    
    if (isDown)
        return baseColour.darker(0.2f);
    if (isHighlighted)
        return baseColour.brighter(0.1f);
    
    return baseColour;
}

} // namespace zenith


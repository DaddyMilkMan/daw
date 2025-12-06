#include "ZenithLookAndFeel.h"

namespace zenith {

ZenithLookAndFeel::ZenithLookAndFeel() {
    // Global
    setColour(juce::ResizableWindow::backgroundColourId, Colors::background);
    setColour(juce::TooltipWindow::backgroundColourId, Colors::panel);
    setColour(juce::TooltipWindow::textColourId, Colors::textPrimary);
    setColour(juce::TooltipWindow::outlineColourId, Colors::border);

    // Text Button
    setColour(juce::TextButton::buttonColourId, Colors::panel);
    setColour(juce::TextButton::buttonOnColourId, Colors::accent.withAlpha(0.3f));
    setColour(juce::TextButton::textColourOffId, Colors::textPrimary);
    setColour(juce::TextButton::textColourOnId, Colors::accent);

    // ComboBox
    setColour(juce::ComboBox::backgroundColourId, Colors::backgroundPanel);
    setColour(juce::ComboBox::outlineColourId, Colors::border);
    setColour(juce::ComboBox::arrowColourId, Colors::textSecondary);
    setColour(juce::ComboBox::focusedOutlineColourId, Colors::accent);

    // Slider
    setColour(juce::Slider::thumbColourId, Colors::textPrimary);
    setColour(juce::Slider::trackColourId, Colors::backgroundPanel);
    setColour(juce::Slider::backgroundColourId, Colors::background);
    setColour(juce::Slider::rotarySliderFillColourId, Colors::accent);
    setColour(juce::Slider::rotarySliderOutlineColourId, Colors::border);

    // Label
    setColour(juce::Label::textColourId, Colors::textPrimary);
    
    // TabbedComponent
    setColour(juce::TabbedComponent::backgroundColourId, Colors::background);
    setColour(juce::TabbedComponent::outlineColourId, Colors::border);
    
    // AlertWindow
    setColour(juce::AlertWindow::backgroundColourId, Colors::panel);
    setColour(juce::AlertWindow::textColourId, Colors::textPrimary);
    setColour(juce::AlertWindow::outlineColourId, Colors::accent);
}

// Static Color Definitions
juce::Colour ZenithLookAndFeel::Colors::background = juce::Colour(0xff121212);
juce::Colour ZenithLookAndFeel::Colors::panel = juce::Colour(0xff1e1e1e);
juce::Colour ZenithLookAndFeel::Colors::backgroundPanel = juce::Colour(0xff181818);
juce::Colour ZenithLookAndFeel::Colors::border = juce::Colour(0xff333333);
juce::Colour ZenithLookAndFeel::Colors::textPrimary = juce::Colours::white;
juce::Colour ZenithLookAndFeel::Colors::textSecondary = juce::Colours::lightgrey;
juce::Colour ZenithLookAndFeel::Colors::accent = juce::Colour(0xff00ffff); // Cyan
juce::Colour ZenithLookAndFeel::Colors::accentGlow = juce::Colour(0xff00ffff).withAlpha(0.6f);

void ZenithLookAndFeel::Colors::setOledMode(bool enabled) {
    if (enabled) {
        background = juce::Colours::black;
        panel = juce::Colour(0xff050505);
        backgroundPanel = juce::Colours::black;
        border = juce::Colour(0xff202020);
    } else {
        // Reset
        background = juce::Colour(0xff121212);
        panel = juce::Colour(0xff1e1e1e);
        backgroundPanel = juce::Colour(0xff181818);
        border = juce::Colour(0xff333333);
    }
}

//==============================================================================
// Component Drawing Overrides
//==============================================================================

void ZenithLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, const float rotaryStartAngle,
                                         const float rotaryEndAngle, juce::Slider& slider) {
    auto radius = (float)juce::jmin(width / 2, height / 2) - 4.0f;
    auto centreX = (float)x + (float)width * 0.5f;
    auto centreY = (float)y + (float)height * 0.5f;
    auto rx = centreX - radius;
    auto ry = centreY - radius;
    auto rw = radius * 2.0f;
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // 1. Background Track (Dark Ring)
    g.setColour(Colors::backgroundPanel);
    g.fillEllipse(rx, ry, rw, rw);
    g.setColour(Colors::border);
    g.drawEllipse(rx, ry, rw, rw, 2.0f);

    // 2. Value Arc (Neon)
    juce::Path p;
    p.addArc(rx + 2, ry + 2, rw - 4, rw - 4, rotaryStartAngle, angle, true);
    
    g.setColour(Colors::accent);
    g.strokePath(p, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 3. Indicator Dot
    auto pointerLength = radius * 0.7f;
    juce::Point<float> thumbPos(centreX + pointerLength * std::cos(angle),
                                centreY + pointerLength * std::sin(angle));
    
    g.setColour(juce::Colours::white);
    g.fillEllipse(thumbPos.x - 3, thumbPos.y - 3, 6, 6);
}

void ZenithLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float minSliderPos, float maxSliderPos,
                                         const juce::Slider::SliderStyle style, juce::Slider& slider) {
    juce::ignoreUnused(minSliderPos, maxSliderPos, slider);
    
    auto trackWidth = 4.0f;
    juce::Point<float> startPoint((float)x + (float)width * 0.5f, (float)y + (float)height);
    juce::Point<float> endPoint((float)x + (float)width * 0.5f, (float)y);
    
    if (style == juce::Slider::LinearHorizontal) {
        startPoint = juce::Point<float>((float)x, (float)y + (float)height * 0.5f);
        endPoint = juce::Point<float>((float)x + (float)width, (float)y + (float)height * 0.5f);
    }

    // Track
    g.setColour(Colors::backgroundPanel);
    g.fillRoundedRectangle(juce::Rectangle<float>(startPoint, endPoint).expanded(2), 2.0f);

    // Fill (Neon)
    g.setColour(Colors::accent);
    if (style == juce::Slider::LinearVertical) {
        g.drawLine(startPoint.x, startPoint.y, startPoint.x, sliderPos, trackWidth);
    } else {
        g.drawLine(startPoint.x, startPoint.y, sliderPos, startPoint.y, trackWidth);
    }

    // Thumb (Handle)
    auto thumbWidth = (style == juce::Slider::LinearVertical) ? 20.0f : 10.0f;
    auto thumbHeight = (style == juce::Slider::LinearVertical) ? 10.0f : 20.0f;
    
    g.setColour(Colors::textPrimary);
    if (style == juce::Slider::LinearVertical) {
        g.fillRoundedRectangle(startPoint.x - thumbWidth * 0.5f, sliderPos - thumbHeight * 0.5f, thumbWidth, thumbHeight, 2.0f);
    } else {
        g.fillRoundedRectangle(sliderPos - thumbWidth * 0.5f, startPoint.y - thumbHeight * 0.5f, thumbWidth, thumbHeight, 2.0f);
    }
}

void ZenithLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                             const juce::Colour& backgroundColour,
                                             bool shouldDrawButtonAsHighlighted,
                                             bool shouldDrawButtonAsDown) {
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    auto cornerSize = 4.0f;

    auto baseColour = backgroundColour;
    if (shouldDrawButtonAsDown) baseColour = baseColour.darker(0.2f);
    else if (shouldDrawButtonAsHighlighted) baseColour = baseColour.brighter(0.1f);

    // Fill
    g.setColour(baseColour);
    g.fillRoundedRectangle(bounds, cornerSize);

    // Glow/Border
    if (button.hasKeyboardFocus(true) || shouldDrawButtonAsHighlighted) {
        g.setColour(Colors::accent.withAlpha(0.5f));
        g.drawRoundedRectangle(bounds, cornerSize, 2.0f);
    } else {
        g.setColour(Colors::border);
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
    }
}

void ZenithLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                                     int buttonX, int buttonY, int buttonW, int buttonH,
                                     juce::ComboBox& box) {
    auto cornerSize = 4.0f;
    juce::Rectangle<int> boxBounds(0, 0, width, height);

    g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(boxBounds.toFloat(), cornerSize);

    g.setColour(box.findColour(juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(boxBounds.toFloat().reduced(0.5f, 0.5f), cornerSize, 1.0f);

    // Arrow
    juce::Path arrow;
    arrow.addTriangle(width * 0.85f, height * 0.4f,
                      width * 0.9f, height * 0.6f,
                      width * 0.95f, height * 0.4f);
    g.setColour(box.findColour(juce::ComboBox::arrowColourId));
    g.fillPath(arrow);
}

void ZenithLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height) {
    g.fillAll(Colors::panel);
    g.setColour(Colors::border);
    g.drawRect(0, 0, width, height);
}

ZenithLookAndFeel& ZenithLookAndFeel::getInstance() {
    static ZenithLookAndFeel instance;
    return instance;
}

} // namespace zenith

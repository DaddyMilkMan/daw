#include "ZenithLookAndFeel.h"

namespace zenith {

ZenithLookAndFeel::ZenithLookAndFeel() {
    setColour(juce::Slider::thumbColourId, juce::Colour(0xffdddddd));
    setColour(juce::Slider::trackColourId, juce::Colour(0xff2a2a2a));
    setColour(juce::Slider::backgroundColourId, juce::Colour(0xff1a1a1a));
    
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a3a));
    setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff5a5a5a));
    setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    setColour(juce::TextButton::textColourOnId, juce::Colours::cyan);
}

// Static Color Definitions
juce::Colour ZenithLookAndFeel::Colors::background = juce::Colour(0xff121212);
juce::Colour ZenithLookAndFeel::Colors::panel = juce::Colour(0xff1e1e1e);
juce::Colour ZenithLookAndFeel::Colors::backgroundPanel = juce::Colour(0xff1e1e1e);
juce::Colour ZenithLookAndFeel::Colors::border = juce::Colour(0xff333333);
juce::Colour ZenithLookAndFeel::Colors::textPrimary = juce::Colours::white;
juce::Colour ZenithLookAndFeel::Colors::textSecondary = juce::Colours::lightgrey;
juce::Colour ZenithLookAndFeel::Colors::accent = juce::Colours::cyan;
juce::Colour ZenithLookAndFeel::Colors::accentGlow = juce::Colours::cyan.withAlpha(0.6f);

void ZenithLookAndFeel::Colors::setOledMode(bool enabled) {
    if (enabled) {
        background = juce::Colours::black;
        panel = juce::Colour(0xff050505); // Almost black
        backgroundPanel = juce::Colours::black;
        border = juce::Colour(0xff202020);
    } else {
        // Reset to default
        background = juce::Colour(0xff121212);
        panel = juce::Colour(0xff1e1e1e);
        backgroundPanel = juce::Colour(0xff1e1e1e);
        border = juce::Colour(0xff333333);
    }
}

void ZenithLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, const float rotaryStartAngle,
                                         const float rotaryEndAngle, juce::Slider& slider) {
    // Logic Pro style knob
    auto radius = (float)juce::jmin(width / 2, height / 2) - 4.0f;
    auto centreX = (float)x + (float)width * 0.5f;
    auto centreY = (float)y + (float)height * 0.5f;
    auto rx = centreX - radius;
    auto ry = centreY - radius;
    auto rw = radius * 2.0f;
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Fill
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillEllipse(rx, ry, rw, rw);

    // Outline
    g.setColour(juce::Colours::black);
    g.drawEllipse(rx, ry, rw, rw, 1.0f);

    // Indicator
    juce::Path p;
    auto pointerLength = radius * 0.8f;
    auto pointerThickness = 3.0f;
    p.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
    p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));

    g.setColour(juce::Colours::white);
    g.fillPath(p);
}

void ZenithLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float minSliderPos, float maxSliderPos,
                                         const juce::Slider::SliderStyle style, juce::Slider& slider) {
    // Logic Pro style fader
    auto trackWidth = 4.0f;
    
    juce::Point<float> startPoint((float)x + (float)width * 0.5f, (float)y + (float)height);
    juce::Point<float> endPoint((float)x + (float)width * 0.5f, (float)y);
    
    if (style == juce::Slider::LinearHorizontal) {
        startPoint = juce::Point<float>((float)x, (float)y + (float)height * 0.5f);
        endPoint = juce::Point<float>((float)x + (float)width, (float)y + (float)height * 0.5f);
    }

    // Track
    g.setColour(juce::Colour(0xff1a1a1a));
    g.drawLine(juce::Line<float>(startPoint, endPoint), trackWidth);
    
    // Thumb
    auto thumbWidth = 20.0f;
    auto thumbHeight = 10.0f;
    
    if (style == juce::Slider::LinearVertical) {
        g.setColour(juce::Colour(0xff4a4a4a));
        g.fillRect(startPoint.x - thumbWidth * 0.5f, sliderPos - thumbHeight * 0.5f, thumbWidth, thumbHeight);
        g.setColour(juce::Colours::black);
        g.drawRect(startPoint.x - thumbWidth * 0.5f, sliderPos - thumbHeight * 0.5f, thumbWidth, thumbHeight, 1.0f);
        g.setColour(juce::Colours::white);
        g.drawLine(startPoint.x - thumbWidth * 0.5f, sliderPos, startPoint.x + thumbWidth * 0.5f, sliderPos, 1.0f);
    }
}

void ZenithLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                             const juce::Colour& backgroundColour,
                                             bool shouldDrawButtonAsHighlighted,
                                             bool shouldDrawButtonAsDown) {
    auto cornerSize = 4.0f;
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);

    auto baseColour = backgroundColour.withMultipliedSaturation(button.hasKeyboardFocus(true) ? 1.3f : 0.9f)
                                      .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);

    if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
        baseColour = baseColour.contrasting(shouldDrawButtonAsDown ? 0.2f : 0.05f);

    g.setColour(baseColour);
    g.fillRoundedRectangle(bounds, cornerSize);

    g.setColour(button.findColour(juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
}

}

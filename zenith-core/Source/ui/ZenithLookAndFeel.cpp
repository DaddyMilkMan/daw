/**
 * @file ZenithLookAndFeel.cpp
 * @brief Implementation of Zenith DAW custom LookAndFeel
 */

#include "ZenithLookAndFeel.h"

//==============================================================================
// ZenithColours - Color palette definition
//==============================================================================

const juce::Colour ZenithColours::backgroundDark     = juce::Colour(0xff1a1a1a);
const juce::Colour ZenithColours::backgroundMedium   = juce::Colour(0xff2a2a2a);
const juce::Colour ZenithColours::backgroundLight    = juce::Colour(0xff3a3a3a);

const juce::Colour ZenithColours::accent             = juce::Colour(0xff4a9eff);
const juce::Colour ZenithColours::accentHover        = juce::Colour(0xff5aafff);
const juce::Colour ZenithColours::accentPressed      = juce::Colour(0xff3a8eef);

const juce::Colour ZenithColours::playButton         = juce::Colour(0xff4ade80);
const juce::Colour ZenithColours::stopButton         = juce::Colour(0xffef4444);
const juce::Colour ZenithColours::recordButton       = juce::Colour(0xfff87171);

const juce::Colour ZenithColours::track1             = juce::Colour(0xff8b5cf6);
const juce::Colour ZenithColours::track2             = juce::Colour(0xffec4899);
const juce::Colour ZenithColours::track3             = juce::Colour(0xff06b6d4);
const juce::Colour ZenithColours::track4             = juce::Colour(0xfff59e0b);

const juce::Colour ZenithColours::textPrimary        = juce::Colour(0xffffffff);
const juce::Colour ZenithColours::textSecondary      = juce::Colour(0xffa0a0a0);
const juce::Colour ZenithColours::border             = juce::Colour(0xff404040);
const juce::Colour ZenithColours::highlight          = juce::Colour(0xff4a9eff);

const juce::Colour ZenithColours::waveformFill       = juce::Colour(0xff4a9eff);
const juce::Colour ZenithColours::waveformOutline    = juce::Colour(0xff6ab0ff);

const juce::Colour ZenithColours::meterGreen         = juce::Colour(0xff22c55e);
const juce::Colour ZenithColours::meterYellow        = juce::Colour(0xffeab308);
const juce::Colour ZenithColours::meterRed           = juce::Colour(0xffef4444);

//==============================================================================
// ZenithLookAndFeel Implementation
//==============================================================================

ZenithLookAndFeel::ZenithLookAndFeel()
{
    // Set default colors for all component types
    setColour(juce::ResizableWindow::backgroundColourId, ZenithColours::backgroundDark);
    setColour(juce::DocumentWindow::backgroundColourId, ZenithColours::backgroundDark);
    setColour(juce::TextButton::buttonColourId, ZenithColours::backgroundMedium);
    setColour(juce::TextButton::textColourOffId, ZenithColours::textPrimary);
    setColour(juce::TextButton::textColourOnId, ZenithColours::textPrimary);
    setColour(juce::Label::textColourId, ZenithColours::textPrimary);
    setColour(juce::Slider::thumbColourId, ZenithColours::accent);
    setColour(juce::Slider::trackColourId, ZenithColours::backgroundLight);
    setColour(juce::Slider::backgroundColourId, ZenithColours::backgroundMedium);

    // Try to use Inter font, fallback to system sans-serif
    setDefaultSansSerifTypefaceName("Inter");
}

//==============================================================================
// Button drawing
//==============================================================================

void ZenithLookAndFeel::drawButtonBackground(juce::Graphics& g,
                                            juce::Button& button,
                                            const juce::Colour& backgroundColour,
                                            bool shouldDrawButtonAsHighlighted,
                                            bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
    auto cornerSize = 6.0f;

    // Get button color based on state and button type
    auto colour = getButtonBackgroundColour(button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

    // Draw shadow for depth (skip when pressed)
    if (!shouldDrawButtonAsDown)
    {
        g.setColour(juce::Colours::black.withAlpha(0.3f));
        g.fillRoundedRectangle(bounds.translated(0, 1), cornerSize);
    }

    // Draw button background with subtle gradient
    juce::ColourGradient gradient(
        colour.brighter(0.1f),
        bounds.getX(), bounds.getY(),
        colour.darker(0.1f),
        bounds.getX(), bounds.getBottom(),
        false
    );

    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, cornerSize);

    // Draw border highlight
    g.setColour(colour.brighter(0.2f));
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
}

void ZenithLookAndFeel::drawButtonText(juce::Graphics& g,
                                      juce::TextButton& button,
                                      bool shouldDrawButtonAsHighlighted,
                                      bool shouldDrawButtonAsDown)
{
    // W4: Use cached font instead of creating new one every paint
    g.setFont(buttonFont);
    g.setColour(button.findColour(button.getToggleState() ? juce::TextButton::textColourOnId
                                                          : juce::TextButton::textColourOffId)
                      .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f));

    auto yIndent = juce::jmin(4, button.proportionOfHeight(0.3f));
    auto cornerSize = juce::jmin(button.getHeight(), button.getWidth()) / 2;

    auto fontHeight = juce::roundToInt(buttonFont.getHeight() * 0.6f);
    auto leftIndent  = juce::jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnLeft() ? 4 : 2));
    auto rightIndent = juce::jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnRight() ? 4 : 2));
    auto textWidth = button.getWidth() - leftIndent - rightIndent;

    if (textWidth > 0)
    {
        auto offset = shouldDrawButtonAsDown ? 1 : 0;
        g.drawFittedText(button.getButtonText(),
                        leftIndent + offset, yIndent + offset,
                        textWidth, button.getHeight() - yIndent * 2 - offset,
                        juce::Justification::centred, 2);
    }
}

juce::Colour ZenithLookAndFeel::getButtonBackgroundColour(juce::Button& button,
                                                         bool isHighlighted,
                                                         bool isDown)
{
    // Check button text for special transport button colors
    auto buttonText = button.getButtonText().toLowerCase();

    if (buttonText.contains("play") || buttonText.contains("pause"))
        return isDown ? ZenithColours::playButton.darker(0.2f) :
               isHighlighted ? ZenithColours::playButton.brighter(0.1f) :
               ZenithColours::playButton;

    if (buttonText.contains("stop"))
        return isDown ? ZenithColours::stopButton.darker(0.2f) :
               isHighlighted ? ZenithColours::stopButton.brighter(0.1f) :
               ZenithColours::stopButton;

    if (buttonText.contains("record"))
        return isDown ? ZenithColours::recordButton.darker(0.2f) :
               isHighlighted ? ZenithColours::recordButton.brighter(0.1f) :
               ZenithColours::recordButton;

    // Default accent color for all other buttons
    if (isDown)
        return ZenithColours::accentPressed;
    if (isHighlighted)
        return ZenithColours::accentHover;

    return ZenithColours::accent;
}

//==============================================================================
// Slider drawing
//==============================================================================

void ZenithLookAndFeel::drawRotarySlider(juce::Graphics& g,
                                        int x, int y,
                                        int width, int height,
                                        float sliderPosProportional,
                                        float rotaryStartAngle,
                                        float rotaryEndAngle,
                                        juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>(x, y, width, height).reduced(10.0f);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto centreX = bounds.getCentreX();
    auto centreY = bounds.getCentreY();
    auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = radius * 0.085f;
    auto arcRadius = radius - lineW * 2.0f;

    // Draw arc background (track)
    juce::Path backgroundArc;
    backgroundArc.addCentredArc(centreX, centreY,
                               arcRadius, arcRadius,
                               0.0f,
                               rotaryStartAngle, rotaryEndAngle,
                               true);

    g.setColour(ZenithColours::backgroundLight);
    g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Draw arc fill (value indicator)
    juce::Path valueArc;
    valueArc.addCentredArc(centreX, centreY,
                          arcRadius, arcRadius,
                          0.0f,
                          rotaryStartAngle, angle,
                          true);

    g.setColour(ZenithColours::accent);
    g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Draw pointer indicator
    juce::Path pointer;
    auto pointerLength = radius * 0.5f;
    auto pointerThickness = lineW * 2.0f;

    pointer.addRectangle(-pointerThickness * 0.5f, -radius + lineW * 2.0f, pointerThickness, pointerLength);
    pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));

    g.setColour(ZenithColours::textPrimary);
    g.fillPath(pointer);

    // Draw center circle
    g.setColour(ZenithColours::backgroundMedium);
    g.fillEllipse(centreX - lineW * 1.5f, centreY - lineW * 1.5f, lineW * 3.0f, lineW * 3.0f);
}

void ZenithLookAndFeel::drawLinearSlider(juce::Graphics& g,
                                        int x, int y,
                                        int width, int height,
                                        float sliderPos,
                                        float minSliderPos,
                                        float maxSliderPos,
                                        const juce::Slider::SliderStyle style,
                                        juce::Slider& slider)
{
    auto trackWidth = juce::jmin(6.0f, (float)(style == juce::Slider::LinearHorizontal ? height : width) * 0.25f);

    juce::Point<float> startPoint(style == juce::Slider::LinearHorizontal ? x : x + width * 0.5f,
                                  style == juce::Slider::LinearHorizontal ? y + height * 0.5f : height + y);

    juce::Point<float> endPoint(style == juce::Slider::LinearHorizontal ? width + x : startPoint.x,
                               style == juce::Slider::LinearHorizontal ? startPoint.y : y);

    // Draw background track
    juce::Path backgroundTrack;
    backgroundTrack.startNewSubPath(startPoint);
    backgroundTrack.lineTo(endPoint);
    g.setColour(ZenithColours::backgroundLight);
    g.strokePath(backgroundTrack, {trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded});

    // Draw value track
    juce::Path valueTrack;
    juce::Point<float> minPoint, maxPoint;

    auto kx = style == juce::Slider::LinearHorizontal ? sliderPos : (x + width * 0.5f);
    auto ky = style == juce::Slider::LinearHorizontal ? (y + height * 0.5f) : sliderPos;

    minPoint = startPoint;
    maxPoint = {kx, ky};

    valueTrack.startNewSubPath(minPoint);
    valueTrack.lineTo(maxPoint);
    g.setColour(ZenithColours::accent);
    g.strokePath(valueTrack, {trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded});

    // Draw thumb
    auto thumbWidth = trackWidth * 2.5f;
    g.setColour(ZenithColours::textPrimary);
    g.fillEllipse(juce::Rectangle<float>(thumbWidth, thumbWidth).withCentre({kx, ky}));
}

//==============================================================================
// Label drawing
//==============================================================================

void ZenithLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    g.fillAll(label.findColour(juce::Label::backgroundColourId));

    if (!label.isBeingEdited())
    {
        auto alpha = label.isEnabled() ? 1.0f : 0.5f;
        const juce::Font font(getLabelFont(label));

        g.setColour(label.findColour(juce::Label::textColourId).withMultipliedAlpha(alpha));
        g.setFont(font);

        auto textArea = getLabelBorderSize(label).subtractedFrom(label.getLocalBounds());

        g.drawFittedText(label.getText(), textArea, label.getJustificationType(),
                        juce::jmax(1, (int)((float)textArea.getHeight() / font.getHeight())),
                        label.getMinimumHorizontalScale());

        g.setColour(label.findColour(juce::Label::outlineColourId).withMultipliedAlpha(alpha));
    }
    else if (label.isEnabled())
    {
        g.setColour(label.findColour(juce::Label::outlineColourId));
    }

    g.drawRect(label.getLocalBounds());
}

//==============================================================================
// ComboBox drawing
//==============================================================================

void ZenithLookAndFeel::drawComboBox(juce::Graphics& g,
                                    int width, int height,
                                    bool isButtonDown,
                                    int buttonX, int buttonY,
                                    int buttonW, int buttonH,
                                    juce::ComboBox& box)
{
    auto cornerSize = 6.0f;
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(2.0f);

    g.setColour(ZenithColours::backgroundMedium);
    g.fillRoundedRectangle(bounds, cornerSize);

    g.setColour(ZenithColours::border);
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);

    // W4: Draw dropdown arrow using cached path (only created once)
    auto arrowBounds = juce::Rectangle<int>(buttonX, buttonY, buttonW, buttonH).toFloat().reduced(4.0f);

    if (!comboBoxArrowInitialized)
    {
        // Build arrow path once at 10x10 unit square, will be scaled per use
        cachedComboBoxArrow.addTriangle(0.0f, 0.0f, 10.0f, 0.0f, 5.0f, 10.0f);
        comboBoxArrowInitialized = true;
    }

    // Scale and position cached arrow to fit bounds
    auto transform = juce::AffineTransform::scale(arrowBounds.getWidth() / 10.0f,
                                                   arrowBounds.getHeight() / 10.0f)
                        .translated(arrowBounds.getX(), arrowBounds.getY());

    g.setColour(ZenithColours::textSecondary);
    g.fillPath(cachedComboBoxArrow, transform);
}

//==============================================================================
// Scrollbar drawing
//==============================================================================

void ZenithLookAndFeel::drawScrollbar(juce::Graphics& g,
                                     juce::ScrollBar& scrollbar,
                                     int x, int y,
                                     int width, int height,
                                     bool isScrollbarVertical,
                                     int thumbStartPosition,
                                     int thumbSize,
                                     bool isMouseOver,
                                     bool isMouseDown)
{
    // Minimal scrollbar - only draw thumb, no track
    juce::Rectangle<int> thumbBounds;

    if (isScrollbarVertical)
        thumbBounds = {x + width / 4, thumbStartPosition, width / 2, thumbSize};
    else
        thumbBounds = {thumbStartPosition, y + height / 4, thumbSize, height / 2};

    auto colour = isMouseDown ? ZenithColours::accent :
                  isMouseOver ? ZenithColours::accentHover :
                  ZenithColours::backgroundLight;

    g.setColour(colour);
    g.fillRoundedRectangle(thumbBounds.toFloat(), 4.0f);
}

//==============================================================================
// Toggle button drawing
//==============================================================================

void ZenithLookAndFeel::drawToggleButton(juce::Graphics& g,
                                        juce::ToggleButton& button,
                                        bool shouldDrawButtonAsHighlighted,
                                        bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
    auto cornerSize = bounds.getHeight() * 0.5f;

    // Draw background
    if (button.getToggleState())
    {
        g.setColour(ZenithColours::accent);
    }
    else
    {
        g.setColour(ZenithColours::backgroundLight);
    }

    g.fillRoundedRectangle(bounds, cornerSize);

    // Draw border
    g.setColour(button.getToggleState() ? ZenithColours::accent.brighter(0.3f) : ZenithColours::border);
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);

    // Draw toggle circle
    auto circleSize = bounds.getHeight() - 8.0f;
    auto circleX = button.getToggleState() ? bounds.getRight() - circleSize - 4.0f : bounds.getX() + 4.0f;
    auto circleY = bounds.getY() + 4.0f;

    g.setColour(ZenithColours::textPrimary);
    g.fillEllipse(circleX, circleY, circleSize, circleSize);
}

//==============================================================================
// Level meter drawing
//==============================================================================

void ZenithLookAndFeel::drawLevelMeter(juce::Graphics& g,
                                      juce::Rectangle<float> bounds,
                                      float level,
                                      bool isHorizontal)
{
    level = juce::jlimit(0.0f, 1.0f, level);

    // Draw background
    g.setColour(ZenithColours::backgroundDark);
    g.fillRoundedRectangle(bounds, 2.0f);

    // Calculate fill rectangle
    juce::Rectangle<float> fillBounds;
    if (isHorizontal)
    {
        fillBounds = bounds.withWidth(bounds.getWidth() * level);
    }
    else
    {
        auto fillHeight = bounds.getHeight() * level;
        fillBounds = bounds.withTop(bounds.getBottom() - fillHeight).withHeight(fillHeight);
    }

    // Draw meter fill with color based on level
    juce::ColourGradient gradient;

    if (level < 0.7f)
    {
        gradient = juce::ColourGradient(
            ZenithColours::meterGreen,
            fillBounds.getX(), fillBounds.getY(),
            ZenithColours::meterGreen.darker(0.2f),
            fillBounds.getRight(), fillBounds.getBottom(),
            false
        );
    }
    else if (level < 0.9f)
    {
        gradient = juce::ColourGradient(
            ZenithColours::meterYellow,
            fillBounds.getX(), fillBounds.getY(),
            ZenithColours::meterYellow.darker(0.2f),
            fillBounds.getRight(), fillBounds.getBottom(),
            false
        );
    }
    else
    {
        gradient = juce::ColourGradient(
            ZenithColours::meterRed,
            fillBounds.getX(), fillBounds.getY(),
            ZenithColours::meterRed.darker(0.2f),
            fillBounds.getRight(), fillBounds.getBottom(),
            false
        );
    }

    g.setGradientFill(gradient);
    g.fillRoundedRectangle(fillBounds, 2.0f);

    // Draw border
    g.setColour(ZenithColours::border);
    g.drawRoundedRectangle(bounds, 2.0f, 1.0f);
}

//==============================================================================
// Waveform drawing
//==============================================================================

void ZenithLookAndFeel::drawWaveform(juce::Graphics& g,
                                    juce::Rectangle<float> bounds,
                                    const float* audioData,
                                    int numSamples)
{
    if (audioData == nullptr || numSamples == 0)
        return;

    juce::Path waveformPath;
    auto width = bounds.getWidth();
    auto height = bounds.getHeight();
    auto centerY = bounds.getCentreY();

    // Calculate how many samples per pixel
    auto samplesPerPixel = numSamples / width;
    if (samplesPerPixel < 1) samplesPerPixel = 1;

    // Build waveform path
    waveformPath.startNewSubPath(bounds.getX(), centerY);

    for (float x = 0; x < width; ++x)
    {
        int sampleIndex = (int)(x * samplesPerPixel);
        if (sampleIndex >= numSamples) break;

        float sample = audioData[sampleIndex];
        float y = centerY + (sample * height * 0.4f);

        waveformPath.lineTo(bounds.getX() + x, y);
    }

    // Draw filled waveform
    g.setColour(ZenithColours::waveformFill.withAlpha(0.5f));
    g.fillPath(waveformPath);

    // Draw waveform outline
    g.setColour(ZenithColours::waveformOutline);
    g.strokePath(waveformPath, juce::PathStrokeType(1.5f));
}

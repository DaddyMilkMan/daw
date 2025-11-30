#include "../../include/ui/ZenithSlider.h"

namespace zenith {

ZenithSlider::ZenithSlider(Orientation orientation)
    : orientation_(orientation)
{
    startTimerHz(60);
}

ZenithSlider::~ZenithSlider()
{
    stopTimer();
}

void ZenithSlider::setValue(float newValue, bool sendNotification)
{
    targetValue_ = juce::jlimit(0.0f, 1.0f, newValue);

    if (sendNotification && onValueChange)
        onValueChange(getDisplayValue());

    // Restart timer for smooth animation
    if (!isTimerRunning())
        startTimerHz(60);

    repaint();
}

void ZenithSlider::setRange(float min, float max, float defaultVal)
{
    minValue_ = min;
    maxValue_ = max;
    defaultValue_ = defaultVal;
    repaint();
}

float ZenithSlider::getDisplayValue() const
{
    return minValue_ + (value_ * (maxValue_ - minValue_));
}

void ZenithSlider::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    if (orientation_ == Vertical)
        drawVerticalSlider(g, bounds);
    else
        drawHorizontalSlider(g, bounds);
}

void ZenithSlider::resized()
{
    // Nothing to do here - paint handles everything
}

void ZenithSlider::timerCallback()
{
    bool needsAnimation = false;

    // Smooth animation towards target value
    const float speed = 0.3f;
    if (std::abs(targetValue_ - value_) >= 0.001f)
    {
        value_ += (targetValue_ - value_) * speed;
        needsAnimation = true;
    }
    else if (value_ != targetValue_)
    {
        value_ = targetValue_;
        needsAnimation = true; // One final repaint
    }

    // Smooth hover animation
    const float hoverSpeed = 0.2f;
    float targetHover = isHovered_ ? 1.0f : 0.0f;
    if (std::abs(targetHover - hoverAnimation_) >= 0.001f)
    {
        hoverAnimation_ += (targetHover - hoverAnimation_) * hoverSpeed;
        needsAnimation = true;
    }
    else if (hoverAnimation_ != targetHover)
    {
        hoverAnimation_ = targetHover;
        needsAnimation = true; // One final repaint
    }

    // Smooth drag animation
    const float dragSpeed = 0.3f;
    float targetDrag = isDragging_ ? 1.0f : 0.0f;
    if (std::abs(targetDrag - dragAnimation_) >= 0.001f)
    {
        dragAnimation_ += (targetDrag - dragAnimation_) * dragSpeed;
        needsAnimation = true;
    }
    else if (dragAnimation_ != targetDrag)
    {
        dragAnimation_ = targetDrag;
        needsAnimation = true; // One final repaint
    }

    if (needsAnimation)
    {
        repaint();
    }
    else
    {
        // All animations complete, stop timer to save CPU
        stopTimer();
    }
}

void ZenithSlider::mouseDown(const juce::MouseEvent& event)
{
    isDragging_ = true;
    dragStartPos_ = event.getPosition();
    dragStartValue_ = value_;

    // Restart timer for drag animation
    if (!isTimerRunning())
        startTimerHz(60);
}

void ZenithSlider::mouseDrag(const juce::MouseEvent& event)
{
    if (!isDragging_)
        return;

    auto dragDistance = orientation_ == Vertical
        ? -(event.getPosition().y - dragStartPos_.y)
        : (event.getPosition().x - dragStartPos_.x);

    float sensitivity = 1.0f / (orientation_ == Vertical ? getHeight() : getWidth());
    float newValue = dragStartValue_ + (dragDistance * sensitivity);

    setValue(newValue, true);
}

void ZenithSlider::mouseUp(const juce::MouseEvent&)
{
    isDragging_ = false;

    // Restart timer for drag animation fadeout
    if (!isTimerRunning())
        startTimerHz(60);
}

void ZenithSlider::mouseEnter(const juce::MouseEvent&)
{
    isHovered_ = true;

    // Restart timer for hover animation
    if (!isTimerRunning())
        startTimerHz(60);
}

void ZenithSlider::mouseExit(const juce::MouseEvent&)
{
    isHovered_ = false;

    // Restart timer for hover animation
    if (!isTimerRunning())
        startTimerHz(60);
}

void ZenithSlider::mouseDoubleClick(const juce::MouseEvent&)
{
    float normalizedDefault = (defaultValue_ - minValue_) / (maxValue_ - minValue_);
    setValue(normalizedDefault, true);
}

void ZenithSlider::drawVerticalSlider(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    auto trackBounds = bounds.reduced(8.0f);
    auto labelHeight = 20.0f;
    trackBounds.removeFromBottom(labelHeight);

    drawTrack(g, trackBounds);

    // Draw thumb
    float thumbY = trackBounds.getBottom() - (value_ * trackBounds.getHeight());
    auto thumbBounds = juce::Rectangle<float>(trackBounds.getX(), thumbY - 8.0f, trackBounds.getWidth(), 16.0f);
    drawThumb(g, thumbBounds);

    // Draw label
    if (label_.isNotEmpty())
    {
        auto labelBounds = bounds.withTop(bounds.getBottom() - labelHeight);
        drawLabel(g, labelBounds);
    }
}

void ZenithSlider::drawHorizontalSlider(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    auto trackBounds = bounds.reduced(8.0f);

    drawTrack(g, trackBounds);

    // Draw thumb
    float thumbX = trackBounds.getX() + (value_ * trackBounds.getWidth());
    auto thumbBounds = juce::Rectangle<float>(thumbX - 8.0f, trackBounds.getY(), 16.0f, trackBounds.getHeight());
    drawThumb(g, thumbBounds);
}

void ZenithSlider::drawTrack(juce::Graphics& g, const juce::Rectangle<float>& trackBounds)
{
    // Background track
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRoundedRectangle(trackBounds, 4.0f);

    // Value fill
    auto fillBounds = trackBounds;
    if (orientation_ == Vertical)
    {
        fillBounds = fillBounds.withTop(fillBounds.getBottom() - (value_ * fillBounds.getHeight()));
    }
    else
    {
        fillBounds = fillBounds.withWidth(value_ * fillBounds.getWidth());
    }

    g.setColour(juce::Colour(0xff0066cc));
    g.fillRoundedRectangle(fillBounds, 4.0f);
}

void ZenithSlider::drawThumb(juce::Graphics& g, const juce::Rectangle<float>& thumbBounds)
{
    float scale = 1.0f + (hoverAnimation_ * 0.1f) + (dragAnimation_ * 0.05f);
    auto scaledBounds = thumbBounds.withSizeKeepingCentre(
        thumbBounds.getWidth() * scale,
        thumbBounds.getHeight() * scale
    );

    // Shadow
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillEllipse(scaledBounds.translated(0.0f, 1.0f));

    // Thumb
    juce::ColourGradient gradient(
        juce::Colour(0xff4a9eff), scaledBounds.getCentreX(), scaledBounds.getY(),
        juce::Colour(0xff0066cc), scaledBounds.getCentreX(), scaledBounds.getBottom(),
        false
    );
    g.setGradientFill(gradient);
    g.fillEllipse(scaledBounds);

    // Highlight
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    auto highlightBounds = scaledBounds.withHeight(scaledBounds.getHeight() * 0.4f);
    g.fillEllipse(highlightBounds);
}

void ZenithSlider::drawLabel(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(12.0f);
    g.drawText(label_, bounds, juce::Justification::centred);
}

} // namespace zenith

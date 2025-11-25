{
  // Start animation timer at 60 Hz
  startTimerHz(60);
}

ZenithSlider::~ZenithSlider() { stopTimer(); }

//==============================================================================
// Value control
//==============================================================================

void ZenithSlider::setValue(float newValue, bool sendNotification) {
  targetValue_ = juce::jlimit(0.0f, 1.0f, newValue);

  if (sendNotification && onValueChange) {
    onValueChange(getDisplayValue());
  }

  repaint();
}

void ZenithSlider::setRange(float min, float max, float defaultValue) {
  minValue_ = min;
  maxValue_ = max;
  defaultValue_ = juce::jlimit(0.0f, 1.0f, (defaultValue - min) / (max - min));
}

float ZenithSlider::getDisplayValue() const {
  return minValue_ + (value_ * (maxValue_ - minValue_));
}

//==============================================================================
// Component interface
//==============================================================================

void ZenithSlider::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat();

  if (orientation_ == Vertical) {
    drawVerticalSlider(g, bounds);
  } else {
    drawHorizontalSlider(g, bounds);
  }
}

void ZenithSlider::resized() {
  // No special resizing needed
}

void ZenithSlider::timerCallback() {
  const float animationSpeed = 0.15f;

  // Smooth value animation
  value_ += (targetValue_ - value_) * animationSpeed;

  // Hover animation
  float targetHover = isHovered_ ? 1.0f : 0.0f;
  hoverAnimation_ += (targetHover - hoverAnimation_) * animationSpeed;

  // Drag animation
  float targetDrag = isDragging_ ? 1.0f : 0.0f;
  dragAnimation_ += (targetDrag - dragAnimation_) * animationSpeed;

  // Repaint if any animation is active
  if (std::abs(value_ - targetValue_) > 0.001f ||
      std::abs(hoverAnimation_ - targetHover) > 0.01f ||
      std::abs(dragAnimation_ - targetDrag) > 0.01f) {
    repaint();
  }
}

//==============================================================================
// Mouse interaction
//==============================================================================

void ZenithSlider::mouseDown(const juce::MouseEvent &event) {
  isDragging_ = true;
  dragStartPos_ = event.getPosition();
  dragStartValue_ = targetValue_;

  // If clicking on track (not dragging), jump to that position
  if (!event.mods.isShiftDown()) {
    auto bounds = getLocalBounds().toFloat();

    if (orientation_ == Vertical) {
      // Vertical: top = 1.0, bottom = 0.0
      float trackHeight = bounds.getHeight() - 40.0f; // Leave room for label
      float clickY = event.position.y;
      float newValue = 1.0f - (clickY / trackHeight);
      setValue(juce::jlimit(0.0f, 1.0f, newValue), true);
    } else {
      // Horizontal: left = 0.0, right = 1.0
      float trackWidth = bounds.getWidth() - 40.0f; // Leave room for label
      float clickX = event.position.x - 20.0f;
      float newValue = clickX / trackWidth;
      setValue(juce::jlimit(0.0f, 1.0f, newValue), true);
    }
  }
}

void ZenithSlider::mouseDrag(const juce::MouseEvent &event) {
  if (!isDragging_)
    return;

  auto dragDelta = event.getPosition() - dragStartPos_;
  float sensitivity =
      event.mods.isShiftDown() ? 0.002f : 0.005f; // Fine control with Shift

  float valueDelta = 0.0f;

  if (orientation_ == Vertical) {
    // Vertical: drag up = increase value
    valueDelta = -dragDelta.y * sensitivity;
  } else {
    // Horizontal: drag right = increase value
    valueDelta = dragDelta.x * sensitivity;
  }

  float newValue = dragStartValue_ + valueDelta;
  setValue(juce::jlimit(0.0f, 1.0f, newValue), true);
}

void ZenithSlider::mouseUp(const juce::MouseEvent &event) {
  isDragging_ = false;
}

void ZenithSlider::mouseEnter(const juce::MouseEvent &event) {
  isHovered_ = true;

  // Show tooltip with current value
  juce::String tooltipText = juce::String(getDisplayValue(), 2) + suffix_;
  setTooltip(tooltipText);
}

void ZenithSlider::mouseExit(const juce::MouseEvent &event) {
  isHovered_ = false;
}

void ZenithSlider::mouseDoubleClick(const juce::MouseEvent &event) {
  // Reset to default value
  setValue(defaultValue_, true);
}

//==============================================================================
// Drawing methods - Vertical Slider
//==============================================================================

void ZenithSlider::drawVerticalSlider(juce::Graphics &g,
                                      const juce::Rectangle<float> &bounds) {
  // Layout: [Track with thumb] [Label at bottom]
  auto trackBounds =
      bounds.withTrimmedBottom(20.0f).reduced(bounds.getWidth() * 0.3f, 4.0f);

  // Draw track
  drawTrack(g, trackBounds);

  // Calculate thumb position (top = 1.0, bottom = 0.0)
  float thumbY =
      trackBounds.getY() + (trackBounds.getHeight() * (1.0f - value_));
  float thumbSize = trackBounds.getWidth() * 1.5f;
  auto thumbBounds =
      juce::Rectangle<float>(thumbSize, thumbSize)
          .withCentre(juce::Point<float>(trackBounds.getCentreX(), thumbY));

  // Draw thumb
  drawThumb(g, thumbBounds);

  // Draw label at bottom
  auto labelBounds = bounds.withTop(trackBounds.getBottom() + 4.0f);
  drawLabel(g, labelBounds);

  // Draw value text on hover or drag
  if (isHovered_ || isDragging_) {
    juce::String valueText = juce::String(getDisplayValue(), 1) + suffix_;
    g.setColour(juce::Colour(0xffffffff).withAlpha(0.9f));
    g.setFont(11.0f);

    auto valueTextBounds =
        thumbBounds.translated(thumbBounds.getWidth() + 4.0f, 0.0f);
    g.drawText(valueText, valueTextBounds, juce::Justification::centredLeft);
  }
}

//==============================================================================
// Drawing methods - Horizontal Slider
//==============================================================================

void ZenithSlider::drawHorizontalSlider(juce::Graphics &g,
                                        const juce::Rectangle<float> &bounds) {
  // Layout: [Label on left] [Track with thumb]
  auto labelBounds = bounds.withWidth(bounds.getWidth() * 0.3f);
  auto trackBounds = bounds.withTrimmedLeft(labelBounds.getWidth() + 4.0f)
                         .reduced(4.0f, bounds.getHeight() * 0.3f);

  // Draw track
  drawTrack(g, trackBounds);

  // Calculate thumb position (left = 0.0, right = 1.0)
  float thumbX = trackBounds.getX() + (trackBounds.getWidth() * value_);
  float thumbSize = trackBounds.getHeight() * 1.5f;
  auto thumbBounds =
      juce::Rectangle<float>(thumbSize, thumbSize)
          .withCentre(juce::Point<float>(thumbX, trackBounds.getCentreY()));

  // Draw thumb
  drawThumb(g, thumbBounds);

  // Draw label on left
  drawLabel(g, labelBounds);

  // Draw value text on hover or drag (above track)
  if (isHovered_ || isDragging_) {
    juce::String valueText = juce::String(getDisplayValue(), 1) + suffix_;
    g.setColour(juce::Colour(0xffffffff).withAlpha(0.9f));
    g.setFont(11.0f);

    auto valueTextBounds = thumbBounds.withY(thumbBounds.getY() - 16.0f);
    g.drawText(valueText, valueTextBounds, juce::Justification::centred);
  }
}

//==============================================================================
// Drawing methods - Track
//==============================================================================

void ZenithSlider::drawTrack(juce::Graphics &g,
                             const juce::Rectangle<float> &trackBounds) {
  auto center = trackBounds.getCentre();
  float cornerRadius =
      std::min(trackBounds.getWidth(), trackBounds.getHeight()) * 0.5f;

  // Background track with gradient (dark)
  juce::ColourGradient backgroundGradient;

  if (orientation_ == Vertical) {
    // Vertical: lighter at top, darker at bottom
    backgroundGradient = juce::ColourGradient(
        juce::Colour(0xff2a2a2a), center.x, trackBounds.getY(),
        juce::Colour(0xff1a1a1a), center.x, trackBounds.getBottom(), false);
  } else {
    // Horizontal: lighter at left, darker at right
    backgroundGradient = juce::ColourGradient(
        juce::Colour(0xff2a2a2a), trackBounds.getX(), center.y,
        juce::Colour(0xff1a1a1a), trackBounds.getRight(), center.y, false);
  }

  g.setGradientFill(backgroundGradient);
  g.fillRoundedRectangle(trackBounds, cornerRadius);

  // Filled portion (value track) with Apple blue gradient
  juce::Rectangle<float> filledBounds;

  if (orientation_ == Vertical) {
    // Vertical: fill from bottom up to value position
    float fillHeight = trackBounds.getHeight() * value_;
    filledBounds = trackBounds.removeFromBottom(fillHeight);
  } else {
    // Horizontal: fill from left to value position
    float fillWidth = trackBounds.getWidth() * value_;
    filledBounds = trackBounds.removeFromLeft(fillWidth);
  }

  // Value track gradient (Apple blue)
  juce::ColourGradient valueGradient;

  if (orientation_ == Vertical) {
    valueGradient = juce::ColourGradient(
        juce::Colour(0xff5ab4ff), filledBounds.getCentreX(),
        filledBounds.getY(), juce::Colour(0xff4a9eff),
        filledBounds.getCentreX(), filledBounds.getBottom(), false);
  } else {
    valueGradient = juce::ColourGradient(
        juce::Colour(0xff4a9eff), filledBounds.getX(),
        filledBounds.getCentreY(), juce::Colour(0xff5ab4ff),
        filledBounds.getRight(), filledBounds.getCentreY(), false);
  }

  g.setGradientFill(valueGradient);
  g.fillRoundedRectangle(filledBounds, cornerRadius);

  // Hover glow around entire track
  if (isHovered_ && hoverAnimation_ > 0.01f) {
    float glowAlpha = 0.2f * hoverAnimation_;
    g.setColour(juce::Colour(0xff4a9eff).withAlpha(glowAlpha));
    g.drawRoundedRectangle(trackBounds.expanded(1.0f), cornerRadius, 2.0f);
  }
}

//==============================================================================
// Drawing methods - Thumb
//==============================================================================

void ZenithSlider::drawThumb(juce::Graphics &g,
                             const juce::Rectangle<float> &thumbBounds) {
  auto center = thumbBounds.getCentre();
  float radius = thumbBounds.getWidth() * 0.5f;

  // Scale thumb on hover/drag
  float scale = 1.0f + (hoverAnimation_ * 0.08f) + (dragAnimation_ * 0.05f);
  radius *= scale;

  // Shadow
  g.setColour(juce::Colour(0x00000000).withAlpha(0.5f));
  g.fillEllipse(center.x - radius, center.y - radius + 2.0f, radius * 2.0f,
                radius * 2.0f);

  // Thumb body with gradient (lighter at top, darker at bottom)
  juce::ColourGradient thumbGradient(
      juce::Colour(0xff4a4a4a), center.x, center.y - radius,
      juce::Colour(0xff2a2a2a), center.x, center.y + radius, false);
  g.setGradientFill(thumbGradient);
  g.fillEllipse(center.x - radius, center.y - radius, radius * 2.0f,
                radius * 2.0f);

  // Highlight at top (40%)
  juce::Path highlightPath;
  highlightPath.addEllipse(center.x - radius, center.y - radius, radius * 2.0f,
                           radius * 0.8f);
  g.setColour(juce::Colour(0xffffffff).withAlpha(0.15f));
  g.fillPath(highlightPath);

  // Center dot (Apple style)
  float dotRadius = radius * 0.15f;
  g.setColour(juce::Colour(0xffffffff).withAlpha(0.4f));
  g.fillEllipse(center.x - dotRadius, center.y - dotRadius, dotRadius * 2.0f,
                dotRadius * 2.0f);

  // Hover glow
  if (isHovered_) {
    float glowAlpha = 0.4f * hoverAnimation_;
    g.setColour(juce::Colour(0xff4a9eff).withAlpha(glowAlpha));
    g.drawEllipse(center.x - radius - 2.0f, center.y - radius - 2.0f,
                  (radius + 2.0f) * 2.0f, (radius + 2.0f) * 2.0f, 2.0f);
  }

  // Drag glow (brighter)
  if (isDragging_) {
    float dragGlowAlpha = 0.3f * dragAnimation_;
    g.setColour(juce::Colour(0xff34c759).withAlpha(dragGlowAlpha));
    g.drawEllipse(center.x - radius - 3.0f, center.y - radius - 3.0f,
                  (radius + 3.0f) * 2.0f, (radius + 3.0f) * 2.0f, 3.0f);
  }
}

//==============================================================================
// Drawing methods - Label
//==============================================================================

void ZenithSlider::drawLabel(juce::Graphics &g,
                             const juce::Rectangle<float> &bounds) {
  if (label_.isEmpty())
    return;

  g.setColour(juce::Colour(0xffcccccc).withAlpha(0.8f));
  g.setFont(12.0f);
  g.drawText(label_, bounds, juce::Justification::centred);
}

} // namespace zenith

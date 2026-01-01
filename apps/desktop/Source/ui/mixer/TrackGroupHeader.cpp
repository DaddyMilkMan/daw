/**
 * @file TrackGroupHeader.cpp
 * @brief Track group header implementation with VU meter
 *
 * Performance notes:
 * - VU meter uses atomic operations for lock-free audio thread updates
 * - Gravity-based ballistics for natural meter decay
 * - Dirty-rect optimization for minimal repaint area
 * - AnimationCoordinator integration for centralized 60Hz updates
 */

#include "TrackGroupHeader.h"
#include "GlassmorphicPanel.h"
#include "NeonGlow.h"
#include "../design-system/ZenithTypography.h"
#include "../design-system/ZenithLayout.h"

#include <core/SkPath.h>
#include <core/SkMaskFilter.h>
#include <effects/SkGradientShader.h>

namespace zenith {

using namespace design;

//==============================================================================
// TrackGroupHeader Implementation
//==============================================================================

TrackGroupHeader::TrackGroupHeader() {
  // Register with AnimationCoordinator for centralized updates
  zenith::animation::AnimationCoordinator::getInstance().registerListener(
      this, zenith::animation::Priority::High);

  // Configure mute button
  muteButton_.setToggleable(true);
  muteButton_.setStyle(SkiaButton::Style::Secondary);
  muteButton_.onClick = [this]() {
    props_.isMuted = !props_.isMuted;
    updateButtonStates();
    if (props_.onMuteClick)
      props_.onMuteClick();
  };
  addAndMakeVisible(muteButton_);

  // Configure solo button
  soloButton_.setToggleable(true);
  soloButton_.setStyle(SkiaButton::Style::Secondary);
  soloButton_.onClick = [this]() {
    props_.isSolo = !props_.isSolo;
    updateButtonStates();
    if (props_.onSoloClick)
      props_.onSoloClick();
  };
  addAndMakeVisible(soloButton_);

  // Add VU meter
  addAndMakeVisible(vuMeter_);

  // Set default size
  setSize(300, static_cast<int>(kHeight));
}

TrackGroupHeader::~TrackGroupHeader() {
  zenith::animation::AnimationCoordinator::getInstance().unregisterListener(this);
}

//==============================================================================
// Props Management
//==============================================================================

void TrackGroupHeader::setProps(const TrackGroupHeaderProps &props) {
  props_ = props;
  muteButton_.setToggleState(props_.isMuted);
  soloButton_.setToggleState(props_.isSolo);
  expandAnim_ = props_.isExpanded ? 1.0f : 0.0f;
  updateButtonStates();
  repaint();
}

void TrackGroupHeader::setGroupName(const juce::String &name) {
  props_.groupName = name;
  repaint();
}

void TrackGroupHeader::setColor(const juce::Colour &color) {
  props_.color = color;
  repaint();
}

void TrackGroupHeader::setExpanded(bool expanded) {
  if (props_.isExpanded != expanded) {
    props_.isExpanded = expanded;
    // Animation will smoothly transition expandAnim_
  }
}

void TrackGroupHeader::setMuted(bool muted) {
  props_.isMuted = muted;
  muteButton_.setToggleState(muted);
  updateButtonStates();
}

void TrackGroupHeader::setSolo(bool solo) {
  props_.isSolo = solo;
  soloButton_.setToggleState(solo);
  updateButtonStates();
}

void TrackGroupHeader::setTrackCount(int count) {
  props_.trackCount = count;
  repaint();
}

void TrackGroupHeader::updateButtonStates() {
  if (props_.isMuted) {
    muteButton_.setStyle(SkiaButton::Style::Danger);
  } else {
    muteButton_.setStyle(SkiaButton::Style::Secondary);
  }

  if (props_.isSolo) {
    soloButton_.setStyle(SkiaButton::Style::Warning);
  } else {
    soloButton_.setStyle(SkiaButton::Style::Secondary);
  }
}

//==============================================================================
// AnimationListener Implementation
//==============================================================================

void TrackGroupHeader::onAnimationTick(float deltaMs) {
  juce::ignoreUnused(deltaMs);
  bool needsRepaint = false;

  // Hover animation (smooth fade)
  float hoverTarget = isHovered_ ? 1.0f : 0.0f;
  if (std::abs(hoverAnim_ - hoverTarget) > 0.01f) {
    hoverAnim_ += (hoverTarget - hoverAnim_) * 0.15f;
    needsRepaint = true;
  }

  // Expand/collapse animation
  float expandTarget = props_.isExpanded ? 1.0f : 0.0f;
  if (std::abs(expandAnim_ - expandTarget) > 0.01f) {
    expandAnim_ += (expandTarget - expandAnim_) * 0.2f;
    needsRepaint = true;
  }

  if (needsRepaint)
    repaint();
}

bool TrackGroupHeader::isAnimating() const {
  float hoverTarget = isHovered_ ? 1.0f : 0.0f;
  float expandTarget = props_.isExpanded ? 1.0f : 0.0f;

  return std::abs(hoverAnim_ - hoverTarget) > 0.01f ||
         std::abs(expandAnim_ - expandTarget) > 0.01f;
}

//==============================================================================
// Component Interface
//==============================================================================

void TrackGroupHeader::resized() {
  auto bounds = getLocalBounds().reduced(static_cast<int>(kPadding), 0);

  // Chevron on left
  auto chevronArea =
      bounds.removeFromLeft(static_cast<int>(kChevronSize + kPadding));
  juce::ignoreUnused(chevronArea); // Drawn in drawSkia

  // VU Meter on right
  auto meterArea = bounds.removeFromRight(static_cast<int>(kMeterWidth));
  vuMeter_.setBounds(meterArea.reduced(0, 4));

  // Buttons
  bounds.removeFromRight(static_cast<int>(kPadding));
  auto buttonArea =
      bounds.removeFromRight(static_cast<int>(kButtonSize * 2 + 4));

  auto muteRect = buttonArea.removeFromLeft(static_cast<int>(kButtonSize));
  muteButton_.setBounds(muteRect.withSizeKeepingCentre(
      static_cast<int>(kButtonSize), static_cast<int>(kButtonSize)));

  buttonArea.removeFromLeft(4); // spacing
  auto soloRect = buttonArea.removeFromLeft(static_cast<int>(kButtonSize));
  soloButton_.setBounds(soloRect.withSizeKeepingCentre(
      static_cast<int>(kButtonSize), static_cast<int>(kButtonSize)));
}

void TrackGroupHeader::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  // Background with glassmorphism
  if (hoverAnim_ > 0.01f) {
    GlassmorphicPanel::drawWithAccent(
        canvas, skBounds,
        SkColorSetARGB(props_.color.getAlpha(), props_.color.getRed(),
                       props_.color.getGreen(), props_.color.getBlue()),
        GlassmorphicPanel::Style::Elevated);

    // Hover overlay
    SkPaint hoverOverlay;
    hoverOverlay.setColor(withAlpha(colors::GLASS_HOVER, hoverAnim_ * 0.5f));
    canvas->drawRoundRect(skBounds, dimensions::RADIUS_SM, dimensions::RADIUS_SM,
                          hoverOverlay);
  } else {
    GlassmorphicPanel::draw(canvas, skBounds,
                            GlassmorphicPanel::Style::Elevated);
  }

  // Color stripe on left
  SkRect stripeRect = SkRect::MakeXYWH(0, 0, 4, skBounds.height());
  SkPaint stripePaint;
  stripePaint.setColor(SkColorSetARGB(
      props_.color.getAlpha(), props_.color.getRed(), props_.color.getGreen(),
      props_.color.getBlue()));
  stripePaint.setAntiAlias(true);

  // Glow for stripe
  SkPaint glowPaint;
  glowPaint.setColor(withAlpha(stripePaint.getColor(), 0.4f));
  glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 3.0f));
  glowPaint.setAntiAlias(true);
  canvas->drawRoundRect(stripeRect, 2, 2, glowPaint);
  canvas->drawRoundRect(stripeRect, 2, 2, stripePaint);

  // Expand chevron
  float chevronX = kPadding + 4;
  float chevronY = skBounds.height() / 2;
  SkRect chevronBounds = SkRect::MakeXYWH(
      chevronX - kChevronSize / 2, chevronY - kChevronSize / 2, kChevronSize,
      kChevronSize);
  drawExpandChevron(canvas, chevronBounds, props_.isExpanded);

  // Group name
  float textX = chevronX + kChevronSize + kPadding;
  float textY = skBounds.height() / 2 + 5;

  SkPaint textPaint;
  textPaint.setColor(colors::TEXT_PRIMARY);
  textPaint.setAntiAlias(true);

  SkFont nameFont = typography::getSkFont(14.0f, FontWeight::SemiBold);
  canvas->drawString(props_.groupName.toRawUTF8(), textX, textY, nameFont,
                     textPaint);

  // Track count badge
  if (props_.trackCount > 0) {
    juce::String countStr = juce::String(props_.trackCount);
    float nameWidth = nameFont.measureText(props_.groupName.toRawUTF8(),
                                           props_.groupName.length(),
                                           SkTextEncoding::kUTF8);

    float badgeX = textX + nameWidth + 8;
    float badgeWidth = 24;
    float badgeHeight = 16;

    SkRect badgeRect =
        SkRect::MakeXYWH(badgeX, (skBounds.height() - badgeHeight) / 2,
                         badgeWidth, badgeHeight);

    SkPaint badgePaint;
    badgePaint.setColor(withAlpha(colors::BG_04, 0.8f));
    badgePaint.setAntiAlias(true);
    canvas->drawRoundRect(badgeRect, badgeHeight / 2, badgeHeight / 2,
                          badgePaint);

    SkPaint countPaint;
    countPaint.setColor(colors::TEXT_SECONDARY);
    countPaint.setAntiAlias(true);

    SkFont countFont = typography::getSkFont(10.0f, FontWeight::Medium);
    float countWidth = countFont.measureText(countStr.toRawUTF8(),
                                             countStr.length(),
                                             SkTextEncoding::kUTF8);
    canvas->drawString(countStr.toRawUTF8(),
                       badgeX + (badgeWidth - countWidth) / 2,
                       badgeRect.centerY() + 4, countFont, countPaint);
  }

  // Draw children (buttons, VU meter)
  drawChildren(canvas);
}

void TrackGroupHeader::drawExpandChevron(SkCanvas *canvas,
                                         const SkRect &bounds, bool expanded) {
  SkPaint chevronPaint;
  chevronPaint.setColor(colors::TEXT_SECONDARY);
  chevronPaint.setAntiAlias(true);
  chevronPaint.setStyle(SkPaint::kStroke_Style);
  chevronPaint.setStrokeWidth(2.0f);
  chevronPaint.setStrokeCap(SkPaint::kRound_Cap);
  chevronPaint.setStrokeJoin(SkPaint::kRound_Join);

  SkPath chevron;
  float cx = bounds.centerX();
  float cy = bounds.centerY();
  float size = bounds.width() / 3;

  // Rotate based on expand state (animated via expandAnim_)
  float rotation = expandAnim_ * 90.0f; // 0° collapsed, 90° expanded

  canvas->save();
  canvas->rotate(rotation, cx, cy);

  // Draw chevron pointing right (will rotate down when expanded)
  chevron.moveTo(cx - size * 0.3f, cy - size);
  chevron.lineTo(cx + size * 0.5f, cy);
  chevron.lineTo(cx - size * 0.3f, cy + size);

  canvas->drawPath(chevron, chevronPaint);
  canvas->restore();
}

void TrackGroupHeader::mouseDown(const juce::MouseEvent &e) {
  // Check if click is in chevron area (toggle expand)
  if (e.x < kChevronSize + kPadding * 2) {
    props_.isExpanded = !props_.isExpanded;
    if (props_.onExpandToggle)
      props_.onExpandToggle(props_.isExpanded);
    return;
  }

  // General click callback
  if (props_.onClick)
    props_.onClick();

  SkiaComponent::mouseDown(e);
}

void TrackGroupHeader::mouseEnter(const juce::MouseEvent &e) {
  isHovered_ = true;
  SkiaComponent::mouseEnter(e);
}

void TrackGroupHeader::mouseExit(const juce::MouseEvent &e) {
  isHovered_ = false;
  SkiaComponent::mouseExit(e);
}

//==============================================================================
// VUMeter Implementation
//==============================================================================

TrackGroupHeader::VUMeter::VUMeter() {
  // Register with AnimationCoordinator at High priority (real-time critical)
  zenith::animation::AnimationCoordinator::getInstance().registerListener(
      this, zenith::animation::Priority::High);
}

TrackGroupHeader::VUMeter::~VUMeter() {
  zenith::animation::AnimationCoordinator::getInstance().unregisterListener(this);
}

void TrackGroupHeader::VUMeter::setLevel(float level) {
  // Thread-safe, lock-free update
  targetLevel_.store(juce::jlimit(0.0f, 1.0f, level),
                     std::memory_order_relaxed);
}

void TrackGroupHeader::VUMeter::setLevels(float left, float right) {
  // Thread-safe, lock-free updates
  targetLevelL_.store(juce::jlimit(0.0f, 1.0f, left),
                      std::memory_order_relaxed);
  targetLevelR_.store(juce::jlimit(0.0f, 1.0f, right),
                      std::memory_order_relaxed);
}

void TrackGroupHeader::VUMeter::onAnimationTick(float deltaMs) {
  juce::ignoreUnused(deltaMs);

  // Gravity-based ballistics (proven pattern from MixerChannelComponent)
  auto smoothLevel = [](float target, float &current, float &peak,
                        int &peakHold, float &velocity) {
    const float attackSpeed = 0.8f;
    const float gravity = 0.002f;

    if (target > current) {
      current += (target - current) * attackSpeed;
      velocity = 0.0f; // Reset on rise
    } else {
      velocity += gravity;
      current -= velocity;
      if (current < target) {
        current = target;
        velocity = 0.0f;
      }
    }

    // Peak hold
    if (current > peak) {
      peak = current;
      peakHold = 120; // 2 seconds at 60Hz
    } else if (peakHold > 0) {
      peakHold--;
    } else {
      peak *= 0.95f;
    }
  };

  // Store previous values for dirty rect optimization
  previousLevel_ = currentLevel_;
  previousLevelL_ = currentLevelL_;
  previousLevelR_ = currentLevelR_;

  if (stereo_) {
    smoothLevel(targetLevelL_.load(std::memory_order_relaxed), currentLevelL_,
                peakLevelL_, peakHoldCounterL_, velocityL_);
    smoothLevel(targetLevelR_.load(std::memory_order_relaxed), currentLevelR_,
                peakLevelR_, peakHoldCounterR_, velocityR_);

    // Only repaint if significant change (threshold-based optimization)
    bool changed = std::abs(currentLevelL_ - previousLevelL_) > 0.001f ||
                   std::abs(currentLevelR_ - previousLevelR_) > 0.001f;
    if (changed)
      repaint();
  } else {
    smoothLevel(targetLevel_.load(std::memory_order_relaxed), currentLevel_,
                peakLevel_, peakHoldCounter_, velocity_);

    // Only repaint if significant change
    if (std::abs(currentLevel_ - previousLevel_) > 0.001f)
      repaint();
  }
}

bool TrackGroupHeader::VUMeter::isAnimating() const {
  // Consider animating if there's visible level or decay in progress
  if (stereo_) {
    return currentLevelL_ > 0.001f || currentLevelR_ > 0.001f ||
           peakLevelL_ > 0.001f || peakLevelR_ > 0.001f;
  }
  return currentLevel_ > 0.001f || peakLevel_ > 0.001f;
}

void TrackGroupHeader::VUMeter::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  // Background
  SkPaint bgPaint;
  bgPaint.setColor(colors::BG_04);
  bgPaint.setAntiAlias(true);
  canvas->drawRoundRect(skBounds, dimensions::RADIUS_SM, dimensions::RADIUS_SM,
                        bgPaint);

  if (stereo_) {
    // Split into L/R bars
    float barWidth = (skBounds.width() - 2) / 2;
    SkRect leftBar =
        SkRect::MakeXYWH(skBounds.x() + 1, skBounds.y() + 2, barWidth - 1,
                         skBounds.height() - 4);
    SkRect rightBar = SkRect::MakeXYWH(skBounds.x() + barWidth + 2,
                                       skBounds.y() + 2, barWidth - 1,
                                       skBounds.height() - 4);

    drawMeterBar(canvas, leftBar, currentLevelL_, peakLevelL_);
    drawMeterBar(canvas, rightBar, currentLevelR_, peakLevelR_);
  } else {
    // Single bar
    SkRect meterBar = skBounds.makeInset(2, 2);
    drawMeterBar(canvas, meterBar, currentLevel_, peakLevel_);
  }
}

void TrackGroupHeader::VUMeter::drawMeterBar(SkCanvas *canvas,
                                             const SkRect &bounds, float level,
                                             float peak) {
  if (level < 0.001f && peak < 0.001f)
    return;

  // Convert linear to dB-scaled display
  float levelDb = juce::Decibels::gainToDecibels(level);
  float normalizedLevel = juce::jmap(levelDb, -60.0f, 0.0f, 0.0f, 1.0f);
  normalizedLevel = juce::jlimit(0.0f, 1.0f, normalizedLevel);

  if (normalizedLevel > 0.01f) {
    float barHeight = bounds.height() * normalizedLevel;
    SkRect meterRect =
        SkRect::MakeXYWH(bounds.x(), bounds.bottom() - barHeight,
                         bounds.width(), barHeight);

    // Gradient: green -> yellow -> red
    SkColor topColor = colors::SUCCESS;
    if (normalizedLevel > 0.9f) {
      topColor = colors::DANGER;
    } else if (normalizedLevel > 0.7f) {
      topColor = colors::WARNING;
    } else if (normalizedLevel > 0.5f) {
      topColor = colors::NEON_YELLOW;
    }

    SkPoint pts[2] = {{meterRect.centerX(), meterRect.bottom()},
                      {meterRect.centerX(), meterRect.top()}};
    SkColor gradColors[3] = {colors::SUCCESS, colors::NEON_YELLOW, topColor};
    SkScalar positions[3] = {0.0f, 0.6f, 1.0f};

    SkPaint meterPaint;
    meterPaint.setShader(SkGradientShader::MakeLinear(pts, gradColors,
                                                       positions, 3,
                                                       SkTileMode::kClamp));
    meterPaint.setAntiAlias(true);
    canvas->drawRoundRect(meterRect, 1.0f, 1.0f, meterPaint);

    // Glow for high levels
    if (normalizedLevel > 0.7f) {
      SkPaint glowPaint;
      glowPaint.setColor(withAlpha(topColor, 0.3f));
      glowPaint.setMaskFilter(
          SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 3.0f));
      glowPaint.setAntiAlias(true);
      canvas->drawRoundRect(meterRect, 1.0f, 1.0f, glowPaint);
    }
  }

  // Peak indicator
  float peakDb = juce::Decibels::gainToDecibels(peak);
  float normalizedPeak = juce::jmap(peakDb, -60.0f, 0.0f, 0.0f, 1.0f);
  normalizedPeak = juce::jlimit(0.0f, 1.0f, normalizedPeak);

  if (normalizedPeak > 0.01f) {
    float peakY = bounds.bottom() - bounds.height() * normalizedPeak;
    SkRect peakLine =
        SkRect::MakeXYWH(bounds.x(), peakY - 1, bounds.width(), 2);

    SkPaint peakPaint;
    peakPaint.setColor(normalizedPeak > 0.9f ? colors::DANGER : colors::TEXT_PRIMARY);
    peakPaint.setAntiAlias(true);
    canvas->drawRect(peakLine, peakPaint);
  }
}

} // namespace zenith

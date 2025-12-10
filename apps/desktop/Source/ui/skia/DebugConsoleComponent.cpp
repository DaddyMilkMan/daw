/*
  ==============================================================================

    DebugConsoleComponent.cpp
    Created: 2025-12-07
    Author:  Zenith DAW AI Team

    Implementation of the Debug Console UI Component.

  ==============================================================================
*/

#include "DebugConsoleComponent.h"

#ifdef ZENITH_USE_SKIA
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkFont.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <core/SkPoint.h>
#include <core/SkRRect.h>
#include <effects/SkGradientShader.h>

#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

//==============================================================================
DebugConsoleComponent::DebugConsoleComponent(ai::SessionDebuggerAgent &debugger)
    : debugger_(debugger) {
  debugger_.addListener(this);
  updateCachedPaints();

  setSize(300, static_cast<int>(kCollapsedHeight));

  // Start animation timer
  startTimerHz(30); // 30 FPS for smooth animations
}

DebugConsoleComponent::~DebugConsoleComponent() {
  stopTimer();
  debugger_.removeListener(this);
}

//==============================================================================
void DebugConsoleComponent::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  // Draw background with rounded corners
  SkRRect rrect;
  rrect.setRectXY(skBounds, kCornerRadius, kCornerRadius);
  canvas->drawRRect(rrect, bgPaint_);

  // Draw border
  canvas->drawRRect(rrect, borderPaint_);

  if (isExpanded_) {
    drawExpandedView(canvas, skBounds);
  } else {
    drawCollapsedView(canvas, skBounds);
  }
}

void DebugConsoleComponent::drawCollapsedView(SkCanvas *canvas,
                                              const SkRect &bounds) {
  float centerY = bounds.height() / 2.0f;

  // Health indicator (pulsing circle)
  drawHealthIndicator(canvas, kPadding + 8.0f, centerY, 12.0f);

  // Status text
  textPaint_.setColor(SK_ColorWHITE);

  int issueCount = debugger_.getUnresolvedIssueCount();
  int fixCount = debugger_.getFixCount();

  juce::String statusText;
  if (issueCount == 0 && fixCount == 0) {
    statusText = "Session OK";
  } else if (issueCount == 0 && fixCount > 0) {
    statusText = debugger_.getFixSummary();
  } else {
    statusText = juce::String(issueCount) + " Issue" +
                 (issueCount != 1 ? "s" : "") + " Detected";
  }

  canvas->drawString(statusText.toStdString().c_str(), kPadding + 28.0f,
                     centerY + 4.0f, font_, textPaint_);

  // Quick status icons on the right
  drawStatusIcons(canvas, bounds.width() - kPadding - 80.0f, centerY);

  // Notification badge if there was a recent fix
  if (showNewFixNotification_) {
    drawNotificationBadge(canvas, bounds.width() - kPadding - 10.0f, 8.0f);
  }
}

void DebugConsoleComponent::drawExpandedView(SkCanvas *canvas,
                                             const SkRect &bounds) {
  float y = kPadding;

  // Header row: Health score and title
  drawHealthIndicator(canvas, kPadding + 8.0f, y + 8.0f, 12.0f);

  boldFont_.setSize(14.0f);
  textPaint_.setColor(SK_ColorWHITE);
  canvas->drawString("Session Debugger", kPadding + 28.0f, y + 12.0f, boldFont_,
                     textPaint_);

  // Health score on the right
  juce::String healthText =
      juce::String(static_cast<int>(displayedHealthScore_)) + "%";
  SkColor healthColor = getHealthColor(displayedHealthScore_);
  textPaint_.setColor(healthColor);
  canvas->drawString(healthText.toStdString().c_str(),
                     bounds.width() - kPadding - 40.0f, y + 12.0f, boldFont_,
                     textPaint_);

  y += 28.0f;

  // Divider line
  SkPaint dividerPaint;
  dividerPaint.setColor(SkColorSetARGB(40, 255, 255, 255));
  dividerPaint.setStrokeWidth(1.0f);
  canvas->drawLine(kPadding, y, bounds.width() - kPadding, y, dividerPaint);

  y += 12.0f;

  // Status rows
  smallFont_.setSize(11.0f);
  textPaint_.setColor(SkColorSetARGB(200, 255, 255, 255));

  // CPU row
  float cpu = debugger_.getTotalCpuUsage();
  SkColor cpuColor = (cpu > 80.0f)   ? 0xFFFF4444
                     : (cpu > 50.0f) ? 0xFFFFAA00
                                     : 0xFF44FF44;

  textPaint_.setColor(SkColorSetARGB(150, 255, 255, 255));
  canvas->drawString("CPU", kPadding, y + 10.0f, smallFont_, textPaint_);

  // CPU bar
  float barWidth = 100.0f;
  float barHeight = 6.0f;
  float barX = kPadding + 50.0f;

  SkRect barBg = SkRect::MakeXYWH(barX, y + 4.0f, barWidth, barHeight);
  SkPaint barBgPaint;
  barBgPaint.setColor(SkColorSetARGB(50, 0, 0, 0));
  canvas->drawRoundRect(barBg, 3.0f, 3.0f, barBgPaint);

  float fillWidth = barWidth * (cpu / 100.0f);
  SkRect barFill = SkRect::MakeXYWH(barX, y + 4.0f, fillWidth, barHeight);
  SkPaint barFillPaint;
  barFillPaint.setColor(cpuColor);
  canvas->drawRoundRect(barFill, 3.0f, 3.0f, barFillPaint);

  textPaint_.setColor(cpuColor);
  canvas->drawString((juce::String(cpu, 0) + "%").toStdString().c_str(),
                     barX + barWidth + 10.0f, y + 10.0f, smallFont_,
                     textPaint_);

  y += 20.0f;

  // Latency row
  float latency = debugger_.getTotalLatencyMs();
  SkColor latencyColor = (latency > 50.0f)   ? 0xFFFF4444
                         : (latency > 20.0f) ? 0xFFFFAA00
                                             : 0xFF44FF44;

  textPaint_.setColor(SkColorSetARGB(150, 255, 255, 255));
  canvas->drawString("Latency", kPadding, y + 10.0f, smallFont_, textPaint_);

  textPaint_.setColor(latencyColor);
  canvas->drawString((juce::String(latency, 0) + "ms").toStdString().c_str(),
                     barX, y + 10.0f, smallFont_, textPaint_);

  y += 20.0f;

  // Clipping row
  int clippingCount = debugger_.getClippingTrackCount();
  SkColor clipColor = (clippingCount > 0) ? 0xFFFF4444 : 0xFF44FF44;

  textPaint_.setColor(SkColorSetARGB(150, 255, 255, 255));
  canvas->drawString("Clipping", kPadding, y + 10.0f, smallFont_, textPaint_);

  textPaint_.setColor(clipColor);
  juce::String clipText = (clippingCount > 0)
                              ? juce::String(clippingCount) + " track" +
                                    (clippingCount != 1 ? "s" : "")
                              : "None";
  canvas->drawString(clipText.toStdString().c_str(), barX, y + 10.0f,
                     smallFont_, textPaint_);

  y += 24.0f;

  // Fix summary at bottom
  if (debugger_.getFixCount() > 0) {
    textPaint_.setColor(0xFF00AAFF);
    canvas->drawString(debugger_.getFixSummary().toStdString().c_str(),
                       kPadding, y + 10.0f, smallFont_, textPaint_);
  }
}

void DebugConsoleComponent::drawHealthIndicator(SkCanvas *canvas, float x,
                                                float y, float size) {
  SkColor color = getHealthColor(displayedHealthScore_);

  // Pulsing effect
  float pulse = 1.0f + 0.1f * std::sin(animationProgress_ * 4.0f);
  float pulseSize = size * pulse;

  // Outer glow
  SkPaint glowPaint;
  glowPaint.setAntiAlias(true);
  glowPaint.setColor(SkColorSetA(color, 60));
  canvas->drawCircle(x, y, pulseSize + 4.0f, glowPaint);

  // Inner solid circle
  SkPaint solidPaint;
  solidPaint.setAntiAlias(true);
  solidPaint.setColor(color);
  canvas->drawCircle(x, y, pulseSize, solidPaint);

  // Highlight
  SkPaint highlightPaint;
  highlightPaint.setAntiAlias(true);
  highlightPaint.setColor(SkColorSetA(SK_ColorWHITE, 80));
  canvas->drawCircle(x - pulseSize * 0.3f, y - pulseSize * 0.3f,
                     pulseSize * 0.4f, highlightPaint);
}

void DebugConsoleComponent::drawStatusIcons(SkCanvas *canvas, float x,
                                            float y) {
  // CPU icon
  float cpu = debugger_.getTotalCpuUsage();
  SkColor cpuColor = (cpu > 80.0f)   ? 0xFFFF4444
                     : (cpu > 50.0f) ? 0xFFFFAA00
                                     : 0xFF888888;

  iconPaint_.setColor(cpuColor);
  canvas->drawCircle(x, y, 4.0f, iconPaint_);

  // Latency icon
  float latency = debugger_.getTotalLatencyMs();
  SkColor latencyColor = (latency > 50.0f)   ? 0xFFFF4444
                         : (latency > 20.0f) ? 0xFFFFAA00
                                             : 0xFF888888;

  iconPaint_.setColor(latencyColor);
  canvas->drawCircle(x + 20.0f, y, 4.0f, iconPaint_);

  // Clipping icon
  int clipping = debugger_.getClippingTrackCount();
  SkColor clipColor = (clipping > 0) ? 0xFFFF4444 : 0xFF888888;

  iconPaint_.setColor(clipColor);
  canvas->drawCircle(x + 40.0f, y, 4.0f, iconPaint_);
}

void DebugConsoleComponent::drawNotificationBadge(SkCanvas *canvas, float x,
                                                  float y) {
  // Animated notification dot
  float alpha =
      static_cast<float>(std::abs(std::sin(animationProgress_ * 6.0f)));

  SkPaint badgePaint;
  badgePaint.setAntiAlias(true);
  badgePaint.setColor(
      SkColorSetARGB(static_cast<unsigned>(alpha * 255), 0, 200, 255));

  canvas->drawCircle(x, y, 6.0f, badgePaint);
}

SkColor DebugConsoleComponent::getHealthColor(float score) const {
  if (score >= 80.0f) {
    return 0xFF00FF64; // Green
  } else if (score >= 50.0f) {
    return 0xFFFFAA00; // Orange
  } else {
    return 0xFFFF4444; // Red
  }
}

juce::String DebugConsoleComponent::getHealthStatusText(float score) const {
  if (score >= 90.0f)
    return "Excellent";
  if (score >= 80.0f)
    return "Good";
  if (score >= 60.0f)
    return "Fair";
  if (score >= 40.0f)
    return "Poor";
  return "Critical";
}

void DebugConsoleComponent::resized() {
  // Nothing to do here - layout is handled in draw
}

//==============================================================================
void DebugConsoleComponent::mouseDown(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  setExpanded(!isExpanded_);
}

void DebugConsoleComponent::mouseEnter(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  isHovered_ = true;
  repaint();
}

void DebugConsoleComponent::mouseExit(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  isHovered_ = false;
  repaint();
}

//==============================================================================
void DebugConsoleComponent::issueDetected(const ai::SessionIssue &issue) {
  juce::ignoreUnused(issue);
  repaint();
}

void DebugConsoleComponent::issueResolved(const ai::SessionIssue &issue,
                                          const ai::FixAction &fix) {
  juce::ignoreUnused(issue);
  latestFixMessage_ = fix.description;
  lastFixTime_ = juce::Time::getCurrentTime();
  showNewFixNotification_ = true;
  repaint();
}

void DebugConsoleComponent::sessionHealthChanged(float newHealthScore) {
  healthScore_ = newHealthScore;
  repaint();
}

//==============================================================================
void DebugConsoleComponent::timerCallback() {
  // Update animation progress
  animationProgress_ += 0.05f;
  if (animationProgress_ > 6.28318f) { // 2*PI
    animationProgress_ -= 6.28318f;
  }

  // Animate health score display
  float diff = healthScore_ - displayedHealthScore_;
  if (std::abs(diff) > 0.1f) {
    displayedHealthScore_ += diff * 0.1f;
  } else {
    displayedHealthScore_ = healthScore_;
  }

  // Clear notification after 3 seconds
  if (showNewFixNotification_) {
    auto elapsed = juce::Time::getCurrentTime() - lastFixTime_;
    if (elapsed.inSeconds() > 3.0) {
      showNewFixNotification_ = false;
    }
  }

  repaint();
}

//==============================================================================
void DebugConsoleComponent::setExpanded(bool expanded) {
  if (isExpanded_ == expanded)
    return;

  isExpanded_ = expanded;

  // Animate height change
  int targetHeight = expanded ? static_cast<int>(kExpandedHeight)
                              : static_cast<int>(kCollapsedHeight);

  // For now, just set immediately (animation could be added)
  setSize(getWidth(), targetHeight);

  // Notify parent to re-layout if needed
  if (auto *parent = getParentComponent()) {
    parent->resized();
  }

  repaint();
}

//==============================================================================
void DebugConsoleComponent::updateCachedPaints() {
  // Background - dark with subtle transparency
  bgPaint_.setAntiAlias(true);
  bgPaint_.setColor(SkColorSetARGB(230, 25, 25, 30));
  bgPaint_.setStyle(SkPaint::kFill_Style);

  // Border - subtle glow
  borderPaint_.setAntiAlias(true);
  borderPaint_.setColor(SkColorSetARGB(60, 100, 200, 255));
  borderPaint_.setStyle(SkPaint::kStroke_Style);
  borderPaint_.setStrokeWidth(1.0f);

  // Health colors
  healthGoodPaint_.setAntiAlias(true);
  healthGoodPaint_.setColor(0xFF00FF64);

  healthWarningPaint_.setAntiAlias(true);
  healthWarningPaint_.setColor(0xFFFFAA00);

  healthCriticalPaint_.setAntiAlias(true);
  healthCriticalPaint_.setColor(0xFFFF4444);

  // Text
  textPaint_.setAntiAlias(true);
  textPaint_.setColor(SK_ColorWHITE);

  // Icons
  iconPaint_.setAntiAlias(true);
  iconPaint_.setColor(SK_ColorWHITE);

  // Notification
  notificationPaint_.setAntiAlias(true);
  notificationPaint_.setColor(0xFF00AAFF);

  // Fonts
  font_.setSize(12.0f);
  font_.setSubpixel(true);

  boldFont_.setSize(14.0f);
  boldFont_.setSubpixel(true);
  // Note: Skia font weight would be set via SkFontStyle in a full
  // implementation

  smallFont_.setSize(10.0f);
  smallFont_.setSubpixel(true);
}

#endif // ZENITH_USE_SKIA

} // namespace zenith

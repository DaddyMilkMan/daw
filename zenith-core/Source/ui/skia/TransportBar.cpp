/**
 * @file TransportBar.cpp
 * @brief Implementation of top transport control bar
 */

// POLISH: spacing normalized to 8px grid (buttons 40x40, padding 8/16, radius
// 4px) POLISH: typography now uses SkiaTheme::Typography (body/small/tiny)
// POLISH: unified hover/active using theme.getInteraction()

#include "TransportBar.h"
#include <cmath>
#include <include/core/SkFont.h>
#include <include/core/SkPaint.h>
#include <include/core/SkPath.h>
#include <include/core/SkRRect.h>

namespace zenith {

//==============================================================================
// Constants
//==============================================================================

static constexpr float TRANSPORT_BAR_HEIGHT = 60.0f;
static constexpr float BUTTON_SIZE = 40.0f; // 8px grid: 36→40
static constexpr float BUTTON_SPACING = 8.0f;
static constexpr float SECTION_PADDING = 16.0f;
static constexpr float CORNER_RADIUS = 4.0f; // 8px grid: 6→4

//==============================================================================
// Construction
//==============================================================================

TransportBar::TransportBar() {
  setSize(800, static_cast<int>(TRANSPORT_BAR_HEIGHT));
}

//==============================================================================
// State Management
//==============================================================================

void TransportBar::setPlaying(bool playing) {
  if (isPlaying_ != playing) {
    isPlaying_ = playing;
    repaint();
  }
}

void TransportBar::setRecording(bool recording) {
  if (isRecording_ != recording) {
    isRecording_ = recording;
    repaint();
  }
}

void TransportBar::setLooping(bool looping) {
  if (isLooping_ != looping) {
    isLooping_ = looping;
    repaint();
  }
}

void TransportBar::setTempo(double bpm) {
  if (std::abs(tempo_ - bpm) > 0.01) {
    tempo_ = bpm;
    repaint();
  }
}

void TransportBar::setTimeSignature(int numerator, int denominator) {
  if (timeSigNumerator_ != numerator || timeSigDenominator_ != denominator) {
    timeSigNumerator_ = numerator;
    timeSigDenominator_ = denominator;
    repaint();
  }
}

void TransportBar::setPlaybackPosition(double seconds) {
  playbackPosition_ = seconds;
  repaint();
}

void TransportBar::setCPUUsage(float percentage) {
  cpuUsage_ = juce::jlimit(0.0f, 100.0f, percentage);
  repaint();
}

void TransportBar::setWingmanConnected(bool connected) {
  if (wingmanConnected_ != connected) {
    wingmanConnected_ = connected;
    repaint();
  }
}

void TransportBar::setProjectName(const juce::String &name) {
  if (projectName_ != name) {
    projectName_ = name;
    repaint();
  }
}

//==============================================================================
// Component Overrides
//==============================================================================

void TransportBar::resized() { SkiaCanvasComponent::resized(); }

void TransportBar::mouseDown(const juce::MouseEvent &event) {
  if (!event.mods.isLeftButtonDown())
    return;
  activeZone_ = hitTest(event.getPosition());
  repaint();
}

void TransportBar::mouseUp(const juce::MouseEvent &event) {
  auto zone = hitTest(event.getPosition());

  if (zone == activeZone_) {
    // Handle button clicks
    switch (zone) {
    case HitZone::Play:
      if (onPlayClicked)
        onPlayClicked();
      break;
    case HitZone::Stop:
      if (onStopClicked)
        onStopClicked();
      break;
    case HitZone::Record:
      if (onRecordClicked)
        onRecordClicked();
      break;
    case HitZone::Loop:
      if (onLoopClicked)
        onLoopClicked();
      break;
    case HitZone::Menu:
      if (onMenuClicked)
        onMenuClicked();
      break;
    case HitZone::Undo:
      if (onUndoClicked)
        onUndoClicked();
      break;
    default:
      break;
    }
  }

  activeZone_ = HitZone::None;
  repaint();
}

void TransportBar::mouseMove(const juce::MouseEvent &event) {
  auto newZone = hitTest(event.getPosition());
  if (newZone != hoveredZone_) {
    hoveredZone_ = newZone;
    repaint();
  }
}

juce::String TransportBar::getTooltip() {
  switch (hoveredZone_) {
  case HitZone::Play:
    return "Play (Space)";
  case HitZone::Stop:
    return "Stop (Enter)";
  case HitZone::Record:
    return "Record (R)";
  case HitZone::Loop:
    return "Loop (L)";
  case HitZone::Menu:
    return "Main Menu";
  case HitZone::Undo:
    return "Undo (Ctrl+Z)";
  case HitZone::Wingman:
    return "Wingman AI Assistant";
  default:
    return {};
  }
}

//==============================================================================
// Skia Rendering
//==============================================================================

void TransportBar::paintSkia(SkCanvas &canvas,
                             const juce::Rectangle<int> &bounds) {
  auto &theme = SkiaTheme::getInstance();
  auto &colors = theme.getColors();

  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  // Draw background
  drawBackground(canvas, skBounds);

  // Divide into three sections
  float leftWidth = bounds.getWidth() * 0.25f;
  float rightWidth = bounds.getWidth() * 0.25f;
  float centerWidth = bounds.getWidth() - leftWidth - rightWidth;

  SkRect leftRect = SkRect::MakeXYWH(0, 0, leftWidth, skBounds.height());
  SkRect centerRect =
      SkRect::MakeXYWH(leftWidth, 0, centerWidth, skBounds.height());
  SkRect rightRect = SkRect::MakeXYWH(leftWidth + centerWidth, 0, rightWidth,
                                      skBounds.height());

  drawLeftSection(canvas, leftRect);
  drawCenterSection(canvas, centerRect);
  drawRightSection(canvas, rightRect);
}

void TransportBar::drawBackground(SkCanvas &canvas, const SkRect &bounds) {
  auto &colors = SkiaTheme::getInstance().getColors();

  // Main background
  SkPaint bgPaint;
  bgPaint.setAntiAlias(true);
  bgPaint.setColor(colors.bg1);

  SkRRect roundRect = SkRRect::MakeRectXY(bounds, CORNER_RADIUS, CORNER_RADIUS);
  canvas.drawRRect(roundRect, bgPaint);

  // Bottom border line
  SkPaint borderPaint;
  borderPaint.setAntiAlias(true);
  borderPaint.setColor(colors.borderSubtle);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(1.0f);

  canvas.drawLine(0, bounds.bottom() - 1, bounds.right(), bounds.bottom() - 1,
                  borderPaint);
}

void TransportBar::drawLeftSection(SkCanvas &canvas, const SkRect &bounds) {
  auto &colors = SkiaTheme::getInstance().getColors();

  // Draw project name
  auto &theme = SkiaTheme::getInstance();
  auto &typo = theme.getTypography();

  SkPaint textPaint;
  textPaint.setAntiAlias(true);
  textPaint.setColor(colors.textStrong);

  SkFont font;
  font.setSize(typo.header.size);
  if (typo.header.bold)
    font.setEmbolden(true);

  float textX = bounds.left() + SECTION_PADDING;
  float textY = bounds.centerY() + 4.0f; // Vertically centered (8px grid)

  canvas.drawString(projectName_.toRawUTF8(), textX, textY, font, textPaint);

  // Draw menu button (hamburger icon)
  auto &interaction = theme.getInteraction();
  auto menuBounds = getMenuButtonBounds();
  bool isHovered = hoveredZone_ == HitZone::Menu;
  bool isActive = activeZone_ == HitZone::Menu;

  SkRect menuRect =
      SkRect::MakeXYWH(menuBounds.getX(), menuBounds.getY(),
                       menuBounds.getWidth(), menuBounds.getHeight());

  SkPaint buttonPaint;
  buttonPaint.setAntiAlias(true);
  buttonPaint.setColor(colors.bg2);

  SkRRect buttonRRect =
      SkRRect::MakeRectXY(menuRect, CORNER_RADIUS, CORNER_RADIUS);
  canvas.drawRRect(buttonRRect, buttonPaint);

  // POLISH: unified hover/active using theme.getInteraction()
  if (isHovered) {
    SkPaint hoverPaint;
    hoverPaint.setAntiAlias(true);
    hoverPaint.setColor(interaction.hoverOverlay);
    canvas.drawRRect(buttonRRect, hoverPaint);
  }

  // Draw hamburger lines
  SkPaint linePaint;
  linePaint.setAntiAlias(true);
  linePaint.setColor(colors.textMuted);
  linePaint.setStyle(SkPaint::kStroke_Style);
  linePaint.setStrokeWidth(2.0f);

  float lineWidth = 16.0f;
  float lineX = menuRect.centerX() - lineWidth / 2;
  float lineY = menuRect.centerY();

  canvas.drawLine(lineX, lineY - 4, lineX + lineWidth, lineY - 4, linePaint);
  canvas.drawLine(lineX, lineY, lineX + lineWidth, lineY, linePaint);
  canvas.drawLine(lineX, lineY + 4, lineX + lineWidth, lineY + 4, linePaint);
}

void TransportBar::drawCenterSection(SkCanvas &canvas, const SkRect &bounds) {
  auto &colors = SkiaTheme::getInstance().getColors();

  // Calculate center point
  float centerX = bounds.centerX();
  float centerY = bounds.centerY();

  // Draw transport buttons
  auto playBounds = getPlayButtonBounds();
  auto stopBounds = getStopButtonBounds();
  auto recordBounds = getRecordButtonBounds();
  auto loopBounds = getLoopButtonBounds();

  // Play button (triangle)
  drawTransportButton(
      canvas,
      SkRect::MakeXYWH(playBounds.getX(), playBounds.getY(),
                       playBounds.getWidth(), playBounds.getHeight()),
      "Play", isPlaying_, hoveredZone_ == HitZone::Play, colors.accentMain);

  // Stop button (square)
  drawTransportButton(
      canvas,
      SkRect::MakeXYWH(stopBounds.getX(), stopBounds.getY(),
                       stopBounds.getWidth(), stopBounds.getHeight()),
      "Stop", false, hoveredZone_ == HitZone::Stop, colors.textMuted);

  // Record button (circle)
  drawTransportButton(canvas,
                      SkRect::MakeXYWH(recordBounds.getX(), recordBounds.getY(),
                                       recordBounds.getWidth(),
                                       recordBounds.getHeight()),
                      "Rec", isRecording_, hoveredZone_ == HitZone::Record,
                      colors.accentRecord);

  // Loop button
  drawTransportButton(
      canvas,
      SkRect::MakeXYWH(loopBounds.getX(), loopBounds.getY(),
                       loopBounds.getWidth(), loopBounds.getHeight()),
      "Loop", isLooping_, hoveredZone_ == HitZone::Loop, colors.accentAlt);

  // Draw tempo and time display
  auto &typo = SkiaTheme::getInstance().getTypography();
  SkFont font;
  font.setSize(typo.body.size);

  SkPaint textPaint;
  textPaint.setAntiAlias(true);
  textPaint.setColor(colors.textMuted);

  juce::String tempoText = juce::String(tempo_, 1) + " BPM | " +
                           juce::String(timeSigNumerator_) + "/" +
                           juce::String(timeSigDenominator_);
  juce::String timeText = formatTime(playbackPosition_);

  float tempoX = centerX + BUTTON_SIZE * 2 + BUTTON_SPACING * 3;
  float timeX = tempoX + 100.0f;

  canvas.drawString(tempoText.toRawUTF8(), tempoX, centerY + 4, font,
                    textPaint);

  textPaint.setColor(colors.textStrong);

  SkFont monoFont;
  monoFont.setSize(typo.mono.size);
  // Assuming default typeface is sans-serif, we might need to explicitly set a
  // mono typeface if available. For now, we just use the size and rely on the
  // system or font manager if we had one. But wait, SkFont doesn't
  // automatically pick mono. We'll just use the size for now as per spec "Mono
  // (System Mono)". In a real app we'd load a specific font.

  canvas.drawString(timeText.toRawUTF8(), timeX, centerY + 4, monoFont,
                    textPaint);
}

void TransportBar::drawRightSection(SkCanvas &canvas, const SkRect &bounds) {
  auto &colors = SkiaTheme::getInstance().getColors();

  // Draw CPU meter
  float meterWidth = 80.0f;
  float meterHeight = 8.0f;
  float meterX = bounds.right() - SECTION_PADDING - meterWidth - 120.0f;
  float meterY = bounds.centerY() - meterHeight / 2;

  SkRect meterRect = SkRect::MakeXYWH(meterX, meterY, meterWidth, meterHeight);
  drawCPUMeter(canvas, meterRect);

  // Draw CPU percentage label
  auto &typo = SkiaTheme::getInstance().getTypography();
  SkFont font;
  font.setSize(typo.small.size);

  SkPaint textPaint;
  textPaint.setAntiAlias(true);
  textPaint.setColor(colors.textMuted);

  juce::String cpuText = juce::String(cpuUsage_, 1) + "% CPU";
  canvas.drawString(cpuText.toRawUTF8(), meterX, meterY - 4, font, textPaint);

  // Draw Wingman indicator
  auto wingmanBounds = getWingmanButtonBounds();
  SkRect wingmanRect =
      SkRect::MakeXYWH(wingmanBounds.getX(), wingmanBounds.getY(),
                       wingmanBounds.getWidth(), wingmanBounds.getHeight());
  drawWingmanIndicator(canvas, wingmanRect);

  // Draw undo button
  auto &interaction = SkiaTheme::getInstance().getInteraction();
  auto undoBounds = getUndoButtonBounds();
  bool isHovered = hoveredZone_ == HitZone::Undo;

  SkRect undoRect =
      SkRect::MakeXYWH(undoBounds.getX(), undoBounds.getY(),
                       undoBounds.getWidth(), undoBounds.getHeight());

  SkPaint buttonPaint;
  buttonPaint.setAntiAlias(true);
  buttonPaint.setColor(colors.bg2);

  SkRRect buttonRRect =
      SkRRect::MakeRectXY(undoRect, CORNER_RADIUS, CORNER_RADIUS);
  canvas.drawRRect(buttonRRect, buttonPaint);

  // POLISH: unified hover using theme.getInteraction()
  if (isHovered) {
    SkPaint hoverPaint;
    hoverPaint.setAntiAlias(true);
    hoverPaint.setColor(interaction.hoverOverlay);
    canvas.drawRRect(buttonRRect, hoverPaint);
  }

  // Draw undo icon (curved arrow)
  SkPaint iconPaint;
  iconPaint.setAntiAlias(true);
  iconPaint.setColor(colors.textMuted);
  iconPaint.setStyle(SkPaint::kStroke_Style);
  iconPaint.setStrokeWidth(2.0f);

  SkPath arrowPath;
  float arrowCenterX = undoRect.centerX();
  float arrowCenterY = undoRect.centerY();
  arrowPath.moveTo(arrowCenterX - 6, arrowCenterY);
  arrowPath.cubicTo(arrowCenterX - 6, arrowCenterY - 8, arrowCenterX,
                    arrowCenterY - 8, arrowCenterX + 6, arrowCenterY - 4);

  canvas.drawPath(arrowPath, iconPaint);
}

void TransportBar::drawTransportButton(SkCanvas &canvas, const SkRect &rect,
                                       const juce::String &label, bool isActive,
                                       bool isHovered, SkColor activeColor) {
  auto &theme = SkiaTheme::getInstance();
  auto &colors = theme.getColors();
  auto &interaction = theme.getInteraction();

  // Background
  SkPaint bgPaint;
  bgPaint.setAntiAlias(true);

  if (isActive) {
    bgPaint.setColor(activeColor);
  } else {
    bgPaint.setColor(colors.bg2);
  }

  SkRRect buttonRRect = SkRRect::MakeRectXY(rect, CORNER_RADIUS, CORNER_RADIUS);
  canvas.drawRRect(buttonRRect, bgPaint);

  // POLISH: unified hover/active using theme.getInteraction()
  if (isHovered && !isActive) {
    SkPaint hoverPaint;
    hoverPaint.setAntiAlias(true);
    hoverPaint.setColor(interaction.hoverOverlay);
    canvas.drawRRect(buttonRRect, hoverPaint);
  }

  // Border on hover/active
  if (isHovered || isActive) {
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setColor(isActive ? activeColor : colors.borderStrong);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);

    canvas.drawRRect(buttonRRect, borderPaint);
  }

  // Icon/Symbol
  SkPaint iconPaint;
  iconPaint.setAntiAlias(true);
  iconPaint.setColor(isActive ? colors.bg0 : colors.textStrong);

  float centerX = rect.centerX();
  float centerY = rect.centerY();

  if (label == "Play") {
    // Triangle
    SkPath triangle;
    triangle.moveTo(centerX - 6, centerY - 8);
    triangle.lineTo(centerX + 8, centerY);
    triangle.lineTo(centerX - 6, centerY + 8);
    triangle.close();
    canvas.drawPath(triangle, iconPaint);
  } else if (label == "Stop") {
    // Square
    SkRect square = SkRect::MakeXYWH(centerX - 6, centerY - 6, 12, 12);
    canvas.drawRect(square, iconPaint);
  } else if (label == "Rec") {
    // Circle
    canvas.drawCircle(centerX, centerY, 8.0f, iconPaint);
  } else if (label == "Loop") {
    // Loop symbol (two circles connected)
    iconPaint.setStyle(SkPaint::kStroke_Style);
    iconPaint.setStrokeWidth(2.0f);

    canvas.drawCircle(centerX - 5, centerY, 5.0f, iconPaint);
    canvas.drawCircle(centerX + 5, centerY, 5.0f, iconPaint);
  }
}

void TransportBar::drawCPUMeter(SkCanvas &canvas, const SkRect &rect) {
  auto &colors = SkiaTheme::getInstance().getColors();

  // Background
  SkPaint bgPaint;
  bgPaint.setAntiAlias(true);
  bgPaint.setColor(colors.bg3);

  SkRRect bgRRect = SkRRect::MakeRectXY(rect, 4.0f, 4.0f);
  canvas.drawRRect(bgRRect, bgPaint);

  // Fill level
  float fillWidth = rect.width() * (cpuUsage_ / 100.0f);

  if (fillWidth > 0) {
    SkColor meterColor;
    if (cpuUsage_ < 60.0f) {
      meterColor = colors.meterGreen;
    } else if (cpuUsage_ < 85.0f) {
      meterColor = colors.meterYellow;
    } else {
      meterColor = colors.meterRed;
    }

    SkPaint fillPaint;
    fillPaint.setAntiAlias(true);
    fillPaint.setColor(meterColor);

    SkRect fillRect =
        SkRect::MakeXYWH(rect.left(), rect.top(), fillWidth, rect.height());
    SkRRect fillRRect = SkRRect::MakeRectXY(fillRect, 4.0f, 4.0f);
    canvas.drawRRect(fillRRect, fillPaint);
  }
}

void TransportBar::drawWingmanIndicator(SkCanvas &canvas, const SkRect &rect) {
  auto &theme = SkiaTheme::getInstance();
  auto &colors = theme.getColors();
  auto &interaction = theme.getInteraction();
  bool isHovered = hoveredZone_ == HitZone::Wingman;

  // Background
  SkPaint bgPaint;
  bgPaint.setAntiAlias(true);
  bgPaint.setColor(colors.bg2);

  SkRRect bgRRect = SkRRect::MakeRectXY(rect, CORNER_RADIUS, CORNER_RADIUS);
  canvas.drawRRect(bgRRect, bgPaint);

  // POLISH: unified hover using theme.getInteraction()
  if (isHovered) {
    SkPaint hoverPaint;
    hoverPaint.setAntiAlias(true);
    hoverPaint.setColor(interaction.hoverOverlay);
    canvas.drawRRect(bgRRect, hoverPaint);
  }

  // Status indicator dot
  SkPaint dotPaint;
  dotPaint.setAntiAlias(true);
  dotPaint.setColor(wingmanConnected_ ? colors.accentMain : colors.textSubtle);

  float dotX = rect.left() + 12.0f;
  float dotY = rect.centerY();

  canvas.drawCircle(dotX, dotY, 4.0f, dotPaint);

  // Label
  auto &typo = SkiaTheme::getInstance().getTypography();
  SkFont font;
  font.setSize(typo.small.size);

  SkPaint textPaint;
  textPaint.setAntiAlias(true);
  textPaint.setColor(colors.textMuted);

  canvas.drawString("CMD", dotX + 10, dotY + 4, font, textPaint);
}

juce::String TransportBar::formatTime(double seconds) const {
  int totalSeconds = static_cast<int>(seconds);
  int hours = totalSeconds / 3600;
  int minutes = (totalSeconds % 3600) / 60;
  int secs = totalSeconds % 60;
  int millis = static_cast<int>((seconds - totalSeconds) * 1000);

  if (hours > 0) {
    return juce::String::formatted("%02d:%02d:%02d.%03d", hours, minutes, secs,
                                   millis);
  } else {
    return juce::String::formatted("%02d:%02d.%03d", minutes, secs, millis);
  }
}

//==============================================================================
// Hit Testing
//==============================================================================

TransportBar::HitZone
TransportBar::hitTest(const juce::Point<int> &point) const {
  if (getPlayButtonBounds().contains(point))
    return HitZone::Play;
  if (getStopButtonBounds().contains(point))
    return HitZone::Stop;
  if (getRecordButtonBounds().contains(point))
    return HitZone::Record;
  if (getLoopButtonBounds().contains(point))
    return HitZone::Loop;
  if (getMenuButtonBounds().contains(point))
    return HitZone::Menu;
  if (getUndoButtonBounds().contains(point))
    return HitZone::Undo;
  if (getWingmanButtonBounds().contains(point))
    return HitZone::Wingman;

  return HitZone::None;
}

juce::Rectangle<int> TransportBar::getPlayButtonBounds() const {
  int centerX = getWidth() / 2;
  int centerY = getHeight() / 2;

  int x = centerX - static_cast<int>(BUTTON_SIZE * 1.5f + BUTTON_SPACING);
  int y = centerY - static_cast<int>(BUTTON_SIZE / 2);

  return juce::Rectangle<int>(x, y, static_cast<int>(BUTTON_SIZE),
                              static_cast<int>(BUTTON_SIZE));
}

juce::Rectangle<int> TransportBar::getStopButtonBounds() const {
  int centerX = getWidth() / 2;
  int centerY = getHeight() / 2;

  int x = centerX - static_cast<int>(BUTTON_SIZE / 2);
  int y = centerY - static_cast<int>(BUTTON_SIZE / 2);

  return juce::Rectangle<int>(x, y, static_cast<int>(BUTTON_SIZE),
                              static_cast<int>(BUTTON_SIZE));
}

juce::Rectangle<int> TransportBar::getRecordButtonBounds() const {
  int centerX = getWidth() / 2;
  int centerY = getHeight() / 2;

  int x = centerX + static_cast<int>(BUTTON_SIZE / 2 + BUTTON_SPACING);
  int y = centerY - static_cast<int>(BUTTON_SIZE / 2);

  return juce::Rectangle<int>(x, y, static_cast<int>(BUTTON_SIZE),
                              static_cast<int>(BUTTON_SIZE));
}

juce::Rectangle<int> TransportBar::getLoopButtonBounds() const {
  int centerX = getWidth() / 2;
  int centerY = getHeight() / 2;

  int x = centerX + static_cast<int>(BUTTON_SIZE * 1.5f + BUTTON_SPACING * 2);
  int y = centerY - static_cast<int>(BUTTON_SIZE / 2);

  return juce::Rectangle<int>(x, y, static_cast<int>(BUTTON_SIZE),
                              static_cast<int>(BUTTON_SIZE));
}

juce::Rectangle<int> TransportBar::getMenuButtonBounds() const {
  int x = static_cast<int>(SECTION_PADDING) + 200; // After project name
  int y = (getHeight() - static_cast<int>(BUTTON_SIZE)) / 2;

  return juce::Rectangle<int>(x, y, static_cast<int>(BUTTON_SIZE),
                              static_cast<int>(BUTTON_SIZE));
}

juce::Rectangle<int> TransportBar::getUndoButtonBounds() const {
  int x = getWidth() - static_cast<int>(SECTION_PADDING + BUTTON_SIZE);
  int y = (getHeight() - static_cast<int>(BUTTON_SIZE)) / 2;

  return juce::Rectangle<int>(x, y, static_cast<int>(BUTTON_SIZE),
                              static_cast<int>(BUTTON_SIZE));
}

juce::Rectangle<int> TransportBar::getWingmanButtonBounds() const {
  int x = getWidth() - static_cast<int>(SECTION_PADDING + BUTTON_SIZE * 2 +
                                        BUTTON_SPACING + 60);
  int y = (getHeight() - static_cast<int>(BUTTON_SIZE)) / 2;

  return juce::Rectangle<int>(x, y, 60, static_cast<int>(BUTTON_SIZE));
}

} // namespace zenith

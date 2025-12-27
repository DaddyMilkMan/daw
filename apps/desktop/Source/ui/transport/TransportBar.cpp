/*
  ==============================================================================

    TransportBar.cpp
    Created: 2025-11-28
    Author:  Leo "Lil Bit" Rossi

    Implementation of transport controls with Neon Noir styling.

  ==============================================================================
*/

#include "TransportBar.h"

#include <core/SkBlurTypes.h> // Explicitly include
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkFont.h>
#include <core/SkFontTypes.h>
#include <core/SkMaskFilter.h>
#include <core/SkPaint.h>
#include <core/SkRRect.h>
#include <effects/SkGradientShader.h>

#ifdef ZENITH_USE_SKIA
#include "../design-system/ZenithIcons.h"
#include "../framework/GlassmorphicPanel.h"
#include "../framework/NeonGlow.h"
#include "../controls/SkiaPopupMenu.h"
#include "../controls/ContextMenuManager.h"
#include "../design-system/ColorBridge.h"
#include "../design-system/ZenithDesignSystem.h"
#include <effects/SkGradientShader.h>

namespace zenith {

TransportBar::TransportBar() {
  setSize(800, 60);
}

void TransportBar::visibilityChanged() {
  // Only start timer when:
  // 1. Component is visible
  // 2. Component has a peer (is on desktop) - prevents blocking during construction
  // 3. Timer isn't already running
  if (isVisible() && getPeer() != nullptr && !isTimerRunning()) {
    if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimerHz(60); // Start animation loop when visible and on desktop
  }
}

void TransportBar::resized() {
  using namespace design;
  auto area = getLocalBounds();
  const int width = getWidth();
  
  // Responsive thresholds
  const bool isCompact = width < 600;
  const bool isSuperCompact = width < 400;

  // Use design tokens
  int buttonWidth = static_cast<int>(dimensions::BUTTON_HEIGHT_LG + spacing::SM);
  int buttonSpacing = static_cast<int>(spacing::SM);
  int buttonPadding = static_cast<int>(spacing::SM);

  // 1. Left Section (Transport Controls)
  // Calculate needed width for left controls
  int leftWidth = (buttonWidth * 3) + (buttonSpacing * 2) + static_cast<int>(spacing::MD);
  auto leftSection = area.removeFromLeft(leftWidth);
  
  playButtonBounds_ = leftSection.removeFromLeft(buttonWidth).reduced(buttonPadding);
  leftSection.removeFromLeft(buttonSpacing); // Spacer
  stopButtonBounds_ = leftSection.removeFromLeft(buttonWidth).reduced(buttonPadding);
  leftSection.removeFromLeft(buttonSpacing); // Spacer
  recordButtonBounds_ = leftSection.removeFromLeft(buttonWidth).reduced(buttonPadding);

  // 2. Right Section (Tools & Settings)
  // Determine what to show based on width
  int rightWidth = static_cast<int>(dimensions::TRANSPORT_BAR_HEIGHT) * 3;
  if (isSuperCompact) rightWidth = static_cast<int>(dimensions::TRANSPORT_BAR_HEIGHT) * 1; // Only Settings
  else if (isCompact) rightWidth = static_cast<int>(dimensions::TRANSPORT_BAR_HEIGHT) * 2; // Settings + Toggle
  
  auto rightSection = area.removeFromRight(rightWidth);
  
  // Always show Settings
  settingsButtonBounds_ = rightSection.removeFromRight(static_cast<int>(dimensions::TRANSPORT_BAR_HEIGHT)).reduced(buttonPadding);
  
  // Show Export if space permits
  if (!isSuperCompact) {
      exportButtonBounds_ = rightSection.removeFromRight(static_cast<int>(dimensions::TRANSPORT_BAR_HEIGHT)).reduced(buttonPadding);
  } else {
      exportButtonBounds_ = juce::Rectangle<int>(); // Hidden
  }
  
  // Show View Toggle if space permits
  if (!isCompact && !isSuperCompact) {
      viewToggleButtonBounds_ = rightSection.removeFromRight(static_cast<int>(dimensions::TRANSPORT_BAR_HEIGHT)).reduced(buttonPadding);
  } else {
      viewToggleButtonBounds_ = juce::Rectangle<int>(); // Hidden
  }

  // 3. Center Info Section (Remaining space)
  centerInfoBounds_ = area;
  
  // CPU Meter - hide if extremely small
  if (centerInfoBounds_.getWidth() > 150) {
      cpuMeterBounds_ = centerInfoBounds_.removeFromRight(120).withHeight(20);
      cpuMeterBounds_.setY(area.getCentreY() - 10);
      centerInfoBounds_.removeFromRight(20); // Spacer
  } else {
      cpuMeterBounds_ = juce::Rectangle<int>(); // Hidden
  }

  // Update cached resources on Message Thread (Safe)
  SkRect skBounds = SkRect::MakeWH((float)getWidth(), (float)getHeight());
  updateCachedPaints(skBounds);
  cachedBounds_ = skBounds;
}

void TransportBar::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  // 1. Background (Glassmorphic)
  GlassmorphicPanel::draw(canvas, skBounds, GlassmorphicPanel::Style::Elevated);

  // 2. Bottom Border Glow
  NeonGlow::drawGlow(
      canvas, SkRect::MakeXYWH(0, skBounds.height() - 2, skBounds.width(), 2),
      design::colors::CYAN, NeonGlow::Intensity::Subtle);

  // 3. Draw transport buttons using vector icons
  drawTransportButton(canvas, playButtonBounds_, icons::Play(), isPlaying_,
                      design::colors::NEON_GREEN, playState_);
  drawTransportButton(canvas, stopButtonBounds_, icons::Stop(), !isPlaying_,
                      design::colors::BLUE, stopState_);
  drawTransportButton(canvas, recordButtonBounds_, icons::Record(),
                      isRecording_, design::colors::RED, recordState_);

  // View Toggle - uses ViewToggle icon
  drawTransportButton(canvas, viewToggleButtonBounds_, icons::ViewToggle(),
                      false, design::colors::TEXT_PRIMARY, viewToggleState_);

  // Export Button - uses Download/Save icon
  drawTransportButton(canvas, exportButtonBounds_, icons::Download(), false,
                      design::colors::TEXT_PRIMARY, exportState_);

  // Settings Button - uses Settings gear icon
  drawTransportButton(canvas, settingsButtonBounds_, icons::Settings(), false,
                      design::colors::TEXT_PRIMARY, settingsState_);

  // 4. Draw Info Text (Tempo & Project)
  SkPaint textPaint; // Stack alloc is cheap
  textPaint.setStyle(SkPaint::kFill_Style);
  textPaint.setColor(design::colors::TEXT_PRIMARY);
  textPaint.setAntiAlias(true);

  // Calculate centered positions
  float centerX = centerInfoBounds_.getCentreX();
  float centerY = centerInfoBounds_.getCentreY();
  
  // Tempo with glow (centered)
  juce::String tempoStr = juce::String(tempo_, 1) + " BPM";
  
  // Measure text to center it accurately
  SkRect textBounds;
  font_.measureText(tempoStr.toStdString().c_str(), tempoStr.length(), SkTextEncoding::kUTF8, &textBounds);
  float tempoX = centerX - textBounds.width() / 2.0f;
  float tempoY = centerY - 5.0f; // Slightly above center

  NeonGlow::drawTextGlow(canvas, tempoStr.toStdString().c_str(), tempoX, tempoY,
                         font_, design::colors::CYAN,
                         NeonGlow::Intensity::Subtle);

  // Project Name (Subtle, below Tempo)
  textPaint.setColor(design::colors::TEXT_SECONDARY);
  
  juce::String projectStr = projectName_;
  smallFont_.measureText(projectStr.toStdString().c_str(), projectStr.length(), SkTextEncoding::kUTF8, &textBounds);
  float projX = centerX - textBounds.width() / 2.0f;
  float projY = centerY + 12.0f; // Below center
  
  canvas->drawString(projectStr.toStdString().c_str(), projX, projY,
                     smallFont_, textPaint);

  // 5. Draw CPU meter
  drawMeter(canvas, cpuMeterBounds_, cpuUsage_ / 100.0f, "CPU");
}

void TransportBar::updateCachedPaints(const SkRect &bounds) {
  // 1. Background Paint
  bgPaint_.setAntiAlias(true);
  SkPoint pts[2] = {{0, 0}, {0, bounds.height()}};
  SkColor colors[2] = {design::withAlpha(design::colors::BG_01, 0.94f),
                       design::withAlpha(design::colors::BG_00, 0.94f)};
  bgPaint_.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2,
                                                  SkTileMode::kClamp));
  bgPaint_.setStyle(SkPaint::kFill_Style);

  // 2. Border Paint
  borderPaint_.setAntiAlias(true);
  borderPaint_.setStyle(SkPaint::kStroke_Style);
  borderPaint_.setStrokeWidth(1.0f);
  borderPaint_.setColor(design::withAlpha(design::colors::ACCENT_PRIMARY, 0.2f)); // Themed glow

  // 3. Fonts
  // Use Mono font for Tempo/BPM display to avoid jitter
  font_ = design::getMonoFont(18.0f, design::FontWeight::Medium);
  
  // Use UI font for labels
  smallFont_ = design::getSkFont(14.0f, design::FontWeight::Regular);
}

void TransportBar::drawTransportButton(SkCanvas *canvas,
                                       const juce::Rectangle<int> &bounds,
                                       const SkPath &iconPath, bool isActive,
                                       uint32_t color,
                                       const InteractionState &state) {
  SkRect rect =
      SkRect::MakeXYWH((float)bounds.getX(), (float)bounds.getY(),
                       (float)bounds.getWidth(), (float)bounds.getHeight());

  // Determine visual state based on hover/pressed/active
  bool showGlow = isActive || state.hoverAmount > 0.1f;

  if (isActive) {
    // Active State: Glass panel with accent glow
    GlassmorphicPanel::drawWithAccent(canvas, rect, color,
                                      GlassmorphicPanel::Style::ActiveGlow);
  } else if (state.hoverAmount > 0.01f) {
    // Hover State: Elevated glass, fading in derived from hoverAmount
    GlassmorphicPanel::Options opts;
    opts.style = GlassmorphicPanel::Style::Elevated;

    // Animate opacity based on hover amount
    uint32_t baseColor = design::withAlpha(color, 0.3f);
    opts.accentColor = SkColorSetA(
        baseColor,
        static_cast<U8CPU>(76 * state.hoverAmount)); // 0.3 * 255 = 76

    opts.glowIntensity = 0.5f * state.hoverAmount;
    GlassmorphicPanel::drawWithOptions(canvas, rect, opts);
  } else {
    // Inactive State: Subtle glass panel
    GlassmorphicPanel::draw(canvas, rect, GlassmorphicPanel::Style::Subtle);
  }

  // Pressed overlay (darken slightly)
  if (state.pressAmount > 0.01f) {
    InteractionHelper::drawPressedOverlay(canvas, rect, state.pressAmount,
                                          design::dimensions::RADIUS_SM);
  }

  // Calculate icon size (about 60% of button height)
  float iconSize = bounds.getHeight() * 0.6f;

  // Set up icon style with hover brightness boost
  icons::IconStyle style;
  SkColor iconColor = isActive ? SK_ColorWHITE : design::colors::TEXT_SECONDARY;

  // Brighten icon on hover
  if (!isActive && state.hoverAmount > 0.01f) {
    iconColor = design::interpolateColor(iconColor, SK_ColorWHITE,
                                         state.hoverAmount * 0.5f);
  }

  style.color = iconColor;
  style.filled = isActive; // Filled when active
  style.strokeWidth = icons::STROKE_REGULAR;

  if (showGlow) {
    style.glowRadius =
        isActive ? design::effects::GLOW_STRONG : design::effects::GLOW_SUBTLE;
    if (!isActive) {
      style.glowRadius *= state.hoverAmount; // Fade in glow
    }
    style.glowColor = color;
  }

  // Draw the icon centered in the button
  icons::drawIconCentered(canvas, iconPath, rect, iconSize, style);
}

void TransportBar::drawMeter(SkCanvas *canvas,
                             const juce::Rectangle<int> &bounds, float value,
                             const char *label) {
  SkRect rect =
      SkRect::MakeXYWH((float)bounds.getX(), (float)bounds.getY(),
                       (float)bounds.getWidth(), (float)bounds.getHeight());

  // Background
  SkPaint bgPaint;
  bgPaint.setColor(design::withAlpha(design::colors::BG_00, 0.4f));
  bgPaint.setAntiAlias(true);
  canvas->drawRoundRect(rect, 4.0f, 4.0f, bgPaint);

  // Use NeonGlow helper for the meter bar
  // Note: drawVUMeterGlow takes normalized value
  // We need to draw the filled part ourselves if we want gradient,
  // or we can use the helper if it supports drawing the bar.
  // Checking NeonGlow.h... helper draws "glow at the peak".
  // So we still need to draw the bar itself.
  // Let's implement a consistent bar drawer here or reuse logic.

  // Actually, let's keep it simple and consistent:
  // 1. Draw bar
  float fillWidth = (float)bounds.getWidth() * juce::jlimit(0.0f, 1.0f, value);
  if (fillWidth > 0) {
    SkRect fillRect =
        SkRect::MakeXYWH(rect.left(), rect.top(), fillWidth, rect.height());

    SkPoint pts[2] = {{rect.left(), rect.centerY()},
                      {rect.right(), rect.centerY()}};
    SkColor colors[3] = {design::colors::NEON_GREEN, design::colors::AMBER,
                         design::colors::RED};
    SkScalar pos[3] = {0.0f, 0.6f, 1.0f};

    SkPaint fillPaint;
    fillPaint.setShader(
        SkGradientShader::MakeLinear(pts, colors, pos, 3, SkTileMode::kClamp));
    fillPaint.setAntiAlias(true);
    canvas->drawRoundRect(fillRect, 4.0f, 4.0f, fillPaint);

    // 2. Add Peak Glow
    NeonGlow::drawVUMeterGlow(canvas, rect, value, false); // false = horizontal
  }

  // Label
  SkFont font = design::getSkFont(10.0f, design::FontWeight::Bold);

  SkPaint textPaint;
  textPaint.setColor(design::colors::TEXT_PRIMARY);
  textPaint.setAntiAlias(true);

  canvas->drawString(label, (float)bounds.getX() + 5.0f,
                     (float)bounds.getY() - 5.0f, font, textPaint);
}

void TransportBar::mouseDown(const juce::MouseEvent &e) {
  bool isRightClick = e.mods.isRightButtonDown();
  
  // Set pressed state
  playState_.isPressed = playButtonBounds_.contains(e.getPosition()) && !isRightClick;
  stopState_.isPressed = stopButtonBounds_.contains(e.getPosition()) && !isRightClick;
  recordState_.isPressed = recordButtonBounds_.contains(e.getPosition()) && !isRightClick;
  viewToggleState_.isPressed = viewToggleButtonBounds_.contains(e.getPosition()) && !isRightClick;
  exportState_.isPressed = exportButtonBounds_.contains(e.getPosition()) && !isRightClick;
  settingsState_.isPressed = settingsButtonBounds_.contains(e.getPosition()) && !isRightClick;

  if (playButtonBounds_.contains(e.getPosition())) {
    if (isRightClick) {
      auto menu = ContextMenuManager::createMenu();
      menu->addItem(1, "Restart Playback", true, false, [this]() {
          if (onStopClicked) onStopClicked();
          if (onRewind) onRewind();
          if (onPlayClicked) onPlayClicked();
      });
      menu->addItem(2, "Loop Playback", true, false, [this]() {
          if (onLoopToggled) onLoopToggled();
      });
      ContextMenuManager::getInstance().showMenuAt(std::move(menu), this, e.x, e.y);
    } else if (onPlayClicked) {
      onPlayClicked();
    }
  } else if (stopButtonBounds_.contains(e.getPosition())) {
    if (isRightClick) {
      auto menu = ContextMenuManager::createMenu();
      menu->addItem(1, "Stop & Return to 0", true, false, [this]() {
          if (onStopClicked) onStopClicked();
          if (onRewind) onRewind();
      });
      menu->addItem(2, "Clear All Solo", true, false, [this]() {
          if (onClearAllSolos) onClearAllSolos();
      });
      ContextMenuManager::getInstance().showMenuAt(std::move(menu), this, e.x, e.y);
    } else if (onStopClicked) {
      onStopClicked();
    }
  } else if (recordButtonBounds_.contains(e.getPosition())) {
    if (!isRightClick && onRecordClicked) {
      onRecordClicked();
    }
  } else if (viewToggleButtonBounds_.contains(e.getPosition()) && !isRightClick) {
    if (onViewToggleClicked)
      onViewToggleClicked();
  } else if (exportButtonBounds_.contains(e.getPosition()) && !isRightClick) {
    if (onExportClicked)
      onExportClicked();
  } else if (settingsButtonBounds_.contains(e.getPosition()) && !isRightClick) {
    if (onSettingsClicked)
      onSettingsClicked();
  }
}

void TransportBar::mouseMove(const juce::MouseEvent &e) {
  auto pos = e.getPosition();
  playState_.isHovered = playButtonBounds_.contains(pos);
  stopState_.isHovered = stopButtonBounds_.contains(pos);
  recordState_.isHovered = recordButtonBounds_.contains(pos);
  viewToggleState_.isHovered =
      viewToggleButtonBounds_.contains(pos);
  exportState_.isHovered = exportButtonBounds_.contains(pos);
  settingsState_.isHovered = settingsButtonBounds_.contains(pos);

  // Trigger Global Help Callbacks
  if (globalHelpCallback) {
    if (playState_.isHovered) 
        globalHelpCallback("Start Playback", "Begins audio and MIDI playback from the current position. Shortcut: Space.");
    else if (stopState_.isHovered) 
        globalHelpCallback("Stop Playback", "Stops all rendering and returns playhead to start. Double-click to return to 0.");
    else if (recordState_.isHovered) 
        globalHelpCallback("Record", "Begins recording onto armed tracks. Pro Tip: Use 'Count-in' in settings for a lead-in.");
    else if (viewToggleState_.isHovered) 
        globalHelpCallback("Switch View", "Toggles between linear Arranger and loop-based Session view. Shortcut: Tab.");
    else if (exportState_.isHovered) 
        globalHelpCallback("Export", "Mixes down your project to a high-quality audio file. Support for WAV, MP3, and FLAC.");
    else if (settingsState_.isHovered) 
        globalHelpCallback("Audio Settings", "Configure your sound card, buffer size, and MIDI hardware here.");
  }
}

void TransportBar::mouseEnter(const juce::MouseEvent &e) { mouseMove(e); }

void TransportBar::mouseExit(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  // Clear all hover states
  playState_.isHovered = false;
  stopState_.isHovered = false;
  recordState_.isHovered = false;
  viewToggleState_.isHovered = false;
  exportState_.isHovered = false;
  settingsState_.isHovered = false;
}

void TransportBar::timerCallback() {
  SkiaComponent::timerCallback(); // Call base for global animations

  float dt = 1.0f / 60.0f;
  // Update animations
  playState_.update(dt);
  stopState_.update(dt);
  recordState_.update(dt);
  viewToggleState_.update(dt);
  exportState_.update(dt);
  settingsState_.update(dt);

  // Check if any need repainting
  if (playState_.isAnimating() || stopState_.isAnimating() ||
      recordState_.isAnimating() || viewToggleState_.isAnimating() ||
      exportState_.isAnimating() || settingsState_.isAnimating()) {
    repaint();
  }
}

std::unique_ptr<juce::AccessibilityHandler>
TransportBar::createAccessibilityHandler() {
  // Return a group handler so it exposes children
  return std::make_unique<juce::AccessibilityHandler>(
      *this, juce::AccessibilityRole::group);
}

TransportBar::~TransportBar() = default;

} // namespace zenith

#endif // ZENITH_USE_SKIA

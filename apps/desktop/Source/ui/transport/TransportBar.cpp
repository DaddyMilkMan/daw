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

/**
 * @brief Custom Label that provides real-time typing notifications
 */
class TransportEditorLabel : public juce::Label {
public:
    TransportEditorLabel(const juce::String& name = {}) : juce::Label(name, {}) {}
    
    std::function<void(const juce::String&)> onTyping;
    
    void editorShown(juce::TextEditor* ed) override {
        if (!ed) return;
        
        // Match our aesthetic
        ed->setJustification(getJustificationType());
        ed->setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
        ed->setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        ed->setColour(juce::TextEditor::textColourId, juce::Colours::white);
        
        // Real-time update
        ed->onTextChange = [this, ed] {
            if (onTyping) onTyping(ed->getText());
        };
    }
};

TransportBar::TransportBar() {
  setSize(800, 60);
  setOpaque(false);
  
  // Initialize Editors
  auto bpmEd = std::make_unique<TransportEditorLabel>("bpmLabel");
  bpmEd->setJustificationType(juce::Justification::centred);
  bpmEd->setColour(juce::Label::textColourId, juce::Colours::white);
  bpmEd->setColour(juce::Label::textWhenEditingColourId, juce::Colours::white);
  bpmEd->setColour(juce::Label::backgroundWhenEditingColourId, juce::Colours::transparentBlack);
  bpmEd->setColour(juce::Label::outlineWhenEditingColourId, juce::Colours::transparentBlack);
  bpmEd->setFont(juce::Font("Inter", 32.0f, juce::Font::bold));
  bpmEd->setVisible(false);
  bpmEd->setEditable(false, true, false);
  
  bpmEd->onTyping = [this](const juce::String& text) {
      double newBpm = text.getDoubleValue();
      if (newBpm >= 1.0 && newBpm <= 2000.0) {
          setTempo(newBpm);
          if (onTempoChanged) onTempoChanged(newBpm);
      }
  };
  
  bpmEd->onTextChange = [this, ed = bpmEd.get()] {
      ed->setVisible(false);
      repaint();
  };
  
  bpmEd->onEditorHide = [this, ed = bpmEd.get()] {
     ed->setVisible(false);
     repaint();
  };
  bpmLabel_ = std::move(bpmEd);
  addChildComponent(bpmLabel_.get());
  
  // === TWO-STEP TIME SIGNATURE EDITOR ===
  // Step 1: Numerator editor
  auto numEd = std::make_unique<TransportEditorLabel>("timeSigNum");
  numEd->setJustificationType(juce::Justification::centred);
  numEd->setColour(juce::Label::textColourId, juce::Colours::white);
  numEd->setColour(juce::Label::textWhenEditingColourId, juce::Colours::white);
  numEd->setColour(juce::Label::backgroundWhenEditingColourId, juce::Colours::transparentBlack);
  numEd->setColour(juce::Label::outlineWhenEditingColourId, juce::Colours::transparentBlack);
  numEd->setFont(juce::Font("Inter", 24.0f, juce::Font::bold));
  numEd->setVisible(false);
  numEd->setEditable(false, true, false);
  
  // When numerator is confirmed (Enter pressed), move to denominator
  numEd->onTextChange = [this] {
      int n = timeSigNumLabel_->getText().getIntValue();
      if (n > 0 && n <= 64) {
          timeSigNum_ = n;
          // Hide numerator, show denominator
          timeSigNumLabel_->setVisible(false);
          timeSigDenLabel_->setText(juce::String(timeSigDen_), juce::dontSendNotification);
          timeSigDenLabel_->setVisible(true);
          timeSigDenLabel_->showEditor();
          editingTimeSigNum_ = false;
      }
      repaint();
  };
  
  numEd->onEditorHide = [this] {
      timeSigNumLabel_->setVisible(false);
      repaint();
  };
  
  // Real-time preview for numerator
  numEd->onTyping = [this](const juce::String& text) {
      int n = text.trim().getIntValue();
      if (n > 0 && n <= 64) {
          setTimeSignature(n, timeSigDen_);
          if (onTimeSignatureChanged) onTimeSignatureChanged(n, timeSigDen_);
      }
  };
  
  timeSigNumLabel_ = std::move(numEd);
  // addChildComponent(timeSigNumLabel_.get());
  
  // Step 2: Denominator editor
  auto denEd = std::make_unique<TransportEditorLabel>("timeSigDen");
  denEd->setJustificationType(juce::Justification::centred);
  denEd->setColour(juce::Label::textColourId, juce::Colours::white);
  denEd->setColour(juce::Label::textWhenEditingColourId, juce::Colours::white);
  denEd->setColour(juce::Label::backgroundWhenEditingColourId, juce::Colours::transparentBlack);
  denEd->setColour(juce::Label::outlineWhenEditingColourId, juce::Colours::transparentBlack);
  denEd->setFont(juce::Font("Inter", 24.0f, juce::Font::bold));
  denEd->setVisible(false);
  denEd->setEditable(false, true, false);
  
  // When denominator is confirmed, finalize and close
  denEd->onTextChange = [this] {
      int d = timeSigDenLabel_->getText().getIntValue();
      if (d > 0 && d <= 64) {
          timeSigDen_ = d;
          setTimeSignature(timeSigNum_, timeSigDen_);
          if (onTimeSignatureChanged) onTimeSignatureChanged(timeSigNum_, timeSigDen_);
      }
      timeSigDenLabel_->setVisible(false);
      repaint();
  };
  
  denEd->onEditorHide = [this] {
      timeSigDenLabel_->setVisible(false);
      repaint();
  };
  
  // Real-time preview for denominator
  denEd->onTyping = [this](const juce::String& text) {
      int d = text.trim().getIntValue();
      if (d > 0 && d <= 64) {
          setTimeSignature(timeSigNum_, d);
          if (onTimeSignatureChanged) onTimeSignatureChanged(timeSigNum_, d);
      }
  };
  
  timeSigDenLabel_ = std::move(denEd);
  // addChildComponent(timeSigDenLabel_.get());
}

void TransportBar::visibilityChanged() {
  // DEBUG: Disable timer to test for crash
  // Only start timer when:
  // 1. Component is visible
  // 2. Component has a peer (is on desktop) - prevents blocking during construction
  // 3. Timer isn't already running
  // if (isVisible() && getPeer() != nullptr && !isTimerRunning()) {
  //   if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimerHz(60);
  // }
}

void TransportBar::resized() {
  using namespace design;
  auto area = getLocalBounds();
  
  // LOGIC PRO STYLE LAYOUT
  // 1. LCD Display (The Anchor) - Widened for interactivity
  const int lcdWidth = 380; // Expanded to 380
  const int lcdHeight = 42;
  lcdBounds_ = area.withSizeKeepingCentre(lcdWidth, lcdHeight);
  
  // Define Hit Zones for LCD Interaction
  // BPM: Left half (approx 170px)
  // Time: Right half
  auto tempLcd = lcdBounds_;
  bpmHitBounds_ = tempLcd.removeFromLeft(170).reduced(4, 4);
  timeSigHitBounds_ = tempLcd.removeFromRight(170).reduced(4, 4);
  
  // Setup Editor Bounds (Hidden usually)
  if (bpmLabel_) bpmLabel_->setBounds(bpmHitBounds_.expanded(4, 0));
  
  // Split time sig hit area into numerator (left) and denominator (right)
  auto tsArea = timeSigHitBounds_;
  auto numArea = tsArea.removeFromLeft(tsArea.getWidth() / 2 - 10);
  auto denArea = tsArea.removeFromRight(tsArea.getWidth() - 10);
  if (timeSigNumLabel_) timeSigNumLabel_->setBounds(numArea.expanded(4, 0));
  if (timeSigDenLabel_) timeSigDenLabel_->setBounds(denArea.expanded(4, 0));
  
  // 2. Transport Controls (Immediately Left of LCD)
  const int buttonSize = 38; // 36->38
  const int spacing = 16;   // 12->16 (More breathing room)
  
  int tx = lcdBounds_.getX() - spacing - buttonSize;
  
  // Record
  recordButtonBounds_ = juce::Rectangle<int>(tx, area.getCentreY() - (buttonSize/2), buttonSize, buttonSize);
  tx -= (buttonSize + spacing);
  
  // Play
  playButtonBounds_ = juce::Rectangle<int>(tx, area.getCentreY() - (buttonSize/2), buttonSize, buttonSize);
  tx -= (buttonSize + spacing);
  
  // Stop
  stopButtonBounds_ = juce::Rectangle<int>(tx, area.getCentreY() - (buttonSize/2), buttonSize, buttonSize);


  // 3. Right Section: Tools (Pushed further out)
  auto rightSection = area.removeFromRight(300).reduced(16, 8);
  
  // Settings
  settingsButtonBounds_ = rightSection.removeFromRight(32).withSizeKeepingCentre(32, 32);
  rightSection.removeFromRight(20);
  
  // Export
  exportButtonBounds_ = rightSection.removeFromRight(32).withSizeKeepingCentre(32, 32);
  rightSection.removeFromRight(20);
  
  // CPU Meter
  cpuMeterBounds_ = rightSection.withHeight(12).withY(area.getCentreY() - 6);

  // 4. Left Section: View Toggles
  auto leftSection = area.removeFromLeft(200).reduced(16, 8);
  viewToggleButtonBounds_ = leftSection.removeFromLeft(32).withSizeKeepingCentre(32, 32);

  // Update cached resources
  SkRect skBounds = SkRect::MakeWH((float)getWidth(), (float)getHeight());
  updateCachedPaints(skBounds);
  cachedBounds_ = skBounds;
}

void TransportBar::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());
  
  // 1. Main Background: Deep Glass
  // "Gradient but also solid" -> Elevated style with dark tint
  GlassmorphicPanel::Options barOpts;
  barOpts.style = GlassmorphicPanel::Style::Elevated;
  barOpts.cornerRadius = 0.0f; // Full width bar = no corner radius
  barOpts.customTintColor = SkColorSetA(design::colors::BG_DARKEST, 240); // Very opaque
  barOpts.useBackdropBlur = true;
  barOpts.drawShadow = true;
  GlassmorphicPanel::drawWithOptions(canvas, skBounds, barOpts);
  
  // 1.5 Top Highlight / Rim for the whole bar
  GlassmorphicPanel::drawDivider(canvas, 0, skBounds.bottom(), skBounds.width()); // Bottom separator
  
  // 2. LCD Display (Center)
  {
      SkRect lcdRect = SkRect::MakeXYWH(lcdBounds_.getX(), lcdBounds_.getY(), lcdBounds_.getWidth(), lcdBounds_.getHeight());
      
      // Sunken Glass Effect
      GlassmorphicPanel::Options lcdOpts;
      lcdOpts.style = GlassmorphicPanel::Style::Subtle; // Less blur
      lcdOpts.cornerRadius = 6.0f;
      lcdOpts.customTintColor = SkColorSetA(SK_ColorBLACK, 150); // Darker than bar
      lcdOpts.drawTopHighlight = false; // Inset look
      lcdOpts.drawShadow = false; // No drop shadow for inset
      
      // Draw LCD Background
      GlassmorphicPanel::drawWithOptions(canvas, lcdRect, lcdOpts);
      
      // LCD Inner Border (Subtle inset glow)
      SkPaint insetPaint;
      insetPaint.setStyle(SkPaint::kStroke_Style);
      insetPaint.setStrokeWidth(1.0f);
      insetPaint.setColor(SkColorSetA(design::colors::ACCENT_PRIMARY, 30));
      canvas->drawRoundRect(lcdRect, 6.0f, 6.0f, insetPaint);
      
      // LCD Content: BPM (Left Hit Zone) and Time Sig (Right/Split)
      // Use Hit Bounds for centering
      
      // Divider
      SkPaint divPaint;
      divPaint.setColor(SkColorSetA(design::colors::BORDER_SUBTLE, 50));
      float divX = lcdRect.centerX();
      // canvas->drawLine(divX, lcdRect.top() + 6, divX, lcdRect.bottom() - 6, divPaint); 
      // Maybe just a light separating line
      
      // BPM Section
      SkPaint textPaint;
      textPaint.setAntiAlias(true);
      SkFont bpmFont = design::getMonoFont(22.0f, design::FontWeight::Bold);
      SkFont labelFont = design::getSkFont(10.0f, design::FontWeight::Medium);
      
      // Calculate layout for BPM
      juce::String bpmStr = juce::String(tempo_, 1);
      std::string bpmText = bpmStr.toStdString();
      float bpmValueWidth = bpmFont.measureText(bpmText.c_str(), bpmText.length(), SkTextEncoding::kUTF8);
      float bpmLabelWidth = labelFont.measureText("BPM", 3, SkTextEncoding::kUTF8);
      float spacing = 8.0f;
      float totalBpmWidth = bpmValueWidth + spacing + bpmLabelWidth;
      
      auto bpmCenter = bpmHitBounds_.getCentre();
      float bpmStartX = bpmCenter.getX() - (totalBpmWidth / 2.0f);
      float centerY = lcdRect.centerY() + 8.0f; // Baseline approx
      
      // Draw BPM Value (Only if not being edited)
      if (!bpmLabel_ || !bpmLabel_->isVisible()) {
          // Draw BPM Value
          textPaint.setColor(design::colors::TEXT_PRIMARY);
          canvas->drawString(bpmText.c_str(), bpmStartX, centerY, bpmFont, textPaint);
          
          // Draw BPM Label
          textPaint.setColor(design::colors::TEXT_SECONDARY);
          canvas->drawString("BPM", bpmStartX + bpmValueWidth + spacing, centerY, labelFont, textPaint);
      }
      // Calculate layout for Time Sig
      juce::String sigStr = juce::String(timeSigNum_) + " / " + juce::String(timeSigDen_);
      std::string sigText = sigStr.toStdString();
      float sigValueWidth = bpmFont.measureText(sigText.c_str(), sigText.length(), SkTextEncoding::kUTF8);
      float sigLabelWidth = labelFont.measureText("TIME", 4, SkTextEncoding::kUTF8);
      float totalSigWidth = sigValueWidth + spacing + sigLabelWidth;
      
      auto timeCenter = timeSigHitBounds_.getCentre();
      float sigStartX = timeCenter.getX() - (totalSigWidth / 2.0f);
      
      // Draw Time Sig Value
      textPaint.setColor(design::colors::TEXT_PRIMARY);
      canvas->drawString(sigText.c_str(), sigStartX, centerY, bpmFont, textPaint);
      
      // Draw TIME Label
      textPaint.setColor(design::colors::TEXT_SECONDARY);
      canvas->drawString("TIME", sigStartX + sigValueWidth + spacing, centerY - 6.0f, labelFont, textPaint);
      
      // Interaction Hints (Hover)
      if (isDraggingBpm_) {
           SkPaint hl;
           hl.setColor(SkColorSetA(design::colors::ACCENT_PRIMARY, 30));
           auto r = bpmHitBounds_.toFloat();
           canvas->drawRoundRect(SkRect::MakeXYWH(r.getX(), r.getY(), r.getWidth(), r.getHeight()), 4.0f, 4.0f, hl);
      }
      if (isDraggingTimeSig_) {
           SkPaint hl;
           hl.setColor(SkColorSetA(design::colors::ACCENT_PRIMARY, 30));
           auto r = timeSigHitBounds_.toFloat();
           canvas->drawRoundRect(SkRect::MakeXYWH(r.getX(), r.getY(), r.getWidth(), r.getHeight()), 4.0f, 4.0f, hl);
      }
  }

  // 3. Transport Buttons
  // Play - Triangle
  drawTransportButton(canvas, playButtonBounds_, icons::Play(), isPlaying_,
                      design::colors::NEON_GREEN, playState_);
                      
  // Stop - PERFECT SQUARE as requested
  drawTransportButton(canvas, stopButtonBounds_, icons::Stop(), !isPlaying_,
                      design::colors::BLUE, stopState_);
                      
  // Record - Circle
  drawTransportButton(canvas, recordButtonBounds_, icons::Record(),
                      isRecording_ || recordState_.isHovered,
                      design::colors::RED, recordState_);


  // 4. Secondary Tools (Right)
  drawTransportButton(canvas, settingsButtonBounds_, icons::Settings(), false,
                      design::colors::TEXT_SECONDARY, settingsState_);
  drawTransportButton(canvas, exportButtonBounds_, icons::Download(), false,
                      design::colors::TEXT_SECONDARY, exportState_);

  // 5. View Toggle (Left)
  drawTransportButton(canvas, viewToggleButtonBounds_, icons::ViewToggle(),
                      false, design::colors::TEXT_SECONDARY, viewToggleState_);

  // 6. CPU Meter (Updated visual)
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

  // 1. Background / Pill (Only for Active or Hover)
  // Logic Style: Buttons are just floating icons usually, until interacted with.
  
  if (isActive) {
      // Active State: Subtle filled background with strong glow
      SkPaint activeBg;
      activeBg.setAntiAlias(true);
      // Faint colored background
      activeBg.setColor(SkColorSetA(color, 40)); 
      canvas->drawRoundRect(rect, 6.0f, 6.0f, activeBg);
      
      // Add a border for "pushed" feel?
      SkPaint border;
      border.setStyle(SkPaint::kStroke_Style);
      border.setStrokeWidth(1.0f);
      border.setColor(SkColorSetA(color, 80));
      canvas->drawRoundRect(rect, 6.0f, 6.0f, border);
      
  } else if (state.hoverAmount > 0.01f) {
      // Hover State: White/Bright overlay
      SkPaint hoverBg;
      hoverBg.setAntiAlias(true);
      hoverBg.setColor(SkColorSetA(SK_ColorWHITE, (uint8_t)(20 * state.hoverAmount)));
      canvas->drawRoundRect(rect, 6.0f, 6.0f, hoverBg);
  }
  
  // 2. Icon Rendering
  icons::IconStyle style;
  
  // Base Icon Properties
  style.strokeWidth = 2.0f;
  style.filled = isActive; // Fill only when active? Or always stroke?
  // Logic uses filled icons often. Let's stick to fill for active.
  
  if (isActive) {
      style.color = color; // Example: Neon Green icon
      style.glowColor = color;
      style.glowRadius = 15.0f; // Strong glow
  } else {
      // Idle: White, high opacity (so it pops against dark glass)
      // Hover: Go to full white
      float opacity = 0.7f + (0.3f * state.hoverAmount);
      style.color = design::withAlpha(SK_ColorWHITE, opacity);
      
      if (state.hoverAmount > 0.1f) {
          style.glowColor = SK_ColorWHITE;
          style.glowRadius = 5.0f * state.hoverAmount;
      }
  }

  // Draw scaled and centered icon using helper for consistency
  float iconSize = rect.width() * 0.45f;
  icons::drawIconCentered(canvas, iconPath, rect, iconSize, style);
}

void TransportBar::drawMeter(SkCanvas *canvas,
                             const juce::Rectangle<int> &bounds, float value,
                             const char *label) {
  SkRect rect =
      SkRect::MakeXYWH((float)bounds.getX(), (float)bounds.getY(),
                       (float)bounds.getWidth(), (float)bounds.getHeight());

  // "Linear" modern meter - a horizontal glossy bar
  
  // 1. Label (Tiny, on top or left)
  SkPaint textPaint;
  textPaint.setColor(design::colors::TEXT_SECONDARY);
  textPaint.setAntiAlias(true);
  
  // Draw label on left of bar?
  // Let's draw it small inside the bar on left? No, cleaner outside.
  // Actually, user wants "Accurate info".
  // Let's draw value as text next to it.
  
  juce::String valStr = juce::String((int)(value * 100)) + "% CPU";
  canvas->drawString(valStr.toStdString().c_str(), rect.left(), rect.centerY() + 4, smallFont_, textPaint);
  
  // Shift rect for bar 
  SkRect barRect = rect;
  float textWidth = 60.0f; // Approx
  barRect.fLeft += textWidth;
  
  if (barRect.width() > 10) {
      // Background track
      SkPaint bgPaint;
      bgPaint.setColor(SkColorSetA(design::colors::BG_03, 150));
      bgPaint.setAntiAlias(true);
      canvas->drawRoundRect(barRect, 4.0f, 4.0f, bgPaint);
      
      // Fill
      float fillW = barRect.width() * value;
      if (fillW > 0) {
          SkRect fillRect = barRect;
          fillRect.fRight = fillRect.fLeft + fillW;
          
          SkPaint fillPaint;
          // Gradient from green to red based on load
          SkColor color = design::colors::NEON_GREEN;
          if (value > 0.5f) color = design::colors::AMBER;
          if (value > 0.8f) color = design::colors::RED;
          
          fillPaint.setColor(color);
          fillPaint.setAntiAlias(true);
          
          // Add glow
          fillPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 2.0f));
          canvas->drawRoundRect(fillRect, 4.0f, 4.0f, fillPaint);
          
          // Solid core
          fillPaint.setMaskFilter(nullptr);
          fillPaint.setAlpha(255);
          canvas->drawRoundRect(fillRect, 4.0f, 4.0f, fillPaint);
      }
  }
}

void TransportBar::mouseDown(const juce::MouseEvent &e) {
  // 1. LCD Interaction (BPM / TimeSig)
  if (bpmHitBounds_.contains(e.getPosition())) {
      isDraggingBpm_ = true;
      dragStartValue_ = tempo_;
      dragStartPos_ = e.getPosition();
      return;
  }
  
  if (timeSigHitBounds_.contains(e.getPosition())) {
      isDraggingTimeSig_ = true;
      dragStartNum_ = timeSigNum_;
      dragStartDen_ = timeSigDen_;
      dragStartPos_ = e.getPosition();
      return;
  }

  bool isRightClick = e.mods.isRightButtonDown();
  
  // Set pressed state
  playState_.isPressed = playButtonBounds_.contains(e.getPosition()) && !isRightClick;
  stopState_.isPressed = stopButtonBounds_.contains(e.getPosition()) && !isRightClick;
  recordState_.isPressed = recordButtonBounds_.contains(e.getPosition()) && !isRightClick;
  viewToggleState_.isPressed = viewToggleButtonBounds_.contains(e.getPosition()) && !isRightClick;
  exportState_.isPressed = exportButtonBounds_.contains(e.getPosition()) && !isRightClick;
  settingsState_.isPressed = settingsButtonBounds_.contains(e.getPosition()) && !isRightClick;
  
  repaint();

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
    } 
    // Regular click handled on mouseUp usually, but let's conform to existing pattern:
    // Existing code triggered on mouseDown. I'll keep it but ensure pressed state is visualized.
    else if (onPlayClicked) { onPlayClicked(); }
    
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
    } else if (onStopClicked) { onStopClicked(); }
    
  } else if (recordButtonBounds_.contains(e.getPosition())) {
    if (!isRightClick && onRecordClicked) { onRecordClicked(); }
  } else if (viewToggleButtonBounds_.contains(e.getPosition()) && !isRightClick) {
    if (onViewToggleClicked) onViewToggleClicked();
  } else if (exportButtonBounds_.contains(e.getPosition()) && !isRightClick) {
    if (onExportClicked) onExportClicked();
  } else if (settingsButtonBounds_.contains(e.getPosition()) && !isRightClick) {
    if (onSettingsClicked) onSettingsClicked();
  }
}

void TransportBar::mouseDrag(const juce::MouseEvent &e) {
  if (isDraggingBpm_) {
      int dy = dragStartPos_.y - e.getPosition().y; // Drag UP to increase
      double change = dy * 0.5; // Sensitivity
      if (e.mods.isShiftDown()) change *= 0.1; // Fine tune
      
      double newTempo = juce::jlimit(1.0, 2000.0, dragStartValue_ + change);
      setTempo(newTempo);
      if (onTempoChanged) onTempoChanged(newTempo);
      return;
  }
  
  if (isDraggingTimeSig_) {
      int dy = dragStartPos_.y - e.getPosition().y;
      int steps = dy / 15; // Requires more movement
      int newNum = juce::jlimit(1, 64, dragStartNum_ + steps);
      
      if (newNum != timeSigNum_) {
          setTimeSignature(newNum, timeSigDen_);
          if (onTimeSignatureChanged) onTimeSignatureChanged(timeSigNum_, timeSigDen_);
      }
      return;
  }
}

void TransportBar::mouseUp(const juce::MouseEvent &e) {
  isDraggingBpm_ = false;
  isDraggingTimeSig_ = false;
  
  // Clear pressed states
  playState_.isPressed = false;
  stopState_.isPressed = false;
  recordState_.isPressed = false;
  viewToggleState_.isPressed = false;
  exportState_.isPressed = false;
  settingsState_.isPressed = false;
  repaint();
}

void TransportBar::mouseDoubleClick(const juce::MouseEvent &e) {
    if (bpmHitBounds_.contains(e.getPosition())) {
        if (bpmLabel_) {
            bpmLabel_->setText(juce::String(tempo_), juce::dontSendNotification);
            bpmLabel_->setVisible(true);
            bpmLabel_->showEditor();
        }
    } else if (timeSigHitBounds_.contains(e.getPosition())) {
        // Start two-step entry: numerator first
        if (timeSigNumLabel_) {
            editingTimeSigNum_ = true;
            timeSigNumLabel_->setText(juce::String(timeSigNum_), juce::dontSendNotification);
            timeSigNumLabel_->setVisible(true);
            timeSigNumLabel_->showEditor();
        }
    }
}

void TransportBar::mouseEnter(const juce::MouseEvent &e) { mouseMove(e); }

void TransportBar::mouseExit(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  // Clear all hover states
  bool wasHovered = playState_.isHovered || stopState_.isHovered || recordState_.isHovered ||
                    viewToggleState_.isHovered || exportState_.isHovered || settingsState_.isHovered;
                    
  playState_.isHovered = false;
  stopState_.isHovered = false;
  recordState_.isHovered = false;
  viewToggleState_.isHovered = false;
  exportState_.isHovered = false;
  settingsState_.isHovered = false;
  
  if (wasHovered) repaint();
}

void TransportBar::mouseMove(const juce::MouseEvent &e) {
  auto pos = e.getPosition();
  
  bool anyChanged = false;
  auto update = [&](InteractionState& state, const juce::Rectangle<int>& bounds) {
      bool nowHovered = bounds.contains(pos);
      if (state.isHovered != nowHovered) {
          state.isHovered = nowHovered;
          anyChanged = true;
      }
  };

  update(playState_, playButtonBounds_);
  update(stopState_, stopButtonBounds_);
  update(recordState_, recordButtonBounds_);
  update(viewToggleState_, viewToggleButtonBounds_);
  update(exportState_, exportButtonBounds_);
  update(settingsState_, settingsButtonBounds_);
  
  if (anyChanged) repaint();

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


bool TransportBar::hitTest(int x, int y) {
    if (playButtonBounds_.contains(x, y)) return true;
    if (stopButtonBounds_.contains(x, y)) return true;
    if (recordButtonBounds_.contains(x, y)) return true;
    if (viewToggleButtonBounds_.contains(x, y)) return true;
    if (settingsButtonBounds_.contains(x, y)) return true;
    if (exportButtonBounds_.contains(x, y)) return true;
    
    // NEW: Allow interaction with LCD
    if (lcdBounds_.contains(x, y)) return true;
    
    return false;
}

} // namespace zenith

#endif // ZENITH_USE_SKIA

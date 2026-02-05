/*
  ==============================================================================

    TransportBar.cpp
    Created: 2025-11-28
    Author:  Leo "Lil Bit" Rossi

    Implementation of transport controls with Neon Noir styling.

  ==============================================================================
*/

#include "TransportBar.h"

#ifndef NDEBUG
#define TB_DEBUG(x) DBG(x)
#else
#define TB_DEBUG(x) ((void)0)
#endif


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
#include "../design-system/MeterRenderer.h"
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
  bpmEd->setFont(juce::FontOptions(juce::String("Inter"), 32.0f, juce::Font::bold));
  bpmEd->setVisible(false);
  bpmEd->setEditable(false, true, false);
  
  bpmEd->onTyping = [this](const juce::String& text) {
      double newBpm = text.getDoubleValue();
      if (newBpm >= 1.0 && newBpm <= 2000.0) {
          setTempo(newBpm);
          if (onTempoChanged) onTempoChanged(newBpm);
      }
  };
  
  bpmEd->onTextChange = [this] {
      bpmLabel_->setVisible(false);
      repaint();
  };
  
  bpmEd->onEditorHide = [this] {
     bpmLabel_->setVisible(false);
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
  numEd->setFont(juce::FontOptions(juce::String("Inter"), 24.0f, juce::Font::bold));
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
  addChildComponent(timeSigNumLabel_.get());
  // Fix: Disable mouse interception on the label itself so clicks pass through to TransportBar (drag zones).
  // Allow clicks on children (the TextEditor) so editing still works when activated.
  timeSigNumLabel_->setInterceptsMouseClicks(false, true);
  
  // Step 2: Denominator editor
  auto denEd = std::make_unique<TransportEditorLabel>("timeSigDen");
  denEd->setJustificationType(juce::Justification::centred);
  denEd->setColour(juce::Label::textColourId, juce::Colours::white);
  denEd->setColour(juce::Label::textWhenEditingColourId, juce::Colours::white);
  denEd->setColour(juce::Label::backgroundWhenEditingColourId, juce::Colours::transparentBlack);
  denEd->setColour(juce::Label::outlineWhenEditingColourId, juce::Colours::transparentBlack);
  denEd->setFont(juce::FontOptions(juce::String("Inter"), 24.0f, juce::Font::bold));
  denEd->setVisible(false);
  denEd->setEditable(false, true, false);
  
  // When denominator is confirmed, finalize and close
  denEd->onTextChange = [this] {
      int d = timeSigDenLabel_->getText().getIntValue();
      if (d > 0 && d <= 64) {
          timeSigDen_ = d;
      }
      setTimeSignature(timeSigNum_, timeSigDen_);
      if (onTimeSignatureChanged) onTimeSignatureChanged(timeSigNum_, timeSigDen_);
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
  addChildComponent(timeSigDenLabel_.get());
  // Fix: Disable mouse interception on the label itself.
  timeSigDenLabel_->setInterceptsMouseClicks(false, true);
  
  ZENITH_REGISTER_ANIMATION(zenith::animation::Priority::High);
  
  // Initialize Update Service
  updateService_ = std::make_unique<zenith::network::UpdateService>();
  updateService_->checkForUpdates([this](const zenith::network::UpdateService::UpdateInfo& info) {
      if (info.available) {
          isUpdateAvailable_ = true;
          if (onUpdateAvailable) onUpdateAvailable();
          repaint();
      }
  });
  
  // Initialize smoothed CPU to 0
  smoothedCpu_ = 0.0f;
  
  createButtons();
}

void TransportBar::onAnimationTick(float deltaMs) {
  SkiaComponent::updateInternalAnimations(deltaMs);

  // FIX: Smooth CPU updates using simple LERP
  // Move 10% towards target per frame (approx 60fps)
  float kCpuSmoothing = 0.1f;
  smoothedCpu_ += (cpuUsage_ - smoothedCpu_) * kCpuSmoothing;
  
  if (std::abs(smoothedCpu_ - cpuUsage_) > 0.01f) {
      requestRepaint();
  }

  // Restore animation state updates
  float dt = deltaMs / 1000.0f;
  playState_.update(dt);
  stopState_.update(dt);
  recordState_.update(dt);
  viewToggleState_.update(dt);
  wingmanState_.update(dt);
  settingsState_.update(dt);

  if (playState_.isAnimating() || stopState_.isAnimating() ||
      recordState_.isAnimating() || viewToggleState_.isAnimating() ||
      wingmanState_.isAnimating() || settingsState_.isAnimating()) {
    requestRepaint();
  }
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
  // 1. LCD Display (The Anchor) - Compact for tighter spacing
  const int lcdWidth = 280; // Reduced for tighter BPM/TimeSig layout
  const int lcdHeight = 42;
  lcdBounds_ = area.withSizeKeepingCentre(lcdWidth, lcdHeight);
  
  // Define Hit Zones for LCD Interaction
  // Split LCD evenly - no dead zones!
  auto tempLcd = lcdBounds_;
  int halfWidth = tempLcd.getWidth() / 2;
  bpmHitBounds_ = tempLcd.removeFromLeft(halfWidth).reduced(4, 4);
  timeSigHitBounds_ = tempLcd.reduced(4, 4); // Remainder goes to time sig
  
  // Setup Editor Bounds (Hidden usually)
  if (bpmLabel_) bpmLabel_->setBounds(bpmHitBounds_.expanded(4, 0));
  
  // Split time sig hit area: [NUM] [/] [DEN] [▼ templates]
  // Make dropdown zone larger for easier clicking
  auto tsArea = timeSigHitBounds_;
  int totalW = tsArea.getWidth();
  int dropW = 30;  // Fixed width for dropdown arrow
  int contentW = totalW - dropW;
  int numW = contentW / 3;
  int slashW = contentW / 6;
  int denW = contentW - numW - slashW;
  
  timeSigNumBounds_ = tsArea.removeFromLeft(numW);
  tsArea.removeFromLeft(slashW);  // Slash area (drawn, not interactive)
  timeSigDenBounds_ = tsArea.removeFromLeft(denW);
  timeSigTemplateBounds_ = tsArea;  // Remaining = dropdown
  
  if (timeSigNumLabel_) timeSigNumLabel_->setBounds(timeSigNumBounds_.expanded(2, 0));
  if (timeSigDenLabel_) timeSigDenLabel_->setBounds(timeSigDenBounds_.expanded(2, 0));
  
  // 2. Transport Controls (Immediately Left of LCD)
  const int buttonSize = 38; // 36->38
  const int spacing = 16;   // 12->16 (More breathing room)
  
  int tx = lcdBounds_.getX() - spacing - buttonSize;
  
  // Record
  recordButtonBounds_ = juce::Rectangle<int>(tx, area.getCentreY() - (buttonSize/2), buttonSize, buttonSize);
  if (recordBtn_) recordBtn_->setBounds(recordButtonBounds_);
  tx -= (buttonSize + spacing);
  
  // Play
  playButtonBounds_ = juce::Rectangle<int>(tx, area.getCentreY() - (buttonSize/2), buttonSize, buttonSize);
  if (playBtn_) playBtn_->setBounds(playButtonBounds_);
  tx -= (buttonSize + spacing);
  
  // Stop
  stopButtonBounds_ = juce::Rectangle<int>(tx, area.getCentreY() - (buttonSize/2), buttonSize, buttonSize);
  if (stopBtn_) stopBtn_->setBounds(stopButtonBounds_);


  // 3. Right Section: Tools (Pushed further out)
  auto rightSection = area.removeFromRight(300).reduced(16, 8);
  int barCenterY = getHeight() / 2; // Use full height for proper centering
  
  // Settings (rightmost)
  settingsButtonBounds_ = rightSection.removeFromRight(32).withSizeKeepingCentre(32, 32);
  if (settingsBtn_) settingsBtn_->setBounds(settingsButtonBounds_);
  rightSection.removeFromRight(16); // Spacing
  
  // CPU Meter - Fixed width, properly centered vertically
  const int cpuMeterWidth = 140;
  const int cpuMeterHeight = 16;
  auto cpuArea = rightSection.removeFromRight(cpuMeterWidth);
  cpuMeterBounds_ = cpuArea.withSizeKeepingCentre(cpuMeterWidth, cpuMeterHeight);

  // 4. Left Section: View Toggles + Wingman AI
  auto leftSection = area.removeFromLeft(200).reduced(16, 8);
  viewToggleButtonBounds_ = leftSection.removeFromLeft(32).withSizeKeepingCentre(32, 32);
  if (viewToggleBtn_) viewToggleBtn_->setBounds(viewToggleButtonBounds_);
  leftSection.removeFromLeft(16);  // Spacing
  wingmanButtonBounds_ = leftSection.removeFromLeft(32).withSizeKeepingCentre(32, 32);
  if (wingmanBtn_) wingmanBtn_->setBounds(wingmanButtonBounds_);

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
  
  // NOTE: Removed random bottom divider line - it served no purpose
  
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
      // Calculate layout for Time Sig - CLEAN INVISIBLE STYLE
      // No visible boxes, just numbers with a slash - but separate interactive zones
      SkFont numFont = design::getMonoFont(22.0f, design::FontWeight::Bold);
      SkFont slashFont = design::getMonoFont(18.0f, design::FontWeight::Regular);
      
      // Calculate center position for the time signature display
      // Draw Numerator (only highlight when dragging)
      {
          std::string numStr = std::to_string(timeSigNum_);
          float tw = numFont.measureText(numStr.c_str(), numStr.length(), SkTextEncoding::kUTF8);
          auto r = timeSigNumBounds_.toFloat();
          float numX = r.getCentreX() - (tw / 2.0f);
          
          // Subtle highlight only when dragging
          if (isDraggingTimeSigNum_) {
              SkPaint hl;
              hl.setAntiAlias(true);
              hl.setColor(SkColorSetA(design::colors::ACCENT_PRIMARY, 30));
              canvas->drawRoundRect(SkRect::MakeXYWH(r.getX(), r.getY(), r.getWidth(), r.getHeight()), 4.0f, 4.0f, hl);
          }
          
          textPaint.setColor(design::colors::TEXT_PRIMARY);
          canvas->drawString(numStr.c_str(), numX, centerY, numFont, textPaint);
      }
      
      // Draw Slash
      {
          textPaint.setColor(design::colors::TEXT_SECONDARY);
          // Position slash centrally between Num and Den zones
          float slashX = (timeSigNumBounds_.getRight() + timeSigDenBounds_.getX()) / 2.0f - 4.0f;
          canvas->drawString("/", slashX, centerY, slashFont, textPaint);
      }
      
      // Draw Denominator (only highlight when dragging)
      {
          std::string denStr = std::to_string(timeSigDen_);
          float tw = numFont.measureText(denStr.c_str(), denStr.length(), SkTextEncoding::kUTF8);
          auto r = timeSigDenBounds_.toFloat();
          float denX = r.getCentreX() - (tw / 2.0f);
          
          // Subtle highlight only when dragging
          if (isDraggingTimeSigDen_) {
              SkPaint hl;
              hl.setAntiAlias(true);
              hl.setColor(SkColorSetA(design::colors::ACCENT_PRIMARY, 30));
              canvas->drawRoundRect(SkRect::MakeXYWH(r.getX(), r.getY(), r.getWidth(), r.getHeight()), 4.0f, 4.0f, hl);
          }
          
          textPaint.setColor(design::colors::TEXT_PRIMARY);
          canvas->drawString(denStr.c_str(), denX, centerY, numFont, textPaint);
      }
      
      // Draw Templates Dropdown Arrow
      {
          auto dropRect = timeSigTemplateBounds_.toFloat();
          
          // Draw small down arrow (▼)
          SkPath arrow;
          float arrowSize = 5.0f;
          float cx = dropRect.getCentreX();
          float cy = dropRect.getCentreY();
          arrow.moveTo(cx - arrowSize, cy - arrowSize/2);
          arrow.lineTo(cx + arrowSize, cy - arrowSize/2);
          arrow.lineTo(cx, cy + arrowSize/2);
          arrow.close();
          
          SkPaint arrowPaint;
          arrowPaint.setAntiAlias(true);
          arrowPaint.setColor(design::colors::TEXT_SECONDARY);
          arrowPaint.setStyle(SkPaint::kFill_Style);
          canvas->drawPath(arrow, arrowPaint);
      }
      
      // BPM Interaction Hint
      if (isDraggingBpm_) {
           SkPaint hl;
           hl.setColor(SkColorSetA(design::colors::ACCENT_PRIMARY, 30));
           auto r = bpmHitBounds_.toFloat();
           canvas->drawRoundRect(SkRect::MakeXYWH(r.getX(), r.getY(), r.getWidth(), r.getHeight()), 4.0f, 4.0f, hl);
      }
  }

  // 3. Transport Buttons
  // Play - Triangle
  if (playBtn_)
    drawTransportButton(canvas, *playBtn_, icons::Play(), isPlaying_,
                        design::colors::NEON_GREEN, playState_);
                       
  // Stop - PERFECT SQUARE (hover-only glow, no permanent active state)
  // NOTE: Pass 'false' for isActive - stop should never have a permanent glow,
  // only glow on hover. The old code used !isPlaying_ which was wrong.
  if (stopBtn_)
    drawTransportButton(canvas, *stopBtn_, icons::Stop(), false,
                        design::colors::TEXT_SECONDARY, stopState_);
                       
  // Record - Circle
  if (recordBtn_)
    drawTransportButton(canvas, *recordBtn_, icons::Record(),
                        isRecording_ || recordState_.isHovered,
                        design::colors::RED, recordState_);


  if (settingsBtn_)
    drawTransportButton(canvas, *settingsBtn_, icons::Settings(), false,
                        design::colors::TEXT_SECONDARY, settingsState_);


  // 5. Wingman AI - Partnership icon (human + digital handshake)
  // This toggles the right-side AI panel for creative suggestions
  if (wingmanBtn_)
    drawTransportButton(canvas, *wingmanBtn_, icons::Partnership(),
                        false, design::colors::NEON_PURPLE, wingmanState_);

  // 6. View Toggle (Left)
  if (viewToggleBtn_)
    drawTransportButton(canvas, *viewToggleBtn_, icons::ViewToggle(),
                        false, design::colors::TEXT_SECONDARY, viewToggleState_);

  // 6. CPU Meter (Updated visual - use smoothed value)
  drawMeter(canvas, cpuMeterBounds_, juce::jlimit(0.0f, 1.0f, smoothedCpu_ / 100.0f), "CPU");
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

void TransportBar::drawTransportButton(SkCanvas *canvas, GhostButton& btn,
                                       const SkPath &iconPath, bool isActive,
                                       uint32_t color,
                                       InteractionState &state,
                                       bool isFilled) {
  // Sync Interaction State from JUCE Component
  bool isHovered = btn.isMouseOver();
  bool isDown = btn.isDown();
  bool isFocused = btn.hasKeyboardFocus(true);
  
  // Update our animation state helper
  state.isHovered = isHovered;
  state.isPressed = isDown;
  state.isFocused = isFocused;
  
  // Get bounds from the component
  auto b = btn.getBounds();
  SkRect rect = SkRect::MakeXYWH((float)b.getX(), (float)b.getY(), (float)b.getWidth(), (float)b.getHeight());

  // Ableton-style shrink animation when pressed
  // Scale down to 90% when pressed, with smooth interpolation
  float pressScale = 1.0f - (0.10f * state.pressAmount);  // 10% shrink max
  
  // Apply scale transform around button center
  canvas->save();
  float cx = rect.centerX();
  float cy = rect.centerY();
  canvas->translate(cx, cy);
  canvas->scale(pressScale, pressScale);
  canvas->translate(-cx, -cy);

  // 1. Background / Pill (Only for Active or Hover)
  if (isActive) {
      // Active State: Subtle filled background with strong glow
      SkPaint activeBg;
      activeBg.setAntiAlias(true);
      activeBg.setColor(SkColorSetA(color, 40)); 
      canvas->drawRoundRect(rect, 6.0f, 6.0f, activeBg);
      
      // Add a border
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

  // Draw focus ring
  InteractionHelper::drawFocusRing(canvas, rect, state.focusAmount, 6.0f);
  
  // 2. Icon Rendering
  icons::IconStyle style;
  
  style.strokeWidth = 2.0f;
  style.filled = isFilled ? isActive : false; // Use parameter unless forced off
  
  // If explicitly NOT filled, force stroke mode even if active
  if (!isFilled) {
      style.filled = false;
  }
  
  if (isActive) {
      style.color = color;
      style.glowColor = color;
      style.glowRadius = (color == design::colors::NEON_GREEN) ? 0.0f : 15.0f;
  } else {
      float opacity = 0.7f + (0.3f * state.hoverAmount);
      style.color = design::withAlpha(SK_ColorWHITE, opacity);
      
      if (state.hoverAmount > 0.1f) {
          style.glowColor = SK_ColorWHITE;
          style.glowRadius = 5.0f * state.hoverAmount;
      }
  }

  // Draw scaled and centered icon
  // FIX: Increased from 0.40 to 0.48 for better visibility and premium feel
  float iconSize = rect.width() * 0.48f; 
  icons::drawIconCentered(canvas, iconPath, rect, iconSize, style);
  
  canvas->restore();  // Restore from scale transform
}

void TransportBar::drawMeter(SkCanvas *canvas,
                             const juce::Rectangle<int> &bounds, float value,
                             const char *label) {
  SkRect rect =
      SkRect::MakeXYWH((float)bounds.getX(), (float)bounds.getY(),
                       (float)bounds.getWidth(), (float)bounds.getHeight());

  juce::String valStr = juce::String((int)(value * 100)) + "% " + label;
  zenith::design::MeterRenderer::drawHorizontalMeter(canvas, rect, value, valStr);
}

void TransportBar::mouseDown(const juce::MouseEvent &e) {
  auto pos = e.getPosition();
  
  // DEBUG: Print hit zone info
  TB_DEBUG("=== TransportBar::mouseDown ===");
  TB_DEBUG("  Click pos: " << pos.x << ", " << pos.y);
  TB_DEBUG("  bpmHitBounds_: " << bpmHitBounds_.toString());
  TB_DEBUG("  timeSigHitBounds_: " << timeSigHitBounds_.toString());
  TB_DEBUG("  timeSigNumBounds_: " << timeSigNumBounds_.toString());
  TB_DEBUG("  timeSigDenBounds_: " << timeSigDenBounds_.toString());
  TB_DEBUG("  timeSigTemplateBounds_: " << timeSigTemplateBounds_.toString());
  TB_DEBUG("  lcdBounds_: " << lcdBounds_.toString());
  TB_DEBUG("  Contains checks:");
  TB_DEBUG("    bpmHitBounds_.contains: " << (bpmHitBounds_.contains(pos) ? "YES" : "no"));
  TB_DEBUG("    timeSigTemplateBounds_.contains: " << (timeSigTemplateBounds_.contains(pos) ? "YES" : "no"));
  TB_DEBUG("    timeSigDenBounds_.contains: " << (timeSigDenBounds_.contains(pos) ? "YES" : "no"));
  TB_DEBUG("    timeSigNumBounds_.contains: " << (timeSigNumBounds_.contains(pos) ? "YES" : "no"));
  
  // 1. LCD Interaction (BPM / TimeSig)
  if (bpmHitBounds_.contains(pos)) {
      TB_DEBUG("  -> BPM drag started");
      isDraggingBpm_ = true;
      dragStartValue_ = tempo_;
      dragStartPos_ = pos;
      return;
  }
  
  // Time Signature - CHECK DROPDOWN FIRST (rightmost zone)
  // Then check DEN, then NUM (left to right priority reversed to avoid overlap issues)
  
  // Templates dropdown arrow - show popup menu  
  // Templates dropdown arrow - show popup menu  
  if (timeSigTemplateBounds_.contains(e.getPosition())) {
      TB_DEBUG("TransportBar: Dropdown clicked");
      juce::PopupMenu menu;
      
      // Use standard JUCE PopupMenu::Item syntax
      menu.addItem(juce::PopupMenu::Item("4/4 (Common)").setTicked(timeSigNum_ == 4 && timeSigDen_ == 4).setAction([this] { 
          setTimeSignature(4, 4); 
          if (onTimeSignatureChanged) onTimeSignatureChanged(4, 4); 
      }));
      
      menu.addItem(juce::PopupMenu::Item("3/4 (Waltz)").setTicked(timeSigNum_ == 3 && timeSigDen_ == 4).setAction([this] { 
          setTimeSignature(3, 4); 
          if (onTimeSignatureChanged) onTimeSignatureChanged(3, 4); 
      }));
      
      menu.addItem(juce::PopupMenu::Item("6/8 (Compound)").setTicked(timeSigNum_ == 6 && timeSigDen_ == 8).setAction([this] { 
          setTimeSignature(6, 8); 
          if (onTimeSignatureChanged) onTimeSignatureChanged(6, 8); 
      }));
      
      menu.addItem(juce::PopupMenu::Item("2/4 (March)").setTicked(timeSigNum_ == 2 && timeSigDen_ == 4).setAction([this] { 
          setTimeSignature(2, 4); 
          if (onTimeSignatureChanged) onTimeSignatureChanged(2, 4); 
      }));
      
      menu.addItem(juce::PopupMenu::Item("5/4 (Take Five)").setTicked(timeSigNum_ == 5 && timeSigDen_ == 4).setAction([this] { 
          setTimeSignature(5, 4); 
          if (onTimeSignatureChanged) onTimeSignatureChanged(5, 4); 
      }));
      
      menu.addItem(juce::PopupMenu::Item("7/8").setTicked(timeSigNum_ == 7 && timeSigDen_ == 8).setAction([this] { 
          setTimeSignature(7, 8); 
          if (onTimeSignatureChanged) onTimeSignatureChanged(7, 8); 
      }));
      
      menu.addItem(juce::PopupMenu::Item("12/8 (Shuffle)").setTicked(timeSigNum_ == 12 && timeSigDen_ == 8).setAction([this] { 
          setTimeSignature(12, 8); 
          if (onTimeSignatureChanged) onTimeSignatureChanged(12, 8); 
      }));
      
      menu.addItem(juce::PopupMenu::Item("9/8").setTicked(timeSigNum_ == 9 && timeSigDen_ == 8).setAction([this] { 
          setTimeSignature(9, 8); 
          if (onTimeSignatureChanged) onTimeSignatureChanged(9, 8); 
      }));
      
      menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this).withMousePosition());
      return;
  }
  
  // Denominator drag zone
  if (timeSigDenBounds_.contains(pos)) {
      TB_DEBUG("  -> Denominator drag started");
      isDraggingTimeSigDen_ = true;
      isDraggingTimeSigNum_ = false;
      dragStartDen_ = timeSigDen_;
      dragStartPos_ = pos;
      return;
  }
  
  // Numerator drag zone
  if (timeSigNumBounds_.contains(pos)) {
      TB_DEBUG("  -> Numerator drag started");
      isDraggingTimeSigNum_ = true;
      isDraggingTimeSigDen_ = false;
      dragStartNum_ = timeSigNum_;
      dragStartPos_ = pos;
      return;
  }
  
  TB_DEBUG("  -> No time sig zone hit, checking transport buttons...");

  bool isRightClick = e.mods.isRightButtonDown();
  
  // Set pressed state
  playState_.isPressed = playButtonBounds_.contains(e.getPosition()) && !isRightClick;
  stopState_.isPressed = stopButtonBounds_.contains(e.getPosition()) && !isRightClick;
  recordState_.isPressed = recordButtonBounds_.contains(e.getPosition()) && !isRightClick;
  viewToggleState_.isPressed = viewToggleButtonBounds_.contains(e.getPosition()) && !isRightClick;
  wingmanState_.isPressed = wingmanButtonBounds_.contains(e.getPosition()) && !isRightClick;
  settingsState_.isPressed = settingsButtonBounds_.contains(pos) && !isRightClick;

  
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
    }
    
  } else if (recordButtonBounds_.contains(e.getPosition())) {
    // Record right click?
  } else if (viewToggleButtonBounds_.contains(e.getPosition()) && !isRightClick) {
    // Handled by onClick
  } else if (wingmanButtonBounds_.contains(e.getPosition()) && !isRightClick) {
    // Handled by onClick
  } else if (settingsButtonBounds_.contains(e.getPosition()) && !isRightClick) {
    // Handled by onClick
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
  
  // Dragging numerator (vertical)
  if (isDraggingTimeSigNum_) {
      int dy = dragStartPos_.y - e.getPosition().y;
      int numSteps = dy / 12;
      if (e.mods.isShiftDown()) numSteps = dy / 24; // Fine control
      int newNum = juce::jlimit(1, 64, dragStartNum_ + numSteps);
      
      if (newNum != timeSigNum_) {
          setTimeSignature(newNum, timeSigDen_);
          if (onTimeSignatureChanged) onTimeSignatureChanged(timeSigNum_, timeSigDen_);
      }
      return;
  }
  
  // Dragging denominator (vertical, 1-interval)
  if (isDraggingTimeSigDen_) {
      int dy = dragStartPos_.y - e.getPosition().y;
      int numSteps = dy / 12;
      if (e.mods.isShiftDown()) numSteps = dy / 24; // Fine control
      int newDen = juce::jlimit(1, 64, dragStartDen_ + numSteps);
      
      if (newDen != timeSigDen_) {
          setTimeSignature(timeSigNum_, newDen);
          if (onTimeSignatureChanged) onTimeSignatureChanged(timeSigNum_, timeSigDen_);
      }
      return;
  }
}

void TransportBar::mouseUp(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  isDraggingBpm_ = false;
  isDraggingTimeSigNum_ = false;
  isDraggingTimeSigDen_ = false;
  
  // Clear pressed states
  playState_.isPressed = false;
  stopState_.isPressed = false;
  recordState_.isPressed = false;
  viewToggleState_.isPressed = false;
  wingmanState_.isPressed = false;
  settingsState_.isPressed = false;

  requestRepaint();
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
                    viewToggleState_.isHovered || wingmanState_.isHovered || settingsState_.isHovered;
                    
  playState_.isHovered = false;
  stopState_.isHovered = false;
  recordState_.isHovered = false;
  viewToggleState_.isHovered = false;
  wingmanState_.isHovered = false;
  settingsState_.isHovered = false;

  
  if (wasHovered) requestRepaint();
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
  update(recordState_, recordButtonBounds_);
  update(viewToggleState_, viewToggleButtonBounds_);
  update(wingmanState_, wingmanButtonBounds_);
  update(settingsState_, settingsButtonBounds_);
  
  if (anyChanged) requestRepaint();

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
    else if (wingmanState_.isHovered)
        globalHelpCallback("Wingman AI", "Your creative partner. Get intelligent suggestions for mixing, arrangement, and sound design. Shortcut: Cmd+W.");
    else if (settingsState_.isHovered) 
        globalHelpCallback("Audio Settings", "Configure your sound card, buffer size, and MIDI hardware here.");

  }
  
  // Cursor feedback for editable areas
  if (bpmHitBounds_.contains(pos)) {
      setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
  } else if (timeSigNumBounds_.contains(pos) || timeSigDenBounds_.contains(pos)) {
      setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
  } else if (timeSigTemplateBounds_.contains(pos)) {
      setMouseCursor(juce::MouseCursor::PointingHandCursor);
  } else if (playButtonBounds_.contains(pos) ||
             stopButtonBounds_.contains(pos) ||
             recordButtonBounds_.contains(pos) ||
             viewToggleButtonBounds_.contains(pos) ||
             wingmanButtonBounds_.contains(pos) ||
             settingsButtonBounds_.contains(pos)) {
      setMouseCursor(juce::MouseCursor::PointingHandCursor);
  } else {
      setMouseCursor(juce::MouseCursor::NormalCursor);
  }
}

void TransportBar::mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel) {
    auto pos = e.getPosition();
    
    // BPM scroll
    if (bpmHitBounds_.contains(pos)) {
        double delta = wheel.deltaY * 5.0; // 5 BPM per scroll notch
        if (e.mods.isShiftDown()) delta *= 0.1; // Fine control
        
        double newTempo = juce::jlimit(1.0, 2000.0, tempo_ + delta);
        setTempo(newTempo);
        if (onTempoChanged) onTempoChanged(newTempo);
        return;
    }
    
    // Numerator scroll
    if (timeSigNumBounds_.contains(pos)) {
        int delta = wheel.deltaY > 0 ? 1 : -1;
        if (e.mods.isShiftDown()) delta *= 5; // Bigger steps with shift
        int newNum = juce::jlimit(1, 64, timeSigNum_ + delta);
        if (newNum != timeSigNum_) {
            setTimeSignature(newNum, timeSigDen_);
            if (onTimeSignatureChanged) onTimeSignatureChanged(timeSigNum_, timeSigDen_);
        }
        return;
    }
    
    // Denominator scroll (cycles through powers of 2)
    if (timeSigDenBounds_.contains(pos)) {
        static constexpr int validDenoms[] = {1, 2, 4, 8, 16, 32, 64};
        static constexpr int numDenoms = 7;
        
        int currentIdx = 3;
        for (int i = 0; i < numDenoms; ++i) {
            if (validDenoms[i] == timeSigDen_) {
                currentIdx = i;
                break;
            }
        }
        int newIdx = juce::jlimit(0, numDenoms - 1, currentIdx + (wheel.deltaY > 0 ? 1 : -1));
        int newDen = validDenoms[newIdx];
        if (newDen != timeSigDen_) {
            setTimeSignature(timeSigNum_, newDen);
            if (onTimeSignatureChanged) onTimeSignatureChanged(timeSigNum_, newDen);
        }
        return;
    }
}

// onAnimationTick moved to top of file

std::unique_ptr<juce::AccessibilityHandler>
TransportBar::createAccessibilityHandler() {
  // Return a group handler so it exposes children
  return std::make_unique<juce::AccessibilityHandler>(
      *this, juce::AccessibilityRole::group);
}

TransportBar::~TransportBar() {
  ZENITH_UNREGISTER_ANIMATION();
}


bool TransportBar::hitTest(int x, int y) {
    if (playButtonBounds_.contains(x, y)) return true;
    if (stopButtonBounds_.contains(x, y)) return true;
    if (recordButtonBounds_.contains(x, y)) return true;
    if (viewToggleButtonBounds_.contains(x, y)) return true;
    if (wingmanButtonBounds_.contains(x, y)) return true;
    if (settingsButtonBounds_.contains(x, y)) return true;

    
    // NEW: Allow interaction with LCD
    if (lcdBounds_.contains(x, y)) return true;
    
    return false;
}

juce::String TransportBar::getTooltip() {
    auto pos = getMouseXYRelative();

    if (playButtonBounds_.contains(pos)) return "Play (Space)";
    if (stopButtonBounds_.contains(pos)) return "Stop (Return)";
    if (recordButtonBounds_.contains(pos)) return "Record";
    if (viewToggleButtonBounds_.contains(pos)) return "Toggle View (Tab)";
    if (wingmanButtonBounds_.contains(pos)) return "Wingman AI (Cmd+W)";
    if (settingsButtonBounds_.contains(pos)) return "Audio Settings";

    if (bpmHitBounds_.contains(pos)) return "BPM: Drag to change";
    if (timeSigHitBounds_.contains(pos)) return "Time Signature";

    return {};
}

} // namespace zenith

#endif // ZENITH_USE_SKIA

void TransportBar::createButtons() {
    auto createBtn = [this](const juce::String& name, const juce::String& tooltip) {
        auto btn = std::make_unique<GhostButton>(name);
        setupButton(*btn, tooltip);
        return btn;
    };
    
    // Create Buttons
    rewindBtn_ = createBtn("Rewind", "Rewind");
    stopBtn_ = createBtn("Stop", "Stop Playback (Space)");
    playBtn_ = createBtn("Play", "Start Playback (Space)");
    recordBtn_ = createBtn("Record", "Record (R)");
    loopBtn_ = createBtn("Loop", "Toggle Loop (L)");
    
    viewToggleBtn_ = createBtn("ViewToggle", "Switch View (Tab)");
    wingmanBtn_ = createBtn("Wingman", "Wingman AI (Cmd+W)");
    settingsBtn_ = createBtn("Settings", "Audio Settings");
    
    // Assign Callbacks
    rewindBtn_->onClick = [this] { if (onRewind) onRewind(); };
    stopBtn_->onClick = [this] { if (onStopClicked) onStopClicked(); };
    playBtn_->onClick = [this] { if (onPlayClicked) onPlayClicked(); };
    recordBtn_->onClick = [this] { if (onRecordClicked) onRecordClicked(); };
    loopBtn_->onClick = [this] { if (onLoopToggled) onLoopToggled(); };
    
    viewToggleBtn_->onClick = [this] { if (onViewToggleClicked) onViewToggleClicked(); };
    wingmanBtn_->onClick = [this] { if (onWingmanClicked) onWingmanClicked(); };
    settingsBtn_->onClick = [this] { if (onSettingsClicked) onSettingsClicked(); };
}

void TransportBar::setupButton(GhostButton& btn, const juce::String& tooltip) {
    btn.setTooltip(tooltip);
    addAndMakeVisible(btn);
}

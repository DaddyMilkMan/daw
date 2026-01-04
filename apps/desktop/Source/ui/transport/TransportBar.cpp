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

  // Initialize Buttons
  createButtons();
  
  // Initialize Editors
  auto bpmEd = std::make_unique<TransportEditorLabel>("bpmLabel");
  bpmEd->setJustificationType(juce::Justification::centred);
  bpmEd->setColour(juce::Label::textColourId, juce::Colours::white);
  bpmEd->setColour(juce::Label::textWhenEditingColourId, juce::Colours::white);
  bpmEd->setColour(juce::Label::backgroundWhenEditingColourId, juce::Colours::transparentBlack);
  bpmEd->setColour(juce::Label::outlineWhenEditingColourId, juce::Colours::transparentBlack);
  bpmEd->setFont(juce::Font(juce::FontOptions("Inter", 32.0f, juce::Font::bold)));
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
  numEd->setFont(juce::Font(juce::FontOptions("Inter", 24.0f, juce::Font::bold)));
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
  
  // Step 2: Denominator editor
  auto denEd = std::make_unique<TransportEditorLabel>("timeSigDen");
  denEd->setJustificationType(juce::Justification::centred);
  denEd->setColour(juce::Label::textColourId, juce::Colours::white);
  denEd->setColour(juce::Label::textWhenEditingColourId, juce::Colours::white);
  denEd->setColour(juce::Label::backgroundWhenEditingColourId, juce::Colours::transparentBlack);
  denEd->setColour(juce::Label::outlineWhenEditingColourId, juce::Colours::transparentBlack);
  denEd->setFont(juce::Font(juce::FontOptions("Inter", 24.0f, juce::Font::bold)));
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
  
  ZENITH_REGISTER_ANIMATION(zenith::animation::Priority::High);
  
  // Initialize fonts immediately for cache updates
  monoFont_ = design::getMonoFont(18.0f, design::FontWeight::Medium);
  labelFont_ = design::getSkFont(14.0f, design::FontWeight::Regular);
  
  // Initial Cache Update
  updateBpmCache();
  updateTimeSigCache();
}

void TransportBar::createButtons() {
    auto createBtn = [&](const juce::String& name, const juce::String& tooltip) {
        auto btn = std::make_unique<GhostButton>(name);
        setupButton(*btn, tooltip);
        return btn;
    };
    
    // Create Buttons
    rewindBtn_ = createBtn("Rewind", "Return to Zero (Double-click stop)");
    stopBtn_ = createBtn("Stop", "Stop Playback (Space)");
    playBtn_ = createBtn("Play", "Start Playback (Space)");
    recordBtn_ = createBtn("Record", "Record (R)");
    loopBtn_ = createBtn("Loop", "Toggle Loop (L)");
    metroBtn_ = createBtn("Metronome", "Metronome Click");
    
    viewToggleBtn_ = createBtn("ViewToggle", "Switch View (Tab)");
    settingsBtn_ = createBtn("Settings", "Audio Settings");
    exportBtn_ = createBtn("Export", "Export Audio");
    
    // Assign Callbacks
    rewindBtn_->onClick = [this] { if (onRewindClicked) onRewindClicked(); };
    stopBtn_->onClick = [this] { if (onStopClicked) onStopClicked(); };
    playBtn_->onClick = [this] { if (onPlayClicked) onPlayClicked(); };
    recordBtn_->onClick = [this] { if (onRecordClicked) onRecordClicked(); };
    loopBtn_->onClick = [this] { if (onLoopToggled) onLoopToggled(); };
    metroBtn_->onClick = [this] { if (onMetronomeToggled) onMetronomeToggled(); };
    
    viewToggleBtn_->onClick = [this] { if (onViewToggleClicked) onViewToggleClicked(); };
    settingsBtn_->onClick = [this] { if (onSettingsClicked) onSettingsClicked(); };
    exportBtn_->onClick = [this] { if (onExportClicked) onExportClicked(); };
    
    // Special Right Click logic needed? 
    // JUCE Buttons handle standard clicks. Context menus can be attached to MouseDown/Up on components if needed,
    // but standard behavior is onClick.
}

void TransportBar::setupButton(GhostButton& btn, const juce::String& tooltip) {
    btn.setTooltip(tooltip);
    addAndMakeVisible(btn);
}

void TransportBar::visibilityChanged() {}

void TransportBar::resized() {
  using namespace juce;
  auto area = getLocalBounds();
  
  // Layout Constants
  constexpr int lcdWidth = 380;
  constexpr int buttonSize = 38;
  constexpr int smallButtonSize = 32;
  constexpr int spacing = 8;
  constexpr int groupSpacing = 24;
  
  // Manual Layout: LCD-Centered Design
  // The LCD is the anchor point in the center, with transport controls to the left
  // and utility buttons (settings, export) to the right, view toggle on far left.
  
  // 1. LCD is Anchor (centered in component)
  lcdBounds_ = Rectangle<int>(0, 0, lcdWidth, 42).withCentre(area.getCentre());
  
  // 2. Buttons Left of LCD
  int btnX = lcdBounds_.getX() - groupSpacing;
  auto layoutLeft = [&](Component* c) {
      btnX -= buttonSize;
      c->setBounds(btnX, area.getCentreY() - buttonSize/2, buttonSize, buttonSize);
      btnX -= spacing;
  };
  layoutLeft(metroBtn_.get());
  layoutLeft(loopBtn_.get());
  layoutLeft(recordBtn_.get());
  layoutLeft(playBtn_.get());
  layoutLeft(stopBtn_.get());
  layoutLeft(rewindBtn_.get());
  
  // 3. Buttons Right of LCD
  int toolX = lcdBounds_.getRight() + groupSpacing;
  auto layoutRight = [&](Component* c) {
      c->setBounds(toolX, area.getCentreY() - smallButtonSize/2, smallButtonSize, smallButtonSize);
      toolX += smallButtonSize + spacing;
  };
  layoutRight(settingsBtn_.get());
  layoutRight(exportBtn_.get());
  
  // CPU Meter
  cpuMeterBounds_ = Rectangle<int>(toolX + 10, area.getCentreY() - 6, 60, 12);
  
  // View Toggle (Far Left)
  viewToggleBtn_->setBounds(20, area.getCentreY() - smallButtonSize/2, smallButtonSize, smallButtonSize);

  // Update Hit Zones for LCD Interaction
  auto tempLcd = lcdBounds_;
  bpmHitBounds_ = tempLcd.removeFromLeft(170).reduced(4, 4);
  timeSigHitBounds_ = tempLcd.removeFromRight(170).reduced(4, 4);
  
  // Update Editors
  if (bpmLabel_) bpmLabel_->setBounds(bpmHitBounds_.expanded(4, 0));
  
  auto tsArea = timeSigHitBounds_;
  auto numArea = tsArea.removeFromLeft(tsArea.getWidth() / 2 - 10);
  auto denArea = tsArea.removeFromRight(tsArea.getWidth() - 10);
  if (timeSigNumLabel_) timeSigNumLabel_->setBounds(numArea.expanded(4, 0));
  if (timeSigDenLabel_) timeSigDenLabel_->setBounds(denArea.expanded(4, 0));

  // Update cached resources
  SkRect skBounds = SkRect::MakeWH((float)getWidth(), (float)getHeight());
  updateCachedPaints(skBounds);
  cachedBounds_ = skBounds;
}

void TransportBar::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());
  
  // 1. Main Background
  GlassmorphicPanel::Options barOpts;
  barOpts.style = GlassmorphicPanel::Style::Elevated;
  barOpts.cornerRadius = 0.0f;
  barOpts.customTintColor = SkColorSetA(design::colors::BG_DARKEST, 240);
  barOpts.useBackdropBlur = true;
  barOpts.drawShadow = true;
  GlassmorphicPanel::drawWithOptions(canvas, skBounds, barOpts);
  GlassmorphicPanel::drawDivider(canvas, 0, skBounds.bottom(), skBounds.width());
  
  // 2. LCD Display
  {
      SkRect lcdRect = SkRect::MakeXYWH(lcdBounds_.getX(), lcdBounds_.getY(), lcdBounds_.getWidth(), lcdBounds_.getHeight());
      
      GlassmorphicPanel::Options lcdOpts;
      lcdOpts.style = GlassmorphicPanel::Style::Subtle;
      lcdOpts.cornerRadius = 6.0f;
      lcdOpts.customTintColor = SkColorSetA(SK_ColorBLACK, 150);
      lcdOpts.drawTopHighlight = false;
      lcdOpts.drawShadow = false;
      
      GlassmorphicPanel::drawWithOptions(canvas, lcdRect, lcdOpts);
      
      SkPaint insetPaint;
      insetPaint.setStyle(SkPaint::kStroke_Style);
      insetPaint.setStrokeWidth(1.0f);
      insetPaint.setColor(SkColorSetA(design::colors::ACCENT_PRIMARY, 30));
      canvas->drawRoundRect(lcdRect, 6.0f, 6.0f, insetPaint);
      
      // Draw Cached Text
      SkPaint textPaint;
      textPaint.setAntiAlias(true);
      
      float centerY = lcdRect.centerY() + 8.0f; // Baseline approx

      // BPM
      if (!bpmLabel_ || !bpmLabel_->isVisible()) {
          auto bpmCenter = bpmHitBounds_.getCentre();
          float bpmStartX = bpmCenter.getX() - (cachedBpm_.width / 2.0f);
          
          textPaint.setColor(design::colors::TEXT_PRIMARY);
          canvas->drawString(cachedBpm_.text.c_str(), bpmStartX, centerY, monoFont_, textPaint);
          
          textPaint.setColor(design::colors::TEXT_SECONDARY);
          canvas->drawString("BPM", bpmStartX + cachedBpm_.xOffset, centerY, labelFont_, textPaint);
      }
      
      // Time Sig
      {
          auto timeCenter = timeSigHitBounds_.getCentre();
          float sigStartX = timeCenter.getX() - (cachedTimeSig_.width / 2.0f);
          
          textPaint.setColor(design::colors::TEXT_PRIMARY);
          canvas->drawString(cachedTimeSig_.text.c_str(), sigStartX, centerY, monoFont_, textPaint);
          
          textPaint.setColor(design::colors::TEXT_SECONDARY);
          canvas->drawString("TIME", sigStartX + cachedTimeSig_.xOffset, centerY - 6.0f, labelFont_, textPaint);
      }
      
      // Interaction Hints
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

  // 3. Transport Buttons (Driven by Component State)
  drawTransportButton(canvas, *rewindBtn_, icons::Rewind(), false, design::colors::TEXT_SECONDARY, rewindState_);
  drawTransportButton(canvas, *stopBtn_, icons::Stop(), !isPlaying_, design::colors::NEON_RED, stopState_);
  drawTransportButton(canvas, *playBtn_, icons::Play(), isPlaying_, design::colors::NEON_GREEN, playState_);
  drawTransportButton(canvas, *recordBtn_, icons::Record(), isRecording_, design::colors::NEON_RED, recordState_);
  drawTransportButton(canvas, *loopBtn_, icons::Loop(), isLooping_, design::colors::ORANGE, loopState_);
  drawTransportButton(canvas, *metroBtn_, icons::Metronome(), isMetronomeOn_, design::colors::NEON_PURPLE, metroState_);

  // 4. Tools
  drawTransportButton(canvas, *settingsBtn_, icons::Settings(), false, design::colors::TEXT_SECONDARY, settingsState_);
  drawTransportButton(canvas, *exportBtn_, icons::Download(), false, design::colors::TEXT_SECONDARY, exportState_);

  // 5. View Toggle
  drawTransportButton(canvas, *viewToggleBtn_, icons::ViewToggle(), false, design::colors::TEXT_SECONDARY, viewToggleState_);

  // 6. CPU Meter
  drawMeter(canvas, cpuMeterBounds_, cpuUsage_ / 100.0f, "CPU");
}

void TransportBar::updateCachedPaints(const SkRect &bounds) {
  bgPaint_.setAntiAlias(true);
  bgPaint_.setStyle(SkPaint::kFill_Style);
  // Shader creation is expensive? Only do it on resize/init.
  SkPoint pts[2] = {{0, 0}, {0, bounds.height()}};
  SkColor colors[2] = {design::withAlpha(design::colors::BG_01, 0.94f),
                       design::withAlpha(design::colors::BG_00, 0.94f)};
  bgPaint_.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2, SkTileMode::kClamp));

  borderPaint_.setAntiAlias(true);
  borderPaint_.setStyle(SkPaint::kStroke_Style);
  borderPaint_.setStrokeWidth(1.0f);
  borderPaint_.setColor(design::withAlpha(design::colors::ACCENT_PRIMARY, 0.2f));

  // Initialize Fonts once
  monoFont_ = design::getMonoFont(18.0f, design::FontWeight::Medium);
  labelFont_ = design::getSkFont(14.0f, design::FontWeight::Regular);
}

void TransportBar::drawTransportButton(SkCanvas *canvas, GhostButton& btn,
                                       const SkPath &iconPath, bool isActive,
                                       uint32_t color,
                                       InteractionState &state) {
  // Sync Interaction State from JUCE Component
  bool isHovered = btn.isMouseOver();
  bool isDown = btn.isDown();
  
  // Update our animation state helper
  state.isHovered = isHovered;
  state.isPressed = isDown;
  
  // Get bounds from the component
  auto b = btn.getBounds();
  SkRect rect = SkRect::MakeXYWH(b.getX(), b.getY(), b.getWidth(), b.getHeight());

  if (isActive) {
      SkPaint activeBg;
      activeBg.setAntiAlias(true);
      activeBg.setColor(SkColorSetA(color, 40)); 
      canvas->drawRoundRect(rect, 6.0f, 6.0f, activeBg);
      
      SkPaint border;
      border.setStyle(SkPaint::kStroke_Style);
      border.setStrokeWidth(1.0f);
      border.setColor(SkColorSetA(color, 80));
      canvas->drawRoundRect(rect, 6.0f, 6.0f, border);
      
  } else if (state.hoverAmount > 0.01f) {
      SkPaint hoverBg;
      hoverBg.setAntiAlias(true);
      hoverBg.setColor(SkColorSetA(SK_ColorWHITE, (uint8_t)(20 * state.hoverAmount)));
      canvas->drawRoundRect(rect, 6.0f, 6.0f, hoverBg);
  }
  
  icons::IconStyle style;
  style.strokeWidth = 2.0f;
  style.filled = isActive;
  
  if (isActive) {
      style.color = color;
      style.glowColor = color;
      style.glowRadius = 15.0f;
  } else {
      float opacity = 0.7f + (0.3f * state.hoverAmount);
      style.color = design::withAlpha(SK_ColorWHITE, opacity);
      
      if (state.hoverAmount > 0.1f) {
          style.glowColor = SK_ColorWHITE;
          style.glowRadius = 5.0f * state.hoverAmount;
      }
  }

  float iconSize = rect.width() * 0.45f;
  icons::drawIconCentered(canvas, iconPath, rect, iconSize, style);
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

// === Caching & Setters ===

void TransportBar::updateBpmCache() {
    juce::String bpmStr = juce::String(tempo_, 1);
    cachedBpm_.text = bpmStr.toStdString();
    
    float bpmValueWidth = monoFont_.measureText(cachedBpm_.text.c_str(), cachedBpm_.text.length(), SkTextEncoding::kUTF8);
    float bpmLabelWidth = labelFont_.measureText("BPM", 3, SkTextEncoding::kUTF8);
    float spacing = 8.0f;
    
    cachedBpm_.width = bpmValueWidth + spacing + bpmLabelWidth;
    cachedBpm_.xOffset = bpmValueWidth + spacing;
}

void TransportBar::updateTimeSigCache() {
    juce::String sigStr = juce::String(timeSigNum_) + " / " + juce::String(timeSigDen_);
    cachedTimeSig_.text = sigStr.toStdString();
    
    float sigValueWidth = monoFont_.measureText(cachedTimeSig_.text.c_str(), cachedTimeSig_.text.length(), SkTextEncoding::kUTF8);
    float sigLabelWidth = labelFont_.measureText("TIME", 4, SkTextEncoding::kUTF8);
    float spacing = 8.0f;
    
    cachedTimeSig_.width = sigValueWidth + spacing + sigLabelWidth;
    cachedTimeSig_.xOffset = sigValueWidth + spacing;
}

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
void TransportBar::setMetronomeEnabled(bool enabled) {
    if (isMetronomeOn_ != enabled) {
        isMetronomeOn_ = enabled;
        repaint();
    }
}
void TransportBar::setTempo(double bpm) {
    if (std::abs(tempo_ - bpm) > 0.01) {
        tempo_ = bpm;
        updateBpmCache();
        repaint();
    }
}
void TransportBar::setCPU(float percent) {
    cpuUsage_ = percent;
    repaint();
}
void TransportBar::setPosition(double seconds) {
    position_ = seconds;
    repaint();
}
void TransportBar::setTimeSignature(int num, int den) {
    if (timeSigNum_ != num || timeSigDen_ != den) {
        timeSigNum_ = num;
        timeSigDen_ = den;
        updateTimeSigCache();
        repaint();
    }
}


// === Mouse Interaction for LCD (Custom) ===

void TransportBar::mouseDown(const juce::MouseEvent &e) {
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
}

void TransportBar::mouseDrag(const juce::MouseEvent &e) {
  if (isDraggingBpm_) {
      int dy = dragStartPos_.y - e.getPosition().y;
      double change = dy * 0.5;
      if (e.mods.isShiftDown()) change *= 0.1;
      
      double newTempo = juce::jlimit(1.0, 2000.0, dragStartValue_ + change);
      setTempo(newTempo);
      if (onTempoChanged) onTempoChanged(newTempo);
      return;
  }
  
  if (isDraggingTimeSig_) {
      int dy = dragStartPos_.y - e.getPosition().y;
      int steps = dy / 15;
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
        if (timeSigNumLabel_) {
            editingTimeSigNum_ = true;
            timeSigNumLabel_->setText(juce::String(timeSigNum_), juce::dontSendNotification);
            timeSigNumLabel_->setVisible(true);
            timeSigNumLabel_->showEditor();
        }
    }
}

void TransportBar::mouseMove(const juce::MouseEvent &e) {
    // LCD Hover effects handled in draw via hit test? 
    // Or we can just repaint. Button hovers handled by GhostButtons.
}
void TransportBar::mouseEnter(const juce::MouseEvent &e) {}
void TransportBar::mouseExit(const juce::MouseEvent &e) {}

void TransportBar::onAnimationTick(float deltaMs) {
  SkiaComponent::updateInternalAnimations(deltaMs);

  float dt = deltaMs / 1000.0f;
  
  // Update animations driven by component state
  playState_.update(dt);
  stopState_.update(dt);
  recordState_.update(dt);
  loopState_.update(dt);
  rewindState_.update(dt);
  metroState_.update(dt);
  viewToggleState_.update(dt);
  exportState_.update(dt);
  settingsState_.update(dt);

  // If animating, request repaint
  // (Optimization: Check if any actually changed)
  repaint();
}

std::unique_ptr<juce::AccessibilityHandler>
TransportBar::createAccessibilityHandler() {
  return std::make_unique<juce::AccessibilityHandler>(
      *this, juce::AccessibilityRole::group);
}

TransportBar::~TransportBar() {
  ZENITH_UNREGISTER_ANIMATION();
}



} // namespace zenith

#endif // ZENITH_USE_SKIA

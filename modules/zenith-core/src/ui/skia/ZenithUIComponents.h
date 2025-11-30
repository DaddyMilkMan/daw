/*
  ==============================================================================

    ZenithUIComponents.h
    Created: 2025-11-27
    Author:  Zenith DAW

    Custom Skia-rendered components for the Zenith PolySynth UI.
    Style: Neon Noir / Glassmorphism.
    Base class for Zenith controls with common hover/value logic
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>
#include "SkiaComponent.h"
#include "../../instruments/ZenithPolySynth.h"

// Skia headers
#ifdef ZENITH_ENABLE_SKIA
#include <include/core/SkCanvas.h>
#include <include/core/SkPaint.h>
#include <include/core/SkPath.h>
#include <include/core/SkRRect.h>
#include <include/core/SkRect.h>
#include <include/core/SkFont.h>
#include <include/core/SkColor.h>
#include <include/utils/SkTextUtils.h>
#include <include/effects/SkGradientShader.h>
#include <include/core/SkMaskFilter.h>
#include <include/core/SkBlurTypes.h>
#include <include/core/SkShader.h>
#include <include/core/SkString.h>
#endif

namespace zenith {

// Alias for compatibility with UI components expecting SkiaCanvasComponent
using SkiaCanvasComponent = SkiaComponent;

class ZenithControl : public SkiaComponent,
                      public juce::AudioProcessorParameter::Listener {
public:
  ZenithControl(const juce::String& name) : name_(name) {}
  ~ZenithControl() override {
    if (parameter_) parameter_->removeListener(this);
  }

  void setParameter(juce::RangedAudioParameter* param) {
    if (parameter_) parameter_->removeListener(this);
    parameter_ = param;
    if (parameter_) {
      range_ = parameter_->getNormalisableRange();
      float initialValue = parameter_->convertFrom0to1(parameter_->getValue());
      cachedValue_.store(initialValue, std::memory_order_release);
      parameter_->addListener(this);
    }
  }

  void setValue(float value, bool sendNotification) {
    float currentValue = cachedValue_.load(std::memory_order_acquire);
    if (currentValue != value) {
      cachedValue_.store(value, std::memory_order_release);
      if (sendNotification && parameter_) {
        parameter_->beginChangeGesture();
        parameter_->setValueNotifyingHost(parameter_->convertTo0to1(value));
        parameter_->endChangeGesture();
      }
      repaint();
    }
  }

  float getValue() const { return cachedValue_.load(std::memory_order_acquire); }

  // ParameterListener - Thread-safe parameter update
  void parameterValueChanged(int parameterIndex, float newValue) override {
    // Store normalized value directly from audio thread (thread-safe)
    if (parameter_) {
      float convertedValue = parameter_->convertFrom0to1(newValue);
      cachedValue_.store(convertedValue, std::memory_order_release);
    }
    // Trigger repaint on message thread
    juce::MessageManager::callAsync([this]() {
      repaint();
    });
  }

  void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {}

protected:
  juce::String name_;
  juce::RangedAudioParameter* parameter_ = nullptr;
  juce::NormalisableRange<float> range_;
  std::atomic<float> cachedValue_{0.0f};  // Thread-safe cached value (fixes race condition)
  float value_ = 0.5f;  // Direct value when not using parameter
  float defaultValue_ = 0.5f;
  bool isHovered_ = false;

  // Colors
#ifdef ZENITH_ENABLE_SKIA
  SkColor accentColor_ = SkColorSetRGB(0, 255, 255); // Cyan default
#else
  uint32_t accentColor_ = 0xFF00FFFF; // Cyan default (ARGB format)
#endif
  juce::String tooltipText_;
  juce::String textSuffix_;

  void mouseEnter(const juce::MouseEvent&) override { 
      isHovered_ = true; 
      if (onHoverStateChanged) onHoverStateChanged(this);
      repaint(); 
  }
  void mouseExit(const juce::MouseEvent&) override { 
      isHovered_ = false; 
      if (onHoverStateChanged) onHoverStateChanged(nullptr);
      repaint(); 
  }

public:
  void setTooltip(const juce::String& text) { tooltipText_ = text; }
  juce::String getTooltip() const { return tooltipText_; }
  std::function<void(ZenithControl*)> onHoverStateChanged;
};

//==============================================================================
/**
    Glass Overlay for Learning Mode
*/
class ZenithTooltipOverlay : public SkiaComponent {
public:
  void setTarget(ZenithControl* target) {
    if (target_ != target) {
        target_ = target;
        repaint();
    }
  }

  void drawSkia(SkCanvas* canvas) override {
#ifdef ZENITH_ENABLE_SKIA
    if (!target_) return;

    auto bounds = getLocalBounds().toFloat();

    // 1. Dim Background with Cutout
    SkPaint paint;
    paint.setColor(SkColorSetARGB(200, 10, 10, 15)); // Dark blue-black overlay
    paint.setAntiAlias(true);

    SkPath path;
    path.addRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()));

    // Get target bounds relative to parent (assuming overlay is sibling or covers parent)
    // We need target's bounds relative to THIS component.
    // If both are children of ZenithPolySynthUI, and overlay is full size at (0,0):
    auto targetBounds = target_->getBounds().toFloat();
    SkRect targetRect = SkRect::MakeXYWH(targetBounds.getX(), targetBounds.getY(), targetBounds.getWidth(), targetBounds.getHeight());

    // Add cutout
    path.addRect(targetRect, SkPathDirection::kCCW);
    canvas->drawPath(path, paint);

    // 2. Glow around target
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(2.0f);
    paint.setColor(SkColorSetRGB(0, 255, 255));
    paint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 8.0f));
    canvas->drawRect(targetRect, paint);

    // Sharp border
    paint.setMaskFilter(nullptr);
    paint.setStrokeWidth(1.0f);
    paint.setColor(SkColorSetARGB(150, 255, 255, 255));
    canvas->drawRect(targetRect, paint);

    // 3. Tooltip Card
    drawTooltipCard(canvas, targetRect);
#else
    juce::ignoreUnused(canvas);
#endif
  }
  
  // Pass through mouse events so we don't block interaction?
  // If overlay is visible, we might want to block interaction with other controls?
  // "Learning Mode" usually implies inspection.
  // But if we want to click through, we need `setInterceptsMouseClicks(false, false)`.
  // But then we can't hover?
  // Wait, if overlay is on top, it will steal hover.
  // So overlay should be `hitTest` false?
  // If `hitTest` is false, `mouseEnter` on `ZenithControl` (underneath) will still fire.
  // Yes. So `ZenithTooltipOverlay` should be transparent to mouse events.
  
  bool hitTest(int x, int y) override { return false; }

private:
  ZenithControl* target_ = nullptr;
  
#ifdef ZENITH_ENABLE_SKIA
  void drawTooltipCard(SkCanvas* canvas, const SkRect& targetRect) {
      SkPaint paint;
      paint.setAntiAlias(true);

      // Position card to the right or bottom
      float cardX = targetRect.right() + 20.0f;
      float cardY = targetRect.top();
      float cardW = 200.0f;
      float cardH = 100.0f;

      // Check bounds
      if (cardX + cardW > getWidth()) {
          cardX = targetRect.left() - cardW - 20.0f; // Flip to left
      }

      SkRect cardRect = SkRect::MakeXYWH(cardX, cardY, cardW, cardH);
      SkRRect rrect = SkRRect::MakeRectXY(cardRect, 8.0f, 8.0f);

      // Card Background (Glass)
      paint.setStyle(SkPaint::kFill_Style);
      paint.setColor(SkColorSetARGB(220, 30, 30, 40));
      canvas->drawRRect(rrect, paint);

      // Card Border
      paint.setStyle(SkPaint::kStroke_Style);
      paint.setStrokeWidth(1.0f);
      paint.setColor(SkColorSetARGB(100, 255, 255, 255));
      canvas->drawRRect(rrect, paint);

      // Text
      if (target_) {
          SkFont font;
          font.setSize(14.0f);
          paint.setStyle(SkPaint::kFill_Style);
          paint.setColor(SK_ColorWHITE);

          juce::String text = target_->getTooltip();
          if (text.isEmpty()) text = "No description available.";

          // Simple text wrapping (very basic)
          std::string str = text.toStdString();
          canvas->drawSimpleText(str.c_str(), str.length(), SkTextEncoding::kUTF8,
                                 cardX + 10.0f, cardY + 25.0f, font, paint);
      }
  }
#endif
};

//==============================================================================
/**
    Neon Glowing Knob
*/
class ZenithKnob : public ZenithControl {
public:
  // Default constructor for compatibility
  ZenithKnob() : ZenithControl("") {
#ifdef ZENITH_ENABLE_SKIA
    accentColor_ = SkColorSetRGB(0, 255, 255);
#else
    accentColor_ = 0xFF00FFFF;
#endif
  }

#ifdef ZENITH_ENABLE_SKIA
  explicit ZenithKnob(const juce::String& name, SkColor color = SkColorSetRGB(0, 255, 255))
#else
  explicit ZenithKnob(const juce::String& name, uint32_t color = 0xFF00FFFF)
#endif
    : ZenithControl(name) {
    accentColor_ = color;
  }

  // API compatibility methods
  void setRange(double newMin, double newMax, double /*interval*/ = 0.0) {
    range_ = juce::NormalisableRange<float>(static_cast<float>(newMin), static_cast<float>(newMax));
  }

  void setDefaultValue(double defaultVal) {
    defaultValue_ = static_cast<float>(defaultVal);
    value_ = defaultValue_;
  }

  void setLabel(const juce::String& newLabel) {
    name_ = newLabel;
  }

  std::function<void()> onValueChange;

  void drawSkia(SkCanvas* canvas) override {
#ifdef ZENITH_ENABLE_SKIA
    auto bounds = getLocalBounds().toFloat();
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    float radius = std::min(bounds.getWidth(), bounds.getHeight()) * 0.4f;

    SkPaint paint;
    paint.setAntiAlias(true);

    // 1. Background Track (Dark Grey Ring)
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(4.0f);
    paint.setColor(SkColorSetARGB(50, 255, 255, 255));

    SkRect arcRect = SkRect::MakeXYWH(cx - radius, cy - radius, radius * 2, radius * 2);
    canvas->drawArc(arcRect, 135.0f, 270.0f, false, paint);

    // 2. Value Arc (Neon Glow)
    float normValue = 0.0f;
    if (parameter_) normValue = parameter_->getValue();
    else normValue = juce::jmap(value_, range_.start, range_.end, 0.0f, 1.0f);

    float sweepAngle = normValue * 270.0f;

    paint.setColor(accentColor_);
    paint.setStrokeWidth(4.0f);

    // Add glow
    if (isHovered_) {
        paint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 4.0f));
        canvas->drawArc(arcRect, 135.0f, sweepAngle, false, paint);
        paint.setMaskFilter(nullptr); // Reset for sharp line
    }

    canvas->drawArc(arcRect, 135.0f, sweepAngle, false, paint);

    // 3. Label
    SkFont font;
    font.setSize(12.0f);
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(SkColorSetARGB(200, 255, 255, 255));

    std::string labelStr = name_.toStdString();
    canvas->drawSimpleText(labelStr.c_str(), labelStr.length(), SkTextEncoding::kUTF8,
                           cx - (labelStr.length() * 3.0f), cy + radius + 15.0f, font, paint);
#else
    juce::ignoreUnused(canvas);
#endif
  }

  void mouseDrag(const juce::MouseEvent& e) override {
    if (parameter_) {
      float delta = (e.getDistanceFromDragStartY() - e.getDistanceFromDragStartX()) * 0.005f;
      float current = parameter_->getValue();
      float next = juce::jlimit(0.0f, 1.0f, current + delta);
      parameter_->setValueNotifyingHost(next);
    }
  }
};

//==============================================================================
/**
    Minimalist Vertical Slider
*/
class ZenithSlider : public ZenithControl {
public:
  // Default constructor for compatibility
  ZenithSlider() : ZenithControl("") {
#ifdef ZENITH_ENABLE_SKIA
    accentColor_ = SkColorSetRGB(255, 0, 255);
#else
    accentColor_ = 0xFFFF00FF;
#endif
  }

#ifdef ZENITH_ENABLE_SKIA
  explicit ZenithSlider(const juce::String& name, SkColor color = SkColorSetRGB(255, 0, 255))
#else
  explicit ZenithSlider(const juce::String& name, uint32_t color = 0xFFFF00FF)
#endif
    : ZenithControl(name) {
    accentColor_ = color;
  }

  // API compatibility methods
  void setRange(double newMin, double newMax, double /*interval*/ = 0.0) {
    range_ = juce::NormalisableRange<float>(static_cast<float>(newMin), static_cast<float>(newMax));
  }

  void setTextSuffix(const juce::String& suffix) {
    textSuffix_ = suffix;
  }

  std::function<void()> onValueChange;

  void drawSkia(SkCanvas* canvas) override {
#ifdef ZENITH_ENABLE_SKIA
    auto bounds = getLocalBounds().toFloat();
    float w = bounds.getWidth();
    float h = bounds.getHeight();

    SkPaint paint;
    paint.setAntiAlias(true);

    // 1. Track
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(2.0f);
    paint.setColor(SkColorSetARGB(50, 255, 255, 255));
    canvas->drawLine(w/2, h * 0.1f, w/2, h * 0.9f, paint);

    // 2. Handle/Fill
    float normValue = 0.0f;
    if (parameter_) normValue = parameter_->getValue();
    else normValue = juce::jmap(value_, range_.start, range_.end, 0.0f, 1.0f);

    float yPos = h * 0.9f - (normValue * (h * 0.8f));

    paint.setColor(accentColor_);
    paint.setStrokeWidth(4.0f);
    if (isHovered_) {
        paint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 4.0f));
        canvas->drawLine(w/2, h * 0.9f, w/2, yPos, paint);
        paint.setMaskFilter(nullptr);
    }
    canvas->drawLine(w/2, h * 0.9f, w/2, yPos, paint);

    // Handle Circle
    paint.setStyle(SkPaint::kFill_Style);
    canvas->drawCircle(w/2, yPos, 4.0f, paint);
#else
    juce::ignoreUnused(canvas);
#endif
  }

  void mouseDrag(const juce::MouseEvent& e) override {
    if (parameter_) {
      float delta = -e.getDistanceFromDragStartY() * 0.005f; // Vertical drag
      float current = parameter_->getValue();
      float next = juce::jlimit(0.0f, 1.0f, current + delta);
      parameter_->setValueNotifyingHost(next);
    }
  }
};

//==============================================================================
/**
    Glassy Button
*/
class ZenithButton : public SkiaComponent {
public:
  enum ButtonStyle { Primary, Secondary, Danger, Warning, Success };

  // Default constructor for compatibility
  ZenithButton() : text_(""), onClick_(nullptr) {}

  // Constructor with text and callback
  explicit ZenithButton(const juce::String& text, std::function<void()> onClick = nullptr)
    : text_(text), onClick_(onClick) {}

  // API compatibility methods
  void setButtonText(const juce::String& text) { text_ = text; repaint(); }
  void setToggleable(bool toggleable) { isToggleable_ = toggleable; }
  void setToggleState(bool state, bool sendNotification = true) {
    isToggled_ = state;
    repaint();
    if (sendNotification && onClick) onClick();
  }
  bool getToggleState() const { return isToggled_; }

  // Public onClick for compatibility
  std::function<void()> onClick;

  void drawSkia(SkCanvas* canvas) override {
#ifdef ZENITH_ENABLE_SKIA
    auto bounds = getLocalBounds().toFloat();
    SkRect rect = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());
    SkRRect rrect = SkRRect::MakeRectXY(rect, 4.0f, 4.0f);

    SkPaint paint;
    paint.setAntiAlias(true);

    // Background - different colors based on toggle state and style
    if (isToggleable_ && isToggled_) {
      // Toggled state - use style color
      switch (currentStyle_) {
        case Danger: paint.setColor(SkColorSetARGB(180, 255, 59, 48)); break; // Red
        case Warning: paint.setColor(SkColorSetARGB(180, 255, 204, 0)); break; // Yellow
        case Success: paint.setColor(SkColorSetARGB(180, 52, 199, 89)); break; // Green
        default: paint.setColor(SkColorSetARGB(80, 255, 255, 255)); break;
      }
    } else if (isDown_) {
      paint.setColor(SkColorSetARGB(50, 255, 255, 255));
    } else if (isHovered_) {
      paint.setColor(SkColorSetARGB(30, 255, 255, 255));
    } else {
      paint.setColor(SkColorSetARGB(10, 255, 255, 255));
    }

    canvas->drawRRect(rrect, paint);

    // Border
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(1.0f);
    paint.setColor(SkColorSetARGB(100, 255, 255, 255));
    canvas->drawRRect(rrect, paint);

    // Text
    SkFont font;
    font.setSize(14.0f);
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(SK_ColorWHITE);

    std::string str = text_.toStdString();
    float textWidth = font.measureText(str.c_str(), str.length(), SkTextEncoding::kUTF8);
    canvas->drawSimpleText(str.c_str(), str.length(), SkTextEncoding::kUTF8,
                           bounds.getCentreX() - textWidth/2, bounds.getCentreY() + 5.0f, font, paint);
#else
    juce::ignoreUnused(canvas);
#endif
  }

  void mouseEnter(const juce::MouseEvent&) override { isHovered_ = true; repaint(); }
  void mouseExit(const juce::MouseEvent&) override { isHovered_ = false; repaint(); }
  void mouseDown(const juce::MouseEvent&) override { isDown_ = true; repaint(); }
  void mouseUp(const juce::MouseEvent&) override {
    isDown_ = false;
    if (isToggleable_) {
      isToggled_ = !isToggled_;
    }
    repaint();
    if (onClick) onClick();       // Use public onClick
    else if (onClick_) onClick_(); // Fallback to private onClick_
  }

  void setButtonStyle(ButtonStyle style) { currentStyle_ = style; repaint(); }
  ButtonStyle getButtonStyle() const { return currentStyle_; }

private:
  juce::String text_;
  std::function<void()> onClick_;
  bool isHovered_ = false;
  bool isDown_ = false;
  bool isToggleable_ = false;
  bool isToggled_ = false;
  ButtonStyle currentStyle_ = Secondary;
};
//==============================================================================
class ZenithModMatrix : public SkiaComponent {
public:
  ZenithModMatrix(ZenithPolySynthProcessor& p) : processor_(p) {}

  void drawSkia(SkCanvas* const canvas) override {
#ifdef ZENITH_ENABLE_SKIA
    auto bounds = getLocalBounds().toFloat();
    SkPaint paint;
    paint.setAntiAlias(true);

    // Background
    paint.setColor(SkColorSetARGB(30, 0, 0, 0));
    canvas->drawRect(SkRect::MakeXYWH(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight()), paint);

    // Grid Dimensions
    int numRows = (int)ModulationSource::NumSources - 1;
    int numCols = (int)ModulationDestination::NumDestinations - 1;

    // Bounds check: prevent division by zero if mod matrix is empty
    if (numRows <= 0 || numCols <= 0) {
      // Draw empty state message
      paint.setStyle(SkPaint::kFill_Style);
      paint.setColor(SkColorSetARGB(150, 255, 255, 255));
      SkFont font;
      font.setSize(14.0f);
      canvas->drawString("No modulation sources configured", 10, bounds.getCentreY(), font, paint);
      return;
    }

    float cellWidth = bounds.getWidth() / (float)numCols;
    float cellHeight = bounds.getHeight() / (float)numRows;

    // Draw Cells
    for (int r = 0; r < numRows; ++r) {
      for (int c = 0; c < numCols; ++c) {
        ModulationSource src = (ModulationSource)(r + 1);
        ModulationDestination dst = (ModulationDestination)(c + 1);

        float val = processor_.getModulationMatrix(static_cast<int>(src), static_cast<int>(dst));

        float x = c * cellWidth;
        float y = r * cellHeight;
        float cx = x + cellWidth * 0.5f;
        float cy = y + cellHeight * 0.5f;
        float radius = std::min(cellWidth, cellHeight) * 0.3f;

        // Faint dot for empty
        paint.setColor(SkColorSetARGB(50, 255, 255, 255));
        canvas->drawCircle(cx, cy, 2.0f, paint);

        // Active value
        if (std::abs(val) > 0.01f) {
           SkColor color = val > 0 ? SkColorSetRGB(0, 255, 100) : SkColorSetRGB(255, 50, 50);
           paint.setColor(color);

           // Draw arc or filled circle based on amount
           float amount = std::abs(val);
           canvas->drawCircle(cx, cy, radius * amount + 2.0f, paint);

           // Glow
           paint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 5.0f));
           canvas->drawCircle(cx, cy, radius * amount + 2.0f, paint);
           paint.setMaskFilter(nullptr);
        }

        // Hover highlight
        if (hoverRow_ == r && hoverCol_ == c) {
           paint.setColor(SkColorSetARGB(100, 255, 255, 255));
           paint.setStyle(SkPaint::kStroke_Style);
           paint.setStrokeWidth(1.0f);
           canvas->drawRect(SkRect::MakeXYWH(x, y, cellWidth, cellHeight), paint);
           paint.setStyle(SkPaint::kFill_Style);
        }
      }
    }
#else
    juce::ignoreUnused(canvas);
#endif
  }
  
  void mouseMove(const juce::MouseEvent& e) override {
    updateHover(e);
  }
  
  void mouseExit(const juce::MouseEvent&) override {
    hoverRow_ = -1;
    hoverCol_ = -1;
    repaint();
  }
  
  void mouseDown(const juce::MouseEvent& e) override {
    updateHover(e);
    if (hoverRow_ >= 0 && hoverCol_ >= 0) {
       isDragging_ = true;
       lastMouseY_ = e.position.y;
       
       ModulationSource src = (ModulationSource)(hoverRow_ + 1);
       ModulationDestination dst = (ModulationDestination)(hoverCol_ + 1);
       startVal_ = processor_.getModulationMatrix(static_cast<int>(src), static_cast<int>(dst));
    }
  }
  
  void mouseDrag(const juce::MouseEvent& e) override {
    if (isDragging_ && hoverRow_ >= 0 && hoverCol_ >= 0) {
       float diff = (lastMouseY_ - e.position.y) * 0.01f; // Drag up to increase
       float newVal = juce::jlimit(-1.0f, 1.0f, startVal_ + diff);
       
       ModulationSource src = (ModulationSource)(hoverRow_ + 1);
       ModulationDestination dst = (ModulationDestination)(hoverCol_ + 1);
       processor_.setModulationMatrix(static_cast<int>(src), static_cast<int>(dst), newVal);
       repaint();
    }
  }
  
  void mouseUp(const juce::MouseEvent&) override {
    isDragging_ = false;
  }

private:
  ZenithPolySynthProcessor& processor_;
  int hoverRow_ = -1;
  int hoverCol_ = -1;
  bool isDragging_ = false;
  float lastMouseY_ = 0.0f;
  float startVal_ = 0.0f;
  
  void updateHover(const juce::MouseEvent& e) {
    auto bounds = getLocalBounds();
    int numRows = (int)ModulationSource::NumSources - 1;
    int numCols = (int)ModulationDestination::NumDestinations - 1;

    // Bounds check: prevent division by zero
    if (numRows <= 0 || numCols <= 0) {
      hoverRow_ = -1;
      hoverCol_ = -1;
      return;
    }

    float cellWidth = bounds.getWidth() / (float)numCols;
    float cellHeight = bounds.getHeight() / (float)numRows;

    int r = (int)(e.position.y / cellHeight);
    int c = (int)(e.position.x / cellWidth);
    
    if (r >= 0 && r < numRows && c >= 0 && c < numCols) {
       if (hoverRow_ != r || hoverCol_ != c) {
         hoverRow_ = r;
         hoverCol_ = c;
         repaint();
       }
    } else {
       hoverRow_ = -1;
       hoverCol_ = -1;
       repaint();
    }
  }
};

//==============================================================================
//==============================================================================
/**
    Real-time Visualizer (Oscilloscope/Spectrum)
*/
class ZenithVisualizer : public SkiaComponent, private juce::Timer {
public:
  explicit ZenithVisualizer(ZenithPolySynthProcessor& p) : processor_(p) {
    startTimerHz(60);
    // Initialize display buffer
    displayBuffer_.resize(4096, 0.0f);
    tempBuffer_.resize(4096, 0.0f);
  }

  void drawSkia(SkCanvas * const canvas) override {
#ifdef ZENITH_ENABLE_SKIA
    auto bounds = getLocalBounds().toFloat();
    float w = bounds.getWidth();
    float h = bounds.getHeight();
    float cy = h / 2.0f;

    SkPaint paint;
    paint.setAntiAlias(true);

    // Background
    paint.setColor(SkColorSetARGB(50, 0, 0, 0));
    canvas->drawRect(SkRect::MakeWH(w, h), paint);

    // Grid
    paint.setColor(SkColorSetARGB(20, 255, 255, 255));
    paint.setStrokeWidth(1.0f);
    for (float x = 0; x < w; x += 40.0f) canvas->drawLine(x, 0, x, h, paint);
    for (float y = 0; y < h; y += 40.0f) canvas->drawLine(0, y, w, y, paint);

    // Waveform
    SkPath path;
    path.moveTo(0, cy);

    // Draw the latest N samples that fit the width
    int numSamplesToDraw = (int)w; // 1 pixel per sample
    int bufferSize = (int)displayBuffer_.size();

    // Thread-safe read of write pointer
    int currentWritePtr = writePtr_.load(std::memory_order_acquire);
    int readPtr = (currentWritePtr - numSamplesToDraw + bufferSize) % bufferSize;

    for (int x = 0; x < numSamplesToDraw; ++x) {
      // Read sample from display buffer
      float sample = displayBuffer_[(readPtr + x) % bufferSize];
      float y = cy - sample * (h * 0.4f); // Scale amplitude
      if (x == 0) path.moveTo((float)x, y);
      else path.lineTo((float)x, y);
    }

    // Glow effect
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(3.0f);
    paint.setColor(SkColorSetRGB(0, 255, 255)); // Cyan
    paint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 4.0f));
    canvas->drawPath(path, paint);

    // Sharp line
    paint.setMaskFilter(nullptr);
    paint.setStrokeWidth(2.0f);
    paint.setColor(SkColorSetRGB(200, 255, 255));
    canvas->drawPath(path, paint);
#else
    juce::ignoreUnused(canvas);
#endif
  }

  void timerCallback() override {
    // Read from processor (audio thread writes to processor's internal buffer)
    int numRead = processor_.readFromVisualizer(tempBuffer_.data(), (int)tempBuffer_.size());
    if (numRead > 0) {
        int bufferSize = (int)displayBuffer_.size();
        int currentWritePtr = writePtr_.load(std::memory_order_acquire);

        // Write to ring buffer
        for (int i = 0; i < numRead; ++i) {
            displayBuffer_[currentWritePtr] = tempBuffer_[i];
            currentWritePtr = (currentWritePtr + 1) % bufferSize;
        }

        // Thread-safe update of write pointer
        writePtr_.store(currentWritePtr, std::memory_order_release);
        repaint();
    }
  }

private:
  ZenithPolySynthProcessor& processor_;
  std::vector<float> tempBuffer_;  // Non-atomic temp buffer for reading from processor
  std::vector<float> displayBuffer_;  // Display buffer (thread-safe access via mutex or single-writer pattern)
  std::atomic<int> writePtr_{0};  // Thread-safe write pointer
};

//==============================================================================
/**
    Preset Selection Bar
*/
class ZenithPresetBar : public SkiaComponent {
public:
  ZenithPresetBar() {
      prevBtn_ = std::make_unique<ZenithButton>("<", [this]() { if (onPrev_) onPrev_(); });
      nextBtn_ = std::make_unique<ZenithButton>(">", [this]() { if (onNext_) onNext_(); });
      
      addChildComponent(prevBtn_.get());
      addChildComponent(nextBtn_.get());
      
      prevBtn_->setVisible(true);
      nextBtn_->setVisible(true);
  }

  void setPresetName(const juce::String& name) {
      presetName_ = name;
      repaint();
  }

  void setCallbacks(std::function<void()> onPrev, std::function<void()> onNext, std::function<void()> onMenu) {
      onPrev_ = onPrev;
      onNext_ = onNext;
      onMenu_ = onMenu;
  }

  void resized() override {
      auto bounds = getLocalBounds();
      prevBtn_->setBounds(bounds.removeFromLeft(30).reduced(2));
      nextBtn_->setBounds(bounds.removeFromRight(30).reduced(2));
  }

  void drawSkia(SkCanvas* canvas) override {
#ifdef ZENITH_ENABLE_SKIA
      auto bounds = getLocalBounds().toFloat();
      float cx = bounds.getCentreX();
      float cy = bounds.getCentreY();

      SkPaint paint;
      paint.setAntiAlias(true);

      // Background
      paint.setColor(SkColorSetARGB(40, 0, 0, 0));
      canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()), paint);

      // Preset Name
      SkFont font;
      font.setSize(16.0f);
      font.setEmbolden(true);

      std::string str = presetName_.toStdString();
      float textWidth = font.measureText(str.c_str(), str.length(), SkTextEncoding::kUTF8);

      // Glow effect for text
      paint.setStyle(SkPaint::kFill_Style);
      paint.setColor(SkColorSetRGB(0, 255, 255));
      paint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 10.0f));
      canvas->drawSimpleText(str.c_str(), str.length(), SkTextEncoding::kUTF8,
                             cx - textWidth/2, cy + 6.0f, font, paint);

      // Sharp text
      paint.setMaskFilter(nullptr);
      paint.setColor(SK_ColorWHITE);
      canvas->drawSimpleText(str.c_str(), str.length(), SkTextEncoding::kUTF8,
                             cx - textWidth/2, cy + 6.0f, font, paint);
#else
      juce::ignoreUnused(canvas);
#endif
  }

private:
  std::unique_ptr<ZenithButton> prevBtn_;
  std::unique_ptr<ZenithButton> nextBtn_;
  juce::String presetName_ = "Default";
  
  std::function<void()> onPrev_;
  std::function<void()> onNext_;
  std::function<void()> onMenu_;
};

} // namespace zenith


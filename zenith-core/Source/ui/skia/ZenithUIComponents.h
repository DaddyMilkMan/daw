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
#include <memory>
#include "SkiaComponent.h"

#ifdef ZENITH_USE_SKIA
#include <skia/include/core/SkCanvas.h>
#include <skia/include/core/SkPaint.h>
#include <skia/include/core/SkPath.h>
#include <skia/include/core/SkRect.h>
#include <skia/include/core/SkRRect.h>
#include <skia/include/core/SkFont.h>
#include <skia/include/core/SkColor.h>
#include <skia/include/core/SkColorSpace.h>
#include <skia/include/effects/SkGradientShader.h>
#include <skia/include/effects/SkImageFilters.h>
#include <skia/include/core/SkMaskFilter.h>
#include <skia/include/core/SkBlurTypes.h>
#else
// Fallback types if Skia is disabled
class SkCanvas;
#endif

#include "../../instruments/ZenithPolySynth.h"

namespace zenith {

//==============================================================================
/**
    Base class for Zenith Controls (Knobs, Sliders, Buttons)
*/
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
  std::atomic<bool> isHovered_{false};    // JANE'S FIX: Atomic to prevent race with render thread
  
  // Colors - using uint32_t to avoid SkColor dependency issues in header if possible
  uint32_t accentColor_ = 0xFF00FFFF; // Cyan default (0xAARRGGBB)
  juce::String tooltipText_;

  void mouseEnter(const juce::MouseEvent&) override { 
      isHovered_.store(true, std::memory_order_release);  // JANE'S FIX: Atomic store
      if (onHoverStateChanged) onHoverStateChanged(this);
      repaint(); 
  }
  void mouseExit(const juce::MouseEvent&) override { 
      isHovered_.store(false, std::memory_order_release);  // JANE'S FIX: Atomic store
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
#ifdef ZENITH_USE_SKIA
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
#endif
  }
  
  bool hitTest(int x, int y) override { return false; }

private:
  ZenithControl* target_ = nullptr;
  
  void drawTooltipCard(SkCanvas* canvas, const SkRect& targetRect) {
#ifdef ZENITH_USE_SKIA
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
#endif
  }
};

//==============================================================================
/**
    Neon Glowing Knob
*/
class ZenithKnob : public ZenithControl {
public:
  explicit ZenithKnob(const juce::String& name, uint32_t color = 0xFF00FFFF) 
    : ZenithControl(name) {
    accentColor_ = color;
    setSize(80, 80);
  }

  void paint(juce::Graphics& g) override {
    // Fallback paint
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::white);
    g.drawText(name_, getLocalBounds(), juce::Justification::centred);
  }

  void drawSkia(SkCanvas* canvas) override {
#ifdef ZENITH_USE_SKIA
    auto bounds = getLocalBounds().toFloat();
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    float radius = std::min(bounds.getWidth(), bounds.getHeight()) * 0.4f;

    SkPaint paint;
    paint.setAntiAlias(true);

    // 0. Drop Shadow
    SkPaint shadowPaint;
    shadowPaint.setColor(SkColorSetARGB(100, 0, 0, 0));
    shadowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 8.0f));
    canvas->drawCircle(cx, cy + 4.0f, radius, shadowPaint);

    // 1. Background Track (Dark Grey Ring with Inner Shadow)
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(6.0f); // Thicker track
    paint.setColor(SkColorSetARGB(255, 30, 30, 35)); // Solid dark
    
    SkRect arcRect = SkRect::MakeXYWH(cx - radius, cy - radius, radius * 2, radius * 2);
    canvas->drawArc(arcRect, 135.0f, 270.0f, false, paint);
    
    // Inner groove
    paint.setStrokeWidth(2.0f);
    paint.setColor(SkColorSetARGB(255, 10, 10, 15));
    canvas->drawArc(arcRect, 135.0f, 270.0f, false, paint);

    // 2. Value Arc (Neon Glow with Gradient)
    float normValue = 0.0f;
    if (parameter_) normValue = parameter_->getValue();
    else normValue = juce::jmap(cachedValue_.load(), range_.start, range_.end, 0.0f, 1.0f);
    
    float sweepAngle = normValue * 270.0f;

    // Gradient Shader for Arc
    SkPoint pts[2] = { SkPoint::Make(cx - radius, cy + radius), SkPoint::Make(cx + radius, cy - radius) };
    SkColor colors[2] = { SkColorSetARGB(255, 0, 100, 255), accentColor_ }; // Blue to Accent
    paint.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2, SkTileMode::kClamp));

    paint.setStrokeWidth(4.0f);
    paint.setStrokeCap(SkPaint::kRound_Cap); // Rounded ends
    
    // Add glow
    if (isHovered_.load(std::memory_order_acquire)) {  // JANE'S FIX: Atomic load
        paint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 6.0f));
        canvas->drawArc(arcRect, 135.0f, sweepAngle, false, paint);
        paint.setMaskFilter(nullptr); // Reset for sharp line
    }
    
    canvas->drawArc(arcRect, 135.0f, sweepAngle, false, paint);
    paint.setShader(nullptr); // Reset shader

    // 3. Label
    SkFont font;
    font.setSize(12.0f);
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(SkColorSetARGB(200, 255, 255, 255));
    
    std::string labelStr = name_.toStdString();
    canvas->drawSimpleText(labelStr.c_str(), labelStr.length(), SkTextEncoding::kUTF8, 
                           cx - (labelStr.length() * 3.0f), cy + radius + 15.0f, font, paint);
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
  explicit ZenithSlider(const juce::String& name, uint32_t color = 0xFFFF00FF) 
    : ZenithControl(name) {
    accentColor_ = color;
  }

  void paint(juce::Graphics& g) override {
      g.fillAll(juce::Colours::black);
  }

  void drawSkia(SkCanvas* canvas) override {
#ifdef ZENITH_USE_SKIA
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
    else normValue = juce::jmap(cachedValue_.load(), range_.start, range_.end, 0.0f, 1.0f);

    float yPos = h * 0.9f - (normValue * (h * 0.8f));

    paint.setColor(accentColor_);
    paint.setStrokeWidth(4.0f);
    if (isHovered_.load(std::memory_order_acquire)) {  // JANE'S FIX: Atomic load
        paint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 4.0f));
        canvas->drawLine(w/2, h * 0.9f, w/2, yPos, paint);
        paint.setMaskFilter(nullptr);
    }
    canvas->drawLine(w/2, h * 0.9f, w/2, yPos, paint);

    // Handle Circle
    paint.setStyle(SkPaint::kFill_Style);
    canvas->drawCircle(w/2, yPos, 4.0f, paint);
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
  explicit ZenithButton(const juce::String& text, std::function<void()> onClick) 
    : text_(text), onClick_(onClick) {}

  void paint(juce::Graphics& g) override {
      g.fillAll(juce::Colours::black);
  }

  void drawSkia(SkCanvas* canvas) override {
#ifdef ZENITH_USE_SKIA
    auto bounds = getLocalBounds().toFloat();
    SkRect rect = SkRect::MakeXYWH(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight());
    
    SkPaint paint;
    paint.setAntiAlias(true);
    
    // Background
    if (isDown_) paint.setColor(SkColorSetARGB(50, 255, 255, 255));
    else if (isHovered_) paint.setColor(SkColorSetARGB(30, 255, 255, 255));
    else paint.setColor(SkColorSetARGB(10, 255, 255, 255));
    
    canvas->drawRoundRect(rect, 4.0f, 4.0f, paint);

    // Border
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(1.0f);
    paint.setColor(SkColorSetARGB(100, 255, 255, 255));
    canvas->drawRoundRect(rect, 4.0f, 4.0f, paint);

    // Text
    SkFont font;
    font.setSize(14.0f);
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(SK_ColorWHITE);
    
    std::string str = text_.toStdString();
    // Centering text (approximate)
    float textWidth = font.measureText(str.c_str(), str.length(), SkTextEncoding::kUTF8);
    float x = bounds.getCentreX() - textWidth / 2;
    float y = bounds.getCentreY() + 5.0f;
    
    canvas->drawSimpleText(str.c_str(), str.length(), SkTextEncoding::kUTF8, x, y, font, paint);
#endif
  }

  void mouseDown(const juce::MouseEvent&) override {
      isDown_ = true;
      repaint();
  }
  
  void mouseUp(const juce::MouseEvent&) override {
      isDown_ = false;
      if (onClick_) onClick_();
      repaint();
  }
  
  void mouseEnter(const juce::MouseEvent&) override { isHovered_ = true; repaint(); }
  void mouseExit(const juce::MouseEvent&) override { isHovered_ = false; repaint(); }

private:
  juce::String text_;
  std::function<void()> onClick_;
  bool isHovered_ = false;
  bool isDown_ = false;
};

//==============================================================================
/**
    Modulation Matrix Component
*/
class ZenithModMatrix : public SkiaComponent {
public:
    ZenithModMatrix(ZenithPolySynthProcessor& p) : processor(p) {}

    void drawSkia(SkCanvas* canvas) override {
#ifdef ZENITH_USE_SKIA
        // Draw grid
        auto bounds = getLocalBounds().toFloat();
        SkPaint paint;
        paint.setColor(SkColorSetARGB(50, 255, 255, 255));
        paint.setStyle(SkPaint::kStroke_Style);
        
        SkRect rect = SkRect::MakeXYWH(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight());
        canvas->drawRect(rect, paint);
        
        // TODO: Implement matrix grid
#endif
    }
    
private:
    ZenithPolySynthProcessor& processor;
};

//==============================================================================
/**
    Visualizer Component
*/
class ZenithVisualizer : public SkiaComponent {
public:
    ZenithVisualizer(ZenithPolySynthProcessor& p) : processor(p) {}

    void drawSkia(SkCanvas* canvas) override {
#ifdef ZENITH_USE_SKIA
        // Draw waveform
        auto bounds = getLocalBounds().toFloat();
        SkPaint paint;
        paint.setColor(SkColorSetRGB(0, 255, 255));
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(2.0f);
        
        SkPath path;
        path.moveTo(bounds.getX(), bounds.getCentreY());
        path.lineTo(bounds.getRight(), bounds.getCentreY());
        
        canvas->drawPath(path, paint);
#endif
    }
    
private:
    ZenithPolySynthProcessor& processor;
};

//==============================================================================
/**
    Preset Bar Component
*/
class ZenithPresetBar : public SkiaComponent {
public:
    void setPresetName(const juce::String& name) {
        presetName_ = name;
        repaint();
    }
    
    void setCallbacks(std::function<void()> prev, std::function<void()> next, std::function<void()> menu) {
        onPrev_ = prev;
        onNext_ = next;
        onMenu_ = menu;
    }

    void drawSkia(SkCanvas* canvas) override {
#ifdef ZENITH_USE_SKIA
        auto bounds = getLocalBounds().toFloat();
        SkPaint paint;
        
        // Background
        paint.setColor(SkColorSetARGB(100, 20, 20, 30));
        canvas->drawRect(SkRect::MakeXYWH(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight()), paint);
        
        // Text
        SkFont font;
        font.setSize(16.0f);
        paint.setColor(SK_ColorWHITE);
        
        std::string str = presetName_.toStdString();
        float textWidth = font.measureText(str.c_str(), str.length(), SkTextEncoding::kUTF8);
        canvas->drawSimpleText(str.c_str(), str.length(), SkTextEncoding::kUTF8, 
                               bounds.getCentreX() - textWidth/2, bounds.getCentreY() + 6.0f, font, paint);
        
        // Arrows (Simple triangles)
        // Left
        SkPath leftArrow;
        leftArrow.moveTo(20, bounds.getCentreY());
        leftArrow.lineTo(30, bounds.getCentreY() - 5);
        leftArrow.lineTo(30, bounds.getCentreY() + 5);
        leftArrow.close();
        paint.setColor(SK_ColorWHITE);
        canvas->drawPath(leftArrow, paint);
        
        // Right
        SkPath rightArrow;
        rightArrow.moveTo(bounds.getWidth() - 20, bounds.getCentreY());
        rightArrow.lineTo(bounds.getWidth() - 30, bounds.getCentreY() - 5);
        rightArrow.lineTo(bounds.getWidth() - 30, bounds.getCentreY() + 5);
        rightArrow.close();
        canvas->drawPath(rightArrow, paint);
#endif
    }
    
    void mouseDown(const juce::MouseEvent& e) override {
        if (e.x < 50 && onPrev_) onPrev_();
        else if (e.x > getWidth() - 50 && onNext_) onNext_();
        else if (onMenu_) onMenu_();
    }

private:
    juce::String presetName_ = "Init Preset";
    std::function<void()> onPrev_, onNext_, onMenu_;
// ... existing code ...
};

// Particle System (Visualist Request)
class ParticleSystem : public juce::Component, private juce::Timer {
public:
    struct Particle {
        float x, y;
        float vx, vy;
        float life; // 0.0 to 1.0
        juce::Colour color;
    };

    ParticleSystem() {
        setInterceptsMouseClicks(false, false);
    }
    
    void spawnParticles(int x, int y, int count = 20) {
        juce::Random rng;
        for (int i = 0; i < count; ++i) {
            Particle p;
            p.x = (float)x;
            p.y = (float)y;
            float angle = rng.nextFloat() * juce::MathConstants<float>::twoPi;
            float speed = rng.nextFloat() * 5.0f + 2.0f;
            p.vx = std::cos(angle) * speed;
            p.vy = std::sin(angle) * speed;
            p.life = 1.0f;
            p.color = juce::Colours::cyan.withHue(rng.nextFloat());
            particles.push_back(p);
        }
        
        if (!isTimerRunning())
            startTimerHz(60);
    }
    
    void timerCallback() override {
        if (particles.empty()) {
            stopTimer();
            return;
        }
        
        for (auto& p : particles) {
            p.x += p.vx;
            p.y += p.vy;
            p.vy += 0.2f; // Gravity
            p.life -= 0.02f;
        }
        
        // Remove dead particles
        particles.erase(std::remove_if(particles.begin(), particles.end(),
            [](const Particle& p) { return p.life <= 0.0f; }), particles.end());
            
        repaint();
    }
    
    void paint(juce::Graphics& g) override {
        for (const auto& p : particles) {
            g.setColour(p.color.withAlpha(p.life));
            g.fillEllipse(p.x - 2, p.y - 2, 4, 4);
        }
    }

private:
    std::vector<Particle> particles;
};

} // namespace zenith

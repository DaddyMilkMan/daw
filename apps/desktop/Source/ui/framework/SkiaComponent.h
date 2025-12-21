/*
  ==============================================================================

    SkiaComponent.h
    Created: 2025-11-30
    Authors: Sarah Chen (lead), with input from ENTIRE TEAM

    Base class for all Skia-rendered components in Zenith DAW.
  ==============================================================================
*/

#pragma once

extern "C++" {
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkFont.h>
#include <core/SkMaskFilter.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <core/SkRRect.h>
#include <core/SkRect.h>
#include <core/SkShader.h>
}
#include <juce_gui_basics/juce_gui_basics.h>

#include "SkiaAccessibility.h"
#include "ZenithDesignSystem.h"
#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace zenith {

  class AnimatedValue;

  template <typename T> class ValueHistory {
  public:
    void push(T value) {
      // Remove redo history
      if (!history_.empty() && currentIndex_ < history_.size() - 1) {
        history_.erase(history_.begin() + currentIndex_ + 1, history_.end());
      }

      history_.push_back(value);
      currentIndex_ = history_.size() - 1;

      // Limit history size
      if (history_.size() > 50) {
        history_.erase(history_.begin());
        if (currentIndex_ > 0)
          currentIndex_--;
      }
    }

    bool canUndo() const { return !history_.empty() && currentIndex_ > 0; }
    bool canRedo() const {
      return !history_.empty() && currentIndex_ < history_.size() - 1;
    }
    T undo() {
      if (canUndo()) {
        currentIndex_--;
        return history_[currentIndex_];
      }
      return history_.empty() ? T() : history_[currentIndex_];
    }
    T redo() {
      if (canRedo()) {
        currentIndex_++;
        return history_[currentIndex_];
      }
      return history_.empty() ? T() : history_[currentIndex_];
    }
    T getCurrent() const {
      return history_.empty() ? T() : history_[currentIndex_];
    }

  private:
    std::vector<T> history_;
    size_t currentIndex_ = 0;
  };

  class SkiaComponent : public juce::Component,
                        public virtual juce::Timer,
                        public juce::KeyListener {
  public:
    struct AIElementInfo {
      SkRect bounds;
      juce::String type; // "knob", "fader", "button"
      juce::String id;
      juce::String label;
      juce::String parameterId;
      float currentValue = 0.0f;
    };

    explicit SkiaComponent();
    ~SkiaComponent() override;

    virtual void drawSkia(SkCanvas *canvas) = 0;

    // AI Vision Hook
    virtual std::vector<AIElementInfo> getInspectableElements() { return {}; }

#ifdef DEBUG
    virtual void drawDebug(SkCanvas *canvas);
#endif

    void setGlowEnabled(bool enabled) {
      glowEnabled_ = enabled;
      markDirty();
    }
    bool isGlowEnabled() const { return glowEnabled_; }

    void setGlowColor(SkColor color) {
      glowColor_ = color;
      markDirty();
    }
    SkColor getGlowColor() const { return glowColor_; }

    void setGlowRadius(float radius) {
      glowRadius_ = radius;
      markDirty();
    }
    float getGlowRadius() const { return glowRadius_; }

    // Lifecycle hooks
    virtual void onShow() {}
    virtual void onHide() {}
    virtual void onResize() {}
    virtual void onParentChanged() {}

    // Interaction hooks
    virtual void onHoverEnter() {}
    virtual void onHoverExit() {}
    virtual void onFocusGained() {}
    virtual void onFocusLost() {}

    // Widget delegation hooks
    virtual void onMouseDown(const juce::MouseEvent &e) {
      juce::ignoreUnused(e);
    }
    virtual void onMouseDrag(const juce::MouseEvent &e) {
      juce::ignoreUnused(e);
    }
    virtual void onMouseUp(const juce::MouseEvent &e) { juce::ignoreUnused(e); }

    // Force hit testing for transparent Skia components
    bool hitTest(int x, int y) override;

    void animateTo(const juce::String &property, float target, int durationMs);
    void animateWithSpring(const juce::String &property, float target,
                           float stiffness, float damping);
    void stopAnimation(const juce::String &property);
    void stopAllAnimations();

    float getAnimatedValue(const juce::String &property) const;
    bool isAnimating(const juce::String &property) const;

    bool isHovered() const { return isHovered_; }
    void setHovered(bool hovered) {
      isHovered_ = hovered;
      markDirty();
    }
    bool isFocused() const { return juce::Component::hasKeyboardFocus(true); }

    void markDirty() {
      needsRepaint_ = true;
      repaint();
    }
    bool isDirty() const { return needsRepaint_; }

    void paint(juce::Graphics &g) override;
    void resized() override;
    void mouseEnter(const juce::MouseEvent &e) override;
    void mouseExit(const juce::MouseEvent &e) override;
    void focusGained(juce::Component::FocusChangeType cause) override;
    void focusLost(juce::Component::FocusChangeType cause) override;
    void timerCallback() override;

    // Avoid hiding Component::keyPressed
    using juce::Component::keyPressed;
    bool keyPressed(const juce::KeyPress &key,
                    juce::Component *origin) override;

    virtual void onEnterPressed() {}
    virtual void onEscapePressed() {}

    virtual void showContextMenu();
    virtual void handleContextMenuResult(int result);

    // Context menu actions
    virtual void resetToDefault() {}
    virtual void copyValue() {}
    virtual void pasteValue() {}
    virtual void toggleMIDILearn() {}

    static void setTargetFPS(int fps);
    static int getTargetFPS();

  protected:
    void drawChildren(SkCanvas *canvas); // Helper to draw child components
    SkCanvas *getSkiaCanvas(juce::Graphics &g);

    void applyGlow(SkPaint &paint, float intensity = 1.0f);
    void animateColorChange();

  private:
    bool isHovered_ = false;
    bool needsRepaint_ = true;
    SkColor glowColor_ = design::colors::NEON_GREEN;
    float glowRadius_ = 0.0f;
    bool glowEnabled_ = false;
    bool isMIDILearning_ = false;

    static int systemRefreshRate_;
    static int targetFPS_;
    static int getSystemRefreshRate();

    std::map<juce::String, std::unique_ptr<AnimatedValue>> animations_;
    mutable juce::CriticalSection animationLock_;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaComponent)
  };

  class AnimatedValue {
  public:
    enum class EasingCurve { Linear, EaseIn, EaseOut, EaseInOut, Spring };

    AnimatedValue(float initial = 0.0f);

    void setTarget(float target, int durationMs,
                   EasingCurve curve = EasingCurve::EaseOut);
    void setSpring(float target, float stiffness, float damping);
    void stop();

    float getCurrentValue() const { return currentValue_; }
    bool isAnimating() const { return isAnimating_; }

    void update(float deltaTimeMs);

  private:
    float currentValue_;
    float targetValue_;
    float startValue_;
    float velocity_;

    int durationMs_;
    int elapsedMs_;

    EasingCurve curve_;
    bool isAnimating_;

    float springStiffness_;
    float springDamping_;
    bool useSpring_;

    float easeValue(float t) const;
  };

} // namespace zenith
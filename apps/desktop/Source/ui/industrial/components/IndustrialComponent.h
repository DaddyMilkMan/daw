#pragma once

#include "../../ZenithSkia.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <string>
#include <functional>

namespace zenith::industrial {

struct MouseEvent {
  float x = 0.0f;
  float y = 0.0f;
  bool shiftDown = false;
};

class IndustrialComponent {
public:
  virtual ~IndustrialComponent() = default;

  virtual void render(SkCanvas* canvas) = 0;
  virtual void handleMouseDown(const MouseEvent& e) { juce::ignoreUnused(e); }
  virtual void handleMouseDrag(const MouseEvent& e) { juce::ignoreUnused(e); }
  virtual void handleMouseUp(const MouseEvent& e) { juce::ignoreUnused(e); }
  virtual void handleMouseMove(const MouseEvent& e) {
    setHovered(hitTest(e.x, e.y));
  }

  virtual bool hitTest(float x, float y) const {
    return bounds_.contains(x, y);
  }

  void setBounds(const SkRect& bounds) { bounds_ = bounds; }
  SkRect getBounds() const { return bounds_; }

  void setVisible(bool visible) { isVisible_ = visible; }
  bool isVisible() const { return isVisible_; }

  void setAlpha(float alpha) { alpha_ = juce::jlimit(0.0f, 1.0f, alpha); }
  float getAlpha() const { return alpha_; }

  bool isHovered() const { return isHovered_; }
  bool isPressed() const { return isPressed_; }
  void setHovered(bool hovered) { isHovered_ = hovered; }

  // Value change callback for parameter binding
  using ValueChangedCallback = std::function<void(float)>;
  void setValueCallback(ValueChangedCallback callback) {
    onValueChanged_ = std::move(callback);
  }

protected:
  SkRect bounds_ = SkRect::MakeEmpty();
  bool isHovered_ = false;
  bool isPressed_ = false;
  bool isVisible_ = true;
  float alpha_ = 1.0f;

  ValueChangedCallback onValueChanged_;

  void notifyValueChanged(float newValue) {
    if (onValueChanged_) {
      onValueChanged_(newValue);
    }
  }
};

} // namespace zenith::industrial

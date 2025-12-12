/*
  ==============================================================================

    ZenithControl.cpp
    Created: 2025-12-12
    Author:  Zenith DAW

    Implementation of the base control class.

  ==============================================================================
*/

#include "ZenithControl.h"

namespace zenith {

ZenithControl::ZenithControl(const juce::String &name) : name_(name) {
  setWantsKeyboardFocus(true);
}

ZenithControl::~ZenithControl() {
  if (parameter_ != nullptr) {
    parameter_->removeListener(this);
  }
}

void ZenithControl::setParameter(juce::RangedAudioParameter *param) {
  if (parameter_ != nullptr) {
    parameter_->removeListener(this);
  }

  parameter_ = param;

  if (parameter_ != nullptr) {
    range_ = parameter_->getNormalisableRange();
    float initialValue = parameter_->convertFrom0to1(parameter_->getValue());
    cachedValue_.store(initialValue, std::memory_order_release);
    defaultValue_ = parameter_->convertFrom0to1(parameter_->getDefaultValue());
    parameter_->addListener(this);
  }

  repaint();
}

void ZenithControl::setValue(float value, bool sendNotification) {
  float constrainedValue = constrainValue(value);
  float currentValue = cachedValue_.load(std::memory_order_acquire);

  if (currentValue != constrainedValue) {
    cachedValue_.store(constrainedValue, std::memory_order_release);

    if (sendNotification && parameter_ != nullptr) {
      parameter_->beginChangeGesture();
      parameter_->setValueNotifyingHost(
          parameter_->convertTo0to1(constrainedValue));
      parameter_->endChangeGesture();
    }

    notifyValueChange();
    repaint();
  }
}

float ZenithControl::getValue() const {
  return cachedValue_.load(std::memory_order_acquire);
}

float ZenithControl::getNormalizedValue() const {
  if (parameter_ != nullptr) {
    return parameter_->getValue();
  }

  float value = getValue();
  if (range_.end - range_.start != 0.0f) {
    return (value - range_.start) / (range_.end - range_.start);
  }
  return 0.0f;
}

void ZenithControl::resetToDefault() { setValue(defaultValue_, true); }

void ZenithControl::setRange(float min, float max, float interval) {
  range_ = juce::NormalisableRange<float>(min, max, interval);
  repaint();
}

juce::String ZenithControl::getValueAsText() const {
  float value = getValue();

  // Format based on range
  juce::String text;
  if (range_.interval >= 1.0f) {
    text = juce::String(static_cast<int>(value));
  } else if (range_.interval >= 0.1f) {
    text = juce::String(value, 1);
  } else {
    text = juce::String(value, 2);
  }

  if (textSuffix_.isNotEmpty()) {
    text += " " + textSuffix_;
  }

  return text;
}

void ZenithControl::parameterValueChanged(int parameterIndex, float newValue) {
  juce::ignoreUnused(parameterIndex);

  // Thread-safe value update from audio thread
  if (parameter_ != nullptr) {
    float convertedValue = parameter_->convertFrom0to1(newValue);
    cachedValue_.store(convertedValue, std::memory_order_release);
  }

  // Safely schedule repaint on message thread
  juce::Component::SafePointer<ZenithControl> safeThis(this);
  juce::MessageManager::callAsync([safeThis]() {
    if (safeThis != nullptr) {
      safeThis->repaint();
    }
  });
}

void ZenithControl::parameterGestureChanged(int parameterIndex,
                                            bool gestureIsStarting) {
  juce::ignoreUnused(parameterIndex, gestureIsStarting);
}

void ZenithControl::mouseEnter(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  isHovered_ = true;

  if (onHoverStateChanged) {
    onHoverStateChanged(this);
  }

  repaint();
}

void ZenithControl::mouseExit(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  isHovered_ = false;

  if (onHoverStateChanged) {
    onHoverStateChanged(nullptr);
  }

  repaint();
}

void ZenithControl::mouseDown(const juce::MouseEvent &e) {
  if (!isEnabled())
    return;

  isDragging_ = true;
  dragStartValue_ = getValue();
  dragStartPos_ = e.position;
  isFineMode_ = e.mods.isShiftDown();

  if (parameter_ != nullptr) {
    parameter_->beginChangeGesture();
  }

  if (onDragStart) {
    onDragStart();
  }

  repaint();
}

void ZenithControl::mouseUp(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);

  if (isDragging_) {
    isDragging_ = false;
    isFineMode_ = false;

    if (parameter_ != nullptr) {
      parameter_->endChangeGesture();
    }

    if (onDragEnd) {
      onDragEnd();
    }

    repaint();
  }
}

void ZenithControl::mouseDrag(const juce::MouseEvent &e) {
  // To be overridden by subclasses for specific drag behavior
  juce::ignoreUnused(e);
}

void ZenithControl::mouseDoubleClick(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);

  if (doubleClickToReset_ && isEnabled()) {
    resetToDefault();
  }
}

void ZenithControl::mouseWheelMove(const juce::MouseEvent &e,
                                   const juce::MouseWheelDetails &wheel) {
  if (!scrollWheelEnabled_ || !isEnabled()) {
    Component::mouseWheelMove(e, wheel);
    return;
  }

  // Calculate step size based on range
  float step = (range_.end - range_.start) * 0.01f; // 1% of range

  // Apply fine control modifier
  if (e.mods.isShiftDown()) {
    step *= fineControlMultiplier_;
  }

  // Apply wheel delta
  float delta = wheel.deltaY * step * 10.0f;
  float newValue = getValue() + delta;

  setValue(newValue, true);
}

void ZenithControl::modifierKeysChanged(const juce::ModifierKeys &modifiers) {
  bool newFineMode = modifiers.isShiftDown();

  if (isDragging_ && newFineMode != isFineMode_) {
    isFineMode_ = newFineMode;
    // Update drag start position to prevent jumps
    dragStartValue_ = getValue();
    // Note: This requires storing current mouse position
  }
}

float ZenithControl::constrainValue(float value) const {
  return juce::jlimit(range_.start, range_.end, value);
}

void ZenithControl::notifyValueChange() {
  if (onValueChanged) {
    onValueChanged(getValue());
  }
}

} // namespace zenith

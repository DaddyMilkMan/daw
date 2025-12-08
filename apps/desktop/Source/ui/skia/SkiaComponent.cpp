/*
  ==============================================================================

    SkiaComponent.cpp
    Created: 2025-11-30
    Authors: Sarah Chen, Dr. Aris Vokos, Viktor Volkov, Diego Martinez

    Implementation of base Skia component.
*/

#include "SkiaComponent.h"
#include <core/SkBlurTypes.h> // Explicitly include

#ifdef _WIN32
#include <windows.h>
#endif

namespace zenith {

int SkiaComponent::systemRefreshRate_ = 60;
int SkiaComponent::targetFPS_ = 60;

SkiaComponent::SkiaComponent() {
  setOpaque(false);
  setVisible(true); // Ensure visible by default

  setInterceptsMouseClicks(true, true);

  setWantsKeyboardFocus(true);

  // Initialize refresh rate if not already done
  if (systemRefreshRate_ == 60) {
    getSystemRefreshRate();
  }
}

SkiaComponent::~SkiaComponent() { stopAllAnimations(); }

// ============================================================================
// RENDERING
// ============================================================================

void SkiaComponent::paint(juce::Graphics &g) {
  juce::ignoreUnused(g);
  // Skia components are rendered via drawSkia() by the parent renderer.
  // No JUCE painting or fallback.
}

SkCanvas *SkiaComponent::getSkiaCanvas(juce::Graphics &g) {
  juce::ignoreUnused(g);
  // This method is legacy/experimental for when JUCE_USE_SKIA is enabled
  // globally. For our manual integration, we pass the canvas down via
  // drawSkia().
  return nullptr;
}

// paintFallback removed

void SkiaComponent::applyGlow(SkPaint &paint, float intensity) {
  // Apply global glow intensity
  float globalIntensity = design::Settings::getGlowIntensity();
  float finalIntensity = intensity * globalIntensity;

  float clampedIntensity = juce::jlimit(0.0f, 1.0f, finalIntensity);

  // Glow with blur
  // If global intensity is 0 (Flat Mode), disable blur completely
  if (globalIntensity < 0.01f) {
    paint.setMaskFilter(nullptr);
    // Make it solid but semi-transparent
    paint.setColor(design::withAlpha(glowColor_, 0.8f));
  } else {
    paint.setMaskFilter(
        SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, clampedIntensity * 4.0f));
    paint.setColor(design::withAlpha(glowColor_, clampedIntensity * 0.8f));
  }

  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(2.0f + (clampedIntensity * 3.0f));
}

#ifdef DEBUG
void SkiaComponent::drawDebug(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();

  SkPaint debugPaint;
  debugPaint.setAntiAlias(true);
  debugPaint.setStyle(SkPaint::kStroke_Style);
  debugPaint.setColor(SK_ColorRED);

  // Draw bounds
  canvas->drawRect(SkRect::MakeXYWH(bounds.getX(), bounds.getY(),
                                    bounds.getWidth(), bounds.getHeight()),
                   debugPaint);

  // Draw center cross
  debugPaint.setColor(SK_ColorGREEN);
  float cx = bounds.getCentreX();
  float cy = bounds.getCentreY();
  canvas->drawLine(cx - 5, cy, cx + 5, cy, debugPaint);
  canvas->drawLine(cx, cy - 5, cx, cy + 5, debugPaint);
}
#endif

// ============================================================================
// LIFECYCLE
// ============================================================================

void SkiaComponent::resized() {
  onResize();
  markDirty();
}

void SkiaComponent::mouseEnter(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  isHovered_ = true;
  onHoverEnter();
  markDirty();
}

void SkiaComponent::mouseExit(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  isHovered_ = false;
  onHoverExit();
  markDirty();
}

// Force hit test to true for transparent components
bool SkiaComponent::hitTest(int x, int y) {
  // Simple bounds check (JUCE passes local coords)
  return x >= 0 && y >= 0 && x < getWidth() && y < getHeight();
}

void SkiaComponent::focusGained(juce::Component::FocusChangeType cause) {
  juce::ignoreUnused(cause);
  onFocusGained();
  markDirty();
}

void SkiaComponent::focusLost(juce::Component::FocusChangeType cause) {
  juce::ignoreUnused(cause);
  onFocusLost();
  markDirty();
}

void SkiaComponent::drawChildren(SkCanvas *canvas) {
  for (auto *child : getChildren()) {
    if (child->isVisible()) {
      if (auto *skiaChild = dynamic_cast<SkiaComponent *>(child)) {
        canvas->save();
        canvas->translate((float)child->getX(), (float)child->getY());
        skiaChild->drawSkia(canvas);
        canvas->restore();
      }
    }
  }
}

void SkiaComponent::animateColorChange() {
  // Placeholder for color animation
  markDirty();
}

// ============================================================================
// ANIMATION SYSTEM
// ============================================================================

void SkiaComponent::animateTo(const juce::String &property, float target,
                              int durationMs) {
  auto it = animations_.find(property);
  if (it == animations_.end()) {
    // Create new animation
    animations_[property] = std::make_unique<AnimatedValue>(0.0f);
    it = animations_.find(property);
  }

  it->second->setTarget(target, durationMs);

  // Start animation timer if not already running
  startTimer(1000 / targetFPS_); // Use target FPS
}

void SkiaComponent::animateWithSpring(const juce::String &property,
                                      float target, float stiffness,
                                      float damping) {
  auto it = animations_.find(property);
  if (it == animations_.end()) {
    animations_[property] = std::make_unique<AnimatedValue>(0.0f);
    it = animations_.find(property);
  }

  it->second->setSpring(target, stiffness, damping);
  startTimer(1000 / targetFPS_);
}

void SkiaComponent::stopAnimation(const juce::String &property) {
  auto it = animations_.find(property);
  if (it != animations_.end()) {
    it->second->stop();
  }
}

void SkiaComponent::stopAllAnimations() {
  for (auto &pair : animations_) {
    pair.second->stop();
  }
  stopTimer();
}

float SkiaComponent::getAnimatedValue(const juce::String &property) const {
  auto it = animations_.find(property);
  return it != animations_.end() ? it->second->getCurrentValue() : 0.0f;
}

bool SkiaComponent::isAnimating(const juce::String &property) const {
  auto it = animations_.find(property);
  return it != animations_.end() && it->second->isAnimating();
}

void SkiaComponent::timerCallback() {
  bool anyAnimating = false;
  float deltaTimeMs = 1000.0f / targetFPS_;

  for (auto &pair : animations_) {
    if (pair.second->isAnimating()) {
      pair.second->update(deltaTimeMs);
      anyAnimating = true;
    }
  }

  if (anyAnimating) {
    markDirty();
  } else {
    stopTimer();
  }
}

// ============================================================================
// CONTEXT MENU & UX
// ============================================================================

void SkiaComponent::showContextMenu() {
  juce::PopupMenu menu;

  // Standard items
  menu.addItem(1, "MIDI Learn", true, isMIDILearning_);
  menu.addSeparator();
  menu.addItem(2, "Copy Value");
  menu.addItem(3, "Paste Value");
  menu.addSeparator();
  menu.addItem(4, "Reset to Default");

  // Show menu asynchronously
  menu.showMenuAsync(juce::PopupMenu::Options().withParentComponent(this),
                     [this](int result) { handleContextMenuResult(result); });
}

void SkiaComponent::handleContextMenuResult(int result) {
  switch (result) {
  case 1:
    toggleMIDILearn();
    break;
  case 2:
    copyValue();
    break;
  case 3:
    pasteValue();
    break;
  case 4:
    resetToDefault();
    break;
  default:
    break;
  }
}

// ============================================================================
// SYSTEM REFRESH RATE
// ============================================================================

int SkiaComponent::getSystemRefreshRate() {
#ifdef _WIN32
  DEVMODE devMode;
  devMode.dmSize = sizeof(DEVMODE);
  devMode.dmDriverExtra = 0;

  if (EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &devMode)) {
    systemRefreshRate_ = devMode.dmDisplayFrequency;
    // Ensure reasonable bounds (e.g., 30Hz to 360Hz)
    if (systemRefreshRate_ < 30)
      systemRefreshRate_ = 30;
    if (systemRefreshRate_ > 360)
      systemRefreshRate_ = 360;

    targetFPS_ = systemRefreshRate_; // Default to system rate
    return systemRefreshRate_;
  }
#endif

  // Fallback for other platforms or if detection fails
  systemRefreshRate_ = 60;
  targetFPS_ = 60;
  return 60;
}

void SkiaComponent::setTargetFPS(int fps) {
  targetFPS_ =
      juce::jlimit(30, systemRefreshRate_ > 0 ? systemRefreshRate_ : 60, fps);
}

int SkiaComponent::getTargetFPS() { return targetFPS_; }

// ============================================================================
// KEYBOARD NAVIGATION
// ============================================================================

bool SkiaComponent::keyPressed(const juce::KeyPress &key,
                               juce::Component *origin) {
  juce::ignoreUnused(origin);
  if (key == juce::KeyPress::returnKey) {
    onEnterPressed();
    return true;
  }

  if (key == juce::KeyPress::escapeKey) {
    onEscapePressed();
    return true;
  }

  // Context menu shortcut (Shift+F10 or Menu key)
  if (key.isKeyCode(juce::KeyPress::F10Key) &&
      key.getModifiers().isShiftDown()) {
    showContextMenu();
    return true;
  }

  // Let parent handle tab navigation if needed, or implement custom tab logic
  // here
  return false;
}

// ============================================================================
// ANIMATED VALUE IMPLEMENTATION
// ============================================================================

AnimatedValue::AnimatedValue(float initial)
    : currentValue_(initial), targetValue_(initial), startValue_(initial),
      velocity_(0.0f), durationMs_(0), elapsedMs_(0),
      curve_(EasingCurve::EaseOut), isAnimating_(false), springStiffness_(0.5f),
      springDamping_(0.7f), useSpring_(false) {}

void AnimatedValue::setTarget(float target, int durationMs, EasingCurve curve) {
  targetValue_ = target;
  startValue_ = currentValue_;
  durationMs_ = durationMs;
  elapsedMs_ = 0;
  curve_ = curve;
  isAnimating_ = true;
  useSpring_ = false;
}

void AnimatedValue::setSpring(float target, float stiffness, float damping) {
  targetValue_ = target;
  springStiffness_ = stiffness;
  springDamping_ = damping;
  isAnimating_ = true;
  useSpring_ = true;
}

void AnimatedValue::stop() {
  isAnimating_ = false;
  velocity_ = 0.0f;
}

void AnimatedValue::update(float deltaTimeMs) {
  if (!isAnimating_)
    return;

  if (useSpring_) {
    // Spring physics
    float displacement = currentValue_ - targetValue_;
    float springForce = -springStiffness_ * displacement;
    float dampingForce = -springDamping_ * velocity_;

    velocity_ += (springForce + dampingForce) * (deltaTimeMs / 1000.0f);
    currentValue_ += velocity_ * (deltaTimeMs / 1000.0f);

    // Stop if close enough and slow enough
    if (std::abs(displacement) < 0.001f && std::abs(velocity_) < 0.001f) {
      currentValue_ = targetValue_;
      velocity_ = 0.0f;
      isAnimating_ = false;
    }
  } else {
    // Easing curve
    elapsedMs_ += static_cast<int>(deltaTimeMs);

    if (elapsedMs_ >= durationMs_) {
      currentValue_ = targetValue_;
      isAnimating_ = false;
    } else {
      float t =
          static_cast<float>(elapsedMs_) / static_cast<float>(durationMs_);
      float easedT = easeValue(t);
      currentValue_ = startValue_ + (targetValue_ - startValue_) * easedT;
    }
  }
}

float AnimatedValue::easeValue(float t) const {
  switch (curve_) {
  case EasingCurve::Linear:
    return t;

  case EasingCurve::EaseIn:
    return t * t;

  case EasingCurve::EaseOut:
    return t * (2.0f - t);

  case EasingCurve::EaseInOut:
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;

  default:
    return t;
  }
}

} // namespace zenith

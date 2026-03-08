/*
  ==============================================================================

    SkiaComponent.cpp
    Created: 2025-11-30
    Authors: Sarah Chen, Dr. Aris Vokos, Viktor Volkov, Diego Martinez

    Implementation of base Skia component.
*/

#include "SkiaComponent.h"
#include "../../engine/ZenithLogger.h"
#include "../controls/ContextMenuManager.h"
#include <core/SkBlurTypes.h> // Explicitly include

#include "PlatformDisplayUtils.h"

namespace zenith {

int SkiaComponent::systemRefreshRate_ = 60;
int SkiaComponent::targetFPS_ = 60;
std::function<void(const juce::String &, const juce::String &)> SkiaComponent::globalHelpCallback;

SkiaComponent::SkiaComponent() {
  setOpaque(false);
  setVisible(true); // Ensure visible by default

  setInterceptsMouseClicks(true, true);

  setWantsKeyboardFocus(true);

  // Initialize refresh rate if not already done
  static bool refreshRateInitialized = false;
  if (!refreshRateInitialized) {
    systemRefreshRate_ = PlatformDisplayUtils::getSystemRefreshRate();
    if (systemRefreshRate_ <= 0) systemRefreshRate_ = 60;
    refreshRateInitialized = true;
  }

  // Initialize with theme accent
  glowColor_ = design::colors::ACCENT_PRIMARY;

  // Property to identify SkiaComponent without RTTI
  getProperties().set("zenith_is_skia", true);
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
  
  if (globalHelpCallback && helpTitle_.isNotEmpty()) {
      globalHelpCallback(helpTitle_, helpDescription_);
  }
  
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
      if (child->getProperties().contains("zenith_is_skia")) {
        auto *skiaChild = static_cast<SkiaComponent *>(child);
        canvas->save();
        
        // Translate to child's position
        canvas->translate((float)child->getX(), (float)child->getY());
        
        // Clip to child's bounds to prevent bleeding
        canvas->clipRect(SkRect::MakeWH((float)child->getWidth(), (float)child->getHeight()));
        
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
    animations_[property] = std::make_unique<AnimatedValue>(0.0f);
    it = animations_.find(property);
  }

  it->second->setTarget(target, durationMs, ::zenith::animation::Easing::EaseOut);
  ZENITH_REGISTER_ANIMATION(zenith::animation::Priority::High);
}

void SkiaComponent::animateWithSpring(const juce::String &property,
                                      float target, float stiffness,
                                      float damping) {
  auto it = animations_.find(property);
  if (it == animations_.end()) {
    animations_[property] = std::make_unique<AnimatedValue>(0.0f);
    it = animations_.find(property);
  }

  ::zenith::animation::SpringConfig config;
  config.stiffness = stiffness * 1000.0f; // Scale to match new engine range
  config.damping = damping * 100.0f;     // Scale to match new engine range
  
  it->second->setTargetSpring(target, config);
  ZENITH_REGISTER_ANIMATION(zenith::animation::Priority::High);
}

void SkiaComponent::stopAnimation(const juce::String &property) {
  auto it = animations_.find(property);
  if (it != animations_.end()) {
    it->second->cancel();
  }
}

void SkiaComponent::stopAllAnimations() {
  for (auto &pair : animations_) {
    pair.second->cancel();
  }
  ZENITH_UNREGISTER_ANIMATION();
  stopTimer(); // Ensure legacy timer is also stopped if running
}

float SkiaComponent::getAnimatedValue(const juce::String &property) const {
  auto it = animations_.find(property);
  return it != animations_.end() ? it->second->get() : 0.0f;
}

bool SkiaComponent::isAnimating(const juce::String &property) const {
  auto it = animations_.find(property);
  return it != animations_.end() && it->second->isAnimating();
}

void SkiaComponent::timerCallback() {
  // Legacy timer support. 
  // We no longer update internal animations here, they use AnimationCoordinator.
  // Subclasses calling startTimerHz() will still have their overrides called.
}

bool SkiaComponent::updateInternalAnimations(float deltaTimeMs) {
  bool anyAnimating = false;
  
  for (auto &pair : animations_) {
    if (pair.second->isAnimating()) {
      pair.second->update(deltaTimeMs);
      anyAnimating = true;
    }
  }

  if (anyAnimating) {
    markDirty();
  }
  
  return anyAnimating;
}

void SkiaComponent::onAnimationTick(float deltaMs) {
  if (!updateInternalAnimations(deltaMs)) {
    ZENITH_UNREGISTER_ANIMATION();
  }
}

// ============================================================================
// CONTEXT MENU & UX
// ============================================================================

void SkiaComponent::showContextMenu() {
  auto menu = ContextMenuManager::createMenu();

  menu->addItem(1, "MIDI Learn", true, isMIDILearning_,
                [this]() { handleContextMenuResult(1); });
  menu->addSeparator();
  menu->addItem(2, "Copy Value", true, false,
                [this]() { handleContextMenuResult(2); });
  menu->addItem(3, "Paste Value", true, false,
                [this]() { handleContextMenuResult(3); });
  menu->addSeparator();
  menu->addItem(4, "Reset to Default", true, false,
                [this]() { handleContextMenuResult(4); });

  ContextMenuManager::getInstance().showMenuAtMouse(std::move(menu));
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
  systemRefreshRate_ = PlatformDisplayUtils::getSystemRefreshRate();
  targetFPS_ = systemRefreshRate_;
  return systemRefreshRate_;
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

  return false;
}

} // namespace zenith

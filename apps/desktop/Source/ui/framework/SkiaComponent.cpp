/*
  ==============================================================================

    SkiaComponent.cpp
    Created: 2025-11-30
    Authors: Sarah Chen, Dr. Aris Vokos, Viktor Volkov, Diego Martinez

    Implementation of base Skia component.
*/

#include "SkiaComponent.h"

#if defined(ZENITH_USE_SKIA) && ZENITH_USE_SKIA
#include "ZenithSkia.h" // Explicitly include
#include <core/SkImageInfo.h>
#include <core/SkPixmap.h>
#include <core/SkSurface.h>
#endif

#include "../../engine/ZenithLogger.h"
#include "PlatformDisplayUtils.h"

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
    ZENITH_LOG_INFO("SkiaComponent: Initializing refresh rate...");
    getSystemRefreshRate();
    ZENITH_LOG_INFO("SkiaComponent: Refresh rate initialized: " +
                    juce::String(systemRefreshRate_));
  }
}

SkiaComponent::~SkiaComponent() { stopAllAnimations(); }

// ============================================================================
// RENDERING
// ============================================================================

void SkiaComponent::paint(juce::Graphics &g) {
#if defined(ZENITH_USE_SKIA) && ZENITH_USE_SKIA
  // Check if we're in the main Skia-integrated window hierarchy
  // If not (e.g., in a DialogWindow), we need to render via raster fallback

  // Walk up the parent chain to find a SkiaMainWindowIntegration
  juce::Component *parent = getParentComponent();
  bool hasSkiaParent = false;
  while (parent != nullptr) {
    // Check if parent is a SkiaComponent that's part of the main rendering
    // The main window (MainComponent) extends SkiaMainWindowIntegration
    // which handles rendering all children via drawSkiaContent()
    if (parent->getParentComponent() == nullptr) {
      // Reached top-level component
      // Check if it's opaque (MainComponent) - if so, assume Skia handles it
      hasSkiaParent = parent->isOpaque();
      break;
    }
    parent = parent->getParentComponent();
  }

  if (hasSkiaParent) {
    // Part of main Skia hierarchy - rendering handled by drawSkia()
    return;
  }

  // RASTER FALLBACK - Used when component is in a standalone window (e.g.,
  // DialogWindow)
  const int width = getWidth();
  const int height = getHeight();

  if (width <= 0 || height <= 0)
    return;

  // Create Raster Surface
  SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
  auto rasterSurface = SkSurfaces::Raster(info);

  if (!rasterSurface) {
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::red);
    g.drawText("Skia Raster Failed", getLocalBounds(),
               juce::Justification::centred, true);
    return;
  }

  SkCanvas *canvas = rasterSurface->getCanvas();
  canvas->clear(SK_ColorTRANSPARENT);

  // Call the virtual drawSkia method
  drawSkia(canvas);

  // Draw children
  drawChildren(canvas);

  // Convert to JUCE Image
  sk_sp<SkImage> img(rasterSurface->makeImageSnapshot());
  if (img) {
    SkPixmap pixmap;
    if (img->peekPixels(&pixmap)) {
      juce::Image juceImage(juce::Image::ARGB, width, height, true);
      juce::Image::BitmapData bd(juceImage, juce::Image::BitmapData::writeOnly);

      if (pixmap.readPixels(SkImageInfo::Make(width, height,
                                              kBGRA_8888_SkColorType,
                                              kPremul_SkAlphaType),
                            bd.data, bd.lineStride)) {
        g.drawImageAt(juceImage, 0, 0);
        needsRepaint_ = false;
        return;
      }
    }
  }

  // Ultimate fallback
  g.fillAll(juce::Colours::darkgrey);
#else
  g.fillAll(juce::Colours::darkgrey);
  g.setColour(juce::Colours::white);
  g.drawText("Skia Disabled", getLocalBounds(), juce::Justification::centred,
             true);
#endif
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
#if defined(ZENITH_USE_SKIA) && ZENITH_USE_SKIA
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
#else
  juce::ignoreUnused(paint, intensity);
#endif
}

#ifdef DEBUG
void SkiaComponent::drawDebug(SkCanvas *canvas) {
#if defined(ZENITH_USE_SKIA) && ZENITH_USE_SKIA
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
#else
  juce::ignoreUnused(canvas);
#endif
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

void SkiaComponent::mouseDown(const juce::MouseEvent &e) {
  onMouseDown(e);
  markDirty();
}

void SkiaComponent::mouseDrag(const juce::MouseEvent &e) {
  onMouseDrag(e);
  markDirty();
}

void SkiaComponent::mouseUp(const juce::MouseEvent &e) {
  onMouseUp(e);
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
#if defined(ZENITH_USE_SKIA) && ZENITH_USE_SKIA
  for (auto *child : getChildren()) {
    if (child->isVisible()) {
      canvas->save();
      canvas->translate((float)child->getX(), (float)child->getY());

      if (auto *skiaChild = dynamic_cast<SkiaComponent *>(child)) {
        // Native Skia component - draw directly
        skiaChild->drawSkia(canvas);
      } else {
        // Regular JUCE component - render via raster fallback
        const int width = child->getWidth();
        const int height = child->getHeight();

        if (width > 0 && height > 0) {
          // Create raster surface for JUCE component
          SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
          auto rasterSurface = SkSurfaces::Raster(info);

          if (rasterSurface) {
            // Create JUCE image and paint the component to it
            juce::Image juceImage(juce::Image::ARGB, width, height, true);
            juce::Graphics g(juceImage);
            child->paintEntireComponent(g, false);

            // Copy JUCE image data to Skia surface
            juce::Image::BitmapData bd(juceImage,
                                       juce::Image::BitmapData::readOnly);
            SkPixmap srcPixmap(SkImageInfo::Make(width, height,
                                                 kBGRA_8888_SkColorType,
                                                 kPremul_SkAlphaType),
                               bd.data, static_cast<size_t>(bd.lineStride));

            // Draw the rasterized JUCE component onto our canvas
            auto skImage = SkImages::RasterFromPixmapCopy(srcPixmap);
            if (skImage) {
              canvas->drawImage(skImage, 0, 0);
            }
          }
        }
      }

      canvas->restore();
    }
  }
#else
  juce::ignoreUnused(canvas);
#endif
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

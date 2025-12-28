# Animation System Usage Guide

This document is for future AI agents working on Zenith DAW. **Read this before touching any UI animation code.**

## The Correct Way

### Namespace
There are **TWO** animation namespaces. Learn them:

1.  `zenith::animation` — Located in `Animation.h`. Contains `AnimatedValue<T>`, `AnimatedColor`, `SpringSolver`. Use for **per-component animated state**.
2.  `zenith::design::animation` — Located in `ZenithDesignSystem.h`. Contains `Animator` (singleton), `Curve`, and duration constants. Use for **fire-and-forget animations triggered by events**.

### Using `Animator` (Singleton)
```cpp
#include "../design-system/ZenithDesignSystem.h"

void MyComponent::mouseEnter(const juce::MouseEvent& e) {
  using namespace zenith::design::animation;
  Animator::getInstance().animate(
      "myComponent_hover", // ID - use unique string
      hoverOpacity_,       // start value
      1.0f,                // end value
      DURATION_FAST,       // 100ms
      Curve::EaseOutQuad,  // easing
      [this](float val) {
          hoverOpacity_ = val;
          repaint();
      }
  );
}
```

### Using `AnimatedValue<T>` (Per-Component State)
```cpp
#include "../framework/Animation.h"

class MyComponent : public SkiaComponent, juce::Timer {
  zenith::animation::AnimatedValue<float> opacity_{0.0f};
  
  void timerCallback() override {
    if (opacity_.update(16.67f)) { // deltaMs
      repaint();
    }
    if (!opacity_.isAnimating()) stopTimer();
  }
  
  void show() {
    opacity_.setTarget(1.0f, 200, zenith::animation::Easing::EaseOut);
    startTimerHz(60);
  }
};
```

## Critical Rules

### 1. NEVER Mix Namespaces
`Easing` is in `zenith::animation`. `Curve` is in `zenith::design::animation`. They are **not interchangeable**.

| Enum | Namespace | Use With |
|------|-----------|----------|
| `Easing::EaseOut` | `zenith::animation` | `AnimatedValue<T>::setTarget()` |
| `Curve::EaseOutQuad` | `zenith::design::animation` | `Animator::animate()` |

### 2. NEVER Capture `this` Dangerously in Callbacks
The `Animator` callback may fire after the component is destroyed.
```cpp
// DANGEROUS
Animator::getInstance().animate("foo", ..., [this](float val) { repaint(); });

// SAFE (preferred for short-lived components)
auto safeThis = juce::Component::SafePointer<MyComponent>(this);
Animator::getInstance().animate("foo", ..., [safeThis](float val) {
    if (safeThis) safeThis->repaint();
});
```

### 3. Use Unique IDs
Animation IDs must be **unique per animation target**. If you have multiple tracks, include the track ID:
```cpp
trackId_ + "_hover"  // GOOD
"hover"               // BAD - all tracks share the same animation!
```

### 4. Cancel Animations on Destruction
```cpp
MyComponent::~MyComponent() {
  zenith::design::animation::Animator::getInstance().cancel(trackId_ + "_hover");
}
```

## Duration Constants (zenith::design::animation)

| Constant | Value | Use Case |
|----------|-------|----------|
| `DURATION_INSTANT` | 0ms | Immediate (no animation) |
| `DURATION_FAST` | 100ms | Hover states, micro-interactions |
| `DURATION_NORMAL` | 200ms | Standard transitions |
| `DURATION_SLOW` | 300ms | Major state changes |
| `DURATION_SLOWER` | 500ms | Modal transitions |

## Recent Refactoring (2025-12-28)

The `Animator` class was refactored to fix UI freezes:
- IDs are now `juce::Identifier` (interned strings), not `juce::String`.
- The `timerCallback` releases the mutex before calling user callbacks.
- This prevents deadlocks when `repaint()` is called from within an animation callback.

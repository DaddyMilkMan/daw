## 2024-05-22 - [Custom Cursor Feedback]
**Learning:** Custom-drawn UI elements (using Skia or `paint()`) do not inherit standard OS mouse cursors (like the pointing hand) automatically. Users rely on cursor changes to know if an element is clickable.
**Action:** When creating custom interactive regions that are not full Components, always manually handle `mouseMove` to set `PointingHandCursor` over clickable zones.

## 2025-05-23 - [ZenithButton Accessibility]
**Learning:** `ZenithButton` inherits from `SkiaComponent` (and thus `juce::Component`) but NOT `juce::Button`. This means it lacks default accessibility support (ARIA roles, actions) provided by JUCE's button classes. It was invisible to screen readers as a button.
**Action:** When creating custom controls in Zenith UI that mimic standard controls, explicitly override `createAccessibilityHandler` to provide `juce::AccessibilityHandler` implementation with correct Role, Actions, and State.

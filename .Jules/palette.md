## 2024-05-22 - [Custom Cursor Feedback]
**Learning:** Custom-drawn UI elements (using Skia or `paint()`) do not inherit standard OS mouse cursors (like the pointing hand) automatically. Users rely on cursor changes to know if an element is clickable.
**Action:** When creating custom interactive regions that are not full Components, always manually handle `mouseMove` to set `PointingHandCursor` over clickable zones.

## 2025-12-14 - [Skia Component Accessibility Gap]
**Learning:** Components inheriting from `SkiaComponent` (like `ZenithButton`) do not inherit standard accessibility behaviors (role, name, actions) found in `juce::Button`. They require a custom `AccessibilityHandler` implementation.
**Action:** Always implement `createAccessibilityHandler()` for interactive `SkiaComponent` subclasses, ensuring proper Title (with tooltip fallback), Role, and Actions are exposed.

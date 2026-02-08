## 2024-05-22 - [Custom Cursor Feedback]
**Learning:** Custom-drawn UI elements (using Skia or `paint()`) do not inherit standard OS mouse cursors (like the pointing hand) automatically. Users rely on cursor changes to know if an element is clickable.
**Action:** When creating custom interactive regions that are not full Components, always manually handle `mouseMove` to set `PointingHandCursor` over clickable zones.

## 2025-02-23 - [Keyboard Accessibility Gaps in Custom Controls]
**Learning:** `SkiaComponent` and `ZenithControl` implement mouse interactions heavily but lack default implementations for keyboard navigation (Arrow keys). This requires subclasses like `ZenithSlider` to manually implement `keyPressed(const KeyPress&)` to support standard accessibility patterns.
**Action:** When creating new `ZenithControl` subclasses, always verify if `keyPressed` needs implementation for keyboard accessibility (arrow keys, modifiers).

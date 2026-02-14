## 2024-05-22 - [Custom Cursor Feedback]
**Learning:** Custom-drawn UI elements (using Skia or `paint()`) do not inherit standard OS mouse cursors (like the pointing hand) automatically. Users rely on cursor changes to know if an element is clickable.
**Action:** When creating custom interactive regions that are not full Components, always manually handle `mouseMove` to set `PointingHandCursor` over clickable zones.

## 2025-05-27 - [Keyboard Accessibility for Custom Controls]
**Learning:** Custom Skia-based controls (like `ZenithToggle`) often miss standard keyboard interactions (Space/Return) and focus rings, making them inaccessible to keyboard users. Inheriting from a base component (`SkiaComponent`) isn't enough if the base doesn't implement the specific control logic.
**Action:** Always verify `keyPressed` implementation and visual focus states (`hasKeyboardFocus`) when creating or reviewing custom UI components.

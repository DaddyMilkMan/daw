## 2024-05-22 - [Custom Cursor Feedback]
**Learning:** Custom-drawn UI elements (using Skia or `paint()`) do not inherit standard OS mouse cursors (like the pointing hand) automatically. Users rely on cursor changes to know if an element is clickable.
**Action:** When creating custom interactive regions that are not full Components, always manually handle `mouseMove` to set `PointingHandCursor` over clickable zones.

## 2025-12-14 - [Custom Button Accessibility]
**Learning:** Custom components inheriting `SkiaComponent` (or `juce::Component`) have zero accessibility by default. Screen readers see nothing.
**Action:** Always implement `createAccessibilityHandler()` for custom controls, exposing role, title, and actions (Press/Toggle). Also override `keyPressed` for Space/Enter support.

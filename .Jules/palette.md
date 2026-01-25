## 2024-05-22 - [Custom Cursor Feedback]
**Learning:** Custom-drawn UI elements (using Skia or `paint()`) do not inherit standard OS mouse cursors (like the pointing hand) automatically. Users rely on cursor changes to know if an element is clickable.
**Action:** When creating custom interactive regions that are not full Components, always manually handle `mouseMove` to set `PointingHandCursor` over clickable zones.

## 2025-12-14 - [JUCE Accessibility Handlers]
**Learning:** Custom components in JUCE require manual implementation of `createAccessibilityHandler`. The API has evolved: `AccessibilityHandler` constructor now requires passing Role and Actions upfront (unlike older versions where virtual methods were used). `AccessibleState` (not `AccessibilityState`) uses a builder pattern (`withChecked()`, `withFocusable()`) instead of struct fields.
**Action:** When making custom components accessible, define a helper `AccessibilityHandler` subclass that calculates Role and Actions in its constructor or a static helper, and use `AccessibleState` correctly in `getCurrentState`.

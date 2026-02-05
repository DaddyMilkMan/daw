## 2024-05-22 - [Custom Cursor Feedback]
**Learning:** Custom-drawn UI elements (using Skia or `paint()`) do not inherit standard OS mouse cursors (like the pointing hand) automatically. Users rely on cursor changes to know if an element is clickable.
**Action:** When creating custom interactive regions that are not full Components, always manually handle `mouseMove` to set `PointingHandCursor` over clickable zones.

## 2024-05-23 - [Tooltips for Skia Components]
**Learning:** Single-component UIs drawing multiple "virtual" controls (like TransportBar) prevent standard tooltip attachment. Tooltips must be handled by overriding `getTooltip()` and manually hit-testing against the mouse position.
**Action:** Implement `getTooltip()` using `getMouseXYRelative()` and bounds checking for all custom-drawn interactive zones.

## 2025-10-27 - Transport Bar Focus Visibility
**Learning:** JUCE Components painted via custom Skia renderers often lose their native focus ring visualization. Inherited `juce::Button` behavior for focus is invisible if `paintButton` is empty.
**Action:** Always check `hasKeyboardFocus` in the Skia drawing loop and use `InteractionHelper::drawFocusRing` to restore accessibility.

## 2025-12-14 - [JUCE Accessibility Handlers & Skia Components]
**Learning:** Custom components (especially those inheriting from `SkiaComponent`) require manual implementation of `createAccessibilityHandler`. The API has evolved: `AccessibilityHandler` constructor now requires passing Role and Actions upfront. `AccessibleState` (not `AccessibilityState`) uses a builder pattern (`withChecked()`, `withFocusable()`).
**Action:** Always implement `createAccessibilityHandler()` for interactive `SkiaComponent` subclasses. Define a helper `AccessibilityHandler` subclass that calculates Role and Actions in its constructor and use `AccessibleState` correctly.

## 2025-12-14 - [Safe Component Callbacks]
**Learning:** In UI components, input handlers (like `keyPressed` or Accessibility actions) often trigger callbacks (like `onClick`) that may result in the immediate destruction of the component (e.g., closing a dialog). Accessing member variables or scheduling timers after invoking these callbacks leads to Use-After-Free crashes.
**Action:** Always use `juce::Component::SafePointer` to guard access to `this` after invoking user-supplied callbacks or when scheduling asynchronous UI updates.

## 2025-12-15 - [Accessibility Action Consistency]
**Learning:** Accessibility actions (e.g. 'press') must mirror the logic order of standard input handlers (e.g. `mouseDown`). Specifically, for toggle buttons, the state change should occur *before* the `onClick` callback to ensure the callback observes the new state. Inconsistencies here cause bugs where screen readers and mouse clicks produce different application states.
**Action:** When implementing `AccessibilityHandler::createActions`, verify that the lambda logic exactly matches the `mouseUp` or `keyPressed` logic, including order of operations.

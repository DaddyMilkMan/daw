## 2024-05-22 - [Custom Cursor Feedback]
**Learning:** Custom-drawn UI elements (using Skia or `paint()`) do not inherit standard OS mouse cursors (like the pointing hand) automatically. Users rely on cursor changes to know if an element is clickable.
**Action:** When creating custom interactive regions that are not full Components, always manually handle `mouseMove` to set `PointingHandCursor` over clickable zones.

## 2025-12-14 - [Safe Component Callbacks]
**Learning:** In UI components, input handlers (like `keyPressed` or Accessibility actions) often trigger callbacks (like `onClick`) that may result in the immediate destruction of the component (e.g., closing a dialog). Accessing member variables or scheduling timers after invoking these callbacks leads to Use-After-Free crashes.
**Action:** Always use `juce::Component::SafePointer` to guard access to `this` after invoking user-supplied callbacks or when scheduling asynchronous UI updates.

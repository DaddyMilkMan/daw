## 2024-05-22 - [Custom Cursor Feedback]
**Learning:** Custom-drawn UI elements (using Skia or `paint()`) do not inherit standard OS mouse cursors (like the pointing hand) automatically. Users rely on cursor changes to know if an element is clickable.
**Action:** When creating custom interactive regions that are not full Components, always manually handle `mouseMove` to set `PointingHandCursor` over clickable zones.

## 2024-05-23 - [Tooltips for Custom Regions]
**Learning:** Custom-drawn UI regions (like buttons inside a single Component) are invisible to the standard tooltip system unless `juce::TooltipClient` is implemented.
**Action:** Implement `TooltipClient` and override `getTooltip()` for monolithic components to provide context-sensitive tooltips for their internal interactive zones.

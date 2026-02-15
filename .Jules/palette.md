## 2024-05-22 - [Custom Cursor Feedback]
**Learning:** Custom-drawn UI elements (using Skia or `paint()`) do not inherit standard OS mouse cursors (like the pointing hand) automatically. Users rely on cursor changes to know if an element is clickable.
**Action:** When creating custom interactive regions that are not full Components, always manually handle `mouseMove` to set `PointingHandCursor` over clickable zones.

## 2025-05-27 - [Dynamic Tooltips for Custom Components]
**Learning:** For components that draw multiple interactive elements internally (like `TransportBar`), standard component tooltips are static. To provide context-sensitive help, the tooltip text must be dynamically updated in `mouseMove` based on the hovered region.
**Action:** Use `setTooltip(newText)` inside `mouseMove` when hovering virtual controls, and ensure `juce::TooltipWindow` exists at the application root to display them.

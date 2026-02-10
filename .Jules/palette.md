## 2024-05-22 - [Custom Cursor Feedback]
**Learning:** Custom-drawn UI elements (using Skia or `paint()`) do not inherit standard OS mouse cursors (like the pointing hand) automatically. Users rely on cursor changes to know if an element is clickable.
**Action:** When creating custom interactive regions that are not full Components, always manually handle `mouseMove` to set `PointingHandCursor` over clickable zones.

## 2024-05-24 - [Disabled State Cursors]
**Learning:** Users can be confused if disabled buttons still show a pointing hand cursor. It implies interactivity where none exists.
**Action:** Always revert the cursor to `NormalCursor` (or default) when a component is disabled, using `enablementChanged()` to catch state updates.

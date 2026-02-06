## 2024-05-22 - [Custom Cursor Feedback]
**Learning:** Custom-drawn UI elements (using Skia or `paint()`) do not inherit standard OS mouse cursors (like the pointing hand) automatically. Users rely on cursor changes to know if an element is clickable.
**Action:** When creating custom interactive regions that are not full Components, always manually handle `mouseMove` to set `PointingHandCursor` over clickable zones.

## 2024-05-23 - [Keyboard Accessibility in Skia Components]
**Learning:** `SkiaComponent` handles `returnKey` via `onEnterPressed` but does not handle `spaceKey` by default. Standard accessibility requires buttons to trigger on both.
**Action:** When inheriting from `SkiaComponent` for button-like controls, explicitly override `keyPressed` to handle `spaceKey` (and `returnKey` if `onEnterPressed` is not sufficient) before delegating to the base class.

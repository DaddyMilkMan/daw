# Zenith UI/UX Design Specification

> **Source of Truth**: This doc is the single source of truth for visual + interaction rules. If code and this spec disagree, the spec wins and code should be fixed.

## 1. Visual Language

Zenith aims for a "Premium, Modern, and Focused" aesthetic. The interface should feel invisible, letting the music take center stage.

### 1.1. Color Palette

We use a strict semantic color system. **Do not hardcode hex values**; use the `SkiaTheme` or `ZenithLookAndFeel` tokens.

**Code Mapping Table**:

| Spec Token | Code Token (`SkiaTheme`) | Usage |
| :--- | :--- | :--- |
| `bg-app` | `bg0` | Main application background |
| `bg-panel` | `bg1` | Component backgrounds (Browser, Inspector) |
| `bg-surface` | `bg2` | Cards, inputs, active regions |
| `accent-primary` | `accentMain` | Primary actions, selection highlight |
| `accent-secondary` | `accentSecondary` | Success, safe states, playback active |
| `accent-warn` | `accentWarn` | Warnings, solo state |
| `accent-danger` | `accentRecord` | Destructive actions, recording, mute state |
| `text-primary` | `textStrong` | Main headings, active text |
| `text-secondary` | `textMuted` | Labels, secondary info |
| `border-subtle` | `borderSubtle` | Dividers, inactive borders |

**Contrast Rule**: All text vs background pairs must meet **WCAG 2.1 AA** (4.5:1 for normal text, 3:1 for large text). Key UI components (buttons, sliders) should meet 3:1 contrast against their background.

### 1.2. Typography

Font Family: **Inter** (fallback to system sans-serif).

**Code Mapping Table**:

| Spec Scale | Code Name (`SkiaTheme::Typography`) | Size | Usage |
| :--- | :--- | :--- | :--- |
| `Display` | *(Not used yet)* | 24px | Empty state headers |
| `H1` | `title` | 18px | Panel titles |
| `H2` | `header` | 14px | Section headers |
| `Body` | `body` | 12px | Default UI text |
| `Small` | `small` | 10px | Metadata, ruler labels |
| `Tiny` | `tiny` | 9px | Micro-labels |
| `Mono` | *(System Mono)* | 11px | Timecode, values only |

### 1.3. Spacing & Layout

- **Base Unit**: 8px.
- **Margins/Padding**: 4px (xs), 8px (sm), 16px (md), 24px (lg).
- **Corner Radius**:
    - Small: 4px (Inputs, Buttons)
    - Large: 8px (Modals, Cards)

## 2. Implementation Checklist

If you add a new Skia component, you **MUST**:

- [ ] Use `SkiaTheme` for colors, typography, and selection; **no hardcoded values**.
- [ ] Use the **8px spacing grid** for all layout calculations.
- [ ] Use `InteractionColors` for hover, active, and focus states.
- [ ] **Check Contrast**: Ensure text/controls meet WCAG AA.
- [ ] **Tooltips**: All non-obvious interactive elements MUST have a tooltip. Simple buttons with clear icons/labels MAY omit them.

## 3. Component Specifications

### 3.1. Transport Bar (`TransportBar`)
- **Height**: 60px (current implementation); internal padding/controls must snap to 8px grid.
- **Elements**:
    - **Left**: Undo/Redo, Save.
    - **Center**: Play (Space), Stop, Record (R), Loop (L).
    - **Right**: Master Meter (Implemented), CPU Load (Planned), MIDI Activity (Planned).
- **Behavior**:
    - Play button toggles to Pause when playing.
    - **Record**: Uses `accent-danger` (`accentRecord`). May pulse at ~1-2Hz. Implementation must ensure no extra CPU/allocations per frame.

### 3.2. Arranger View (`ArrangerComponent`)
- **Track Headers**:
    - Left side. Resizable width.
    - **Current**: Track Name, Mute (M), Solo (S), Arm (R).
    - **Roadmap**: Volume Slider and Pan Knob (may be added later).
    - **Selection**: Click to select track. Shift+Click for range. Cmd/Ctrl+Click for toggle.
- **Timeline**:
    - **Ruler**: Top. Shows Bars/Beats or Time.
    - **Grid**: Vertical lines based on zoom level and snap setting.
    - **Playhead**: Vertical line, `accent-primary` color. Follows playback.
- **Clips**:
    - **Audio Clips**: Show waveform cache.
    - **MIDI Clips**: Show miniature note data.
    - **Interaction**:
        - **Hover**: Show resize handles at edges.
        - **Drag**: Move in time or between tracks.
        - **Double Click**: Open in Bottom Panel.

### 3.3. Piano Roll (`PianoRollEditor`)
- **Grid**:
    - Horizontal: Time (sync with Arranger).
    - Vertical: Pitch (Piano keys on left).
- **Notes**:
    - Rectangles. Color matches Track color.
    - **Velocity**: Currently opacity-based. Dedicated lane design is TBD.
- **Navigation**:
    - Mouse Wheel: Scroll vertical.
    - Shift + Wheel: Scroll horizontal.
    - Ctrl/Cmd + Wheel: Zoom.
    - **Constraint**: Vertical zoom must maintain key labels readable; clamp min row height.

### 3.4. Mixer (`MixerComponent`)
> **Status**: Future State / Not Yet Implemented in Skia path.

- **Target Design**:
    - Vertical Channel Strips.
    - Long throw faders (100px+).
    - Inserts/Sends racks.
    - Visuals: Mute (`accent-danger`), Solo (`accent-warn`).

## 4. Interaction Patterns

### 4.1. Selection States
- **Implementation**: Must use `SkiaTheme::SelectionStyle` and `InteractionColors`.
- **Visuals**:
    - **Unselected**: Default state.
    - **Hover**: `bg-surface` lighten 5%.
    - **Selected**: Border `accent-primary`. Background tint `accent-primary` (10% opacity).
- **Focus vs Selection**:
    - A component can be selected but not focused.
    - **Focus Ring**: Key component (e.g., Arranger vs Mixer) must have a distinguishable border/glow (3:1 contrast) when it has keyboard focus.

### 4.2. Mouse Actions
- **Left Click**: Select.
- **Right Click**: Context Menu.
- **Double Click**: Default primary action.
- **Drag**: Move object.
    - **Shift + Drag**: Fine adjustment.
    - **Ctrl/Cmd + Drag**: Lasso select (in empty space).
    - **Right Drag**: Current: No-op. Scrub behaviour will be specified if/when implemented.

### 4.3. Automation
- **Performance**: Curve evaluation must be O(segments visible), not brute-forced per sample in UI. Never introduce denormals.
- **Interaction**: Click to add node, double-click to remove. Drag line for Bezier curve.

## 5. Accessibility

### 5.1. Standards
- **Contrast**: All text must meet WCAG AA (4.5:1 normal, 3:1 large). UI components must meet 3:1.
- **Tooltips**: Required for all keyboard-accessible controls without visible text labels.

### 5.2. Keyboard Shortcuts
**Global Rule**: Transport shortcuts (Space, Enter, R) are reserved globally. No component may override them without a very strong reason.

- **Transport**:
    - `Space`: Play/Pause.
    - `Enter`: Stop / Return to Start.
    - `R`: Record.
- **Track Navigation**:
    - `Up/Down`: Move track selection.
    - `Left/Right`: Collapse/Expand track folder (if applicable).

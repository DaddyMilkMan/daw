# Zenith DAW Design Specification v1.0

## 1. Design Philosophy
**Target:** Ableton’s clarity + Bitwig’s modern dark look + Studio One’s timeline ergonomics + Reaper-level theming, **without** the clutter.

### Core Principles
1.  **Dark, Neutral Base:** Structural elements are shades of grey. Color is reserved for meaning (track types, state, musical data).
2.  **High Information Density, Low Noise:** No gratuitous effects. 8px spacing grid.
3.  **Always-Visible Global Structure:** Mini-map, selected track, active mode always visible.
4.  **First-Class Theming:** Expose controls for contrast, brightness, and color intensity.

---

## 2. Color System (Design Tokens)

### Neutrals
| Token | Hex | Usage |
|-------|-----|-------|
| `bg0` | `#050608` | App background |
| `bg1` | `#111418` | Primary panel background |
| `bg2` | `#181C22` | Elevated panel / headers |
| `bg3` | `#1F242C` | Controls / input fields / mixer strips |
| `border-subtle` | `#2A313A` | Dividers, grid lines |
| `border-strong` | `#3A4450` | Active borders, strong dividers |

### Text
| Token | Hex | Usage |
|-------|-----|-------|
| `text-strong` | `#F8FAFF` | Primary text, headers |
| `text-muted` | `#A4ACBA` | Secondary text, labels |
| `text-subtle` | `#6C7380` | Disabled text, placeholders |
| `text-danger` | `#FF5C5C` | Error text |

### Accents
| Token | Hex | Usage |
|-------|-----|-------|
| `accent-main` | `#00D4AA` | Primary brand color (Teal) |
| `accent-alt` | `#4C8DFF` | Selection / Focus (Blue) |
| `accent-record` | `#FF3B30` | Record / Armed (Red) |
| `accent-warning` | `#FFC857` | Warnings (Yellow) |

### Musical Data (Clip Palette)
*Base hues to be blended ~60% over `bg1` for track backgrounds.*
*   **Drums:** `#FF8A65`
*   **Bass:** `#FFD54F`
*   **Harmony:** `#81C784`
*   **Leads:** `#64B5F6`
*   **FX:** `#BA68C8`

---

## 3. Typography & Icons
*   **Font:** Inter / SF Pro / Noto Sans (Geometric Sans)
*   **Sizes:**
    *   **H1 (App Title):** 16-18px Semibold
    *   **H2 (Panel Headers):** 13-14px Semibold, tracked slightly
    *   **Body:** 12px Regular
    *   **Meta:** 10px Regular
*   **Icons:** Single-stroke, 2px lines, rounded caps. Solid monochrome (white on dark).

---

## 4. Layout & Zones

### Global Layout
*   **Top Strip (60px):** Global Bar (Transport, Status, Wingman)
*   **Left Sidebar (260px):** Browser (Devices, Clips, Files)
*   **Center:** Arrange View (Tracks + Ruler + Automation) + Bottom Editor (Piano Roll)
*   **Right:** Scratch Pad / Wingman Stack
*   **Bottom:** Hybrid Transport / Keybed / Micro-Mixer

### 4.1 Transport Bar
*   **Height:** 56-60px
*   **Background:** `bg2`
*   **Left:** Project Name, Session Dropdown
*   **Center:** Play/Stop/Rec (32px pill/circle), Tempo, Time Sig, Position Readout
*   **Right:** CPU/DSP, Wingman Status

### 4.2 Track Headers
*   **Width:** Fixed ~220px
*   **Row Height:** 28-32px
*   **Style:** `bg1` background. Track color as 3-4px left bar.
*   **Controls:** [Color] [Name] [Arm] [Solo] [Mute] [Monitor] [FX] [I/O]

### 4.3 Arrange View
*   **Background:** `bg1`
*   **Grid:** Bar lines (`border-subtle`), Beat lines (`border-subtle` @ 40%)
*   **Clips:** Rounded rects (4px radius), slight vertical gradient.
*   **Selection:** Double border (inner accent, outer neutral).

### 4.4 Scratch Pads (Right Panel)
*   **Concept:** Alternate timelines.
*   **Visuals:** Desaturated track colors (30-40% less). Distinct ruler tint.

### 4.5 Wingman Console
*   **Background:** `bg2`
*   **Header:** Title + Mode Pills [Chat] [Actions] [History]
*   **Body:** Muted bubbles. Skia-rendered diff previews for actions.
*   **Input:** Monospaced command line.

### 4.6 Mixer Strip
*   **Background:** `bg3` (Darkest)
*   **Style:** Color strip top, meter next to fader.

### 4.7 Piano Roll
*   **Keys:** Flat. White (`#E3E6EB`), Black (`#252A33`).
*   **Notes:** Track color fill. Glow on playhead.

---

## 5. Theming Engine
*   **Global:** Gamma, Brightness, Contrast sliders.
*   **Tracks:** Color strength slider (Bar only -> Tinted Strip -> Full Row).
*   **Selection:** Style toggle (Border+Dot / Full Row / Underline).
*   **Palette:** 16-swatch palette with "Recolor Project" action.

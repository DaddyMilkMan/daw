# Ultimate Piano Roll - User Manual

## Overview
The Ultimate Piano Roll is a professional-grade MIDI editor designed for speed, precision, and creativity. It combines standard editing workflows with advanced features like velocity curves, smart duplication, and scale highlighting.

## Key Features

### 🎹 Core Editing
- **Smart Tool**: Automatically switches between move, resize, and selection based on cursor position.
- **Multi-Selection**: Select multiple notes with `Ctrl/Cmd + Click` or Marquee drag.
- **Velocity Lane**: Edit note velocities with visual feedback (color gradients from blue to red).
- **Batched Undo**: Complex operations like "Delete 50 notes" are a single undo step.

### 🚀 Advanced Workflows
- **Smart Duplicate (`Ctrl/Cmd + D`)**: Duplicates selected notes and automatically advances the position by the selection length. Perfect for creating repetitive patterns quickly.
- **Quantize (`Q`)**: Snaps notes to the grid. Supports "Swing" and "Strength" parameters for humanized feel.
- **Humanize Velocity (`H`)**: Adds subtle random variations to velocity for a more natural sound.
- **Scale Highlighting**: Visualizes scale notes on the piano roll background. Never hit a wrong note again!
- **Chord Detection**: Real-time display of the chord formed by selected notes (e.g., "C Maj7", "G Dim").

### 🎨 Visuals
- **Velocity Coloring**: Notes change color based on velocity (Blue = Soft, Green = Medium, Red = Loud).
- **Context Cursors**: Clear visual feedback for resizing, moving, and editing.
- **Chord Overlay**: Shows the current chord name in the top-right corner.

## Keyboard Shortcuts

| Action | Shortcut | Description |
|--------|----------|-------------|
| **Select All** | `Ctrl + A` | Selects all notes in the clip |
| **Copy** | `Ctrl + C` | Copies selected notes to clipboard |
| **Paste** | `Ctrl + V` | Pastes notes at the playhead or view start |
| **Cut** | `Ctrl + X` | Copies and deletes selected notes |
| **Duplicate** | `Ctrl + D` | Smart duplicate (repeats pattern) |
| **Delete** | `Del` / `Backspace` | Deletes selected notes |
| **Undo** | `Ctrl + Z` | Undo last action |
| **Redo** | `Ctrl + Shift + Z` | Redo last action |
| **Quantize** | `Q` | Snap selected notes to grid |
| **Humanize** | `H` | Randomize velocity slightly |
| **Mute/Unmute** | `Ctrl + M` | Toggle mute state of selected notes |
| **Invert Selection** | `Ctrl + I` | Selects all unselected notes |
| **Zoom In** | `+` / `=` | Zoom in horizontally |
| **Zoom Out** | `-` | Zoom out horizontally |
| **Scroll** | `Shift + Wheel` | Horizontal scroll |
| **Zoom** | `Ctrl + Wheel` | Horizontal zoom |
| **Vertical Zoom** | `Alt + Wheel` | Vertical zoom |

## Mouse Actions

- **Left Click**: Select note / Move note
- **Left Drag (Background)**: Marquee select
- **Left Drag (Note Center)**: Move note(s)
- **Left Drag (Note Edge)**: Resize note
- **Double Click**: Delete note (or create if on background)
- **Ctrl + Click**: Add to selection
- **Velocity Lane Drag**: Edit velocity

## Tips & Tricks
1. **Create Rolls**: Use the "Create Roll" feature (API only currently) to instantly create 1/16th note rolls.
2. **Velocity Curves**: Select a group of notes and apply curves (Ramp Up/Down) for expressive swells.
3. **Chord Building**: Use the Chord Detection overlay to learn new voicings.

/*
  ==============================================================================

    ZenithIcons.h
    Created: 2025-12-12
    Author:  Leo "Lil Bit" Rossi & AI Assistant

    Professional vector icon system for Zenith DAW.
    Replaces Unicode glyphs with crisp, scalable SkPath icons.

    All icons are defined in a 24x24 viewport and scale cleanly to any size.
    "If it doesn't scale, it doesn't ship!" - Leo

  ==============================================================================
*/

#pragma once

#include <core/SkBlurTypes.h>
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkMaskFilter.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>

namespace zenith {
namespace icons {

// ============================================================================
// CONSTANTS
// ============================================================================

// Standard icon viewport (like Material/Phosphor icons)
constexpr float ICON_VIEWPORT = 24.0f;

// Default stroke widths for different icon sizes
constexpr float STROKE_THIN = 1.5f;
constexpr float STROKE_REGULAR = 2.0f;
constexpr float STROKE_BOLD = 2.5f;

// ============================================================================
// ICON STYLE
// ============================================================================

struct IconStyle {
  float strokeWidth = STROKE_REGULAR;
  SkColor color = SK_ColorWHITE;
  bool filled = false;
  float glowRadius = 0.0f;        // 0 = no glow
  SkColor glowColor = 0x00000000; // Transparent = use main color
};

// ============================================================================
// TRANSPORT ICONS
// ============================================================================

/** Play triangle - right-pointing arrow */
inline SkPath Play() {
  SkPath path;
  // Triangle in 24x24 viewport, slightly offset for optical centering
  path.moveTo(7.0f, 5.0f);
  path.lineTo(19.0f, 12.0f);
  path.lineTo(7.0f, 19.0f);
  path.close();
  return path;
}

/** Pause - two vertical bars */
inline SkPath Pause() {
  SkPath path;
  // Left bar
  path.addRect(SkRect::MakeLTRB(6.0f, 5.0f, 10.0f, 19.0f));
  // Right bar
  path.addRect(SkRect::MakeLTRB(14.0f, 5.0f, 18.0f, 19.0f));
  return path;
}

/** Stop - square */
inline SkPath Stop() {
  SkPath path;
  path.addRect(SkRect::MakeLTRB(6.0f, 6.0f, 18.0f, 18.0f));
  return path;
}

/** Record - filled circle */
inline SkPath Record() {
  SkPath path;
  path.addCircle(12.0f, 12.0f, 7.0f);
  return path;
}

/** Fast Forward - two right-pointing triangles */
inline SkPath FastForward() {
  SkPath path;
  // First triangle
  path.moveTo(4.0f, 5.0f);
  path.lineTo(12.0f, 12.0f);
  path.lineTo(4.0f, 19.0f);
  path.close();
  // Second triangle
  path.moveTo(12.0f, 5.0f);
  path.lineTo(20.0f, 12.0f);
  path.lineTo(12.0f, 19.0f);
  path.close();
  return path;
}

/** Rewind - two left-pointing triangles */
inline SkPath Rewind() {
  SkPath path;
  // First triangle
  path.moveTo(12.0f, 5.0f);
  path.lineTo(4.0f, 12.0f);
  path.lineTo(12.0f, 19.0f);
  path.close();
  // Second triangle
  path.moveTo(20.0f, 5.0f);
  path.lineTo(12.0f, 12.0f);
  path.lineTo(20.0f, 19.0f);
  path.close();
  return path;
}

/** Skip Forward - triangle with end bar */
inline SkPath SkipForward() {
  SkPath path;
  // Triangle
  path.moveTo(5.0f, 5.0f);
  path.lineTo(15.0f, 12.0f);
  path.lineTo(5.0f, 19.0f);
  path.close();
  // End bar
  path.addRect(SkRect::MakeLTRB(17.0f, 5.0f, 19.0f, 19.0f));
  return path;
}

/** Skip Back - bar with triangle */
inline SkPath SkipBack() {
  SkPath path;
  // Start bar
  path.addRect(SkRect::MakeLTRB(5.0f, 5.0f, 7.0f, 19.0f));
  // Triangle
  path.moveTo(19.0f, 5.0f);
  path.lineTo(9.0f, 12.0f);
  path.lineTo(19.0f, 19.0f);
  path.close();
  return path;
}

/** Loop - circular arrow (stroked) */
inline SkPath Loop() {
  SkPath path;
  // Main circle arc (about 300 degrees)
  SkRect oval = SkRect::MakeLTRB(5.0f, 5.0f, 19.0f, 19.0f);
  path.arcTo(oval, -60.0f, 300.0f, true);
  // Arrow head at the end
  path.moveTo(17.0f, 4.0f);
  path.lineTo(20.0f, 7.0f);
  path.lineTo(14.0f, 7.0f);
  return path;
}

/** Metronome/Click track */
inline SkPath Metronome() {
  SkPath path;
  // Triangle base
  path.moveTo(6.0f, 20.0f);
  path.lineTo(18.0f, 20.0f);
  path.lineTo(12.0f, 4.0f);
  path.close();
  // Pendulum arm (angled line)
  path.moveTo(12.0f, 8.0f);
  path.lineTo(16.0f, 4.0f);
  return path;
}

// ============================================================================
// UI ICONS
// ============================================================================

/** Settings gear - 8-tooth cog with center hole (stroked) */
inline SkPath Settings() {
  SkPath path;
  // Simplified gear: outer circle with notches
  const float cx = 12.0f;
  const float cy = 12.0f;
  const float outerR = 9.0f;
  const float innerR = 7.0f;
  const float toothDepth = 2.5f;
  const int teeth = 8;

  for (int i = 0; i < teeth; ++i) {
    float angle1 = (i * 360.0f / teeth) * (3.14159265f / 180.0f);
    float angle2 = ((i + 0.3f) * 360.0f / teeth) * (3.14159265f / 180.0f);
    float angle3 = ((i + 0.7f) * 360.0f / teeth) * (3.14159265f / 180.0f);
    float angle4 = ((i + 1.0f) * 360.0f / teeth) * (3.14159265f / 180.0f);

    float x1 = cx + outerR * cosf(angle1);
    float y1 = cy + outerR * sinf(angle1);
    float x2 = cx + (outerR + toothDepth) * cosf(angle2);
    float y2 = cy + (outerR + toothDepth) * sinf(angle2);
    float x3 = cx + (outerR + toothDepth) * cosf(angle3);
    float y3 = cy + (outerR + toothDepth) * sinf(angle3);
    float x4 = cx + outerR * cosf(angle4);
    float y4 = cy + outerR * sinf(angle4);

    if (i == 0) {
      path.moveTo(x1, y1);
    } else {
      path.lineTo(x1, y1);
    }
    path.lineTo(x2, y2);
    path.lineTo(x3, y3);
    path.lineTo(x4, y4);
  }
  path.close();

  // Center hole
  path.addCircle(cx, cy, 3.0f, SkPathDirection::kCCW);

  return path;
}

/** Menu - hamburger icon (3 horizontal lines) */
inline SkPath Menu() {
  SkPath path;
  path.addRect(SkRect::MakeLTRB(4.0f, 6.0f, 20.0f, 8.0f));
  path.addRect(SkRect::MakeLTRB(4.0f, 11.0f, 20.0f, 13.0f));
  path.addRect(SkRect::MakeLTRB(4.0f, 16.0f, 20.0f, 18.0f));
  return path;
}

/** Close - X mark (stroked) */
inline SkPath Close() {
  SkPath path;
  path.moveTo(6.0f, 6.0f);
  path.lineTo(18.0f, 18.0f);
  path.moveTo(18.0f, 6.0f);
  path.lineTo(6.0f, 18.0f);
  return path;
}

/** Plus - addition symbol (stroked) */
inline SkPath Plus() {
  SkPath path;
  path.moveTo(12.0f, 4.0f);
  path.lineTo(12.0f, 20.0f);
  path.moveTo(4.0f, 12.0f);
  path.lineTo(20.0f, 12.0f);
  return path;
}

/** Minus - subtraction symbol (stroked) */
inline SkPath Minus() {
  SkPath path;
  path.moveTo(5.0f, 12.0f);
  path.lineTo(19.0f, 12.0f);
  return path;
}

/** Search - magnifying glass (stroked) */
inline SkPath Search() {
  SkPath path;
  // Circle
  path.addCircle(10.0f, 10.0f, 6.0f);
  // Handle
  path.moveTo(14.5f, 14.5f);
  path.lineTo(20.0f, 20.0f);
  return path;
}

/** View Toggle - tab/switch icon */
inline SkPath ViewToggle() {
  SkPath path;
  // Two overlapping rectangles
  path.addRoundRect(SkRect::MakeLTRB(4.0f, 6.0f, 14.0f, 14.0f), 2.0f, 2.0f);
  path.addRoundRect(SkRect::MakeLTRB(10.0f, 10.0f, 20.0f, 18.0f), 2.0f, 2.0f);
  return path;
}

// ============================================================================
// NAVIGATION ICONS (CHEVRONS)
// ============================================================================

/** Chevron Down (stroked) */
inline SkPath ChevronDown() {
  SkPath path;
  path.moveTo(6.0f, 9.0f);
  path.lineTo(12.0f, 15.0f);
  path.lineTo(18.0f, 9.0f);
  return path;
}

/** Chevron Up (stroked) */
inline SkPath ChevronUp() {
  SkPath path;
  path.moveTo(6.0f, 15.0f);
  path.lineTo(12.0f, 9.0f);
  path.lineTo(18.0f, 15.0f);
  return path;
}

/** Chevron Left (stroked) */
inline SkPath ChevronLeft() {
  SkPath path;
  path.moveTo(15.0f, 6.0f);
  path.lineTo(9.0f, 12.0f);
  path.lineTo(15.0f, 18.0f);
  return path;
}

/** Chevron Right (stroked) */
inline SkPath ChevronRight() {
  SkPath path;
  path.moveTo(9.0f, 6.0f);
  path.lineTo(15.0f, 12.0f);
  path.lineTo(9.0f, 18.0f);
  return path;
}

// ============================================================================
// CONTENT ICONS
// ============================================================================

/** Folder icon */
inline SkPath Folder() {
  SkPath path;
  // Folder shape with tab
  path.moveTo(4.0f, 8.0f);
  path.lineTo(4.0f, 18.0f);
  path.lineTo(20.0f, 18.0f);
  path.lineTo(20.0f, 8.0f);
  path.lineTo(12.0f, 8.0f);
  path.lineTo(10.0f, 6.0f);
  path.lineTo(4.0f, 6.0f);
  path.close();
  return path;
}

/** File/Document icon */
inline SkPath File() {
  SkPath path;
  // File with corner fold
  path.moveTo(6.0f, 4.0f);
  path.lineTo(6.0f, 20.0f);
  path.lineTo(18.0f, 20.0f);
  path.lineTo(18.0f, 9.0f);
  path.lineTo(13.0f, 4.0f);
  path.close();
  // Corner fold
  path.moveTo(13.0f, 4.0f);
  path.lineTo(13.0f, 9.0f);
  path.lineTo(18.0f, 9.0f);
  return path;
}

/** Audio waveform icon */
inline SkPath Audio() {
  SkPath path;
  // Vertical bars of varying heights (waveform style)
  path.addRect(SkRect::MakeLTRB(4.0f, 10.0f, 6.0f, 14.0f));
  path.addRect(SkRect::MakeLTRB(7.0f, 7.0f, 9.0f, 17.0f));
  path.addRect(SkRect::MakeLTRB(10.0f, 4.0f, 12.0f, 20.0f));
  path.addRect(SkRect::MakeLTRB(13.0f, 8.0f, 15.0f, 16.0f));
  path.addRect(SkRect::MakeLTRB(16.0f, 6.0f, 18.0f, 18.0f));
  path.addRect(SkRect::MakeLTRB(19.0f, 9.0f, 21.0f, 15.0f));
  return path;
}

/** MIDI icon - piano keys or note */
inline SkPath MIDI() {
  SkPath path;
  // Piano keys representation
  // White keys background
  path.addRect(SkRect::MakeLTRB(4.0f, 8.0f, 20.0f, 18.0f));
  return path;
}

/** MIDI note for overlay */
inline SkPath MIDINote() {
  SkPath path;
  // Musical note shape
  path.addCircle(8.0f, 16.0f, 3.0f);
  path.addCircle(16.0f, 14.0f, 3.0f);
  path.addRect(SkRect::MakeLTRB(10.5f, 6.0f, 11.5f, 16.0f));
  path.addRect(SkRect::MakeLTRB(18.5f, 4.0f, 19.5f, 14.0f));
  path.addRect(SkRect::MakeLTRB(11.0f, 4.0f, 19.0f, 6.0f));
  return path;
}

/** Plugin - puzzle piece */
inline SkPath Plugin() {
  SkPath path;
  // Simplified puzzle piece
  path.moveTo(8.0f, 4.0f);
  path.lineTo(8.0f, 7.0f);
  path.cubicTo(6.0f, 7.0f, 6.0f, 10.0f, 8.0f, 10.0f);
  path.lineTo(8.0f, 14.0f);
  path.cubicTo(6.0f, 14.0f, 6.0f, 17.0f, 8.0f, 17.0f);
  path.lineTo(8.0f, 20.0f);
  path.lineTo(16.0f, 20.0f);
  path.lineTo(16.0f, 17.0f);
  path.cubicTo(18.0f, 17.0f, 18.0f, 14.0f, 16.0f, 14.0f);
  path.lineTo(16.0f, 10.0f);
  path.cubicTo(18.0f, 10.0f, 18.0f, 7.0f, 16.0f, 7.0f);
  path.lineTo(16.0f, 4.0f);
  path.close();
  return path;
}

// ============================================================================
// TRACK ICONS
// ============================================================================

/** Solo - "S" badge */
inline SkPath Solo() {
  SkPath path;
  // S-shaped path for Solo
  path.moveTo(15.0f, 7.0f);
  path.cubicTo(15.0f, 5.0f, 12.0f, 4.0f, 10.0f, 5.0f);
  path.cubicTo(7.0f, 6.0f, 7.0f, 9.0f, 10.0f, 10.0f);
  path.lineTo(14.0f, 11.0f);
  path.cubicTo(17.0f, 12.0f, 17.0f, 16.0f, 14.0f, 18.0f);
  path.cubicTo(12.0f, 19.0f, 9.0f, 18.0f, 9.0f, 16.0f);
  return path;
}

/** Mute - speaker with X or "M" */
inline SkPath Mute() {
  SkPath path;
  // Speaker body
  path.moveTo(5.0f, 9.0f);
  path.lineTo(9.0f, 9.0f);
  path.lineTo(13.0f, 5.0f);
  path.lineTo(13.0f, 19.0f);
  path.lineTo(9.0f, 15.0f);
  path.lineTo(5.0f, 15.0f);
  path.close();
  // X mark
  path.moveTo(16.0f, 9.0f);
  path.lineTo(20.0f, 15.0f);
  path.moveTo(20.0f, 9.0f);
  path.lineTo(16.0f, 15.0f);
  return path;
}

/** Arm/Record Enable - microphone or circle */
inline SkPath Arm() {
  SkPath path;
  // Simple microphone shape
  path.addRoundRect(SkRect::MakeLTRB(9.0f, 4.0f, 15.0f, 13.0f), 3.0f, 3.0f);
  // Stand base
  path.moveTo(6.0f, 11.0f);
  path.cubicTo(6.0f, 15.0f, 9.0f, 17.0f, 12.0f, 17.0f);
  path.cubicTo(15.0f, 17.0f, 18.0f, 15.0f, 18.0f, 11.0f);
  // Stem
  path.moveTo(12.0f, 17.0f);
  path.lineTo(12.0f, 20.0f);
  path.moveTo(9.0f, 20.0f);
  path.lineTo(15.0f, 20.0f);
  return path;
}

/** Freeze - snowflake */
inline SkPath Freeze() {
  SkPath path;
  const float cx = 12.0f;
  const float cy = 12.0f;
  // Main cross
  path.moveTo(cx, cy - 8.0f);
  path.lineTo(cx, cy + 8.0f);
  path.moveTo(cx - 7.0f, cy - 4.0f);
  path.lineTo(cx + 7.0f, cy + 4.0f);
  path.moveTo(cx - 7.0f, cy + 4.0f);
  path.lineTo(cx + 7.0f, cy - 4.0f);
  // Small ticks
  path.moveTo(cx - 2.0f, cy - 6.0f);
  path.lineTo(cx, cy - 8.0f);
  path.lineTo(cx + 2.0f, cy - 6.0f);
  return path;
}

// ============================================================================
// EDIT ICONS (Undo, Redo, Copy, Paste, Cut, Delete)
// ============================================================================

/** Undo - counterclockwise arrow */
inline SkPath Undo() {
  SkPath path;
  // Arrow arc
  path.moveTo(7.0f, 11.0f);
  path.lineTo(4.0f, 8.0f);
  path.lineTo(7.0f, 5.0f);
  // Curved arrow body
  path.moveTo(4.0f, 8.0f);
  path.lineTo(12.0f, 8.0f);
  path.cubicTo(17.0f, 8.0f, 20.0f, 11.0f, 20.0f, 15.0f);
  path.cubicTo(20.0f, 19.0f, 17.0f, 20.0f, 12.0f, 20.0f);
  path.lineTo(8.0f, 20.0f);
  return path;
}

/** Redo - clockwise arrow */
inline SkPath Redo() {
  SkPath path;
  // Arrow head
  path.moveTo(17.0f, 11.0f);
  path.lineTo(20.0f, 8.0f);
  path.lineTo(17.0f, 5.0f);
  // Curved arrow body
  path.moveTo(20.0f, 8.0f);
  path.lineTo(12.0f, 8.0f);
  path.cubicTo(7.0f, 8.0f, 4.0f, 11.0f, 4.0f, 15.0f);
  path.cubicTo(4.0f, 19.0f, 7.0f, 20.0f, 12.0f, 20.0f);
  path.lineTo(16.0f, 20.0f);
  return path;
}

/** Copy - two overlapping documents */
inline SkPath Copy() {
  SkPath path;
  // Back document
  path.addRect(SkRect::MakeLTRB(8.0f, 4.0f, 18.0f, 16.0f));
  // Front document
  path.addRect(SkRect::MakeLTRB(6.0f, 8.0f, 16.0f, 20.0f));
  return path;
}

/** Paste - clipboard with document */
inline SkPath Paste() {
  SkPath path;
  // Clipboard outline
  path.moveTo(6.0f, 6.0f);
  path.lineTo(6.0f, 20.0f);
  path.lineTo(18.0f, 20.0f);
  path.lineTo(18.0f, 6.0f);
  path.close();
  // Clipboard clip
  path.moveTo(9.0f, 4.0f);
  path.lineTo(9.0f, 6.0f);
  path.lineTo(15.0f, 6.0f);
  path.lineTo(15.0f, 4.0f);
  path.lineTo(16.0f, 4.0f);
  path.lineTo(16.0f, 7.0f);
  path.lineTo(8.0f, 7.0f);
  path.lineTo(8.0f, 4.0f);
  path.close();
  return path;
}

/** Cut - scissors */
inline SkPath Cut() {
  SkPath path;
  // Two circles (finger holes)
  path.addCircle(7.0f, 17.0f, 3.0f);
  path.addCircle(17.0f, 17.0f, 3.0f);
  // Blades crossing
  path.moveTo(7.0f, 14.0f);
  path.lineTo(17.0f, 6.0f);
  path.moveTo(17.0f, 14.0f);
  path.lineTo(7.0f, 6.0f);
  return path;
}

/** Delete/Trash - trash can */
inline SkPath Delete() {
  SkPath path;
  // Trash can body
  path.moveTo(6.0f, 8.0f);
  path.lineTo(7.0f, 20.0f);
  path.lineTo(17.0f, 20.0f);
  path.lineTo(18.0f, 8.0f);
  path.close();
  // Lid
  path.moveTo(5.0f, 8.0f);
  path.lineTo(19.0f, 8.0f);
  // Handle
  path.moveTo(9.0f, 5.0f);
  path.lineTo(9.0f, 8.0f);
  path.moveTo(15.0f, 5.0f);
  path.lineTo(15.0f, 8.0f);
  path.moveTo(9.0f, 5.0f);
  path.lineTo(15.0f, 5.0f);
  return path;
}

/** Save - floppy disk */
inline SkPath Save() {
  SkPath path;
  // Disk body
  path.moveTo(5.0f, 4.0f);
  path.lineTo(19.0f, 4.0f);
  path.lineTo(19.0f, 20.0f);
  path.lineTo(5.0f, 20.0f);
  path.close();
  // Label area
  path.addRect(SkRect::MakeLTRB(8.0f, 4.0f, 16.0f, 10.0f));
  // Write slot
  path.addRect(SkRect::MakeLTRB(8.0f, 14.0f, 16.0f, 18.0f));
  return path;
}

/** Edit/Pencil - pencil */
inline SkPath Edit() {
  SkPath path;
  // Pencil body (angled rectangle)
  path.moveTo(16.0f, 4.0f);
  path.lineTo(20.0f, 8.0f);
  path.lineTo(8.0f, 20.0f);
  path.lineTo(4.0f, 20.0f);
  path.lineTo(4.0f, 16.0f);
  path.close();
  // Tip line
  path.moveTo(14.0f, 6.0f);
  path.lineTo(18.0f, 10.0f);
  return path;
}

/** ZoomIn - magnifier with plus */
inline SkPath ZoomIn() {
  SkPath path;
  // Circle
  path.addCircle(10.0f, 10.0f, 6.0f);
  // Handle
  path.moveTo(14.5f, 14.5f);
  path.lineTo(20.0f, 20.0f);
  // Plus
  path.moveTo(10.0f, 7.0f);
  path.lineTo(10.0f, 13.0f);
  path.moveTo(7.0f, 10.0f);
  path.lineTo(13.0f, 10.0f);
  return path;
}

/** ZoomOut - magnifier with minus */
inline SkPath ZoomOut() {
  SkPath path;
  // Circle
  path.addCircle(10.0f, 10.0f, 6.0f);
  // Handle
  path.moveTo(14.5f, 14.5f);
  path.lineTo(20.0f, 20.0f);
  // Minus
  path.moveTo(7.0f, 10.0f);
  path.lineTo(13.0f, 10.0f);
  return path;
}

/** Download - arrow pointing down into tray */
inline SkPath Download() {
  SkPath path;
  // Arrow down
  path.moveTo(12.0f, 4.0f);
  path.lineTo(12.0f, 14.0f);
  path.moveTo(8.0f, 10.0f);
  path.lineTo(12.0f, 14.0f);
  path.lineTo(16.0f, 10.0f);
  // Tray
  path.moveTo(4.0f, 17.0f);
  path.lineTo(4.0f, 20.0f);
  path.lineTo(20.0f, 20.0f);
  path.lineTo(20.0f, 17.0f);
  return path;
}

/** Upload - arrow pointing up from tray */
inline SkPath Upload() {
  SkPath path;
  // Arrow up
  path.moveTo(12.0f, 14.0f);
  path.lineTo(12.0f, 4.0f);
  path.moveTo(8.0f, 8.0f);
  path.lineTo(12.0f, 4.0f);
  path.lineTo(16.0f, 8.0f);
  // Tray
  path.moveTo(4.0f, 17.0f);
  path.lineTo(4.0f, 20.0f);
  path.lineTo(20.0f, 20.0f);
  path.lineTo(20.0f, 17.0f);
  return path;
}

/** Lock - padlock closed */
inline SkPath Lock() {
  SkPath path;
  // Lock body
  path.addRoundRect(SkRect::MakeLTRB(6.0f, 11.0f, 18.0f, 20.0f), 2.0f, 2.0f);
  // Shackle
  path.moveTo(8.0f, 11.0f);
  path.lineTo(8.0f, 8.0f);
  path.cubicTo(8.0f, 5.0f, 10.0f, 4.0f, 12.0f, 4.0f);
  path.cubicTo(14.0f, 4.0f, 16.0f, 5.0f, 16.0f, 8.0f);
  path.lineTo(16.0f, 11.0f);
  return path;
}

/** Unlock - padlock open */
inline SkPath Unlock() {
  SkPath path;
  // Lock body
  path.addRoundRect(SkRect::MakeLTRB(6.0f, 11.0f, 18.0f, 20.0f), 2.0f, 2.0f);
  // Open shackle
  path.moveTo(8.0f, 11.0f);
  path.lineTo(8.0f, 8.0f);
  path.cubicTo(8.0f, 5.0f, 10.0f, 4.0f, 12.0f, 4.0f);
  path.cubicTo(14.0f, 4.0f, 16.0f, 5.0f, 16.0f, 8.0f);
  return path;
}

/** Visible/Eye - eye icon */
inline SkPath Eye() {
  SkPath path;
  // Eye shape
  path.moveTo(4.0f, 12.0f);
  path.cubicTo(4.0f, 12.0f, 8.0f, 6.0f, 12.0f, 6.0f);
  path.cubicTo(16.0f, 6.0f, 20.0f, 12.0f, 20.0f, 12.0f);
  path.cubicTo(20.0f, 12.0f, 16.0f, 18.0f, 12.0f, 18.0f);
  path.cubicTo(8.0f, 18.0f, 4.0f, 12.0f, 4.0f, 12.0f);
  path.close();
  // Pupil
  path.addCircle(12.0f, 12.0f, 3.0f);
  return path;
}

/** Hidden/EyeOff - eye with slash */
inline SkPath EyeOff() {
  SkPath path;
  // Eye shape
  path.moveTo(4.0f, 12.0f);
  path.cubicTo(4.0f, 12.0f, 8.0f, 6.0f, 12.0f, 6.0f);
  path.cubicTo(16.0f, 6.0f, 20.0f, 12.0f, 20.0f, 12.0f);
  path.cubicTo(20.0f, 12.0f, 16.0f, 18.0f, 12.0f, 18.0f);
  path.cubicTo(8.0f, 18.0f, 4.0f, 12.0f, 4.0f, 12.0f);
  path.close();
  // Slash
  path.moveTo(4.0f, 4.0f);
  path.lineTo(20.0f, 20.0f);
  return path;
}

// ============================================================================
// DRAWING HELPERS
// ============================================================================

/**
 * Draw an icon at a specific position with given size.
 * The icon is scaled from 24x24 viewport to the target size.
 *
 * @param canvas The Skia canvas to draw on
 * @param icon The icon path to draw
 * @param x Left edge of icon bounding box
 * @param y Top edge of icon bounding box
 * @param size Target icon size (width & height)
 * @param style Icon styling (color, stroke, fill, glow)
 */
inline void drawIcon(SkCanvas *canvas, const SkPath &icon, float x, float y,
                     float size, const IconStyle &style) {
  if (canvas == nullptr)
    return;

  canvas->save();

  // Scale from 24x24 viewport to desired size
  float scale = size / ICON_VIEWPORT;
  canvas->translate(x, y);
  canvas->scale(scale, scale);

  // Draw glow if requested
  if (style.glowRadius > 0.0f) {
    SkPaint glowPaint;
    glowPaint.setAntiAlias(true);
    SkColor glowCol =
        (style.glowColor == 0x00000000) ? style.color : style.glowColor;
    glowPaint.setColor(SkColorSetA(glowCol, 100));
    glowPaint.setStyle(style.filled ? SkPaint::kFill_Style
                                    : SkPaint::kStroke_Style);
    glowPaint.setStrokeWidth((style.strokeWidth + style.glowRadius) / scale);
    glowPaint.setMaskFilter(
        SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, style.glowRadius));
    canvas->drawPath(icon, glowPaint);
  }

  // Main icon paint
  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setColor(style.color);
  paint.setStyle(style.filled ? SkPaint::kFill_Style : SkPaint::kStroke_Style);
  paint.setStrokeWidth(style.strokeWidth);
  paint.setStrokeCap(SkPaint::kRound_Cap);
  paint.setStrokeJoin(SkPaint::kRound_Join);

  canvas->drawPath(icon, paint);
  canvas->restore();
}

/**
 * Draw an icon centered within given bounds.
 * Useful for button icons.
 */
inline void drawIconCentered(SkCanvas *canvas, const SkPath &icon,
                             const SkRect &bounds, float iconSize,
                             const IconStyle &style) {
  float x = bounds.centerX() - iconSize / 2.0f;
  float y = bounds.centerY() - iconSize / 2.0f;
  drawIcon(canvas, icon, x, y, iconSize, style);
}

/**
 * Draw an icon button with background, hover state, and active state.
 *
 * @param canvas Canvas to draw on
 * @param icon Icon path
 * @param bounds Button bounds
 * @param style Icon style
 * @param isHovered Whether the button is hovered
 * @param isActive Whether the button is in active state (pressed/toggled)
 * @param bgColor Optional background color (0 = transparent)
 */
inline void drawIconButton(SkCanvas *canvas, const SkPath &icon,
                           const SkRect &bounds, IconStyle style,
                           bool isHovered, bool isActive,
                           SkColor bgColor = 0x00000000) {
  if (canvas == nullptr)
    return;

  // Background
  if (bgColor != 0x00000000 || isHovered || isActive) {
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);

    if (isActive) {
      bgPaint.setColor(SkColorSetARGB(60, SkColorGetR(style.color),
                                      SkColorGetG(style.color),
                                      SkColorGetB(style.color)));
    } else if (isHovered) {
      bgPaint.setColor(SkColorSetARGB(30, 255, 255, 255));
    } else {
      bgPaint.setColor(bgColor);
    }

    canvas->drawRoundRect(bounds, 6.0f, 6.0f, bgPaint);
  }

  // Adjust style for active state
  if (isActive) {
    style.filled = true;
    style.glowRadius = 4.0f;
  } else if (isHovered) {
    style.color = SkColorSetA(style.color, 255); // Full opacity on hover
  }

  // Calculate icon size (about 60% of button size)
  float iconSize = std::min(bounds.width(), bounds.height()) * 0.6f;

  drawIconCentered(canvas, icon, bounds, iconSize, style);
}

} // namespace icons
} // namespace zenith

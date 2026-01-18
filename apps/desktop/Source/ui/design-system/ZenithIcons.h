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

#include "ZenithSkia.h"
#include <core/SkPath.h>
#include <core/SkPaint.h>
#include <core/SkMaskFilter.h>

namespace zenith {
namespace icons {

// ============================================================================
// CONSTANTS
// ============================================================================

// Standard icon viewport (like Material/Phosphor icons)
constexpr float ICON_VIEWPORT = 24.0f;

// Default stroke widths for different icon sizes
constexpr float STROKE_THIN = 1.5f;
constexpr float STROKE_LIGHT = 1.75f;
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
  // 14x14 square centered in 24x24
  path.addRect(SkRect::MakeLTRB(5.0f, 5.0f, 19.0f, 19.0f));
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
  // Arrow head at the end (connected)
  // End of arc is roughly at (-60 + 300) = 240 deg.
  // Actually, visual inspection: the arrow is near the top right/bottom right.
  // Let's just connect it using lineTo if it's close.
  // The original moveTo(17, 4) was likely near the START or END depending on
  // direction. Let's assume we want to attach arrow to the end of the arc.
  // However, Skia arcTo leaves the pen at the end of the arc.
  // If we just lineTo the arrow vertices, it will be connected.
  // Re-defining for correct look:
  path.lineTo(14.0f, 7.0f); // Back of arrow
  path.moveTo(17.5f, 5.0f); // Tip (adjusted) - actually let's keep it simple
  // Disconnected arrow is common for "refresh" icons if styled that way, but
  // let's connect it. To verify connection, we'd need exact coords. For now, I
  // will use lineTo to the first point of the arrowhead.
  path.lineTo(17.0f, 4.0f);
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
  const float toothDepth = 2.5f;
  const int teeth = 8;
  const float toRad = SK_ScalarPI / 180.0f;

  for (int i = 0; i < teeth; ++i) {
    float angle1 = (i * 360.0f / teeth) * toRad;
    float angle2 = ((i + 0.3f) * 360.0f / teeth) * toRad;
    float angle3 = ((i + 0.7f) * 360.0f / teeth) * toRad;
    float angle4 = ((i + 1.0f) * 360.0f / teeth) * toRad;

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
  // Black keys (simplified)
  path.addRect(SkRect::MakeLTRB(7.0f, 8.0f, 9.0f, 13.0f));
  path.addRect(SkRect::MakeLTRB(11.0f, 8.0f, 13.0f, 13.0f));
  path.addRect(SkRect::MakeLTRB(15.0f, 8.0f, 17.0f, 13.0f));
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
  // Small ticks on all arms
  // Top
  path.moveTo(cx - 2.0f, cy - 6.0f);
  path.lineTo(cx, cy - 8.0f);
  path.lineTo(cx + 2.0f, cy - 6.0f);
  // Bottom
  path.moveTo(cx - 2.0f, cy + 6.0f);
  path.lineTo(cx, cy + 8.0f);
  path.lineTo(cx + 2.0f, cy + 6.0f);
  // Left
  path.moveTo(cx - 6.0f, cy - 2.0f);
  path.lineTo(cx - 8.0f, cy);
  path.lineTo(cx - 6.0f, cy + 2.0f);
  // Right
  path.moveTo(cx + 6.0f, cy - 2.0f);
  path.lineTo(cx + 8.0f, cy);
  path.lineTo(cx + 6.0f, cy + 2.0f);
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
// HUB ICONS
// ============================================================================

/**
 * ElectronicTemplate - Synthesizer with keyboard and sound waves
 * Represents electronic music production, synth-heavy templates
 */
inline SkPath ElectronicTemplate() {
  SkPath path;
  
  // Synth body (rounded rectangle with keyboard)
  SkRect body = SkRect::MakeLTRB(4.0f, 6.0f, 20.0f, 14.0f);
  SkRRect bodyRect = SkRRect::MakeRectXY(body, 3.0f, 3.0f);
  path.addRRect(bodyRect);
  
  // White keys (4 rows x 7 columns)
  float keyW = 1.8f;
  float keyH = 1.5f;
  float gapX = 0.4f;
  float gapY = 0.3f;
  float startY = 10.0f;
  
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 7; ++col) {
      float kx = 5.5f + (col * (keyW + gapX));
      float ky = startY + (row * (keyH + gapY));
      SkRect key = SkRect::MakeLTRB(kx, ky, kx + keyW, ky + keyH);
      path.addRect(key);
    }
  }
  
  // Sound waves (3 stylized curves)
  SkPath wave1;
  wave1.moveTo(6.0f, 3.0f);
  wave1.cubicTo(7.0f, 1.0f, 9.0f, 1.5f, 11.0f, 2.5f);
  path.addPath(wave1);
  
  SkPath wave2;
  wave2.moveTo(8.0f, 4.5f);
  wave2.cubicTo(9.0f, 3.0f, 11.0f, 4.0f, 13.0f, 4.5f);
  path.addPath(wave2);
  
  SkPath wave3;
  wave3.moveTo(10.0f, 5.5f);
  wave3.cubicTo(11.0f, 5.0f, 13.0f, 5.2f, 14.5f, 5.5f);
  path.addPath(wave3);
  
  return path;
}

/**
 * OrchestralTemplate - Conductor's baton with music notes
 * Represents orchestral, cinematic, classical music templates
 */
inline SkPath OrchestralTemplate() {
  SkPath path;
  
  // Baton stick (angled)
  path.moveTo(10.5f, 19.0f);
  path.lineTo(13.5f, 4.0f);
  
  // Baton head (circle)
  SkRect head = SkRect::MakeLTRB(12.0f, 3.0f, 16.0f, 5.0f);
  path.addOval(head);
  
  // First music note (eighth note - left)
  path.moveTo(5.0f, 9.0f);
  path.addCircle(5.0f, 8.5f, 2.0f);
  path.moveTo(7.0f, 8.0f);
  path.lineTo(9.5f, 8.0f);
  path.lineTo(9.5f, 12.0f);
  path.lineTo(7.0f, 12.0f);
  path.lineTo(7.0f, 9.0f);
  
  // Second music note (quarter note - right)
  path.moveTo(17.5f, 10.0f);
  path.addCircle(17.5f, 9.5f, 2.0f);
  path.moveTo(19.5f, 9.5f);
  path.lineTo(20.0f, 9.5f);
  path.lineTo(20.0f, 14.0f);
  path.lineTo(19.5f, 14.0f);
  path.lineTo(19.5f, 10.5f);
  
  return path;
}

/**
 * RecordingTemplate - Microphone with recording indicator
 * Represents vocal recording, podcast, acoustic templates
 */
inline SkPath RecordingTemplate() {
  SkPath path;
  
  // Microphone body (capsule shape)
  SkRect body = SkRect::MakeLTRB(6.0f, 5.0f, 18.0f, 14.0f);
  SkRRect micRect = SkRRect::MakeRectXY(body, 4.0f, 4.0f);
  path.addRRect(micRect);
  
  // Microphone grille (horizontal lines)
  float grilleY = 9.5f;
  float grilleW = 10.0f;
  float grilleX = 7.0f;
  
  for (int i = 0; i < 5; ++i) {
    float lineX = grilleX + (i * 2.2f);
    path.moveTo(lineX, grilleY - 2.2f);
    path.lineTo(lineX, grilleY + 2.2f);
  }
  
  // Recording indicator (red circle with pulsing dot)
  SkRect indicator = SkRect::MakeLTRB(16.5f, 5.5f, 20.5f, 9.5f);
  path.addOval(indicator);
  
  // Recording dot (white)
  SkRect dot = SkRect::MakeLTRB(17.8f, 6.8f, 19.2f, 8.2f);
  path.addOval(dot);
  
  return path;
}

/** Project - Folder with a project indicator */
inline SkPath Project() {
  SkPath path;
  // Robust folder shape
  path.moveTo(4.0f, 6.0f);
  path.cubicTo(4.0f, 5.0f, 5.0f, 4.0f, 6.0f, 4.0f);
  path.lineTo(9.0f, 4.0f);  // Tab start
  path.lineTo(11.0f, 6.0f); // Tab slope
  path.lineTo(20.0f, 6.0f); // Top right
  path.lineTo(20.0f, 18.0f);
  path.cubicTo(20.0f, 19.0f, 19.0f, 20.0f, 18.0f, 20.0f);
  path.lineTo(4.0f, 20.0f);
  path.close();

  // Project star symbol inside
  path.moveTo(12.0f, 11.0f);
  path.lineTo(13.0f, 14.0f);
  path.lineTo(16.0f, 14.0f);
  path.lineTo(13.5f, 16.0f);
  path.lineTo(14.5f, 19.0f);
  path.lineTo(12.0f, 17.0f);
  path.lineTo(9.5f, 19.0f);
  path.lineTo(10.5f, 16.0f);
  path.lineTo(8.0f, 14.0f);
  path.lineTo(11.0f, 14.0f);
  path.close();
  return path;
}

/** Template - Layout document */
inline SkPath Template() {
  SkPath path;
  // Document outline
  path.addRoundRect(SkRect::MakeLTRB(5.0f, 4.0f, 19.0f, 20.0f), 2.0f, 2.0f);
  // Header section
  path.moveTo(5.0f, 9.0f);
  path.lineTo(19.0f, 9.0f);
  // Sidebar/Column split
  path.moveTo(10.0f, 9.0f);
  path.lineTo(10.0f, 20.0f);
  return path;
}

/** Cloud */
inline SkPath Cloud() {
  SkPath path;
  // Flat bottom
  path.moveTo(6.0f, 19.0f);
  path.lineTo(18.0f, 19.0f);
  // Curves
  path.cubicTo(20.0f, 19.0f, 21.0f, 17.0f, 21.0f, 15.0f); // Right bottom
  path.cubicTo(21.0f, 12.0f, 19.0f, 11.0f, 18.0f, 11.5f); // Right top
  path.cubicTo(17.5f, 8.0f, 13.5f, 7.0f, 12.0f, 9.0f);    // Top main
  path.cubicTo(9.0f, 7.5f, 5.0f, 9.0f, 5.0f, 13.0f);      // Left bubble
  path.cubicTo(3.0f, 13.0f, 3.0f, 19.0f, 6.0f, 19.0f);    // Left bottom
  path.close();
  return path;
}

/** Synth - Keyboard representation */
inline SkPath Synth() {
  SkPath path;
  // Outer frame
  path.addRect(SkRect::MakeLTRB(2.0f, 7.0f, 22.0f, 17.0f));
  // White keys separators
  path.moveTo(7.0f, 7.0f);
  path.lineTo(7.0f, 17.0f);
  path.moveTo(12.0f, 7.0f);
  path.lineTo(12.0f, 17.0f);
  path.moveTo(17.0f, 7.0f);
  path.lineTo(17.0f, 17.0f);
  // Black keys (filled rects usually, but we are stroking)
  // Let's draw small rects at top
  path.addRect(SkRect::MakeLTRB(5.5f, 7.0f, 8.5f, 12.0f));
  path.addRect(
      SkRect::MakeLTRB(10.5f, 7.0f, 13.5f, 12.0f)); // Gap? Usually 2-3 pattern.
  // Let's do a simple pattern: C# D# - F# G# A#
  // Just generic:
  path.addRect(SkRect::MakeLTRB(15.5f, 7.0f, 18.5f, 12.0f));
  return path;
}

/** MusicNote - Beamed note (explicit definition) */
inline SkPath MusicNote() {
  SkPath path;
  // Note heads
  path.addCircle(8.0f, 17.0f, 3.0f);
  path.addCircle(18.0f, 16.0f, 3.0f);
  // Stems
  path.moveTo(10.5f, 17.0f);
  path.lineTo(10.5f, 7.0f);
  path.lineTo(20.5f, 6.0f);
  path.lineTo(20.5f, 16.0f);
  // Beam
  path.moveTo(10.5f, 10.0f);
  path.lineTo(20.5f, 9.0f);
  return path;
}

/** Microphone alias (with strict signature) */
inline SkPath Microphone() { return Arm(); }

/** Info - Circle with "i" */
inline SkPath Info() {
  SkPath path;
  // Circle outline
  path.addCircle(12.0f, 12.0f, 9.0f);
  // Dot
  path.addCircle(12.0f, 7.0f, 1.5f);
  // Line
  path.moveTo(12.0f, 11.0f);
  path.lineTo(12.0f, 17.0f);
  return path;
}

/** Users - Two people silhouettes */
inline SkPath Users() {
  SkPath path;
  // Two people silhouettes (simplified)
  path.addCircle(8.0f, 8.0f, 3.0f);
  path.addCircle(16.0f, 8.0f, 3.0f);
  path.addArc(SkRect::MakeLTRB(4.0f, 12.0f, 12.0f, 20.0f), 180.0f, 180.0f);
  path.addArc(SkRect::MakeLTRB(12.0f, 12.0f, 20.0f, 20.0f), 180.0f, 180.0f);
  return path;
}

/** Waveform alias */
inline SkPath Waveform() { return Audio(); }

// ============================================================================
// AI ICONS
// ============================================================================

/** Brain icon for AI/Thinking - professional anatomical brain with gyri */
inline SkPath Brain() {
  SkPath path;
  
  // Adjusted Y coordinates to prevent cutoff (shifted up by ~2px)
  // Brain outline - rounded peanut shape
  path.moveTo(12.0f, 1.5f); // Was 3.5f
  // Right hemisphere outline
  path.cubicTo(15.5f, 1.5f, 18.5f, 3.0f, 19.5f, 6.0f);
  path.cubicTo(20.5f, 9.0f, 20.0f, 12.0f, 19.0f, 14.0f);
  path.cubicTo(18.0f, 16.0f, 15.5f, 18.0f, 13.0f, 18.0f);
  path.lineTo(12.0f, 18.0f);
  
  // Left hemisphere outline  
  path.lineTo(11.0f, 18.0f);
  path.cubicTo(8.5f, 18.0f, 6.0f, 16.0f, 5.0f, 14.0f);
  path.cubicTo(4.0f, 12.0f, 3.5f, 9.0f, 4.5f, 6.0f);
  path.cubicTo(5.5f, 3.0f, 8.5f, 1.5f, 12.0f, 1.5f);
  path.close();
  
  // Central fissure
  path.moveTo(12.0f, 2.5f);
  path.lineTo(12.0f, 17.0f);
  
  // Gyri details (shifted)
  path.moveTo(5.5f, 6.0f);
  path.cubicTo(7.0f, 5.5f, 8.5f, 7.0f, 9.5f, 6.0f);
  path.cubicTo(10.5f, 5.0f, 11.0f, 6.5f, 11.5f, 6.0f);
  
  path.moveTo(5.0f, 9.5f);
  path.cubicTo(6.5f, 10.5f, 8.0f, 8.5f, 9.5f, 9.5f);
  path.cubicTo(10.5f, 10.0f, 11.0f, 9.0f, 11.5f, 9.5f);
  
  // Brain stem
  path.moveTo(11.0f, 18.0f);
  path.lineTo(11.0f, 20.0f); // Fits within 24px box
  path.moveTo(13.0f, 18.0f);
  path.lineTo(13.0f, 20.0f);

  return path;
}

/** Sparkles/Stars icon for AI/Magic - replaces Brain for modern look */
inline SkPath Sparkles() {
  SkPath path;
  
  // Large star (Top Left)
  // Center roughly at 8, 8
  path.moveTo(8.0f, 2.0f);
  path.cubicTo(9.0f, 5.0f, 11.0f, 7.0f, 14.0f, 8.0f);
  path.cubicTo(11.0f, 9.0f, 9.0f, 11.0f, 8.0f, 14.0f);
  path.cubicTo(7.0f, 11.0f, 5.0f, 9.0f, 2.0f, 8.0f);
  path.cubicTo(5.0f, 7.0f, 7.0f, 5.0f, 8.0f, 2.0f);
  
  // Medium star (Bottom Right)
  // Center at 18, 18
  path.moveTo(18.0f, 14.0f);
  path.cubicTo(18.5f, 16.0f, 20.0f, 17.5f, 22.0f, 18.0f);
  path.cubicTo(20.0f, 18.5f, 18.5f, 20.0f, 18.0f, 22.0f);
  path.cubicTo(17.5f, 20.0f, 16.0f, 18.5f, 14.0f, 18.0f);
  path.cubicTo(16.0f, 17.5f, 17.5f, 16.0f, 18.0f, 14.0f);
  
  // Small star (Top Right)
  // Center at 19, 6
  path.moveTo(19.0f, 4.0f);
  path.cubicTo(19.3f, 5.0f, 20.0f, 5.7f, 21.0f, 6.0f); 
  path.cubicTo(20.0f, 6.3f, 19.3f, 7.0f, 19.0f, 8.0f);
  path.cubicTo(18.7f, 7.0f, 18.0f, 6.3f, 17.0f, 6.0f);
  path.cubicTo(18.0f, 5.7f, 18.7f, 5.0f, 19.0f, 4.0f);

  return path;
}

/** Send icon - paper plane / arrow pointing right-up */
inline SkPath Send() {
  SkPath path;
  // Paper plane style send arrow
  path.moveTo(4.0f, 12.0f);
  path.lineTo(20.0f, 4.0f);    // Top tip
  path.lineTo(20.0f, 20.0f);   // Bottom tip
  path.close();
  
  // Inner fold line
  path.moveTo(4.0f, 12.0f);
  path.lineTo(20.0f, 12.0f);
  
  return path;
}

/** SendArrow icon - simple right-pointing arrow for send button */
inline SkPath SendArrow() {
  SkPath path;
  // Arrow body (left to right)
  path.moveTo(4.0f, 12.0f);
  path.lineTo(18.0f, 12.0f);
  
  // Arrow head
  path.moveTo(14.0f, 7.0f);
  path.lineTo(20.0f, 12.0f);
  path.lineTo(14.0f, 17.0f);
  
  return path;
}

/** Partnership icon - Human hand + Digital wireframe hand clasping (Wingman button)
 *  Lucide "handshake" icon - this version WORKED and rendered correctly
 *  24x24 viewport
 */
inline SkPath Partnership() {
  SkPath path;
  
  // ==========================================================================
  // Lucide Handshake - This rendered correctly before
  // Going back to what works
  // ==========================================================================
  
  // Path 1: "m11 17 2 2a1 1 0 1 0 3-3"
  path.moveTo(11.0f, 17.0f);
  path.rLineTo(2.0f, 2.0f);
  path.arcTo(1.0f, 1.0f, 0.0f, SkPath::kLarge_ArcSize, SkPathDirection::kCCW, 16.0f, 16.0f);
  
  // Path 2: Main handshake
  path.moveTo(14.0f, 14.0f);
  path.rLineTo(2.5f, 2.5f);
  path.arcTo(1.0f, 1.0f, 0.0f, SkPath::kLarge_ArcSize, SkPathDirection::kCCW, 19.5f, 13.5f);
  path.rLineTo(-3.88f, -3.88f);
  path.arcTo(3.0f, 3.0f, 0.0f, SkPath::kSmall_ArcSize, SkPathDirection::kCCW, 11.38f, 9.62f);
  path.rLineTo(-0.88f, 0.88f);
  path.arcTo(1.0f, 1.0f, 0.0f, SkPath::kLarge_ArcSize, SkPathDirection::kCW, 7.5f, 7.5f);
  path.rLineTo(2.81f, -2.81f);
  path.arcTo(5.79f, 5.79f, 0.0f, SkPath::kSmall_ArcSize, SkPathDirection::kCW, 17.37f, 3.82f);
  path.rLineTo(0.47f, 0.28f);
  path.arcTo(2.0f, 2.0f, 0.0f, SkPath::kSmall_ArcSize, SkPathDirection::kCCW, 19.26f, 4.35f);
  path.lineTo(21.0f, 4.0f);
  
  // Path 3: Right arm
  path.moveTo(21.0f, 3.0f);
  path.rLineTo(1.0f, 11.0f);
  path.rLineTo(-2.0f, 0.0f);
  
  // Path 4: Left arm
  path.moveTo(3.0f, 3.0f);
  path.lineTo(2.0f, 14.0f);
  path.rLineTo(6.5f, 6.5f);
  path.arcTo(1.0f, 1.0f, 0.0f, SkPath::kLarge_ArcSize, SkPathDirection::kCCW, 11.5f, 17.5f);
  
  // Path 5: Top bar
  path.moveTo(3.0f, 4.0f);
  path.rLineTo(8.0f, 0.0f);
  
  return path;
}

// Alias for backwards compatibility
inline SkPath Handshake() { return Partnership(); }

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
    glowPaint.setStrokeWidth(style.strokeWidth);
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

// Aliases for Wingman (commented to fix redefinition)
// inline SkPath History() { return Undo(); }
// inline SkPath Lightbulb() { return Brain(); }

} // namespace icons
} // namespace zenith

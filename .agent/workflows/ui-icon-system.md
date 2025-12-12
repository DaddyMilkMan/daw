---
description: Replace Unicode symbol embarrassment with proper SVG/vector icon system
---

# UI Fix #3: Professional Icon System

## MISSION
The current UI uses raw Unicode characters as icons:
```cpp
drawButton(canvas, playButtonBounds_, "▶", isPlaying_, ...);
drawButton(canvas, recordButtonBounds_, "●", isRecording_, ...);
```

This is **unacceptable**. Unicode glyphs render differently per font, per platform, and look amateur. Implement a proper vector icon system.

## PRE-TASK RESEARCH (MANDATORY)

1. **Web Search**: "Skia render SVG path data C++"
   - Check if Skia has native SVG support or requires SkSVGDOM
   
2. **Web Search**: "SkPath moveTo lineTo curveTo example"
   - Understand how to define vector paths programmatically
   
3. **Web Search**: "Phosphor icons SVG download"
   - Phosphor is a clean, professional icon set with consistent stroke weights
   - Alternative: Lucide, Feather, Heroicons
   
4. **Web Search**: "SVG to SkPath converter tool"
   - Find tools to convert SVG paths to Skia code
   
5. **Web Search**: "icon font vs inline SVG performance native app"
   - Determine if icon font (like in web) is viable vs inline SkPaths

## DESIGN DECISION

**Choose One Approach:**

### Option A: Inline SkPath Definitions (RECOMMENDED)
- Define each icon as a static function returning SkPath
- Zero runtime parsing, fast rendering
- Icons scale perfectly at any size
- Consistent stroke width even at small sizes

### Option B: Load SVG at Runtime
- Use Skia's SVG module (requires additional dependency)
- More flexible, icons can be changed without recompile
- Slightly slower startup

### Option C: Icon Font
- Single font file with glyphs for each icon
- Render as text (simple)
- Harder to do multi-color or gradient icons

**For Zenith: Go with Option A for core transport/toolbar icons, Option B for extensibility**

## IMPLEMENTATION STEPS

### Step 1: Create ZenithIcons.h
Location: `Source/ui/skia/ZenithIcons.h`

```cpp
#pragma once
#include <core/SkPath.h>
#include <core/SkCanvas.h>
#include <core/SkPaint.h>

namespace zenith::icons {

// Icon viewport is 24x24 by convention (like Material Icons)
constexpr float ICON_VIEWPORT = 24.0f;

struct IconStyle {
    float strokeWidth = 2.0f;
    SkColor color = SK_ColorWHITE;
    bool filled = false;
};

// Core Transport Icons
SkPath Play();      // Triangle pointing right
SkPath Pause();     // Two vertical bars
SkPath Stop();      // Square
SkPath Record();    // Filled circle
SkPath FastForward();
SkPath Rewind();
SkPath Loop();      // Circular arrow
SkPath Shuffle();

// UI Icons
SkPath Settings();   // Gear
SkPath Menu();       // Hamburger
SkPath Close();      // X
SkPath Minimize();   // Dash
SkPath Maximize();   // Square outline
SkPath ChevronDown();
SkPath ChevronRight();
SkPath Plus();
SkPath Minus();
SkPath Search();
SkPath Folder();
SkPath File();
SkPath Audio();      // Waveform symbol
SkPath MIDI();       // Piano keys or MIDI plug
SkPath Plugin();     // Puzzle piece

// Helper to draw icon at position with size
void drawIcon(SkCanvas* canvas, const SkPath& icon, 
              float x, float y, float size, const IconStyle& style);

// Higher-level button helper
void drawIconButton(SkCanvas* canvas, const SkPath& icon,
                    const SkRect& bounds, const IconStyle& style,
                    bool isHovered, bool isActive);

} // namespace zenith::icons
```

### Step 2: Implement Core Icons
Create `Source/ui/skia/ZenithIcons.cpp`:

Example for Play icon:
```cpp
SkPath icons::Play() {
    SkPath path;
    // Triangle in 24x24 viewport
    path.moveTo(6.0f, 4.0f);
    path.lineTo(20.0f, 12.0f);
    path.lineTo(6.0f, 20.0f);
    path.close();
    return path;
}

SkPath icons::Stop() {
    SkPath path;
    path.addRect(SkRect::MakeLTRB(5.0f, 5.0f, 19.0f, 19.0f));
    return path;
}

SkPath icons::Record() {
    SkPath path;
    path.addCircle(12.0f, 12.0f, 8.0f);
    return path;
}

void icons::drawIcon(SkCanvas* canvas, const SkPath& icon,
                     float x, float y, float size, const IconStyle& style) {
    canvas->save();
    
    // Scale from 24x24 viewport to desired size
    float scale = size / ICON_VIEWPORT;
    canvas->translate(x, y);
    canvas->scale(scale, scale);
    
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(style.color);
    paint.setStyle(style.filled ? SkPaint::kFill_Style : SkPaint::kStroke_Style);
    paint.setStrokeWidth(style.strokeWidth / scale); // Maintain visual stroke
    paint.setStrokeCap(SkPaint::kRound_Cap);
    paint.setStrokeJoin(SkPaint::kRound_Join);
    
    canvas->drawPath(icon, paint);
    canvas->restore();
}
```

### Step 3: Convert Phosphor/Lucide Icons
For complex icons (Settings gear, etc.):

1. Download SVG from icon library
2. Extract the `d=""` path data
3. Use online "SVG to SkPath" converter or write parser
4. Store as static SkPath

Example for Settings (gear):
```cpp
SkPath icons::Settings() {
    SkPath path;
    // Path data from Lucide "settings" icon - simplified
    // Outer gear teeth
    path.moveTo(12.22f, 2.0f);
    path.lineTo(12.22f, 2.0f);
    // ... (actual path commands from SVG)
    // Center circle
    path.addCircle(12.0f, 12.0f, 3.0f, SkPathDirection::kCCW);
    return path;
}
```

### Step 4: Update TransportBar.cpp
Replace:
```cpp
drawButton(canvas, playButtonBounds_, "▶", isPlaying_, design::colors::NEON_GREEN);
```
With:
```cpp
icons::IconStyle style;
style.color = isPlaying_ ? design::colors::NEON_GREEN : design::colors::TEXT_SECONDARY;
style.filled = isPlaying_;
icons::drawIconButton(canvas, icons::Play(), playButtonBoundsSkia, style, 
                      isPlayHovered_, isPlaying_);
```

### Step 5: Update All Icon Usages
Find and replace ALL Unicode icons in:
- TransportBar.cpp
- BrowserPanel.cpp
- TrackHeaderComponent.cpp
- BottomBar.cpp
- MenuBar.cpp
- Any context menus

### Step 6: Add Icon Glow for Active States
In `drawIconButton`:
```cpp
if (isActive) {
    // Draw glow behind icon
    SkPaint glowPaint;
    glowPaint.setColor(design::withAlpha(style.color, 0.4f));
    glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 6.0f));
    canvas->drawPath(scaledPath, glowPaint);
}
```

## VERIFICATION CHECKLIST
- [ ] All transport buttons use vector icons (no unicode)
- [ ] Icons scale cleanly at 1x, 1.25x, 1.5x, 2x DPI
- [ ] Active states show proper fills and glows
- [ ] Stroke width appears consistent across all icon sizes
- [ ] Build succeeds with no warnings
- [ ] Web search confirms SkPath usage matches Skia best practices

## ACCEPTANCE CRITERIA
Compare Play button at 20px, 40px, and 80px size. Icons must look equally crisp at all sizes with proportional stroke weights. No pixelation, no aliasing artifacts.

## ICON REFERENCE LIST
Minimum required icons for MVP:
- Transport: Play, Pause, Stop, Record, Loop, Skip Forward, Skip Back
- Navigation: ChevronLeft, ChevronRight, ChevronDown, ChevronUp
- Actions: Plus, Minus, Close, Settings, Search, Menu
- Content: Folder, File, Audio, MIDI, Plugin
- Track: Solo, Mute, Arm/Record, Freeze

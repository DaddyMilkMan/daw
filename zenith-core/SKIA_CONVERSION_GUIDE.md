# Complete Guide: Converting Zenith DAW to Native Skia Rendering

## Current Status ✅
Your app is already using Skia with **hybrid rendering**:
- ✅ SkiaRenderer is initialized and working
- ✅ Infrastructure is in place (SkiaComponent interface)
- ✅ MainComponent already checks for and calls `paintToSkia()`
- ⚠️ Most components use JUCE fallback (slower, less efficient)

## Goal
Convert all UI components to use **native Skia rendering** for maximum performance.

---

## Architecture Overview

### How Skia Rendering Works in Your Setup

```
MainComponent::paint()
    └─> SkiaRenderer::render(callback)
        └─> For each child component:
            ├─> Check: dynamic_cast<SkiaComponent*>(child)
            ├─> If SkiaComponent: call child->paintToSkia(canvas, bounds)  ← FAST ✓
            └─> Else: Render to JUCE Image → Convert → Blit to Skia       ← SLOW ✗
```

**Fallback path** (current for most components):
1. Create juce::Image
2. Render component to JUCE Graphics
3. Convert pixels to Skia format
4. Copy to SkCanvas
= **Expensive!** Data copying, format conversion

**Native Skia path** (goal):
1. Call `paintToSkia(canvas, bounds)`  
2. Component draws directly to SkCanvas
= **Fast!** No conversion, GPU-accelerated

---

## Step-by-Step Conversion Process

### Template for Converting a Component

#### Before (JUCE-only):
```cpp
class MyComponent : public juce::Component
{
public:
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::darkgrey);
        g.setColour(juce::Colours::white);
        g.drawText("Hello", 10, 10, 100, 20, juce::Justification::left);
    }
};
```

#### After (Skia-native):
```cpp
#include "../ui/skia/SkiaComponent.h"
#include <include/core/SkCanvas.h>
#include <include/core/SkPaint.h>
#include <include/core/SkFont.h>

class MyComponent : public juce::Component,
                    public zenith::SkiaComponent  // ← Add this!
{
public:
    // Keep the JUCE paint() for fallback
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::darkgrey);
        g.setColour(juce::Colours::white);
        g.drawText("Hello", 10, 10, 100, 20, juce::Justification::left);
    }

    // NEW: Implement native Skia rendering
    void paintToSkia(SkCanvas* canvas, SkRect bounds) override
    {
        // Background
        SkPaint bgPaint;
        bgPaint.setColor(0xFF2A2A2A); // ARGB format
        canvas->drawRect(bounds, bgPaint);

        // Text
        SkPaint textPaint;
        textPaint.setColor(SK_ColorWHITE);
        textPaint.setAntiAlias(true);
        
        SkFont font;
        font.setSize(14);
        
        canvas->drawString("Hello", bounds.x() + 10, bounds.y() + 20, font, textPaint);
    }

    // Tell the system we support Skia
    bool supportsSkiaRendering() const override { return true; }
};
```

---

## Priority Components to Convert

### High Priority (Most Rendered)

1. **MixerComponent** - renders every frame for meters
2. **ArrangerComponent** - timeline, waveforms
3. **Transport controls** - buttons, position display
4. **Meters** - VU meters, level displays
5. **Knobs/Sliders** - continuous parameter controls

### Medium Priority

6. **Instrument Browser**
7. **Effect panels**
8. **Piano Roll**
9. **Waveform displays**

### Low Priority (Static UI)

10. **Buttons that don't animate**
11. **Labels**
12. **Menu bars**

---

## Skia Drawing Cheat Sheet

### Common JUCE → Skia Conversions

| JUCE | Skia | Notes |
|------|------|-------|
| `g.fillAll(colour)` | `canvas->clear(SkColorSetARGB(a,r,g,b))` | Full background |
| `g.setColour(colour)` | `paint.setColor(SkColorSetARGB(a,r,g,b))` | Set paint color |
| `g.fillRect(x,y,w,h)` | `canvas->drawRect(SkRect::MakeXYWH(x,y,w,h), paint)` | Filled rectangle |
| `g.drawRect(x,y,w,h)` | `paint.setStyle(SkPaint::kStroke_Style); canvas->drawRect(...)` | Outlined rect |
| `g.drawLine(x1,y1,x2,y2)` | `canvas->drawLine(x1, y1, x2, y2, paint)` | Line |
| `g.drawEllipse(x,y,w,h)` | `canvas->drawOval(SkRect::MakeXYWH(x,y,w,h), paint)` | Ellipse |
| `g.fillEllipse(x,y,w,h)` | Same but `paint.setStyle(kFill_Style)` | Filled ellipse |
| `g.drawText(text, x, y, w, h, just)` | `canvas->drawString(text, x, y, font, paint)` | Simple text |

### Colors in Skia

```cpp
// ARGB format (Alpha, Red, Green, Blue)
SkColor myColor = SkColorSetARGB(255, 255, 100, 50);  // Opaque orange

// Pre-defined colors
SK_ColorBLACK, SK_ColorWHITE, SK_ColorRED, SK_ColorBLUE, SK_ColorGREEN

// From JUCE  Colour
juce::Colour juceColour = juce::Colours::cyan;
SkColor skiaColour = SkColorSetARGB(
    juceColour.getAlpha(),
    juceColour.getRed(),
    juceColour.getGreen(),
    juceColour.getBlue()
);
```

### Gradients

```cpp
SkPoint points[2] = { {0, 0}, {0, 100} };  // Top to bottom
SkColor colors[2] = { SK_ColorBLUE, SK_ColorRED };
SkScalar positions[2] = { 0.0f, 1.0f };

sk_sp<SkShader> gradient = SkGradientShader::MakeLinear(
    points, colors, positions, 2, SkTileMode::kClamp
);

SkPaint paint;
paint.setShader(gradient);
canvas->drawRect(SkRect::MakeWH(100, 100), paint);
```

### Rounded Rectangles

```cpp
SkRRect roundedRect;
roundedRect.setRectXY(SkRect::MakeXYWH(10, 10, 100, 50), 5, 5);  // 5px radius

SkPaint paint;
paint.setColor(SK_ColorBLUE);
canvas->drawRRect(roundedRect, paint);
```

### Paths (Complex Shapes)

```cpp
SkPath path;
path.moveTo(10, 10);
path.lineTo(50, 100);
path.lineTo(90, 10);
path.close();

SkPaint paint;
paint.setColor(SK_ColorGREEN);
paint.setAntiAlias(true);
canvas->drawPath(path, paint);
```

### Text Rendering

```cpp
SkFont font;
font.setSize(16);
// font.setTypeface(customTypeface);  // Optional

SkPaint textPaint;
textPaint.setColor(SK_ColorWHITE);
textPaint.setAntiAlias(true);

canvas->drawString("Hello Skia!", 10, 30, font, textPaint);
```

### Shadows and Effects

```cpp
// Drop shadow
SkPaint shadowPaint;
shadowPaint.setColor(SkColorSetARGB(80, 0, 0, 0));  // Semi-transparent black
shadowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 3.0f));

canvas->drawRect(SkRect::MakeXYWH(12, 12, 100, 50), shadowPaint);  // Shadow
canvas->drawRect(SkRect::MakeXYWH(10, 10, 100, 50), mainPaint);    // Object
```

---

## Example: Convert ArrangerComponent

### Current (JUCE):
```cpp
void ArrangerComponent::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xff1a1a1a));
    
    // Grid lines
    g.setColour(juce::Colour(0xff2a2a2a));
    for (int i = 0; i < getWidth(); i += 50)
        g.drawVerticalLine(i, 0.0f, (float)getHeight());
    
    // Waveform
    g.setColour(juce::Colour(0xff4a9eff));
    juce::Path waveform;
    // ... draw waveform
    g.strokePath(waveform, juce::PathStrokeType(2.0f));
}
```

### Converted (Skia):
```cpp
#include "../ui/skia/SkiaComponent.h"

class ArrangerComponent : public juce::Component,
                          public zenith::SkiaComponent
{
public:
    // Keep JUCE paint() for fallback
    void paint(juce::Graphics& g) override { /* existing code */ }
    
    // NEW: Native Skia rendering
    void paintToSkia(SkCanvas* canvas, SkRect bounds) override
    {
        // Background - GPU accelerated!
        canvas->clear(0xFF1A1A1A);
        
        // Grid lines - single draw call with path
        SkPath gridPath;
        SkPaint gridPaint;
        gridPaint.setColor(0xFF2A2A2A);
        gridPaint.setStrokeWidth(1.0f);
        gridPaint.setStyle(SkPaint::kStroke_Style);
        gridPaint.setAntiAlias(true);
        
        for (float x = 0; x < bounds.width(); x += 50)
        {
            gridPath.moveTo(bounds.x() + x, bounds.y());
            gridPath.lineTo(bounds.x() + x, bounds.y() + bounds.height());
        }
        canvas->drawPath(gridPath, gridPaint);
        
        // Waveform - GPU accelerated path rendering
        SkPath waveformPath;
        SkPaint waveformPaint;
        waveformPaint.setColor(0xFF4A9EFF);
        waveformPaint.setStrokeWidth(2.0f);
        waveformPaint.setStyle(SkPaint::kStroke_Style);
        waveformPaint.setAntiAlias(true);
        
        // ... build waveformPath using moveTo/lineTo ...
        canvas->drawPath(waveformPath, waveformPaint);
    }
    
    bool supportsSkiaRendering() const override { return true; }
};
```

---

## Performance Optimizations

### 1. Cache Static Content
```cpp
class MyComponent : public juce::Component, public zenith::SkiaComponent
{
    sk_sp<SkPicture> cachedBackground_;  // Cache expensive drawing
    
    void updateCache(SkCanvas* canvas)
    {
        SkPictureRecorder recorder;
        SkCanvas* recordCanvas = recorder.beginRecording(bounds);
        
        // Draw expensive background once
        // ... complex gradients, shadows, etc ...
        
        cachedBackground_ = recorder.finishRecordingAsPicture();
    }
    
    void paintToSkia(SkCanvas* canvas, SkRect bounds) override
    {
        if (!cachedBackground_)
            updateCache(canvas);
            
        // Fast playback!
        canvas->drawPicture(cachedBackground_);
        
        // Draw dynamic content only
        // ... meters, waveforms, etc ...
    }
};
```

### 2. Use GPU Layers for Transparency
```cpp
void paintToSkia(SkCanvas* canvas, SkRect bounds) override
{
    canvas->saveLayerAlpha(&bounds, 200);  // 200/255 opacity
    
    // Everything drawn here is composited with alpha
    drawComplexStuff(canvas);
    
    canvas->restore();
}
```

### 3. Clip Regions
```cpp
void paintToSkia(SkCanvas* canvas, SkRect bounds) override
{
    canvas->save();
    canvas->clipRect(bounds);  // Only draw inside bounds
    
    // Drawing is clipped
    
    canvas->restore();
}
```

---

## Testing Your Conversion

### 1. Check Rendering Stats

Your MainComponent already logs this! Look for:
```
RENDERING STATISTICS (TRUTHFUL)
  ✓ Native Skia rendering: X components
  ✗ JUCE fallback: Y components
```

**Goal:** Get Y to 0!

### 2. Visual Verification

Both implementations should look **identical**. If you see differences:
- Check color format (ARGB vs RGBA)
- Check coordinate system (Skia uses floats)
- Check anti-aliasing settings

### 3. Performance Measurement

```cpp
auto start = juce::Time::getHighResolutionTicks();
renderer_->render([](SkCanvas* canvas) { /* ... */ });
auto end = juce::Time::getHighResolutionTicks();
auto ms = juce::Time::highResolutionTicksToSeconds(end - start) * 1000.0;
DBG("Frame time: " << ms << "ms");
```

---

## Common Pitfalls & Solutions

### Problem: "Use of undeclared identifier 'SkCanvas'"
**Solution:** Include Skia headers:
```cpp
#ifdef ZENITH_USE_SKIA
#include <include/core/SkCanvas.h>
#include <include/core/SkPaint.h>
#include <include/core/SkFont.h>
#include <include/core/SkPath.h>
#endif
```

### Problem: Colors look wrong
**Solution:** Skia uses **premultiplied alpha**:
```cpp
// Wrong
SkColor bad = (a << 24) | (r << 16) | (g << 8) | b;

// Correct
SkColor good = SkColorSetARGB(a, r, g, b);  // Handles premultiplication
```

### Problem: Text doesn't render
**Solution:** Create SkFont properly:
```cpp
SkFont font;
font.setSize(14);
font.setEdging(SkFont::Edging::kAntiAlias);  // Smooth text

// On Windows, you may need a typeface:
sk_sp<SkTypeface> typeface = SkTypeface::MakeFromName("Arial", SkFontStyle());
font.setTypeface(typeface);
```

### Problem: Performance is same/worse
**Solution:** Check that `supportsSkiaRendering()` returns `true` and component inherits from `SkiaComponent`

---

## Next Steps

1. **Start with MixerComponent** (highest impact)
2. **Convert one component at a time**
3. **Test each conversion** before moving to next
4. **Check rendering stats** after each conversion
5. **Measure FPS improvement**

Each component you convert:
- ✅ Reduces CPU usage
- ✅ Increases FPS
- ✅ Enables GPU acceleration
- ✅ Improves animation smoothness

---

## Need Help?

Reference implementations in your codebase:
- `Source/ui/skia/SkiaButtonComponent.cpp` - Button rendering
- `Source/ui/skia/SkiaKnobComponent.cpp` - Rotary knob
- `Source/ui/skia/SkiaSliderComponent.cpp` - Slider/fader

These show complete Skia implementations for common UI elements!

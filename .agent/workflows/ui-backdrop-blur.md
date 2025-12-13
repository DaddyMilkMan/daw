---
description: Implement REAL glassmorphism with backdrop blur - no more fake panels
---

# UI Fix #2: Real Backdrop Blur (Glassmorphism Done Right)

## MISSION
The current "GlassmorphicPanel" is a FRAUD. It's just a dark gradient with edge highlights. Real glassmorphism requires blurring the content BEHIND the panel. Fix this embarrassment.

## PRE-TASK RESEARCH (MANDATORY)

1. **Web Search**: "Skia backdrop blur SkImageFilters::Blur example"
   - Understand how to capture and blur what's behind a region
   
2. **Web Search**: "Skia saveLayer with blur filter performance"
   - Check performance implications of backdrop blur
   - Look for optimization techniques (reduced blur quality for large areas)
   
3. **Web Search**: "macOS vibrancy effect implementation Skia"
   - See how Apple-style blur is achieved
   
4. **Web Search**: "CSS backdrop-filter blur equivalent native"
   - Understand the conceptual model
   
5. **Web Search**: "Skia SkRuntimeEffect shader blur GPU"
   - Check if custom shaders are faster than SkImageFilters

## THE PROBLEM

Current fake glassmorphism (`GlassmorphicPanel.h`):
```cpp
// This is NOT blur. This is just transparency.
paint.setColor(withAlpha(colors::BG_DARK, 0.8f));
canvas->drawRRect(rrect, paint);
```

What we NEED:
```cpp
// 1. Save current layer
// 2. Clip to panel bounds
// 3. Apply blur filter to everything rendered before
// 4. Draw semi-transparent tinted overlay on top
// 5. Restore
```

## IMPLEMENTATION STEPS

### Step 1: Create BackdropBlur Utility
Create `Source/ui/skia/BackdropBlur.h`:

```cpp
class BackdropBlur {
public:
    // Call BEFORE drawing panel content
    static void beginBlur(SkCanvas* canvas, const SkRect& bounds, 
                          float blurRadius, SkColor tintColor);
    
    // Call AFTER drawing panel content
    static void endBlur(SkCanvas* canvas);
    
    // Simple one-shot for basic panels
    static void drawBlurredRect(SkCanvas* canvas, const SkRect& bounds,
                                float cornerRadius, float blurRadius,
                                SkColor tintColor, float tintOpacity);
};
```

### Step 2: Implement Backdrop Blur Logic

The key insight: You need to render the blurred version by:
1. Using `SkCanvas::saveLayer()` with an `SkImageFilter`
2. The filter applies to ALL pixels within the layer bounds
3. Draw the tint/overlay on top

Reference implementation pattern:
```cpp
void BackdropBlur::drawBlurredRect(SkCanvas* canvas, const SkRect& bounds,
                                   float cornerRadius, float blurRadius,
                                   SkColor tintColor, float tintOpacity) {
    // Create blur filter
    sk_sp<SkImageFilter> blur = SkImageFilters::Blur(
        blurRadius, blurRadius, SkTileMode::kClamp, nullptr);
    
    SkPaint layerPaint;
    layerPaint.setImageFilter(blur);
    
    // Save layer - this captures and blurs background
    canvas->saveLayer(SkCanvas::SaveLayerRec(&bounds, &layerPaint, nullptr, 0));
    
    // Critical: We need the "src" content to be what's ALREADY there
    // This is the tricky part - we may need to use SkPictureRecorder
    // or render-to-texture approaches depending on Skia context
    
    canvas->restore();
    
    // Now draw tinted overlay
    SkPaint overlayPaint;
    overlayPaint.setColor(SkColorSetA(tintColor, (int)(tintOpacity * 255)));
    overlayPaint.setAntiAlias(true);
    
    if (cornerRadius > 0) {
        canvas->drawRRect(SkRRect::MakeRectXY(bounds, cornerRadius, cornerRadius), 
                          overlayPaint);
    } else {
        canvas->drawRect(bounds, overlayPaint);
    }
}
```

### Step 3: Handle The "Chicken and Egg" Problem

**THIS IS THE HARD PART**. Backdrop blur requires knowing what's behind the panel. Solutions:

**Option A: Two-Pass Rendering**
1. First pass: Render everything EXCEPT glass panels to an offscreen surface
2. Second pass: For each glass panel, sample the offscreen surface with blur
3. Third pass: Composite glass panels on top

**Option B: GPU Shader Approach**
- Use framebuffer read-back in shader
- May not work with all Skia backends

**Option C: Reduced Scope**
- Only blur panels that sit over static/predictable backgrounds
- Pre-compute blurred versions of background regions

**Web Search**: "Skia render to texture offscreen surface GPU" to find best approach

### Step 4: Update GlassmorphicPanel.h

Replace fake blur with real blur:
```cpp
static void draw(SkCanvas* canvas, const SkRect& bounds, Style style) {
    float blurRadius = 0.0f;
    switch (style) {
        case Style::Subtle:   blurRadius = 8.0f; break;
        case Style::Elevated: blurRadius = 16.0f; break;
        case Style::Floating: blurRadius = 24.0f; break;
        // ...
    }
    
    if (blurRadius > 0 && Settings::getGlowIntensity() > 0.01f) {
        BackdropBlur::drawBlurredRect(canvas, bounds, 
            dimensions::RADIUS_LG, blurRadius,
            colors::BG_DARK, 0.7f);
    } else {
        // Fallback to solid for "flat mode" or performance
        drawSolidPanel(canvas, bounds);
    }
    
    // Rest: highlights, borders, glow...
}
```

### Step 5: Performance Optimization

Add configurable blur quality in Settings:
```cpp
enum class BlurQuality { Off, Low, Medium, High };
static BlurQuality backdropBlurQuality = BlurQuality::Medium;

// In blur implementation:
float effectiveBlurRadius = blurRadius;
switch (backdropBlurQuality) {
    case BlurQuality::Low:    effectiveBlurRadius *= 0.5f; break;
    case BlurQuality::Medium: effectiveBlurRadius *= 0.75f; break;
    case BlurQuality::High:   break; // full quality
    case BlurQuality::Off:    return; // skip entirely
}
```

### Step 6: Apply to Key Panels
- TransportBar
- RightSidePanel (Wingman)
- BrowserPanel
- Modal dialogs

## VERIFICATION CHECKLIST
- [ ] Panels visually blur content behind them (not just darken)
- [ ] Performance stays above 60fps with 3+ glass panels visible
- [ ] Settings toggle can disable blur for low-end systems
- [ ] Build succeeds on Debug and Release
- [ ] Web search confirms blur technique is valid for Skia/GPU context

## ACCEPTANCE CRITERIA
Move a colorful waveform behind a glass panel. If the waveform colors are NOT visibly blurred/diffused through the panel, you have failed. Take before/after screenshots.

## KNOWN PITFALLS
- Skia's `SkImageFilter::Blur` on saveLayer blurs the LAYER CONTENT, not what's behind
- You may need `SkPictureRecorder` or dual-surface approach
- Test on both D3D12 and OpenGL backends

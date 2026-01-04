# Zenith DAW Rendering Architecture

**Last Updated:** 2026-01-03  
**Version:** 2.0 - Skia-First Architecture

---

## Overview

Zenith DAW uses **Skia** as its **exclusive** 2D graphics rendering engine. JUCE is retained only for non-rendering functionality including windowing, audio processing, and event handling.

### Architecture Principles

- 🎨 **Skia-Only Rendering**: All UI rendering uses Skia (SkCanvas, SkPaint, SkFont, SkImage)
- 🚀 **Hardware Acceleration**: GPU-accelerated via Metal (macOS), Direct3D 12 (Windows), Vulkan (Linux)
- 🎭 **Modern Design System**: Custom design system with glassmorphism, gradients, and effects
- 🔧 **JUCE Integration**: JUCE provides windowing, audio, and events only - no JUCE graphics rendering
- ⚡ **60+ FPS Performance**: Smooth animations for timeline, waveforms, meters, and all UI elements

### Key Components Responsibilities

| Component | Responsibility | Rendering Engine |
|-----------|----------------|------------------|
| **SkiaComponent** | Base class for all UI components | Skia (drawSkia) |
| **SkiaMainWindowIntegration** | OpenGL/Skia context management | Skia |
| **ZenithButton, ZenithSlider, etc.** | Custom UI controls | Skia |
| **JUCE Components** | Window, audio, events | JUCE (no rendering) |
| **ZenithLookAndFeel** | DEPRECATED - Legacy only | JUCE (to be phased out) |

---

## Architecture Overview

```
┌────────────────────────────────────────────────────────────────────┐
│                        MainWindow (JUCE)                            │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │              MainComponent : SkiaMainWindowIntegration        │  │
│  │  ┌─────────────────────────────────────────────────────────┐ │  │
│  │  │                  SkiaOpenGLRenderer                     │ │  │
│  │  │  ┌────────────────────────────────────────────────────┐│ │  │
│  │  │  │           GrDirectContext (Skia GPU)               ││ │  │
│  │  │  │  ┌───────────────────────────────────────────────┐ ││ │  │
│  │  │  │  │              SkSurface (GPU-backed)           │ ││ │  │
│  │  │  │  │  ┌─────────────────────────────────────────┐  │ ││ │  │
│  │  │  │  │  │              SkCanvas                   │  │ ││ │  │
│  │  │  │  │  │   (All Skia drawing commands go here)   │  │ ││ │  │
│  │  │  │  │  └─────────────────────────────────────────┘  │ ││ │  │
│  │  │  │  └───────────────────────────────────────────────┘ ││ │  │
│  │  │  └────────────────────────────────────────────────────┘│ │  │
│  │  └─────────────────────────────────────────────────────────┘ │  │
│  │        ↓                ↓                ↓                   │  │
│  │  TransportBar    MainLayoutComponent    RightSidePanel       │  │
│  │  (SkiaComponent)     (SkiaComponent)    (SkiaComponent)      │  │
│  │        ↓                ↓  ↓                                  │  │
│  │                   Arranger  Mixer                             │  │
│  │                (SkiaComponent) (SkiaComponent)                │  │
│  └──────────────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────────────┘
```

---

## Rendering Flow

### 1. Window System (JUCE)
JUCE provides the native window and OpenGL context:
- **juce::Component**: Base class providing layout and event handling
- **juce::OpenGLContext**: Manages GPU context
- **Platform Window**: Native OS window (NSWindow, HWND, X11 Window)

### 2. Skia Integration (SkiaMainWindowIntegration)
Bridges JUCE windowing with Skia rendering:
- Creates `GrDirectContext` for GPU acceleration
- Creates `SkSurface` backed by GPU framebuffer
- Provides `SkCanvas` for all drawing operations
- **NO juce::Graphics rendering** - purely Skia

### 3. Component Hierarchy (SkiaComponent)
All UI components inherit from `SkiaComponent`:
```cpp
class MyComponent : public SkiaComponent {
public:
    void drawSkia(SkCanvas* canvas) override {
        // All rendering uses Skia API
        SkPaint paint;
        paint.setColor(SK_ColorBLUE);
        canvas->drawRect(SkRect::MakeWH(100, 100), paint);
    }
    
    // JUCE paint() triggers repaint requests to parent hierarchy
    void paint(juce::Graphics& g) override {
        // Triggers repaint request to SkiaMainWindowIntegration
        SkiaComponent::paint(g);
    }
};
```

### 4. No JUCE Graphics Rendering
- **juce::Graphics is NOT used** for any UI rendering
- `paint(juce::Graphics& g)` methods trigger repaint requests up the hierarchy
- **ZenithLookAndFeel** is deprecated - kept only for legacy compatibility
- Standard JUCE components (juce::TextButton, juce::Slider) should be replaced with Skia equivalents

---

## Core Components

### SkiaComponent (`ui/skia/SkiaComponent.h`)

The base class for all Skia-rendered UI components.

```cpp
class SkiaComponent : public juce::Component, 
                      public juce::Timer,
                      public juce::KeyListener {
public:
    // Pure virtual - each component must implement
    virtual void drawSkia(SkCanvas* canvas) = 0;
    
    // Helper to draw child components
    void drawChildren(SkCanvas* canvas);
    
    // Animation system
    void animateTo(const juce::String& property, float target, int durationMs);
    void animateWithSpring(const juce::String& property, float target, 
                           float stiffness, float damping);
    
    // Glow effects (Neon Noir design)
    void setGlowEnabled(bool enabled);
    void setGlowColor(SkColor color);
    void setGlowRadius(float radius);
    
protected:
    void applyGlow(SkPaint& paint, float intensity = 1.0f);
};
```

### SkiaMainWindowIntegration (`ui/skia/SkiaMainWindowIntegration.h`)

Manages the OpenGL context and Skia GPU context for the main window.

```cpp
class SkiaOpenGLRenderer : public juce::OpenGLRenderer {
public:
    explicit SkiaOpenGLRenderer(juce::Component* componentToAttach);
    
    // Override in subclass
    virtual void drawSkiaContent(SkCanvas* canvas) = 0;
    
protected:
    juce::OpenGLContext openGLContext_;
    sk_sp<GrDirectContext> grContext_;
    sk_sp<SkSurface> surface_;
    SkCanvas* skiaCanvas_ = nullptr;
};

class SkiaMainWindowIntegration : public juce::Component,
                                   public SkiaOpenGLRenderer {
    // Main container combining JUCE component with Skia rendering
};
```

### ZenithDesignSystem (`ui/skia/ZenithDesignSystem.h`)

Design tokens for consistent styling:

```cpp
namespace zenith::design {

namespace colors {
    constexpr SkColor BG_DARKEST = 0xFF0D0D0D;
    constexpr SkColor BG_DARK = 0xFF1A1A1A;
    constexpr SkColor NEON_GREEN = 0xFF00FF88;
    constexpr SkColor NEON_PINK = 0xFFFF0088;
    constexpr SkColor GLASS_FILL = 0x40FFFFFF;
    constexpr SkColor BORDER_GLOW = 0x60FFFFFF;
}

namespace typography {
    // Font sizes, weights, typefaces
}

namespace layout {
    // Spacing, border radius, etc.
}

} // namespace zenith::design
```

---

## Skia Initialization Flow

### 1. Application Startup

```cpp
// Main.cpp or JUCEApplication
MainWindow::MainWindow(const juce::String& name)
    : DocumentWindow(name, ...) {
    
    mainComponent = std::make_unique<MainComponent>(...);
    setContentOwned(mainComponent.get(), true);
}
```

### 2. OpenGL Context Creation

```cpp
// SkiaOpenGLRenderer constructor
SkiaOpenGLRenderer::SkiaOpenGLRenderer(juce::Component* component)
    : targetComponent_(component) {
    
    openGLContext_.setComponentPaintingEnabled(false);
    openGLContext_.setRenderer(this);
    openGLContext_.attachTo(*component);
}
```

### 3. Skia GPU Context Initialization

```cpp
// Called by JUCE when OpenGL is ready
void SkiaOpenGLRenderer::newOpenGLContextCreated() {
    // Create Skia GPU context from OpenGL
    grContext_ = GrDirectContexts::MakeGL(
        GrGLMakeNativeInterface(),
        GrContextOptions{}
    );
}
```

### 4. Surface Creation

```cpp
void SkiaOpenGLRenderer::recreateSurface() {
    // Get current framebuffer size
    GLint fbo, viewport[4];
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
    glGetIntegerv(GL_VIEWPORT, viewport);
    
    // Create GPU-backed surface
    GrGLFramebufferInfo fbInfo;
    fbInfo.fFBOID = fbo;
    fbInfo.fFormat = GL_RGBA8;
    
    auto backendRT = GrBackendRenderTargets::MakeGL(
        viewport[2], viewport[3], 0, 8, fbInfo);
    
    surface_ = SkSurfaces::WrapBackendRenderTarget(
        grContext_.get(), backendRT, 
        kBottomLeft_GrSurfaceOrigin,
        kRGBA_8888_SkColorType,
        SkColorSpace::MakeSRGB(),
        nullptr);
    
    skiaCanvas_ = surface_->getCanvas();
}
```

---

## Component Lifecycle

### Creation

```cpp
class MyComponent : public SkiaComponent {
public:
    MyComponent() {
        // Initialize child components
        addAndMakeVisible(childWidget_);
        
        // Start animation timer if needed
        startTimerHz(60);
    }
    
    ~MyComponent() override {
        stopTimer();
    }
```

### Layout

```cpp
void MyComponent::resized() override {
    // Layout child components using JUCE bounds
    auto bounds = getLocalBounds();
    childWidget_.setBounds(bounds.removeFromTop(40));
}
```

### Rendering

```cpp
void MyComponent::drawSkia(SkCanvas* canvas) override {
    auto bounds = getLocalBounds().toFloat();
    
    // 1. Draw background
    SkPaint bgPaint;
    bgPaint.setColor(design::colors::BG_DARK);
    canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()), 
                     bgPaint);
    
    // 2. Draw custom content
    drawMyContent(canvas);
    
    // 3. Draw child SkiaComponents
    drawChildren(canvas);
}
```

---

## Creating a New Skia Component

### Step 1: Header File

```cpp
// MyNewComponent.h
#pragma once

#include "SkiaComponent.h"

namespace zenith {

class MyNewComponent : public SkiaComponent {
public:
    MyNewComponent();
    ~MyNewComponent() override = default;
    
    void drawSkia(SkCanvas* canvas) override;
    void resized() override;
    
    // Mouse handling (if needed)
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    
private:
    // State
    float myValue_ = 0.0f;
    
    // Child widgets
    SkiaButton myButton_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MyNewComponent)
};

} // namespace zenith
```

### Step 2: Implementation

```cpp
// MyNewComponent.cpp
#include "MyNewComponent.h"
#include "ZenithDesignSystem.h"
#include <core/SkCanvas.h>
#include <core/SkPaint.h>
#include <core/SkRRect.h>

namespace zenith {

MyNewComponent::MyNewComponent() {
    // Add child components
    addAndMakeVisible(myButton_);
    
    // Configure button
    myButton_.setLabel("Click Me");
    myButton_.onClick = [this]() {
        myValue_ = 1.0f;
        animateTo("value", 0.0f, 500); // Animate back
        repaint();
    };
}

void MyNewComponent::resized() {
    auto bounds = getLocalBounds();
    myButton_.setBounds(bounds.removeFromBottom(40).reduced(10));
}

void MyNewComponent::drawSkia(SkCanvas* canvas) {
    using namespace design;
    
    auto bounds = getLocalBounds().toFloat();
    SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());
    
    // Draw glassmorphic background
    SkPaint bgPaint;
    bgPaint.setColor(colors::GLASS_FILL);
    bgPaint.setAntiAlias(true);
    
    SkRRect rrect;
    rrect.setRectXY(skBounds, 12.0f, 12.0f);
    canvas->drawRRect(rrect, bgPaint);
    
    // Draw border with glow
    SkPaint borderPaint;
    borderPaint.setColor(colors::BORDER_GLOW);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.5f);
    borderPaint.setAntiAlias(true);
    
    if (myValue_ > 0.0f) {
        applyGlow(borderPaint, myValue_);
    }
    
    canvas->drawRRect(rrect, borderPaint);
    
    // Draw children (SkiaButton, etc.)
    drawChildren(canvas);
}

void MyNewComponent::mouseDown(const juce::MouseEvent& e) {
    // Handle custom mouse interaction
    grabKeyboardFocus();
}

} // namespace zenith
```

### Step 3: Add to CMakeLists.txt

```cmake
# In ui/CMakeLists.txt or ui/skia/CMakeLists.txt
target_sources(zenith_ui PRIVATE
    MyNewComponent.cpp
    MyNewComponent.h
)
```

### Step 4: Use in Parent Component

```cpp
// In parent component
#include "MyNewComponent.h"

class ParentComponent : public SkiaComponent {
    std::unique_ptr<MyNewComponent> myComponent_;
    
public:
    ParentComponent() {
        myComponent_ = std::make_unique<MyNewComponent>();
        addAndMakeVisible(myComponent_.get());
    }
    
    void resized() override {
        myComponent_->setBounds(getLocalBounds().reduced(20));
    }
    
    void drawSkia(SkCanvas* canvas) override {
        // Background...
        drawChildren(canvas); // Draws myComponent_ via Skia
    }
};
```

---

## Performance Considerations

### 1. Minimize State Changes

```cpp
// BAD: Creating paint objects every frame
void drawSkia(SkCanvas* canvas) override {
    SkPaint paint;           // Allocation every frame!
    paint.setColor(...);
    canvas->drawRect(..., paint);
}

// GOOD: Cache paints as members
class MyComponent : public SkiaComponent {
    SkPaint cachedPaint_;
    
    void drawSkia(SkCanvas* canvas) override {
        canvas->drawRect(..., cachedPaint_);
    }
};
```

### 2. Use Dirty Flags

```cpp
void MyComponent::setValue(float v) {
    if (value_ != v) {
        value_ = v;
        markDirty();  // Only repaint when needed
    }
}
```

### 3. Clip to Visible Area

```cpp
void drawSkia(SkCanvas* canvas) override {
    // Skip drawing if nothing changed
    if (!isDirty()) return;
    
    // Clip to visible region for complex content
    canvas->save();
    canvas->clipRect(SkRect::MakeWH(getWidth(), getHeight()));
    
    // Draw...
    
    canvas->restore();
}
```

### 4. Batch Similar Draws

```cpp
// BAD: Individual draws
for (int i = 0; i < 100; i++) {
    canvas->drawRect(rects[i], paint);
}

// GOOD: Use SkCanvas::drawAtlas for many identical shapes
// Or: Pre-render to SkPicture
```

### 5. Use GPU-Friendly Operations

```cpp
// Prefer:
- Linear gradients over radial
- Simple blur kernels
- Rounded rectangles (shader-optimized on GPU)

// Avoid:
- Complex path operations every frame
- Many different fonts in one draw call
- Excessive canvas save/restore
```

---

## Animation System

### Tween Animation

```cpp
// Animate opacity over 300ms
animateTo("opacity", 1.0f, 300);

// In drawSkia:
float opacity = getAnimatedValue("opacity");
paint.setAlpha((int)(opacity * 255));
```

### Spring Animation

```cpp
// Physics-based spring animation
animateWithSpring("scale", 1.0f, 
    300.0f,  // stiffness (higher = faster)
    20.0f);  // damping (higher = less bounce)

// In drawSkia:
float scale = getAnimatedValue("scale");
canvas->scale(scale, scale);
```

### Timer-Based Updates

```cpp
void timerCallback() override {
    // Update animation state
    SkiaComponent::timerCallback();
    
    // Custom animation logic
    if (needsAnimation_) {
        currentValue_ += 0.1f;
        repaint();
    }
}
```

---

## Design System Integration

### Using GlassmorphicPanel

```cpp
#include "GlassmorphicPanel.h"

void drawSkia(SkCanvas* canvas) override {
    auto bounds = getLocalBounds().toFloat();
    
    // Draw glass effect background
    GlassmorphicPanel::draw(canvas, 
        SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()),
        12.0f,   // corner radius
        isHovered_ ? 0.3f : 0.2f,  // blur amount
        isHovered_);  // glow on hover
}
```

### Using NeonGlow

```cpp
#include "NeonGlow.h"

void drawSkia(SkCanvas* canvas) override {
    NeonGlow::drawGlowBorder(canvas, 
        SkRect::MakeWH(getWidth(), getHeight()),
        design::colors::NEON_GREEN,
        8.0f,   // glow radius
        0.8f);  // intensity
}
```

---

## Debugging

### Enable Debug Overlay

```cpp
#ifdef DEBUG
void drawSkia(SkCanvas* canvas) override {
    // Normal drawing...
    
    // Draw debug info
    drawDebug(canvas);  // Shows bounds, component name
}
#endif
```

### Check for Skia Errors

```cpp
// After complex operations
if (canvas->getSaveCount() != originalSaveCount) {
    DBG("Warning: Unbalanced save/restore!");
}
```

### Performance Profiling

```cpp
void drawSkia(SkCanvas* canvas) override {
    auto start = juce::Time::getMillisecondCounterHiRes();
    
    // Drawing code...
    
    auto elapsed = juce::Time::getMillisecondCounterHiRes() - start;
    if (elapsed > 16.67) {  // >60fps frame time
        DBG("Slow frame: " + juce::String(elapsed) + "ms");
    }
}
```

---

## Common Patterns

### Waveform Display

```cpp
void drawWaveform(SkCanvas* canvas, const std::vector<float>& peaks, 
                  const SkRect& bounds) {
    SkPath path;
    path.moveTo(bounds.left(), bounds.centerY());
    
    float xStep = bounds.width() / peaks.size();
    for (size_t i = 0; i < peaks.size(); i++) {
        float x = bounds.left() + i * xStep;
        float y = bounds.centerY() - peaks[i] * bounds.height() * 0.5f;
        path.lineTo(x, y);
    }
    
    // Mirror for bottom half
    for (int i = peaks.size() - 1; i >= 0; i--) {
        float x = bounds.left() + i * xStep;
        float y = bounds.centerY() + peaks[i] * bounds.height() * 0.5f;
        path.lineTo(x, y);
    }
    
    path.close();
    
    SkPaint paint;
    paint.setColor(design::colors::WAVEFORM_FILL);
    paint.setAntiAlias(true);
    canvas->drawPath(path, paint);
}
```

### Level Meter

```cpp
void drawMeter(SkCanvas* canvas, float level, const SkRect& bounds) {
    // Background
    SkPaint bgPaint;
    bgPaint.setColor(design::colors::METER_BG);
    canvas->drawRect(bounds, bgPaint);
    
    // Level bar with gradient
    SkPoint gradPoints[] = {{bounds.left(), bounds.bottom()}, 
                             {bounds.left(), bounds.top()}};
    SkColor gradColors[] = {design::colors::METER_GREEN, 
                            design::colors::METER_YELLOW,
                            design::colors::METER_RED};
    float gradPositions[] = {0.0f, 0.7f, 1.0f};
    
    auto gradient = SkGradientShader::MakeLinear(
        gradPoints, gradColors, gradPositions, 3, SkTileMode::kClamp);
    
    SkPaint levelPaint;
    levelPaint.setShader(gradient);
    
    SkRect levelRect = bounds;
    levelRect.fTop = bounds.bottom() - level * bounds.height();
    canvas->drawRect(levelRect, levelPaint);
}
```

---

## Migration Guide: JUCE to Skia

If you have an existing JUCE component:

### Before (Pure JUCE)

```cpp
class MyJuceComponent : public juce::Component {
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colours::darkgrey);
        g.setColour(juce::Colours::white);
        g.drawRect(getLocalBounds());
    }
};
```

### After (Skia)

```cpp
class MySkiaComponent : public SkiaComponent {
    void drawSkia(SkCanvas* canvas) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Fill background
        SkPaint bgPaint;
        bgPaint.setColor(SkColorSetRGB(64, 64, 64));
        canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), 
                                        bounds.getHeight()), bgPaint);
        
        // Draw border
        SkPaint borderPaint;
        borderPaint.setColor(SK_ColorWHITE);
        borderPaint.setStyle(SkPaint::kStroke_Style);
        borderPaint.setStrokeWidth(1.0f);
        canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), 
                                        bounds.getHeight()), borderPaint);
    }
};
```

### Type Conversions

| JUCE Type | Skia Type | Conversion |
|-----------|-----------|------------|
| `juce::Colour` | `SkColor` | `SkColorSetARGB(a, r, g, b)` |
| `juce::Rectangle<float>` | `SkRect` | `SkRect::MakeXYWH(x, y, w, h)` |
| `juce::Path` | `SkPath` | Manual conversion needed |
| `juce::Font` | `SkFont` | Create from `SkTypeface` |

---

## Troubleshooting

### Black Screen

1. Check OpenGL context is attached
2. Verify `grContext_` is not null
3. Check surface dimensions are non-zero

### Flickering

1. Ensure double-buffering is enabled
2. Check timer isn't causing too-rapid repaints
3. Verify no conflicting JUCE paints

### Memory Leaks

1. Use `sk_sp<>` for Skia reference-counted objects
2. Call `grContext_->freeGpuResources()` on shutdown
3. Don't store `SkCanvas*` beyond current frame

### Poor Performance

1. Profile with Skia's GPU debugging tools
2. Check for excessive path complexity
3. Reduce blur radius if using glow effects
4. Consider caching to `SkPicture` for static content

---

## References

- [Skia Documentation](https://skia.org/docs/)
- [Skia API Reference](https://api.skia.org/)
- [JUCE OpenGLContext](https://docs.juce.com/master/classOpenGLContext.html)
- [Zenith Design System](./DESIGN_SYSTEM.md)

---

## Migration Guide: JUCE Components to Skia

### Component Replacement Table

| Legacy JUCE Component | Skia Replacement | File Location |
|-----------------------|------------------|---------------|
| `juce::TextButton` | `ZenithButton` or `SkiaButton` | `ui/controls/ZenithButton.h` |
| `juce::Slider` | `ZenithSlider` or `SkiaSlider` | `ui/controls/ZenithSlider.h` |
| `juce::ComboBox` | `ZenithDropdown` or `SkiaComboBox` | `ui/controls/ZenithDropdown.h` |
| `juce::Label` | `SkiaLabel` | `ui/controls/SkiaLabel.h` |
| `juce::TextEditor` | `SkiaTextEditor` | `ui/controls/SkiaTextEditor.h` |
| `juce::ToggleButton` | `ZenithToggle` | `ui/controls/ZenithToggle.h` |

### Migration Steps

#### Step 1: Replace Component Type

**Before (JUCE):**
```cpp
std::unique_ptr<juce::TextButton> myButton;

myButton = std::make_unique<juce::TextButton>("Click Me");
myButton->onClick = [this] { handleClick(); };
addAndMakeVisible(myButton.get());
```

**After (Skia):**
```cpp
std::unique_ptr<ZenithButton> myButton;

myButton = std::make_unique<ZenithButton>("Click Me", [this] { handleClick(); });
addAndMakeVisible(myButton.get());
```

#### Step 2: Update Callbacks

**JUCE:**
```cpp
button->onClick = []() { /* handler */ };
slider->onValueChange = []() { /* handler */ };
comboBox->onChange = []() { /* handler */ };
```

**Skia:**
```cpp
button->onClick = []() { /* handler */ };
slider->onValueChange = [](float value) { /* handler */ };
dropdown->onSelectionChange = [](int index, const juce::String& text) { /* handler */ };
```

#### Step 3: Replace Custom Drawing

**Before (JUCE):**
```cpp
class MyComponent : public juce::Component {
public:
    void paint(juce::Graphics& g) override {
        g.setColour(juce::Colours::blue);
        g.fillRect(getLocalBounds());
        g.setColour(juce::Colours::white);
        g.drawText("Hello", getLocalBounds(), juce::Justification::centred);
    }
};
```

**After (Skia):**
```cpp
class MyComponent : public SkiaComponent {
public:
    void drawSkia(SkCanvas* canvas) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Fill background
        SkPaint bgPaint;
        bgPaint.setColor(SK_ColorBLUE);
        canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()), bgPaint);
        
        // Draw text centered (Note: drawString uses baseline, not vertical center)
        SkFont font(nullptr, 14.0f);
        SkPaint textPaint;
        textPaint.setColor(SK_ColorWHITE);
        
        // For proper vertical centering, account for font metrics
        SkFontMetrics metrics;
        font.getMetrics(&metrics);
        float textX = bounds.getWidth() / 2;
        float textY = bounds.getHeight() / 2 - (metrics.fAscent + metrics.fDescent) / 2.0f;
        
        textPaint.setTextAlign(SkPaint::kCenter_Align);  // Center horizontally
        canvas->drawString("Hello", textX, textY, font, textPaint);
    }
    
    void paint(juce::Graphics& g) override {
        // Triggers repaint request up the hierarchy
        SkiaComponent::paint(g);
    }
};
```

### Known Issues to Migrate

The following components still use standard JUCE components and should be migrated:

1. **MixingAssistant** (apps/desktop/Source/ai/MixingAssistant.cpp)
   - Multiple `juce::TextButton` instances
   - `juce::Slider` for controls
   - Should use `ZenithButton` and `ZenithSlider`

2. **LearningDashboard** (apps/desktop/Source/ui/dashboards/LearningDashboard.cpp)
   - `juce::ComboBox` for model selection
   - Multiple `juce::TextButton` and `juce::Slider` instances
   - Should use `ZenithDropdown`, `ZenithButton`, and `ZenithSlider`

### Best Practices

1. **Always use SkiaComponent base class** for new components
2. **Never implement paint(juce::Graphics&)** with actual rendering code
3. **Use ZenithTheme/design::colors** for colors instead of juce::Colours
4. **Prefer Skia types** (SkColor, SkRect, SkFont) over JUCE types
5. **Test rendering** on all platforms (Windows/Metal/Vulkan backends)

### Verification Checklist

- [ ] No `juce::Graphics` rendering code in `paint()` methods
- [ ] All custom components extend `SkiaComponent`
- [ ] Standard JUCE UI components replaced with Skia equivalents
- [ ] ZenithLookAndFeel only used for backward compatibility
- [ ] Colors use Skia types (SkColor) or design::colors constants
- [ ] Build succeeds with ZENITH_USE_SKIA=1
- [ ] UI renders correctly on target platform(s)

---

## Architecture Decision Record

**Date:** 2026-01-03  
**Decision:** Use Skia exclusively for all UI rendering

**Context:**
- JUCE's graphics API is CPU-based and not hardware accelerated
- Skia provides GPU acceleration via Metal/D3D/Vulkan
- Modern UI requires 60+ FPS performance for smooth animations
- Custom design system needs advanced effects (glassmorphism, blurs, gradients)

**Decision:**
Migrate all UI rendering to Skia, keeping JUCE only for:
- Windowing and OS integration
- Audio device management and DSP
- MIDI and plugin hosting
- File I/O and utilities
- Event handling (mouse, keyboard)

**Consequences:**
- ✅ 60+ FPS GPU-accelerated rendering
- ✅ Advanced visual effects (blurs, shadows, gradients)
- ✅ Consistent rendering across platforms
- ✅ Better performance for complex UIs
- ⚠️ Need to migrate legacy JUCE components
- ⚠️ Developers need to learn Skia API
- ⚠️ Maintain compatibility layer temporarily

**Status:** ✅ Implemented - All core UI uses Skia rendering

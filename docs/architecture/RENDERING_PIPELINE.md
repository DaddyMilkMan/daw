# Zenith DAW - Rendering Pipeline Architecture

**Author:** ArchitectAgent  
**Date:** January 1, 2025  
**Status:** Design Document  
**Priority:** HIGH (Agent 3 depends on this)

---

## 1. OVERVIEW

### 1.1 Problem Statement

The Zenith DAW requires a high-performance 2D rendering pipeline that:

1. **Integrates with JUCE's OpenGL context** - JUCE owns the GL thread
2. **Handles Wayland context invalidation** - Linux compositor can invalidate GL context at ANY time
3. **Wraps JUCE's FBO** - We cannot create our own framebuffers
4. **Targets 60fps minimum** - Adaptive up to 260Hz for high refresh displays  
5. **Handles resize without crashes** - Critical for window management

### 1.2 Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          JUCE Framework Layer                                │
│  ┌────────────────────────────────────────────────────────────────────────┐ │
│  │                    juce::OpenGLContext                                  │ │
│  │  - Owns GL thread (CRITICAL: never call makeCurrent ourselves)         │ │
│  │  - Creates/manages GL context lifecycle                                 │ │
│  │  - Handles buffer swapping automatically                                │ │
│  │  - Invokes our rendering callbacks                                      │ │
│  └────────────────┬───────────────────────────────────────────────────────┘ │
│                   │                                                          │
│                   │ attachTo(component)                                      │
│                   │                                                          │
├───────────────────┼──────────────────────────────────────────────────────────┤
│                   │       JUCE-Skia Integration Layer                        │
│  ┌────────────────▼───────────────────────────────────────────────────────┐ │
│  │                      MainComponent                                      │ │
│  │  - Implements juce::OpenGLRenderer interface                            │ │
│  │  - Owns Zenith::SkiaRenderer instance                                   │ │
│  │  - Routes OpenGL callbacks to Skia                                      │ │
│  │  - Queries component dimensions each frame                              │ │
│  └────────────────┬───────────────────────────────────────────────────────┘ │
│                   │                                                          │
│                   │ owns                                                     │
│                   │                                                          │
├───────────────────┼──────────────────────────────────────────────────────────┤
│                   │       Skia Rendering Layer                               │
│  ┌────────────────▼───────────────────────────────────────────────────────┐ │
│  │                     Zenith::SkiaRenderer                                │ │
│  │  - Wraps JUCE's FBO with GrBackendRenderTarget (NEVER creates own FBO) │ │
│  │  - Manages GrDirectContext (GPU context)                                │ │
│  │  - Creates SkSurface from backend render target each frame              │ │
│  │  - Provides SkCanvas* for all UI drawing                                │ │
│  │  - Validates context EVERY frame (Wayland requirement)                  │ │
│  │  - Handles context loss gracefully                                      │ │
│  └────────────────┬───────────────────────────────────────────────────────┘ │
│                   │                                                          │
│                   │ uses                                                     │
│                   │                                                          │
├───────────────────┼──────────────────────────────────────────────────────────┤
│                   │       Skia Graphics Library                              │
│  ┌────────────────▼───────────────────────────────────────────────────────┐ │
│  │                    Skia (libskia)                                       │ │
│  │  - GPU-accelerated 2D drawing via OpenGL backend                        │ │
│  │  - Cross-platform graphics API                                          │ │
│  │  - SkCanvas for drawing primitives (rect, path, text, etc.)             │ │
│  │  - SkPaint for styling (colors, shaders, effects)                       │ │
│  └────────────────┬───────────────────────────────────────────────────────┘ │
│                   │                                                          │
│                   │ OpenGL calls                                             │
│                   │                                                          │
├───────────────────┼──────────────────────────────────────────────────────────┤
│                   │       Platform Layer                                     │
│  ┌────────────────▼───────────────────────────────────────────────────────┐ │
│  │              OpenGL / Mesa / GPU Driver                                 │ │
│  │  - Linux: EGL + Wayland/X11 + Mesa + AMD/NVIDIA drivers                 │ │
│  │  - macOS: OpenGL (deprecated) or Metal via Skia Metal backend           │ │
│  │  - Windows: OpenGL or Direct3D via Skia D3D backend                     │ │
│  └────────────────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 1.3 Rendering Flow (Per Frame)

```
┌──────────────────────────────────────────────────────────────────────────┐
│ FRAME N                                                                   │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│   1. JUCE calls OpenGLContext::renderOpenGL()                            │
│      └──> JUCE has already called makeCurrent() for us                   │
│                        │                                                 │
│                        ▼                                                 │
│   2. MainComponent::renderOpenGL() callback invoked                      │
│      └──> We're now on the GL thread with valid context                  │
│                        │                                                 │
│                        ▼                                                 │
│   3. SkiaRenderer::renderFrame(width, height) called                     │
│                        │                                                 │
│                        ▼                                                 │
│   4. VALIDATE GL CONTEXT (Wayland requirement)                           │
│      │                                                                   │
│      ├──> grContext_->abandoned() check                                  │
│      │    If true: recreateContext() and return early                    │
│      │                                                                   │
│      └──> GL error check (optional, for debugging)                       │
│                        │                                                 │
│                        ▼                                                 │
│   5. CHECK FOR RESIZE                                                    │
│      │                                                                   │
│      ├──> if (width != lastWidth_ || height != lastHeight_)              │
│      │       recreateSurface(width, height)                              │
│      │                                                                   │
│      └──> This handles Wayland "surprise resizes"                        │
│                        │                                                 │
│                        ▼                                                 │
│   6. QUERY CURRENT FBO (never assume!)                                   │
│      │                                                                   │
│      └──> glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO)             │
│           JUCE owns this FBO - we just read the ID                       │
│                        │                                                 │
│                        ▼                                                 │
│   7. CREATE GrBackendRenderTarget wrapping JUCE's FBO                    │
│      │                                                                   │
│      └──> GrGLFramebufferInfo { fFBOID = currentFBO, fFormat = GL_RGBA8 }│
│           GrBackendRenderTarget(width, height, 0, 8, fbInfo)             │
│                        │                                                 │
│                        ▼                                                 │
│   8. CREATE SkSurface from backend render target                         │
│      │                                                                   │
│      └──> SkSurfaces::WrapBackendRenderTarget(...)                       │
│           kBottomLeft_GrSurfaceOrigin (OpenGL uses bottom-left)          │
│                        │                                                 │
│                        ▼                                                 │
│   9. GET SkCanvas* from surface                                          │
│      │                                                                   │
│      └──> canvas_ = surface_->getCanvas()                                │
│           Only valid for THIS frame!                                     │
│                        │                                                 │
│                        ▼                                                 │
│  10. DRAW UI ELEMENTS                                                    │
│      │                                                                   │
│      ├──> canvas->clear(backgroundColor)                                 │
│      ├──> canvas->drawRect(...) // Transport bar                         │
│      ├──> canvas->drawPath(...) // Waveforms                             │
│      └──> canvas->drawText(...) // Labels                                │
│                        │                                                 │
│                        ▼                                                 │
│  11. FLUSH GPU COMMANDS                                                  │
│      │                                                                   │
│      └──> grContext_->flush()                                            │
│           Submits all queued Skia commands to GPU                        │
│                        │                                                 │
│                        ▼                                                 │
│  12. JUCE SWAPS BUFFERS (automatic)                                      │
│      └──> We don't call swapBuffers - JUCE handles this                  │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘
                    │
                    ▼
              REPEAT FRAME N+1
```

---

## 2. CRITICAL CONSTRAINTS

### 2.1 JUCE Ownership Rules

> [!CAUTION]
> Violating these rules will cause crashes, corruption, or undefined behavior.

**JUCE Controls (DO NOT TOUCH):**

| Resource | JUCE Owns? | Notes |
|----------|------------|-------|
| GL context creation | ✅ YES | Created by `attachTo()` |
| GL context destruction | ✅ YES | Destroyed by `detach()` |
| `makeCurrent()` calls | ✅ YES | Called before `renderOpenGL()` |
| Buffer swapping | ✅ YES | Called after `renderOpenGL()` |
| Thread scheduling | ✅ YES | GL thread managed by JUCE |
| Framebuffer (FBO) | ✅ YES | We WRAP, never CREATE |

**WE MUST NOT:**

```cpp
// ❌ FORBIDDEN - WILL CRASH ON WAYLAND
openGLContext.makeCurrent();           // JUCE owns this

// ❌ FORBIDDEN - BREAKS JUCE'S RENDERING
glGenFramebuffers(1, &myFBO);          // Only JUCE creates FBOs
glBindFramebuffer(GL_FRAMEBUFFER, myFBO);

// ❌ FORBIDDEN - JUCE HANDLES THIS
openGLContext.swapBuffers();           // Called automatically

// ❌ FORBIDDEN - JUCE EXPECTS SPECIFIC STATE
glViewport(0, 0, w, h);                // Don't modify unless restoring

// ❌ FORBIDDEN - WILL LEAK OR CRASH
glDeleteFramebuffers(1, &jucesFBO);    // We don't own it
```

**WE CAN:**

```cpp
// ✅ ALLOWED - Query only
GLint currentFBO;
glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);

// ✅ ALLOWED - Wrap existing FBO with Skia
GrGLFramebufferInfo fbInfo;
fbInfo.fFBOID = static_cast<GrGLuint>(currentFBO);
auto backendRT = GrBackendRenderTargets::MakeGL(..., fbInfo);

// ✅ ALLOWED - Draw via Skia (it uses GL internally)
canvas->drawRect(...);

// ✅ ALLOWED - Create our own textures for caching
glGenTextures(1, &cacheTexture);  // But be careful with lifecycle

// ✅ ALLOWED - Read GL state (for debugging/validation)
GLint viewport[4];
glGetIntegerv(GL_VIEWPORT, viewport);
```

### 2.2 Wayland Specific Requirements

> [!IMPORTANT]
> On Wayland (Pop!_OS, Ubuntu 22.04+), the compositor can invalidate the GL context at ANY time.

**Context Loss Scenarios:**

| Scenario | Frequency | Required Action |
|----------|-----------|-----------------|
| User switches workspace | Common | Recreate Skia context/surface |
| Window moves to different monitor | Common | Recreate surface (DPI change) |
| Compositor restart | Rare | Full context recreation |
| System sleep/wake | Periodic | Validate context on resume |
| GPU driver reset | Rare | Full context recreation |
| VT switch (Ctrl+Alt+F2) | Rare | Recreate everything |

**Handling Strategy:**

```cpp
void SkiaRenderer::renderFrame(int width, int height)
{
    // CRITICAL: Check context validity EVERY frame
    // Wayland can invalidate at ANY time between frames
    if (!grContext_) {
        // Context was never created or was destroyed
        if (!recreateContext()) {
            return; // Can't render without context
        }
    }
    
    if (grContext_->abandoned()) {
        // Wayland invalidated our context!
        // This is NOT an error - it's expected behavior on Wayland
        DBG("GL context abandoned - recreating");
        if (!recreateContext()) {
            return;
        }
    }
    
    // Check for resize (Wayland can resize without notification)
    if (width != lastWidth_ || height != lastHeight_) {
        recreateSurface(width, height);
    }
    
    // Now safe to render
    if (!surface_ || !canvas_) {
        return; // Surface creation failed
    }
    
    // Drawing code here...
}
```

### 2.3 Performance Constraints

**Target Metrics:**

| Metric | Target | Notes |
|--------|--------|-------|
| Frame time (60Hz) | < 16.67ms | Mandatory minimum |
| Frame time (144Hz) | < 6.94ms | High refresh displays |
| Frame time (240Hz) | < 4.17ms | Competitive gaming displays |
| GPU utilization | < 30% | On RX 5700 XT @ 1080p |
| Memory allocations per frame | 0 | No heap allocations in render path |
| Context creation time | < 100ms | Acceptable for app startup |
| Surface recreation time | < 10ms | For resize responsiveness |

**Optimization Strategies:**

```cpp
// 1. CACHE SkPaint objects (don't recreate each frame)
class MyComponent {
    SkPaint cachedBackgroundPaint_;  // Created once in constructor
    SkPaint cachedBorderPaint_;
    
    void initPaints() {
        cachedBackgroundPaint_.setColor(0xFF1A1A1A);
        cachedBackgroundPaint_.setAntiAlias(true);
        // etc.
    }
};

// 2. USE SkPicture for static content
sk_sp<SkPicture> staticContent_;

void cacheStaticContent() {
    SkPictureRecorder recorder;
    SkCanvas* canvas = recorder.beginRecording(width, height);
    drawStaticElements(canvas);
    staticContent_ = recorder.finishRecordingAsPicture();
}

void drawSkia(SkCanvas* canvas) {
    canvas->drawPicture(staticContent_);  // GPU-cached playback
    drawDynamicElements(canvas);
}

// 3. DIRTY RECTANGLE TRACKING
class DirtyRegionTracker {
    juce::Rectangle<int> dirtyRect_;
    
public:
    void markDirty(const juce::Rectangle<int>& rect) {
        dirtyRect_ = dirtyRect_.getUnion(rect);
    }
    
    void draw(SkCanvas* canvas) {
        canvas->save();
        canvas->clipRect(toSkRect(dirtyRect_));
        // Only redraw dirty region
        canvas->restore();
        dirtyRect_ = {};
    }
};

// 4. BATCH DRAW CALLS
// BAD: 100 individual draws
for (int i = 0; i < 100; i++) {
    canvas->drawRect(rects[i], paint);
}

// GOOD: Use SkCanvas::drawAtlas or drawVertices for many shapes
std::vector<SkRSXform> transforms;
std::vector<SkRect> rects;
// ... fill arrays ...
canvas->drawAtlas(atlas, transforms.data(), rects.data(), nullptr, 
                  count, SkBlendMode::kSrcOver, nullptr, nullptr);
```

---

## 3. COMPONENT SPECIFICATIONS

### 3.1 SkiaRenderer Class

**File:** `include/rendering/SkiaRenderer.h`

**Responsibilities:**

1. Manage Skia GPU context (`GrDirectContext`)
2. Wrap JUCE's FBO with Skia (never create our own)
3. Provide `SkCanvas*` for drawing
4. Handle context loss gracefully (Wayland requirement)
5. Track window size changes
6. Provide performance statistics

**Thread Safety:** UI thread only (called from JUCE's OpenGL callbacks)

**Memory Management:**
- Uses `sk_sp<>` smart pointers for Skia objects
- No heap allocations in render path
- Canvas pointer is borrowed (not owned)

### 3.2 MainComponent Integration

**File:** `apps/desktop/src/MainComponent.cpp`

**Changes Required:**

```cpp
class MainComponent : public juce::Component,
                      private juce::OpenGLRenderer
{
public:
    MainComponent()
    {
        // Configure OpenGL - MUST be OpenGL 3.2+ for Skia
        openGLContext.setOpenGLVersionRequired(
            juce::OpenGLContext::OpenGLVersion::openGL3_2);
        
        // CRITICAL: Disable JUCE's component painting
        // We're taking over rendering with Skia
        openGLContext.setComponentPaintingEnabled(false);
        
        // Enable continuous rendering for animation
        openGLContext.setContinuousRepainting(true);
        
        // Attach ourselves as the renderer
        openGLContext.setRenderer(this);
        
        // Attach context to this component
        // This triggers context creation
        openGLContext.attachTo(*this);
        
        setSize(1280, 720);  // Default window size
    }
    
    ~MainComponent() override
    {
        // CRITICAL: Detach before destruction
        // This prevents callbacks to destroyed object
        openGLContext.detach();
    }
    
    //==============================================================================
    // juce::OpenGLRenderer callbacks
    //==============================================================================
    
    void newOpenGLContextCreated() override
    {
        // Called by JUCE when GL context is ready
        // We're on the GL thread here
        skiaRenderer.initialize(openGLContext);
    }
    
    void renderOpenGL() override
    {
        // Called every frame by JUCE
        // GL context is current (JUCE called makeCurrent for us)
        
        // Query current refresh rate for adaptive rendering
        float refreshRate = getMonitorRefreshRate();
        
        skiaRenderer.renderFrame(
            getWidth(),
            getHeight(),
            refreshRate
        );
        
        // Draw our UI
        if (auto* canvas = skiaRenderer.getCanvas()) {
            drawUI(canvas);
        }
        
        // Flush Skia commands
        skiaRenderer.flushAndSubmit();
    }
    
    void openGLContextClosing() override
    {
        // Called when GL context is about to be destroyed
        // Release Skia resources before context dies
        skiaRenderer.handleContextLoss();
    }
    
private:
    juce::OpenGLContext openGLContext;
    Zenith::SkiaRenderer skiaRenderer;
    
    void drawUI(SkCanvas* canvas)
    {
        // Clear background
        canvas->clear(SkColorSetRGB(13, 13, 13)); // BG_DARKEST
        
        // Draw application UI
        drawTransportBar(canvas);
        drawArrangerView(canvas);
        drawMixerView(canvas);
        // etc.
    }
    
    float getMonitorRefreshRate() const
    {
        // TODO: Query actual monitor refresh rate
        // Linux: /sys/class/drm/card*/modes
        // Or parse xrandr output
        // For now: assume 60Hz
        return 60.0f;
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
```

---

## 4. ERROR HANDLING

### 4.1 Context Loss Recovery

**Symptoms:**

| Symptom | Cause | Detection |
|---------|-------|-----------|
| `grContext->abandoned()` returns true | Wayland invalidation | Check every frame |
| GL calls return `GL_INVALID_OPERATION` | Context not current | Shouldn't happen if JUCE controls thread |
| Rendering produces black screen | Surface creation failed | Check surface/canvas for nullptr |
| Application freeze | Infinite retry loop | Add max retry count |

**Recovery Implementation:**

```cpp
bool SkiaRenderer::handleContextLoss()
{
    // Release Skia resources in correct order
    // Surface before context (surface references context)
    
    if (surface_) {
        surface_.reset();
        canvas_ = nullptr;  // Canvas was owned by surface
    }
    
    if (grContext_) {
        // Tell Skia to abandon any GPU resources
        // This prevents Skia from calling GL on invalid context
        grContext_->abandonContext();
        grContext_.reset();
    }
    
    contextValid_ = false;
    lastWidth_ = 0;
    lastHeight_ = 0;
    
    // Don't try to recreate here - let JUCE call newOpenGLContextCreated()
    // when the new context is ready
    return true;
}

bool SkiaRenderer::recreateContext()
{
    // Clean up old context first
    handleContextLoss();
    
    // Create new Skia GL interface
    auto glInterface = GrGLMakeNativeInterface();
    if (!glInterface) {
        DBG("ERROR: Failed to create GL interface");
        return false;
    }
    
    // Create GPU context
    grContext_ = GrDirectContext::MakeGL(glInterface);
    if (!grContext_) {
        DBG("ERROR: Failed to create GrDirectContext");
        return false;
    }
    
    contextValid_ = true;
    return true;
}
```

### 4.2 Surface Creation Failures

**Causes & Solutions:**

| Cause | Solution |
|-------|----------|
| Invalid FBO ID | Query FBO fresh each frame |
| Mismatched color format | Try GL_RGB8 fallback |
| Out of GPU memory | Free cached resources, use software fallback |
| Zero dimensions | Skip frame, wait for valid resize |
| Context lost mid-creation | Retry on next frame |

**Robust Surface Creation:**

```cpp
bool SkiaRenderer::createSkiaSurface(int width, int height)
{
    // Validate dimensions
    if (width <= 0 || height <= 0) {
        DBG("Skipping surface creation: invalid dimensions " 
            << width << "x" << height);
        return false;
    }
    
    // Clean up existing surface
    surface_.reset();
    canvas_ = nullptr;
    
    // Query JUCE's current FBO (CRITICAL: do this EVERY frame on Wayland)
    GLint currentFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
    
    if (currentFBO < 0) {
        DBG("ERROR: Invalid FBO binding");
        return false;
    }
    
    // Create backend render target wrapping JUCE's FBO
    GrGLFramebufferInfo fbInfo;
    fbInfo.fFBOID = static_cast<GrGLuint>(currentFBO);
    fbInfo.fFormat = GL_RGBA8;  // Standard 32-bit color
    
    GrBackendRenderTarget backendRT = GrBackendRenderTargets::MakeGL(
        width, 
        height,
        0,    // sample count (no MSAA for now - performance)
        8,    // stencil bits (required for Skia path rendering)
        fbInfo
    );
    
    // Try to create surface with RGBA8
    surface_ = SkSurfaces::WrapBackendRenderTarget(
        grContext_.get(),
        backendRT,
        kBottomLeft_GrSurfaceOrigin,  // OpenGL uses bottom-left origin
        kRGBA_8888_SkColorType,
        SkColorSpace::MakeSRGB(),
        nullptr  // surface properties
    );
    
    if (!surface_) {
        // Fallback: try RGB8 format
        DBG("RGBA8 surface failed, trying RGB8 fallback");
        fbInfo.fFormat = GL_RGB8;
        
        backendRT = GrBackendRenderTargets::MakeGL(
            width, height, 0, 8, fbInfo);
        
        surface_ = SkSurfaces::WrapBackendRenderTarget(
            grContext_.get(),
            backendRT,
            kBottomLeft_GrSurfaceOrigin,
            kRGBA_8888_SkColorType,  // Still use RGBA color type
            nullptr,
            nullptr
        );
    }
    
    if (!surface_) {
        DBG("ERROR: All surface creation attempts failed");
        return false;
    }
    
    canvas_ = surface_->getCanvas();
    lastWidth_ = width;
    lastHeight_ = height;
    
    return true;
}
```

---

## 5. TESTING STRATEGY

### 5.1 Unit Tests

**File:** `tests/rendering/SkiaRendererTest.cpp`

```cpp
#include <catch2/catch.hpp>
#include "rendering/SkiaRenderer.h"
#include <JuceHeader.h>

class TestOpenGLComponent : public juce::Component,
                            public juce::OpenGLRenderer
{
public:
    juce::OpenGLContext openGLContext;
    Zenith::SkiaRenderer renderer;
    bool contextCreated = false;
    bool renderCalled = false;
    int frameCount = 0;
    
    TestOpenGLComponent()
    {
        openGLContext.setOpenGLVersionRequired(
            juce::OpenGLContext::OpenGLVersion::openGL3_2);
        openGLContext.setRenderer(this);
        openGLContext.attachTo(*this);
    }
    
    ~TestOpenGLComponent() override
    {
        openGLContext.detach();
    }
    
    void newOpenGLContextCreated() override
    {
        contextCreated = renderer.initialize(openGLContext);
    }
    
    void renderOpenGL() override
    {
        renderCalled = true;
        renderer.renderFrame(getWidth(), getHeight(), 60.0f);
        frameCount++;
        
        if (auto* canvas = renderer.getCanvas()) {
            canvas->clear(SK_ColorBLACK);
        }
        
        renderer.flushAndSubmit();
    }
    
    void openGLContextClosing() override
    {
        renderer.handleContextLoss();
    }
};

//==============================================================================
// Test Cases
//==============================================================================

TEST_CASE("SkiaRenderer initialization", "[rendering]")
{
    // Create message manager for JUCE event loop
    juce::ScopedJuceInitialiser_GUI guiScope;
    
    TestOpenGLComponent component;
    component.setSize(800, 600);
    
    // Wait for context creation (JUCE does this async)
    for (int i = 0; i < 100 && !component.contextCreated; ++i) {
        juce::MessageManager::getInstance()->runDispatchLoopUntil(10);
    }
    
    REQUIRE(component.contextCreated);
    REQUIRE(component.renderer.isReady());
}

TEST_CASE("SkiaRenderer handles resize", "[rendering]")
{
    juce::ScopedJuceInitialiser_GUI guiScope;
    
    TestOpenGLComponent component;
    component.setSize(800, 600);
    
    // Wait for initial render
    for (int i = 0; i < 100 && component.frameCount < 1; ++i) {
        juce::MessageManager::getInstance()->runDispatchLoopUntil(10);
    }
    
    REQUIRE(component.renderCalled);
    auto* canvas1 = component.renderer.getCanvas();
    REQUIRE(canvas1 != nullptr);
    
    // Resize
    component.setSize(1920, 1080);
    
    // Wait for post-resize render
    int oldFrameCount = component.frameCount;
    for (int i = 0; i < 100 && component.frameCount < oldFrameCount + 1; ++i) {
        juce::MessageManager::getInstance()->runDispatchLoopUntil(10);
    }
    
    auto* canvas2 = component.renderer.getCanvas();
    REQUIRE(canvas2 != nullptr);
}

TEST_CASE("SkiaRenderer can render 1000 frames without crash", "[rendering][stress]")
{
    juce::ScopedJuceInitialiser_GUI guiScope;
    
    TestOpenGLComponent component;
    component.setSize(1280, 720);
    
    // Wait for 1000 frames
    for (int i = 0; i < 10000 && component.frameCount < 1000; ++i) {
        juce::MessageManager::getInstance()->runDispatchLoopUntil(1);
    }
    
    REQUIRE(component.frameCount >= 1000);
    REQUIRE(component.renderer.isReady());
    // No crashes = pass
}
```

### 5.2 Integration Tests

**File:** `tests/rendering/SkiaIntegrationTest.cpp`

```cpp
TEST_CASE("Full rendering pipeline integration", "[rendering][integration]")
{
    juce::ScopedJuceInitialiser_GUI guiScope;
    
    // Create actual MainComponent (or test harness)
    auto mainComponent = std::make_unique<MainComponent>();
    mainComponent->setSize(1280, 720);
    
    // Run message loop for 60 frames (~1 second at 60Hz)
    auto startTime = juce::Time::currentTimeMillis();
    while (juce::Time::currentTimeMillis() - startTime < 1000) {
        juce::MessageManager::getInstance()->runDispatchLoopUntil(16);
    }
    
    // Verify no crashes, context still valid
    // (Actual assertions depend on MainComponent's exposed state)
}
```

### 5.3 Performance Tests

**File:** `tests/rendering/SkiaPerformanceTest.cpp`

```cpp
TEST_CASE("Frame time under 16ms", "[rendering][performance]")
{
    juce::ScopedJuceInitialiser_GUI guiScope;
    
    std::vector<double> frameTimes;
    
    class TimingComponent : public TestOpenGLComponent
    {
    public:
        std::vector<double>& times;
        juce::int64 lastFrameTime = 0;
        
        TimingComponent(std::vector<double>& t) : times(t) {}
        
        void renderOpenGL() override
        {
            auto now = juce::Time::getHighResolutionTicks();
            if (lastFrameTime > 0) {
                double ms = juce::Time::highResolutionTicksToSeconds(
                    now - lastFrameTime) * 1000.0;
                times.push_back(ms);
            }
            lastFrameTime = now;
            
            TestOpenGLComponent::renderOpenGL();
        }
    };
    
    TimingComponent component(frameTimes);
    component.setSize(1920, 1080);  // Full HD stress test
    
    // Run for 100 frames
    for (int i = 0; i < 1000 && frameTimes.size() < 100; ++i) {
        juce::MessageManager::getInstance()->runDispatchLoopUntil(1);
    }
    
    REQUIRE(frameTimes.size() >= 100);
    
    // Calculate percentiles
    std::sort(frameTimes.begin(), frameTimes.end());
    double p50 = frameTimes[frameTimes.size() / 2];
    double p99 = frameTimes[frameTimes.size() * 99 / 100];
    
    CAPTURE(p50, p99);
    
    // Requirements:
    // - 50th percentile must be under 8ms (headroom for 60Hz)
    // - 99th percentile must be under 16ms (meet 60fps target)
    REQUIRE(p50 < 8.0);
    REQUIRE(p99 < 16.0);
}
```

### 5.4 Manual Verification Steps

1. **Build and Launch**
   ```bash
   cd /home/micah/Desktop/zenith/daw
   rm -rf build && mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   make -j$(nproc)
   ./ZenithDAW_artefacts/Release/ZenithDAW
   ```

2. **Visual Check**
   - Window should display without black screen
   - UI elements should be visible and properly styled
   - No flickering or tearing

3. **Resize Test**
   - Drag window borders to resize
   - Maximize and restore window
   - No crashes during rapid resize

4. **Workspace Switch Test (Wayland only)**
   - Press Super+1, Super+2 to switch workspaces
   - Return to DAW workspace
   - Rendering should resume without crash

5. **Performance Check**
   - Open System Monitor or `htop`
   - GPU usage should be < 30% at idle
   - CPU usage should be < 10% at idle

---

## 6. FUTURE ENHANCEMENTS

### 6.1 Adaptive Refresh Rate

```cpp
float MainComponent::getMonitorRefreshRate() const
{
    #if JUCE_LINUX
    // Parse /sys/class/drm/card*/modes or xrandr
    juce::File drmDir("/sys/class/drm");
    for (const auto& card : drmDir.findChildFiles(
            juce::File::findDirectories, false, "card*")) {
        juce::File modesFile = card.getChildFile("modes");
        if (modesFile.existsAsFile()) {
            auto content = modesFile.loadFileAsString();
            // Parse "1920x1080@144" format
            // Return first mode's refresh rate
        }
    }
    #elif JUCE_MAC
    // Use CVDisplayLink
    CVDisplayLinkRef displayLink;
    CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
    // Get refresh rate from display link
    #elif JUCE_WINDOWS
    // Use EnumDisplaySettings / GetDeviceCaps
    DEVMODE dm;
    EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &dm);
    return static_cast<float>(dm.dmDisplayFrequency);
    #endif
    
    return 60.0f; // Fallback
}
```

### 6.2 Multi-Monitor Support

```cpp
// Track which monitor window is on
class MonitorTracker {
    juce::Displays::Display currentDisplay_;
    
public:
    void update(const juce::Component& component) {
        auto bounds = component.getScreenBounds();
        for (const auto& display : juce::Desktop::getInstance().getDisplays().displays) {
            if (display.totalArea.contains(bounds.getCentre())) {
                if (display.dpi != currentDisplay_.dpi) {
                    // DPI changed - need to recreate surface
                }
                currentDisplay_ = display;
                break;
            }
        }
    }
    
    float getRefreshRate() const {
        // Per-display refresh rate
        return 60.0f; // TODO: Implement per-display query
    }
    
    float getScale() const {
        return currentDisplay_.scale;
    }
};
```

### 6.3 HDR Support (Future)

```cpp
// When HDR support is added:
// 1. Use F16 color format for wide color range
fbInfo.fFormat = GL_RGBA16F;

// 2. Use Rec.2020 color space
auto colorSpace = SkColorSpace::MakeRGB(
    SkNamedTransferFn::kPQ,       // HDR transfer function
    SkNamedGamut::kRec2020        // Wide color gamut
);

// 3. Create surface with HDR color type
surface_ = SkSurfaces::WrapBackendRenderTarget(
    grContext_.get(),
    backendRT,
    kBottomLeft_GrSurfaceOrigin,
    kRGBA_F16_SkColorType,        // 16-bit float per channel
    colorSpace,
    nullptr
);
```

---

## 7. APPENDIX

### 7.1 Skia Include Requirements

```cpp
// Core Skia headers for rendering
#include <include/core/SkCanvas.h>
#include <include/core/SkSurface.h>
#include <include/core/SkPaint.h>
#include <include/core/SkPath.h>
#include <include/core/SkColorSpace.h>
#include <include/core/SkRefCnt.h>  // For sk_sp<>

// GPU/Ganesh headers
#include <include/gpu/GrDirectContext.h>
#include <include/gpu/gl/GrGLInterface.h>
#include <include/gpu/ganesh/GrBackendSurface.h>
#include <include/gpu/ganesh/SkSurfaceGanesh.h>
#include <include/gpu/ganesh/gl/GrGLBackendSurface.h>
#include <include/gpu/ganesh/gl/GrGLDirectContext.h>

// Platform-specific GL
#if JUCE_LINUX
#include <EGL/egl.h>
#include <GLES3/gl3.h>  // or GL/gl.h for full OpenGL
#endif
```

### 7.2 CMake Configuration

```cmake
# In cmake/Dependencies.cmake or similar
find_package(Skia REQUIRED)

target_link_libraries(ZenithDAW PRIVATE
    skia::skia
    # Linux OpenGL
    $<$<PLATFORM_ID:Linux>:EGL GLESv2>
    # Or full OpenGL
    # $<$<PLATFORM_ID:Linux>:EGL GL>
)

target_include_directories(ZenithDAW PRIVATE
    ${SKIA_INCLUDE_DIR}
)
```

### 7.3 References

- [Skia Documentation](https://skia.org/docs/)
- [Skia GPU Backends](https://skia.org/docs/user/api/skcanvas_creation/)
- [JUCE OpenGLContext](https://docs.juce.com/master/classOpenGLContext.html)
- [JUCEOpenGLRenderer](https://docs.juce.com/master/structOpenGLRenderer.html)
- [Wayland EGL](https://wayland.freedesktop.org/egl.html)
- [GrDirectContext API](https://api.skia.org/classGrDirectContext.html)

---

**END OF DESIGN DOCUMENT**

---

## HANDOFF INSTRUCTIONS

**To:** CodeAgent-Rendering (Agent 3)  
**Branch:** `feature/jan1-rendering-design`

**Message:**  
Rendering pipeline design complete. Header file ready at `include/rendering/SkiaRenderer.h`.
Implement according to specifications in this document.

**Critical Implementation Notes:**

1. **Never call `makeCurrent()`** - JUCE owns the GL thread
2. **Query FBO every frame** - Wayland can change it
3. **Check `grContext_->abandoned()` every frame** - context loss is expected
4. **Recreate surface on resize** - don't cache dimensions
5. **No heap allocations in render path** - cache everything

**Files to Implement:**
- `apps/desktop/Source/rendering/SkiaRenderer.cpp` (update existing)
- `apps/desktop/Source/MainComponent.cpp` (integrate renderer)

**Verification:**
- Run unit tests: `./ZenithDAWTests --tag "[rendering]"`
- Manual resize test
- Workspace switch test (Wayland)

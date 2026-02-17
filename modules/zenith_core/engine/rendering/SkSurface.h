/**
 * @file SkiaRenderer.h
 * @brief GPU-accelerated Skia rendering pipeline for Zenith DAW
 *
 * @author ArchitectAgent
 * @date 2025-01-01
 * @see docs/architecture/RENDERING_PIPELINE.md for complete design document
 *
 * ═══════════════════════════════════════════════════════════════════════════
 * ARCHITECTURE OVERVIEW
 * ═══════════════════════════════════════════════════════════════════════════
 *
 *   JUCE OpenGLContext -> MainComponent -> SkiaRenderer -> SkCanvas -> GPU
 *        (owns GL)         (callbacks)    (wraps FBO)    (drawing)
 *
 * ═══════════════════════════════════════════════════════════════════════════
 * CRITICAL DESIGN CONSTRAINTS
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * 1. JUCE OWNS THE GL THREAD
 *    - NEVER call makeCurrent() ourselves
 *    - NEVER create our own framebuffers
 *    - NEVER swap buffers manually
 *    - JUCE calls our renderFrame() when the context is current
 *
 * 2. WAYLAND CAN INVALIDATE GL CONTEXT AT ANY TIME
 *    - User switches workspace
 *    - Window moves to different monitor
 *    - System sleep/wake
 *    - Compositor restart
 *    CHECK grContext_->abandoned() EVERY FRAME!
 *
 * 3. MUST WRAP JUCE'S FBO
 *    - Query current FBO each frame: glGetIntegerv(GL_FRAMEBUFFER_BINDING)
 *    - Wrap with GrBackendRenderTarget
 *    - Never assume FBO persists across frames
 *
 * 4. PERFORMANCE TARGETS
 *    - Frame time: <16ms (60fps minimum)
 *    - Frame time: <4ms for 240Hz displays
 *    - No heap allocations in render path
 *    - Cache SkPaint objects
 *
 * ═══════════════════════════════════════════════════════════════════════════
 * USAGE EXAMPLE
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * @code
 * class MainComponent : public juce::Component,
 *                       private juce::OpenGLRenderer
 * {
 * public:
 *     MainComponent()
 *     {
 *         openGLContext.setOpenGLVersionRequired(
 *             juce::OpenGLContext::OpenGLVersion::openGL3_2);
 *         openGLContext.setComponentPaintingEnabled(false);
 *         openGLContext.setContinuousRepainting(true);
 *         openGLContext.setRenderer(this);
 *         openGLContext.attachTo(*this);
 *     }
 *
 *     ~MainComponent() override
 *     {
 *         openGLContext.detach();
 *     }
 *
 *     void newOpenGLContextCreated() override
 *     {
 *         skiaRenderer_.initialize(openGLContext);
 *     }
 *
 *     void renderOpenGL() override
 *     {
 *         skiaRenderer_.renderFrame(getWidth(), getHeight(), 60.0f);
 *
 *         if (auto* canvas = skiaRenderer_.getCanvas()) {
 *             canvas->clear(SK_ColorBLACK);
 *             // Draw UI here...
 *         }
 *
 *         skiaRenderer_.flushAndSubmit();
 *     }
 *
 *     void openGLContextClosing() override
 *     {
 *         skiaRenderer_.handleContextLoss();
 *     }
 *
 * private:
 *     juce::OpenGLContext openGLContext;
 *     Zenith::SkiaRenderer skiaRenderer_;
 * };
 * @endcode
 */

#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>

// ═══════════════════════════════════════════════════════════════════════════
// Skia forward declarations
// Avoid including Skia headers in this header to keep compile times down.
// Implementation file includes the full Skia headers.
// ═══════════════════════════════════════════════════════════════════════════

class SkSurface;

} // namespace

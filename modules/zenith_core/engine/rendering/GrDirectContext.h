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

class GrDirectContext;

// Smart pointer template for Skia objects
#ifdef ZENITH_USE_SKIA
#if ZENITH_ENABLE_SKIA
#include <core/SkRefCnt.h>
#endif
#else
// Dummy sk_sp when Skia is disabled (for syntax checking without Skia)
template <typename T>
using sk_sp = std::shared_ptr<T>;
#endif

namespace zenith {

/**
 * @class SkiaRenderer
 * @brief Wraps JUCE's OpenGL context with Skia rendering capabilities
 *
 * This class provides the bridge between JUCE's windowing/OpenGL management
 * and Skia's high-performance 2D graphics. It is designed specifically for
 * the constraints of the Wayland compositor on Linux.
 *
 * @section lifetime Lifetime
 * - Create once per OpenGL component
 * - Call initialize() from newOpenGLContextCreated()
 * - Call renderFrame() every frame from renderOpenGL()
 * - Call handleContextLoss() from openGLContextClosing()
 *
 * @section thread_safety Thread Safety
 * This class is NOT thread-safe. All methods must be called from the
 * JUCE OpenGL thread (which is the same thread that calls renderOpenGL).
 * JUCE guarantees the GL context is current when our callbacks are invoked.
 *
 * @section memory_management Memory Management
 * - Uses sk_sp<> smart pointers for Skia objects
 * - getCanvas() returns a borrowed pointer (do NOT delete)
 * - Canvas pointer is only valid during renderFrame() call
 */

} // namespace

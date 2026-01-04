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
 *   JUCE OpenGLContext → MainComponent → SkiaRenderer → SkCanvas → GPU
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

#include <JuceHeader.h>

// ═══════════════════════════════════════════════════════════════════════════
// Skia forward declarations
// Avoid including Skia headers in this header to keep compile times down.
// Implementation file includes the full Skia headers.
// ═══════════════════════════════════════════════════════════════════════════

class SkCanvas;
class SkSurface;
class GrDirectContext;

// Smart pointer template for Skia objects
#ifdef ZENITH_USE_SKIA
#include <core/SkRefCnt.h>
#else
// Dummy sk_sp when Skia is disabled (for syntax checking without Skia)
template <typename T>
using sk_sp = std::shared_ptr<T>;
#endif

namespace Zenith {

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
class SkiaRenderer
{
public:
    // ═══════════════════════════════════════════════════════════════════════
    // Types
    // ═══════════════════════════════════════════════════════════════════════
    
    /**
     * @brief Rendering statistics for performance monitoring
     * 
     * Updated each frame. Use for debugging and profiling.
     * Values are smoothed over a rolling window for stability.
     */
    struct Stats
    {
        double lastFrameTimeMs = 0.0;     ///< Most recent frame time in milliseconds
        double averageFrameTimeMs = 0.0;  ///< Rolling average (60 frame window)
        double maxFrameTimeMs = 0.0;      ///< Max frame time in current window
        int framesPerSecond = 0;          ///< Estimated FPS (updated every second)
        int contextRecreations = 0;       ///< Number of context recreations (Wayland)
        int surfaceRecreations = 0;       ///< Number of surface recreations (resize)
        size_t gpuMemoryBytes = 0;        ///< Estimated GPU memory usage
    };
    
    /**
     * @brief Context validation result
     */
    enum class ContextState
    {
        Valid,              ///< Context is valid, safe to render
        Abandoned,          ///< Context was invalidated (Wayland)
        NotInitialized,     ///< initialize() not yet called
        CreationFailed      ///< Context creation failed (GPU error)
    };

    // ═══════════════════════════════════════════════════════════════════════
    // Lifecycle
    // ═══════════════════════════════════════════════════════════════════════
    
    /**
     * @brief Default constructor
     * 
     * Creates a renderer in uninitialized state.
     * Call initialize() after JUCE's OpenGL context is created.
     */
    SkiaRenderer() = default;
    
    /**
     * @brief Destructor - releases all Skia resources
     * 
     * Safe to call even if initialize() was never called.
     * Will call abandonContext() on the GrDirectContext if it exists.
     */
    ~SkiaRenderer();
    
    // Non-copyable, non-movable (owns GPU resources)
    SkiaRenderer(const SkiaRenderer&) = delete;
    SkiaRenderer& operator=(const SkiaRenderer&) = delete;
    SkiaRenderer(SkiaRenderer&&) = delete;
    SkiaRenderer& operator=(SkiaRenderer&&) = delete;

    // ═══════════════════════════════════════════════════════════════════════
    // Initialization
    // ═══════════════════════════════════════════════════════════════════════
    
    /**
     * @brief Initialize Skia rendering with JUCE's OpenGL context
     * 
     * Must be called from newOpenGLContextCreated() callback.
     * The OpenGL context MUST be current when this is called (JUCE guarantees this).
     * 
     * @param context The JUCE OpenGL context (for reference, not used directly)
     * @return true if initialization succeeded, false otherwise
     * 
     * @note On failure, you can still call isReady() which will return false.
     *       The renderer will not crash if you try to render after failure,
     *       but nothing will be drawn.
     * 
     * @see handleContextLoss()
     */
    bool initialize(juce::OpenGLContext& context);
    
    // ═══════════════════════════════════════════════════════════════════════
    // Rendering
    // ═══════════════════════════════════════════════════════════════════════
    
    /**
     * @brief Prepare for rendering a frame
     * 
     * Call this at the START of renderOpenGL() callback.
     * This performs:
     * 1. Context validation (Wayland may have invalidated it)
     * 2. Resize detection and surface recreation
     * 3. FBO query and Skia surface setup
     * 
     * After this returns true, call getCanvas() to get the drawing surface.
     * 
     * @param width  Current window width in pixels
     * @param height Current window height in pixels
     * @param refreshRate Current monitor refresh rate in Hz (for adaptive timing)
     * @return true if rendering can proceed, false if context is invalid
     * 
     * @code
     * void MainComponent::renderOpenGL() override
     * {
     *     if (!skiaRenderer_.renderFrame(getWidth(), getHeight(), 60.0f)) {
     *         return; // Context invalid, skip this frame
     *     }
     *     
     *     auto* canvas = skiaRenderer_.getCanvas();
     *     // Draw...
     *     
     *     skiaRenderer_.flushAndSubmit();
     * }
     * @endcode
     * 
     * @note The refresh rate is used for performance monitoring only.
     *       It does not control the actual frame rate (JUCE/vsync does that).
     */
    bool renderFrame(int width, int height, float refreshRate = 60.0f);
    
    /**
     * @brief Get the Skia canvas for drawing
     * 
     * Returns the canvas that wraps JUCE's current framebuffer.
     * Only valid AFTER renderFrame() returns true.
     * Only valid UNTIL flushAndSubmit() is called.
     * 
     * @return Pointer to SkCanvas, or nullptr if not ready
     * 
     * @warning DO NOT store this pointer! It may become invalid after
     *          flushAndSubmit() or on the next frame.
     * 
     * @warning DO NOT delete this pointer! It is owned by the SkSurface.
     */
    SkCanvas* getCanvas() const noexcept { return canvas_; }
    
    /**
     * @brief Flush all pending Skia drawing commands to the GPU
     * 
     * Call this at the END of renderOpenGL(), after all drawing is complete.
     * This submits all queued GPU commands and ensures they're ready for
     * JUCE's buffer swap.
     * 
     * @note JUCE handles the actual buffer swap after renderOpenGL() returns.
     *       We just need to flush Skia's command buffer.
     */
    void flushAndSubmit();
    
    // ═══════════════════════════════════════════════════════════════════════
    // Context Management
    // ═══════════════════════════════════════════════════════════════════════
    
    /**
     * @brief Handle OpenGL context loss
     * 
     * Call this from openGLContextClosing() callback.
     * This releases all GPU resources and resets to uninitialized state.
     * 
     * After this call:
     * - isReady() will return false
     * - getCanvas() will return nullptr
     * - renderFrame() will return false until initialize() is called again
     * 
     * @note This is NOT an error condition on Wayland! It's expected behavior.
     *       JUCE will call newOpenGLContextCreated() when the new context
     *       is ready, at which point you call initialize() again.
     */
    void handleContextLoss();
    
    /**
     * @brief Validate the current GL context state
     * 
     * Returns the current state of the Skia context.
     * Useful for debugging and logging.
     * 
     * @return Current context state
     */
    ContextState validateContext() const;
    
    // ═══════════════════════════════════════════════════════════════════════
    // State Queries
    // ═══════════════════════════════════════════════════════════════════════
    
    /**
     * @brief Check if renderer is ready for drawing
     * 
     * @return true if initialize() succeeded and context is valid
     * 
     * @note This performs a quick check only. The context could become
     *       invalid immediately after this returns true (Wayland behavior).
     *       For reliable checking, use renderFrame() return value.
     */
    bool isReady() const noexcept;
    
    /**
     * @brief Check if the context has been abandoned (Wayland)
     * 
     * @return true if context was invalidated and needs recreation
     */
    bool isContextAbandoned() const;
    
    /**
     * @brief Get current rendering statistics
     * 
     * @return Performance statistics struct
     */
    const Stats& getStats() const noexcept { return stats_; }
    
    /**
     * @brief Get current surface dimensions
     * 
     * @return Width and height as int pair, or (0,0) if not initialized
     */
    std::pair<int, int> getSurfaceDimensions() const noexcept;
    
    // ═══════════════════════════════════════════════════════════════════════
    // Advanced / Debug
    // ═══════════════════════════════════════════════════════════════════════
    
    /**
     * @brief Get the raw Skia GPU context (advanced use only)
     * 
     * @return Pointer to GrDirectContext, or nullptr if not initialized
     * 
     * @warning Only use this if you know what you're doing.
     *          Modifying the context state may break rendering.
     */
    GrDirectContext* getGrContext() const noexcept { return grContext_.get(); }
    
    /**
     * @brief Get the raw Skia surface (advanced use only)
     * 
     * @return Pointer to SkSurface, or nullptr if not initialized
     */
    SkSurface* getSurface() const noexcept { return surface_.get(); }
    
    /**
     * @brief Force recreation of the Skia context
     * 
     * Useful for debugging context loss handling.
     * In production, this is called automatically when Wayland invalidates.
     * 
     * @return true if recreation succeeded
     */
    bool forceContextRecreation();
    
    /**
     * @brief Reset statistics counters
     */
    void resetStats();

private:
    // ═══════════════════════════════════════════════════════════════════════
    // Private Implementation
    // ═══════════════════════════════════════════════════════════════════════
    
    /**
     * @brief Create the Skia GPU context from OpenGL
     * 
     * Uses GrGLMakeNativeInterface() to get the GL function pointers,
     * then creates a GrDirectContext for GPU rendering.
     * 
     * @return true if context creation succeeded
     */
    bool createGrContext();
    
    /**
     * @brief Create Skia surface wrapping JUCE's current FBO
     * 
     * This is called every frame because:
     * 1. Wayland can change the FBO at any time
     * 2. Window resize requires new surface
     * 3. It's cheap (just wrapping existing resources)
     * 
     * @param width  Surface width in pixels
     * @param height Surface height in pixels
     * @return true if surface creation succeeded
     */
    bool createSkiaSurface(int width, int height);
    
    /**
     * @brief Attempt to recreate the context after loss
     * 
     * Called when grContext_->abandoned() returns true.
     * Releases old resources and creates new ones.
     * 
     * @return true if recreation succeeded
     */
    bool recreateContext();
    
    /**
     * @brief Update performance statistics
     */
    void updateStats(double frameTimeMs, float targetRefreshRate);
    
    // ═══════════════════════════════════════════════════════════════════════
    // Member Variables
    // ═══════════════════════════════════════════════════════════════════════
    
    // Skia GPU context (owns OpenGL resources)
    sk_sp<GrDirectContext> grContext_;
    
    // Current render surface (wraps JUCE's FBO)
    sk_sp<SkSurface> surface_;
    
    // Current canvas (borrowed from surface, NOT owned)
    SkCanvas* canvas_ = nullptr;
    
    // Last known dimensions (for resize detection)
    int lastWidth_ = 0;
    int lastHeight_ = 0;
    
    // Initialization state
    bool initialized_ = false;
    
    // Performance tracking
    Stats stats_;
    juce::int64 lastFrameTimeTicks_ = 0;
    std::vector<double> frameTimeHistory_; ///< Rolling window for averaging
    juce::int64 fpsCounterStartTicks_ = 0;
    int fpsFrameCount_ = 0;
    
    // Prevent log spam
    int consecutiveFailures_ = 0;
    static constexpr int kMaxConsecutiveFailures = 10;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaRenderer)
};

} // namespace Zenith

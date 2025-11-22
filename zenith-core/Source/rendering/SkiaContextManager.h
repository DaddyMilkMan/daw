/**
 * @file SkiaContextManager.h
 * @brief Centralized Skia GPU context manager for JUCE integration
 *
 * This class solves the fundamental problems with Skia + JUCE integration:
 * 1. Shares a single GPU context across all components (required by Skia)
 * 2. Provides proper JUCE window integration via OpenGL context sharing
 * 3. Manages surface lifecycle and resizing
 * 4. Implements proper present/blit to display Skia content in JUCE
 *
 * Architecture:
 * - ONE GrDirectContext per application (shared across all components)
 * - Surfaces created on-demand per component
 * - Automatic fallback to software rendering if GPU fails
 * - Proper synchronization and resource cleanup
 */

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <unordered_map>
#include <mutex>

#ifdef ZENITH_USE_SKIA
    #include "include/core/SkRefCnt.h"
    #include "include/core/SkSurface.h"
    #include "include/gpu/ganesh/GrDirectContext.h"
#else
    // Forward declarations when Skia is disabled
    class SkSurface;
    class GrDirectContext;
    template<typename T> using sk_sp = std::shared_ptr<T>;
#endif

class SkCanvas;
class SkImage;

namespace zenith {

/**
 * @class SkiaContextManager
 * @brief Singleton manager for shared Skia GPU context and JUCE integration
 *
 * This is the CORRECT way to integrate Skia with JUCE:
 *
 * 1. Create ONE GrDirectContext shared by all components
 * 2. Either:
 *    a) Use JUCE's OpenGL context and create Skia surfaces from it (GPU path)
 *    b) Render to SkImage and blit to JUCE Graphics (software path)
 * 3. Manage surface lifecycle properly
 *
 * Usage:
 * @code
 * // In your component's paint():
 * void MyComponent::paint(Graphics& g)
 * {
 *     auto& manager = SkiaContextManager::getInstance();
 *
 *     manager.renderToComponent(*this, [&](SkCanvas* canvas) {
 *         // Draw with Skia
 *         SkPaint paint;
 *         paint.setColor(SK_ColorBLUE);
 *         canvas->drawRect(SkRect::MakeWH(100, 100), paint);
 *     });
 *
 *     // Manager automatically blits Skia content to JUCE Graphics g
 * }
 * @endcode
 */
class SkiaContextManager
{
public:
    /**
     * Backend rendering mode
     */
    enum class RenderMode
    {
        GPU_Direct3D,    ///< Direct3D 12 GPU rendering (Windows)
        GPU_Metal,       ///< Metal GPU rendering (macOS)
        GPU_Vulkan,      ///< Vulkan GPU rendering (Linux)
        GPU_OpenGL,      ///< OpenGL GPU rendering (cross-platform fallback)
        Software         ///< CPU rasterization (guaranteed fallback)
    };

    //==========================================================================
    // Singleton Access
    //==========================================================================

    /**
     * @brief Get the singleton instance
     */
    static SkiaContextManager& getInstance();

    /**
     * @brief Initialize the Skia context with GPU backend
     * @param preferredMode Preferred rendering backend
     * @return true if initialization succeeded
     *
     * Call this once at application startup. Will try GPU backends and
     * fall back to software if GPU initialization fails.
     */
    bool initialize(RenderMode preferredMode = RenderMode::GPU_OpenGL);

    /**
     * @brief Shutdown and cleanup all Skia resources
     *
     * Call this at application shutdown. Releases all surfaces and GPU context.
     */
    void shutdown();

    /**
     * @brief Check if initialized
     */
    bool isInitialized() const { return initialized_; }

    /**
     * @brief Get current render mode
     */
    RenderMode getRenderMode() const { return currentMode_; }

    //==========================================================================
    // Component Rendering
    //==========================================================================

    /**
     * @brief Render Skia content for a JUCE component
     * @param component The JUCE component to render into
     * @param drawCallback Function that receives SkCanvas for drawing
     *
     * This is the main entry point for rendering. It:
     * 1. Gets or creates a surface for this component
     * 2. Calls your drawing code with SkCanvas
     * 3. Blits the result to the component's JUCE Graphics context
     *
     * Call this from your Component::paint(Graphics& g) method.
     */
    void renderToComponent(juce::Component& component,
                          std::function<void(SkCanvas*)> drawCallback);

    /**
     * @brief Render Skia content and return as JUCE Image
     * @param width Image width
     * @param height Image height
     * @param drawCallback Function that receives SkCanvas for drawing
     * @return JUCE Image with rendered content
     *
     * Use this for off-screen rendering or when you need a JUCE Image.
     */
    juce::Image renderToImage(int width, int height,
                             std::function<void(SkCanvas*)> drawCallback);

    /**
     * @brief Notify that a component was resized
     * @param component The resized component
     *
     * Call this from your Component::resized() to update the surface size.
     */
    void componentResized(juce::Component& component);

    /**
     * @brief Release surface for a component (call when component is destroyed)
     * @param component The destroyed component
     */
    void componentDestroyed(juce::Component& component);

    //==========================================================================
    // Advanced Access
    //==========================================================================

    /**
     * @brief Get the shared GPU context
     * @return Pointer to GrDirectContext, or nullptr if using software rendering
     */
    GrDirectContext* getGpuContext() const { return grContext_.get(); }

    /**
     * @brief Get rendering statistics
     */
    struct Stats
    {
        int activeSurfaces = 0;
        size_t gpuMemoryUsed = 0;
        int frameCount = 0;
        double averageFrameTime = 0.0;
    };

    const Stats& getStats() const { return stats_; }

private:
    SkiaContextManager();
    ~SkiaContextManager();

    // Initialization for different backends
    bool initializeGPU_OpenGL();
    bool initializeGPU_Direct3D();
    bool initializeGPU_Metal();
    bool initializeGPU_Vulkan();
    bool initializeSoftware();

    // Surface management
    struct ComponentSurface
    {
        sk_sp<SkSurface> surface;
        int width = 0;
        int height = 0;
        juce::Time lastUsed;
    };

    sk_sp<SkSurface> getOrCreateSurface(juce::Component& component);
    void cleanupOldSurfaces();

    // Blit Skia surface to JUCE Graphics
    void blitToJUCE(juce::Graphics& g, SkSurface* surface,
                   int width, int height);

    // Member variables
    bool initialized_ = false;
    RenderMode currentMode_ = RenderMode::Software;

    // Skia objects
    sk_sp<GrDirectContext> grContext_;  ///< Shared GPU context (if GPU mode)

    // Surface cache (one per component)
    std::unordered_map<juce::Component*, ComponentSurface> surfaces_;
    std::mutex surfacesMutex_;

    // Stats
    Stats stats_;
    juce::Time lastCleanupTime_;

    // Platform-specific data (void* to avoid platform headers)
    void* platformData_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaContextManager)
};

} // namespace zenith

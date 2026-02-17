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

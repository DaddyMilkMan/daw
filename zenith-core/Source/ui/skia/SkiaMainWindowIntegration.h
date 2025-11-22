/**
 * @file SkiaMainWindowIntegration.h
 * @brief Skia rendering integration for the main application window
 *
 * Provides:
 * - Centralized Skia component management
 * - Theme switching with state persistence
 * - Master animation loop with spring physics
 * - FPS monitoring and adaptive rendering
 * - GPU rendering setup and teardown
 */

#pragma once

#include <JuceHeader.h>

#ifdef ZENITH_USE_SKIA
    #include <include/core/SkCanvas.h>
    #include <include/core/SkPaint.h>
    #include <include/core/SkRect.h>
#endif

#include "SkiaTheme.h"

namespace zenith {

#ifdef ZENITH_USE_SKIA

/**
 * @class SkiaAnimationController
 * @brief Manages spring physics animations across all Skia components
 *
 * Provides:
 * - RK4 integration for spring physics
 * - Per-component animation state
 * - Automatic frame rate adaptation
 * - Animation group batching for efficiency
 */
class SkiaAnimationController
{
public:
    SkiaAnimationController();
    ~SkiaAnimationController() = default;

    /**
     * @brief Update all active animations
     * @param deltaTime Time since last frame in seconds
     */
    void update(float deltaTime);

    /**
     * @brief Register an animation target
     * @param componentId Unique identifier for the component
     * @param targetValue Target animation value (0-1)
     * @param physics Spring physics settings to use
     */
    void animateTo(const juce::String& componentId, float targetValue,
                   const SpringPhysicsSettings& physics);

    /**
     * @brief Get current animation value for a component
     * @param componentId Component identifier
     * @return Current interpolated value (0-1)
     */
    float getValue(const juce::String& componentId) const;

    /**
     * @brief Check if animation is complete
     * @param componentId Component identifier
     * @return True if animation has settled
     */
    bool isAnimationComplete(const juce::String& componentId) const;

    /**
     * @brief Clear all animations
     */
    void clear();

    /**
     * @brief Get current FPS
     */
    float getCurrentFPS() const { return currentFPS_; }

    /**
     * @brief Get target FPS
     */
    float getTargetFPS() const { return targetFPS_; }

    /**
     * @brief Check if any animations are active
     */
    bool hasActiveAnimations() const;

private:
    struct AnimationState
    {
        float current = 0.0f;
        float target = 0.0f;
        float velocity = 0.0f;
        SpringPhysicsSettings physics;
        bool isActive = false;
    };

    std::map<juce::String, AnimationState> animationStates_;
    float currentFPS_ = 60.0f;
    float targetFPS_ = 60.0f;
    float frameTimeAccumulator_ = 0.0f;
    int frameCount_ = 0;

    static constexpr float SPRING_SETTLE_THRESHOLD = 0.001f;
};

/**
 * @class SkiaMainWindowIntegration
 * @brief Core integration point for Skia rendering in main window
 *
 * Responsibilities:
 * - Theme system management
 * - GPU rendering initialization
 * - Animation loop coordination
 * - FPS monitoring and adaptation
 * - Component lifecycle management
 */
class SkiaMainWindowIntegration : public juce::Component,
                                   private juce::Timer
{
public:
    SkiaMainWindowIntegration();
    ~SkiaMainWindowIntegration() override;

    //==========================================================================
    // Theme Management
    //==========================================================================

    /**
     * @brief Get the current theme mode
     */
    ThemeMode getThemeMode() const;

    /**
     * @brief Switch theme mode
     * @param mode New theme mode (Dark or Light)
     * @param animate Whether to animate the transition
     */
    void setThemeMode(ThemeMode mode, bool animate = true);

    /**
     * @brief Toggle between dark and light theme
     */
    void toggleTheme() { setThemeMode(getThemeMode() == ThemeMode::Dark ? ThemeMode::Light : ThemeMode::Dark); }

    //==========================================================================
    // Animation Control
    //==========================================================================

    /**
     * @brief Get the animation controller
     */
    SkiaAnimationController& getAnimationController() { return animationController_; }

    /**
     * @brief Animate a component with spring physics
     */
    void animateComponent(const juce::String& componentId, float targetValue,
                         const SpringPhysicsSettings& physics)
    {
        animationController_.animateTo(componentId, targetValue, physics);
        startAnimationLoop();
    }

    //==========================================================================
    // FPS Management
    //==========================================================================

    /**
     * @brief Get current rendering FPS
     */
    float getCurrentFPS() const { return animationController_.getCurrentFPS(); }

    /**
     * @brief Get target FPS
     */
    float getTargetFPS() const { return animationController_.getTargetFPS(); }

    /**
     * @brief Enable or disable adaptive FPS
     */
    void setAdaptiveFPS(bool enable);

    //==========================================================================
    // GPU Settings
    //==========================================================================

    /**
     * @brief Update GPU settings
     */
    void updateGPUSettings(const SkiaTheme::GPUSettings& settings);

    /**
     * @brief Get GPU settings
     */
    const SkiaTheme::GPUSettings& getGPUSettings() const
    {
        return SkiaTheme::getInstance().getGPUSettings();
    }

    //==========================================================================
    // Lifecycle
    //==========================================================================

    /**
     * @brief Initialize Skia rendering system
     * @return True if initialization successful
     */
    bool initializeSkiaRendering();

    /**
     * @brief Shutdown Skia rendering system
     */
    void shutdownSkiaRendering();

    /**
     * @brief Called when the component is resized
     */
    void resized() override;

    /**
     * @brief Called to paint the component
     */
    void paint(juce::Graphics& g) override;

private:
    //==========================================================================
    // Timer callback for animation loop
    //==========================================================================

    void timerCallback() override;

    /**
     * @brief Start the animation loop if not already running
     */
    void startAnimationLoop();

    /**
     * @brief Stop the animation loop
     */
    void stopAnimationLoop();

    /**
     * @brief Check if animation loop is running
     */
    bool isAnimationLoopRunning() const { return isAnimationLoopRunning_; }

    //==========================================================================
    // Member variables
    //==========================================================================

    SkiaAnimationController animationController_;
    bool isAnimationLoopRunning_ = false;
    juce::Time lastFrameTime_;
    bool skiaInitialized_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaMainWindowIntegration)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith

/**
 * @file SkiaMainWindowIntegration.cpp
 * @brief Implementation of Skia main window integration
 */

#include "SkiaMainWindowIntegration.h"

#ifdef ZENITH_USE_SKIA

#include <cmath>

namespace zenith {

//==============================================================================
// SkiaAnimationController Implementation
//==============================================================================

SkiaAnimationController::SkiaAnimationController()
    : currentFPS_(60.0f), targetFPS_(60.0f), frameTimeAccumulator_(0.0f), frameCount_(0)
{
}

void SkiaAnimationController::update(float deltaTime)
{
    // Update FPS
    frameTimeAccumulator_ += deltaTime;
    frameCount_++;

    if (frameTimeAccumulator_ >= 0.5f) // Update FPS every 0.5 seconds
    {
        currentFPS_ = frameCount_ / frameTimeAccumulator_;
        frameTimeAccumulator_ = 0.0f;
        frameCount_ = 0;
    }

    // Update all active animations using spring physics
    for (auto& [componentId, state] : animationStates_)
    {
        if (!state.isActive)
            continue;

        // Spring physics: F = -k(x - target) - c*v
        float positionError = state.target - state.current;
        float acceleration = (state.physics.stiffness * positionError) - (state.physics.damping * state.velocity);

        // Integrate velocity and position (simple Euler for now, can upgrade to RK4)
        state.velocity += acceleration * deltaTime;
        state.current += state.velocity * deltaTime;

        // Check if animation has settled
        if (std::abs(positionError) < SPRING_SETTLE_THRESHOLD &&
            std::abs(state.velocity) < SPRING_SETTLE_THRESHOLD)
        {
            state.current = state.target;
            state.velocity = 0.0f;
            state.isActive = false;
        }
    }
}

void SkiaAnimationController::animateTo(const juce::String& componentId, float targetValue,
                                        const SpringPhysicsSettings& physics)
{
    if (animationStates_.find(componentId) == animationStates_.end())
    {
        animationStates_[componentId] = AnimationState();
    }

    auto& state = animationStates_[componentId];
    state.target = juce::jlimit(0.0f, 1.0f, targetValue);
    state.physics = physics;
    state.isActive = true;
}

float SkiaAnimationController::getValue(const juce::String& componentId) const
{
    auto it = animationStates_.find(componentId);
    if (it != animationStates_.end())
    {
        return it->second.current;
    }
    return 0.0f;
}

bool SkiaAnimationController::isAnimationComplete(const juce::String& componentId) const
{
    auto it = animationStates_.find(componentId);
    if (it != animationStates_.end())
    {
        return !it->second.isActive;
    }
    return true;
}

void SkiaAnimationController::clear()
{
    animationStates_.clear();
}

bool SkiaAnimationController::hasActiveAnimations() const
{
    for (const auto& [id, state] : animationStates_)
    {
        if (state.isActive)
            return true;
    }
    return false;
}

//==============================================================================
// SkiaMainWindowIntegration Implementation
//==============================================================================

SkiaMainWindowIntegration::SkiaMainWindowIntegration()
    : isAnimationLoopRunning_(false), skiaInitialized_(false)
{
    // Set initial timer interval (will be adjusted by FPS settings)
    startTimer(16); // ~60 FPS
    lastFrameTime_ = juce::Time::getCurrentTime();
}

SkiaMainWindowIntegration::~SkiaMainWindowIntegration()
{
    stopAnimationLoop();
    shutdownSkiaRendering();
}

ThemeMode SkiaMainWindowIntegration::getThemeMode() const
{
    return SkiaTheme::getInstance().getThemeMode();
}

void SkiaMainWindowIntegration::setThemeMode(ThemeMode mode, bool animate)
{
    auto& theme = SkiaTheme::getInstance();

    if (theme.getThemeMode() != mode)
    {
        if (animate)
        {
            // Animate theme transition
            animateComponent("theme_transition", 1.0f, SpringPhysicsSettings::smooth());
        }

        theme.setThemeMode(mode);
        repaint();
    }
}

void SkiaMainWindowIntegration::setAdaptiveFPS(bool enable)
{
    auto gpuSettings = SkiaTheme::getInstance().getGPUSettings();
    gpuSettings.adaptiveFPS = enable;
    updateGPUSettings(gpuSettings);
}

void SkiaMainWindowIntegration::updateGPUSettings(const SkiaTheme::GPUSettings& settings)
{
    SkiaTheme::getInstance().setGPUSettings(settings);

    // Adjust timer interval based on target FPS
    stopTimer();
    int intervalMs = static_cast<int>(1000.0f / settings.targetFPS);
    startTimer(juce::jlimit(1, 100, intervalMs));
}

bool SkiaMainWindowIntegration::initializeSkiaRendering()
{
    DBG("SkiaMainWindowIntegration::initializeSkiaRendering() called");

    if (skiaInitialized_)
    {
        DBG("  Already initialized - returning true");
        return true;
    }

    DBG("  Starting initialization process...");

    try
    {
        DBG("  Step 1: Getting SkiaTheme singleton instance");
        auto& theme = SkiaTheme::getInstance();

        DBG("  Step 2: Loading theme configuration");
        DBG("    Theme mode: " << (theme.getThemeMode() == ThemeMode::Dark ? "Dark" : "Light"));

        auto colors = theme.getColors();
        DBG("    Background color: 0x" << juce::String::toHexString((int)colors.background));
        DBG("    Primary color: 0x" << juce::String::toHexString((int)colors.primary));

        auto typo = theme.getTypography();
        DBG("    Base font size: " << typo.baseSize);

        auto gpu = theme.getGPUSettings();
        DBG("    Target FPS: " << gpu.targetFPS);
        DBG("    Adaptive FPS: " << (gpu.adaptiveFPS ? "enabled" : "disabled"));

        DBG("  Step 3: Marking theme configuration as loaded");
        skiaInitialized_ = true;

        DBG("  ⚠ WARNING: This only initializes theme settings!");
        DBG("  ⚠ SkiaRenderer is NOT created - UI components use JUCE fallback");
        DBG("  ⚠ To use actual Skia rendering, SkiaRenderer must be instantiated");
        DBG("  ✓ Theme configuration loaded successfully");
        return true;
    }
    catch (const std::exception& e)
    {
        DBG("  ✗ EXCEPTION caught during initialization!");
        DBG("    Exception type: std::exception");
        DBG("    Message: " << e.what());
        DBG("  SkiaMainWindowIntegration initialization FAILED");
        return false;
    }
    catch (...)
    {
        DBG("  ✗ UNKNOWN EXCEPTION caught during initialization!");
        DBG("  SkiaMainWindowIntegration initialization FAILED");
        return false;
    }
}

void SkiaMainWindowIntegration::shutdownSkiaRendering()
{
    if (!skiaInitialized_)
        return;

    stopAnimationLoop();
    animationController_.clear();
    skiaInitialized_ = false;

    DBG("SkiaMainWindowIntegration: Shutdown complete");
}

void SkiaMainWindowIntegration::resized()
{
    // Override if needed for layout adjustments
}

void SkiaMainWindowIntegration::paint(juce::Graphics& g)
{
    // Clear background with theme color
    const auto& colors = SkiaTheme::getInstance().getColors();
    g.fillAll(juce::Colour(
        colors.background >> 16 & 0xFF,
        colors.background >> 8 & 0xFF,
        colors.background & 0xFF
    ));
}

void SkiaMainWindowIntegration::timerCallback()
{
    auto currentTime = juce::Time::getCurrentTime();
    float deltaTime = lastFrameTime_.toMilliseconds() > 0
                          ? static_cast<float>((currentTime.toMilliseconds() - lastFrameTime_.toMilliseconds()) / 1000.0)
                          : 0.016f; // Default to 60 FPS if first frame

    lastFrameTime_ = currentTime;

    // Clamp delta time to prevent huge jumps
    deltaTime = juce::jlimit(0.001f, 0.1f, deltaTime);

    // Update animations
    animationController_.update(deltaTime);

    // Stop timer if no animations are running
    if (!animationController_.hasActiveAnimations())
    {
        stopAnimationLoop();
    }

    repaint();
}

void SkiaMainWindowIntegration::startAnimationLoop()
{
    if (!isAnimationLoopRunning_)
    {
        isAnimationLoopRunning_ = true;
        startTimer(16); // ~60 FPS

        // Make sure component is visible for updates
        if (!isVisible())
        {
            setVisible(true);
        }
    }
}

void SkiaMainWindowIntegration::stopAnimationLoop()
{
    if (isAnimationLoopRunning_)
    {
        isAnimationLoopRunning_ = false;
        stopTimer();
    }
}

} // namespace zenith

#endif // ZENITH_USE_SKIA

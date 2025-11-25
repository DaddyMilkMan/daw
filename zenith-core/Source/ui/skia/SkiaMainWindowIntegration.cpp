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
    // 1. Attach OpenGL context to this component
    openGLContext.setRenderer(this);
    openGLContext.setContinuousRepainting(true);
    openGLContext.setComponentPaintingEnabled(true);
    openGLContext.attachTo(*this);

    // Set initial timer interval (will be adjusted by FPS settings)
    startTimer(16); // ~60 FPS
    lastFrameTime_ = juce::Time::getCurrentTime();

    DBG("SkiaMainWindowIntegration: Constructor - OpenGL context attached");
}

SkiaMainWindowIntegration::~SkiaMainWindowIntegration()
{
    // 2. Detach OpenGL context cleanly
    openGLContext.detach();
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

    // Update the actual renderer target FPS if it exists
    if (renderer_) {
        renderer_->setTargetFPS(settings.targetFPS);
    }
}

bool SkiaMainWindowIntegration::initializeSkiaRendering()
{
    // Initialization is now deferred to newOpenGLContextCreated
    // This method is kept for API compatibility
    return true;
}

void SkiaMainWindowIntegration::shutdownSkiaRendering()
{
    renderer_.reset();
    skiaInitialized_ = false;
    DBG("SkiaMainWindowIntegration: Shutdown complete");
}

void SkiaMainWindowIntegration::resized()
{
    // No-op - renderOpenGL handles resize based on current bounds
}

void SkiaMainWindowIntegration::paint(juce::Graphics& g)
{
    // Paint is called for JUCE overlay components on top of OpenGL
    // Usually empty - OpenGL rendering is handled in renderOpenGL()
    if (!skiaInitialized_)
    {
        g.fillAll(juce::Colours::black);
        g.setColour(juce::Colours::white);
        g.drawText("Initializing OpenGL...", getLocalBounds(), juce::Justification::centred, true);
    }
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
        int targetFPS = renderer_ ? renderer_->getTargetFPS() : 60;
        startTimer(1000 / targetFPS);
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

//==============================================================================
// OpenGL Rendering Callbacks
//==============================================================================

void SkiaMainWindowIntegration::newOpenGLContextCreated()
{
    // This runs on the Render Thread
    DBG("SkiaMainWindowIntegration::newOpenGLContextCreated() - Initializing Skia on render thread");

    try
    {
        auto& theme = SkiaTheme::getInstance();
        auto gpu = theme.getGPUSettings();

        renderer_ = std::make_unique<SkiaRenderer>(*this, SkiaRenderer::Backend::OpenGL, true);
        renderer_->setTargetFPS(gpu.targetFPS);

        if (renderer_->initialize())
        {
            skiaInitialized_ = true;
            DBG("✓ Skia initialized on OpenGL render thread");
        }
        else
        {
            DBG("✗ SkiaRenderer::initialize() failed");
        }
    }
    catch (const std::exception& e)
    {
        DBG("✗ Exception in newOpenGLContextCreated: " << e.what());
    }
    catch (...)
    {
        DBG("✗ Unknown exception in newOpenGLContextCreated");
    }
}

void SkiaMainWindowIntegration::renderOpenGL()
{
    // This runs on the Render Thread at the OpenGL refresh rate
    if (!skiaInitialized_ || !renderer_)
        return;

    try
    {
        // Get DPI scaling for Windows 1440p monitors
        const float scale = (float)openGLContext.getRenderingScale();
        currentScale_ = scale;

        // Resize surface if needed (check bounds)
        int scaledWidth = getWidth() * scale;
        int scaledHeight = getHeight() * scale;
        renderer_->resize(scaledWidth, scaledHeight);

        // Render frame with Skia
        renderer_->render([this, scale](SkCanvas* canvas) {
            // Apply DPI scaling
            canvas->save();
            canvas->scale(scale, scale);

            // Clear with theme background
            const auto& colors = SkiaTheme::getInstance().getColors();
            canvas->clear(SkColorSetRGB(
                (colors.background >> 16) & 0xFF,
                (colors.background >> 8) & 0xFF,
                colors.background & 0xFF
            ));

            // Draw test circle to prove GPU rendering works
            SkPaint paint;
            paint.setColor(SK_ColorGREEN);
            paint.setAntiAlias(true);
            canvas->drawCircle(getWidth() / 2.0f, getHeight() / 2.0f, 50.0f, paint);

            // TODO: Delegate to child SkiaComponents here

            canvas->restore();
        });
    }
    catch (const std::exception& e)
    {
        DBG("✗ Exception in renderOpenGL: " << e.what());
    }
    catch (...)
    {
        DBG("✗ Unknown exception in renderOpenGL");
    }
}

void SkiaMainWindowIntegration::openGLContextClosing()
{
    // Clean up Skia resources before OpenGL context is destroyed
    DBG("SkiaMainWindowIntegration::openGLContextClosing() - Cleaning up");

    if (renderer_)
    {
        renderer_->shutdown();
        renderer_.reset();
    }

    skiaInitialized_ = false;
}

} // namespace zenith

#endif // ZENITH_USE_SKIA

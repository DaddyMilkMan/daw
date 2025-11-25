/**
 * @file SkiaMainWindowIntegration.h
 * @brief Skia rendering integration for the main application window
 *
 * Provides:
 * - GPU-accelerated rendering via OpenGL + Skia
 * - Complete DAW shell layout (Transport, Browser, Timeline, Inspector)
 * - DPI-aware scaling for high-resolution monitors
 * - Animation and playhead rendering
 */

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include <juce_opengl/juce_opengl.h>

#include "../../rendering/SkiaRenderer.h"
#include "SkiaTheme.h"

#ifdef ZENITH_USE_SKIA
    #include <include/core/SkCanvas.h>
    #include <include/core/SkPaint.h>
    #include <include/core/SkRRect.h>
    #include <include/core/SkFont.h>
    #include <include/core/SkTypeface.h>
#endif

namespace zenith {

/**
 * @class SkiaMainWindowIntegration
 * @brief Core integration point for GPU-accelerated Skia rendering
 *
 * Manages OpenGL context, renders the complete DAW interface using Skia,
 * and handles DPI scaling for high-resolution monitors.
 */
class SkiaMainWindowIntegration : public juce::Component,
                                  public juce::OpenGLRenderer,
                                  private juce::Timer
{
public:
    SkiaMainWindowIntegration();
    ~SkiaMainWindowIntegration() override;

    // Standard Component Lifecycle
    void paint(juce::Graphics& g) override;
    void resized() override;

    // OpenGL Renderer Lifecycle
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    // Rendering Control
    bool initializeSkiaRendering();
    void shutdownSkiaRendering();

private:
    void timerCallback() override;

    // --- Drawing Helpers for DAW UI ---
    void drawTransportBar(SkCanvas* canvas, const SkRect& bounds, float scale);
    void drawBrowserPanel(SkCanvas* canvas, const SkRect& bounds, float scale);
    void drawTimelineArea(SkCanvas* canvas, const SkRect& bounds, float scale);
    void drawInspectorPanel(SkCanvas* canvas, const SkRect& bounds, float scale);
    void drawPlayhead(SkCanvas* canvas, const SkRect& bounds, float scale);

    // --- Member Variables ---
    juce::OpenGLContext openGLContext;
    std::unique_ptr<SkiaRenderer> renderer_;
    bool skiaInitialized_ = false;

    // Animation State
    float playheadPosition_ = 0.0f;
    bool isPlaying_ = true;
    int64_t frameCounter_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaMainWindowIntegration)
};

} // namespace zenith

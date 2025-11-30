#pragma once

#include <JuceHeader.h>

#ifdef ZENITH_USE_SKIA
#define NOMINMAX
#include <include/core/SkCanvas.h>
#include <include/core/SkSurface.h>
#include <include/core/SkRefCnt.h>
#include <include/gpu/ganesh/GrDirectContext.h>
#include <include/gpu/ganesh/SkSurfaceGanesh.h>
#include <include/gpu/ganesh/GrBackendSurface.h>
#include <include/gpu/ganesh/gl/GrGLBackendSurface.h>
#include <include/gpu/ganesh/gl/GrGLDirectContext.h>
#include <include/core/SkColorSpace.h>
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

/**
 * @brief Base class for main window with Skia rendering
 * 
 * Provides OpenGL context management and Skia canvas rendering.
 * Derived classes override drawSkiaContent() to render their UI.
 */
class SkiaMainWindowIntegration : public juce::Component,
                                  public juce::OpenGLRenderer {
public:
    SkiaMainWindowIntegration();
    ~SkiaMainWindowIntegration() override;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    // OpenGLRenderer overrides
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

protected:
    /**
     * @brief Override this to draw your Skia content
     * @param canvas Skia canvas to draw on
     */
    virtual void drawSkiaContent(SkCanvas* canvas) = 0;

    /**
     * @brief Get the current Skia canvas (valid during renderOpenGL)
     */
    SkCanvas* getSkiaCanvas() { return skiaCanvas_; }

private:
    juce::OpenGLContext openGLContext_;
    GrDirectContext* grContext_ = nullptr;
    SkSurface* surface_ = nullptr;
    SkCanvas* skiaCanvas_ = nullptr;
    
    bool contextInitialized_ = false;
    int lastWidth_ = 0;
    int lastHeight_ = 0;

    void recreateSurface();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaMainWindowIntegration)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith

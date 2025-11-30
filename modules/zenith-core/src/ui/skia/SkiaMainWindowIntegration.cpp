/*
  ==============================================================================

    SkiaMainWindowIntegration.cpp
    Created: 2025-11-28
    Author:  Zenith DAW Team

    Implementation of Skia-rendered main window base class.

  ==============================================================================
*/

#include "SkiaMainWindowIntegration.h"

#ifdef ZENITH_USE_SKIA
#include <include/gpu/ganesh/gl/GrGLInterface.h>
#include <include/core/SkSurface.h>
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

SkiaMainWindowIntegration::SkiaMainWindowIntegration() {
    // Attach OpenGL context to this component
    openGLContext_.setRenderer(this);
    openGLContext_.attachTo(*this);
    openGLContext_.setContinuousRepainting(true);
}

SkiaMainWindowIntegration::~SkiaMainWindowIntegration() {
    if (surface_) surface_->unref();
    openGLContext_.detach();
}

void SkiaMainWindowIntegration::paint(juce::Graphics& g) {
    // OpenGL rendering handles everything
    // This is just a fallback
    g.fillAll(juce::Colour(0xff0a0a0f));
}


void SkiaMainWindowIntegration::resized() {
    // Surface will be recreated in renderOpenGL if size changed
}

void SkiaMainWindowIntegration::newOpenGLContextCreated() {
    // Create Skia GPU context
    auto glInterface = GrGLMakeNativeInterface();
    auto ctx = GrDirectContexts::MakeGL(glInterface);
    if (grContext_) grContext_->unref();
    grContext_ = ctx.release();
    
    if (!grContext_) {
        DBG("Failed to create Skia GrDirectContext!");
        return;
    }

    contextInitialized_ = true;
    recreateSurface();
}

void SkiaMainWindowIntegration::renderOpenGL() {
    if (!contextInitialized_ || !grContext_) {
        return;
    }

    auto width = getWidth();
    auto height = getHeight();

    // Recreate surface if size changed
    if (width != lastWidth_ || height != lastHeight_) {
        recreateSurface();
        lastWidth_ = width;
        lastHeight_ = height;
    }

    if (!surface_) {
        return;
    }

    // Get canvas and clear
    skiaCanvas_ = surface_->getCanvas();
    skiaCanvas_->clear(SkColorSetARGB(255, 10, 10, 15)); // Dark background

    // Let derived class draw
    drawSkiaContent(skiaCanvas_);

    // Flush to GPU
    grContext_->flushAndSubmit();
    skiaCanvas_ = nullptr;
}

void SkiaMainWindowIntegration::openGLContextClosing() {
    if (surface_) {
        surface_->unref();
        surface_ = nullptr;
    }
    if (grContext_) {
        grContext_->unref();
        grContext_ = nullptr;
    }
    contextInitialized_ = false;
}

void SkiaMainWindowIntegration::recreateSurface() {
    if (!grContext_) {
        return;
    }

    auto width = getWidth();
    auto height = getHeight();

    if (width <= 0 || height <= 0) {
        return;
    }

    // Get framebuffer info
    GrGLFramebufferInfo framebufferInfo;
    framebufferInfo.fFBOID = 0; // Default framebuffer
    framebufferInfo.fFormat = 0x8058; // GL_RGBA8

    // Create backend render target
    auto backendRT = GrBackendRenderTargets::MakeGL(
        width, height,
        0, // sample count
        8, // stencil bits
        framebufferInfo
    );

    // Create Skia surface (using new API)
    auto s = SkSurfaces::WrapBackendRenderTarget(
        grContext_,
        backendRT,
        kBottomLeft_GrSurfaceOrigin,
        kRGBA_8888_SkColorType,
        nullptr,
        nullptr
    );
    if (surface_) surface_->unref();
    surface_ = s.release();

    if (!surface_) {
        DBG("Failed to create Skia surface!");
    }
}

#endif // ZENITH_USE_SKIA

} // namespace zenith

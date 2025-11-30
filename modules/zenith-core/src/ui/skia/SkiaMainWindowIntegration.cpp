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
#include <fstream>

static void logDiagnostic(const char* msg) {
    std::ofstream outfile;
    outfile.open("C:\\zenith\\daw\\diagnostic_log.txt", std::ios_base::app);
    outfile << msg << std::endl;
}
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

SkiaMainWindowIntegration::SkiaMainWindowIntegration() {
    logDiagnostic("SkiaMainWindowIntegration: Constructor called.");
    // Attach OpenGL context to this component
    openGLContext_.setRenderer(this);
    openGLContext_.attachTo(*this);
    openGLContext_.setContinuousRepainting(true);
    logDiagnostic("SkiaMainWindowIntegration: OpenGL Context attached.");
}

SkiaMainWindowIntegration::~SkiaMainWindowIntegration() {
    if (surface_) surface_->unref();
    openGLContext_.detach();
}

void SkiaMainWindowIntegration::paint(juce::Graphics& g) {
    // OpenGL rendering handles everything
    // This is just a fallback
    // logDiagnostic("SkiaMainWindowIntegration: paint() called (Software Fallback)");
    g.fillAll(juce::Colour(0xff0a0a0f));
}


void SkiaMainWindowIntegration::resized() {
    // Surface will be recreated in renderOpenGL if size changed
}

void SkiaMainWindowIntegration::newOpenGLContextCreated() {
    logDiagnostic("SkiaMainWindowIntegration: Creating OpenGL Context...");
    // Create Skia GPU context
    auto glInterface = GrGLMakeNativeInterface();
    if (!glInterface) {
        logDiagnostic("ERROR: Failed to create GL Interface!");
        return;
    }
    
    auto ctx = GrDirectContexts::MakeGL(glInterface);
    if (grContext_) grContext_->unref();
    grContext_ = ctx.release();
    
    if (!grContext_) {
        logDiagnostic("ERROR: Failed to create Skia GrDirectContext!");
        return;
    }
    logDiagnostic("SkiaMainWindowIntegration: GrDirectContext created successfully.");

    contextInitialized_ = true;
    recreateSurface();
}

void SkiaMainWindowIntegration::renderOpenGL() {
    if (!contextInitialized_ || !grContext_) {
        // logDiagnostic("SkiaMainWindowIntegration: Not initialized, skipping render.");
        return;
    }

    auto width = getWidth();
    auto height = getHeight();

    // Recreate surface if size changed
    if (width != lastWidth_ || height != lastHeight_) {
        std::string msg = "SkiaMainWindowIntegration: Resizing surface to " + std::to_string(width) + "x" + std::to_string(height);
        logDiagnostic(msg.c_str());
        recreateSurface();
        lastWidth_ = width;
        lastHeight_ = height;
    }

    if (!surface_) {
        return;
    }

    // Get canvas and clear
    skiaCanvas_ = surface_->getCanvas();
    
    // DIAGNOSTIC: Change clear color to MAGENTA
    skiaCanvas_->clear(SkColorSetRGB(255, 0, 255)); 

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

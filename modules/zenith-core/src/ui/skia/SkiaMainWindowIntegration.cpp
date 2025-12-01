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
    setOpaque(true);
    
    // Attach OpenGL context to this component
    openGLContext_.setRenderer(this);
    openGLContext_.setComponentPaintingEnabled(false); // Disable JUCE painting on top
    openGLContext_.setContinuousRepainting(true);
    // openGLContext_.attachTo(*this); // MOVED to initializeSkia() to avoid race condition
}

void SkiaMainWindowIntegration::initializeSkia() {
    openGLContext_.attachTo(*this);
}

SkiaMainWindowIntegration::~SkiaMainWindowIntegration() {
    openGLContext_.detach();
    if (surface_) surface_->unref();
}

void SkiaMainWindowIntegration::paint(juce::Graphics& g) {
    // If we are here, OpenGL is not running or we are painting on top
    // Draw a fallback message
    logDiagnostic("SkiaMainWindowIntegration: paint() called (Software Fallback - BLUE)");
    g.fillAll(juce::Colours::blue);
    g.setColour(juce::Colours::white);
    g.setFont(20.0f);
    g.drawText("Rendering Backend: Software (Blue Screen of Life)", getLocalBounds(), juce::Justification::centred, true);
}


void SkiaMainWindowIntegration::resized() {
    // Surface will be recreated in renderOpenGL if size changed
}

void SkiaMainWindowIntegration::newOpenGLContextCreated() {
    // Create Skia GPU context
    auto glInterface = GrGLMakeNativeInterface();
    if (!glInterface) {
        return;
    }
    
    auto ctx = GrDirectContexts::MakeGL(glInterface);
    if (grContext_) grContext_->unref();
    grContext_ = ctx.release();
    
    if (!grContext_) {
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
    
    // Clear background
    skiaCanvas_->clear(SkColorSetARGB(255, 10, 10, 15)); 

    // Let derived class draw
    renderSkia(skiaCanvas_);

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

void SkiaMainWindowIntegration::renderSkia(SkCanvas* canvas) {
    logDiagnostic("Base SkiaMainWindowIntegration::renderSkia called - MainComponent override not found!");
}

#endif // ZENITH_USE_SKIA

} // namespace zenith

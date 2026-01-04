/**
 * @file SkiaRenderer.cpp
 * @brief Hardware-accelerated Skia rendering via EGL/Wayland for Pop!_OS
 */

#include "SkiaRenderer.h"
#include <core/SkCanvas.h>
#include <core/SkColorSpace.h>
#include <core/SkSurface.h>
#include <gpu/ganesh/GrDirectContext.h>
#include <gpu/ganesh/SkSurfaceGanesh.h>
#include <gpu/ganesh/gl/GrGLDirectContext.h>
#include <gpu/ganesh/gl/GrGLInterface.h>
#include <gpu/ganesh/gl/GrGLAssembleInterface.h>

#if JUCE_LINUX
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif

namespace zenith {

//==============================================================================
SkiaRenderer::SkiaRenderer(juce::Component &component, Backend backend, bool enableVSync)
    : component_(component), backend_(backend), vsyncEnabled_(enableVSync) {
    if (backend_ == Backend::Auto) {
        backend_ = detectBestBackend();
    }
}

SkiaRenderer::~SkiaRenderer() {
    shutdown();
}

bool SkiaRenderer::initialize() {
    if (initialized_) return true;

    // 1. Create GPU Context
    if (!createGpuContext()) {
        // Fallback to software if GPU fails
        backend_ = Backend::Software;
    }

    // 2. Create Initial Surface
    int w = component_.getWidth();
    int h = component_.getHeight();
    if (w > 0 && h > 0) {
        if (!createSurface(w, h)) {
            return false;
        }
    }

    initialized_ = true;
    return true;
}

void SkiaRenderer::shutdown() {
    surface_.reset();
    grContext_.reset();
    initialized_ = false;
}

void SkiaRenderer::resize(int width, int height) {
    if (!initialized_) return;
    createSurface(width, height);
}

SkiaRenderer::Backend SkiaRenderer::detectBestBackend() const {
#if JUCE_LINUX
    return Backend::OpenGL; // EGL/OpenGL is standard for Linux currently
#elif JUCE_MAC
    return Backend::Metal;
#elif JUCE_WINDOWS
    return Backend::Direct3D;
#else
    return Backend::Software;
#endif
}

//==============================================================================
bool SkiaRenderer::createGpuContext() {
    if (backend_ == Backend::OpenGL) {
#if JUCE_LINUX
        // Wayland/Pop!_OS specific EGL Bootstrapping
        auto interface = GrGLMakeAssembledInterface(nullptr, [](void* ctx, const char* name) -> GrGLFuncPtr {
            return (GrGLFuncPtr)eglGetProcAddress(name);
        });

        if (!interface) {
            // Fallback to native
            interface = GrGLMakeNativeInterface();
        }

        if (interface) {
            grContext_ = GrDirectContexts::MakeGL(interface);
            return grContext_ != nullptr;
        }
#else
        // Windows/Mac: Use standard native interface (WGL/CGL)
        // managed by JUCE's OpenGLContext usually, but here we are standalone?
        // For now, try native interface which Skia should be able to resolve 
        // if an OpenGL context is active.
        auto interface = GrGLMakeNativeInterface();
        if (interface) {
            grContext_ = GrDirectContexts::MakeGL(interface);
            return grContext_ != nullptr;
        }
#endif
    }
    
    // Fallback to Software if GPU fails, or if Software requested
    return backend_ == Backend::Software;
}

bool SkiaRenderer::createSurface(int width, int height) {
    if (width <= 0 || height <= 0) return false;
    surface_.reset();

    if (backend_ == Backend::Software || !grContext_) {
        SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
        surface_ = SkSurfaces::Raster(info);
    } else {
        SkImageInfo info = SkImageInfo::MakeN32Premul(width, height, SkColorSpace::MakeSRGB());
        
        // This creates a GPU-backed surface. 
        // In Jan Week 1, we map this to the JUCE OpenGL Framebuffer.
        surface_ = SkSurfaces::RenderTarget(grContext_.get(), skgpu::Budgeted::kNo, info);
    }
    return surface_ != nullptr;
}

void SkiaRenderer::render(std::function<void(SkCanvas*)> drawCallback) {
    if (!initialized_ || !surface_) return;

    SkCanvas* canvas = surface_->getCanvas();
    if (canvas) {
        // We don't clear to BLACK here because Zenith uses AuroraBackground
        // canvas->clear(SK_ColorBLACK); 
        
        if (drawCallback) drawCallback(canvas);
        
        if (grContext_) {
            grContext_->flush();
        }
    }
}

void SkiaRenderer::setTargetFPS(int fps) {
    targetFPS_ = fps;
}

void SkiaRenderer::setVSyncEnabled(bool enable) {
    vsyncEnabled_ = enable;
}

const char* SkiaRenderer::getBackendName(Backend backend) {
    switch (backend) {
        case Backend::Auto: return "Auto";
        case Backend::Direct3D: return "Direct3D";
        case Backend::Metal: return "Metal";
        case Backend::Vulkan: return "Vulkan";
        case Backend::OpenGL: return "OpenGL";
        case Backend::Software: return "Software";
        default: return "Unknown";
    }
}

} // namespace zenith
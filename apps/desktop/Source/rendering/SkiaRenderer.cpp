/**
 * @file SkiaRenderer.cpp
 * @brief Implementation of SkiaRenderer
 */

#include "rendering/SkiaRenderer.h"
#include <juce_opengl/juce_opengl.h> // For GL types and constants

// Check if Skia is enabled
#ifdef ZENITH_USE_SKIA

// Skia Headers
#include <core/SkCanvas.h>
#include <core/SkColorSpace.h>
#include <core/SkSurface.h>
#include <include/gpu/ganesh/GrBackendSurface.h> // Defines GrBackendRenderTarget
#include <include/gpu/ganesh/GrDirectContext.h>
#include <include/gpu/ganesh/gl/GrGLDirectContext.h> // For GrDirectContexts::MakeGL
#include <include/gpu/ganesh/gl/GrGLBackendSurface.h> // For GrBackendRenderTargets::MakeGL
#include <include/gpu/ganesh/SkSurfaceGanesh.h> // For SkSurfaces::WrapBackendRenderTarget
#include <include/gpu/ganesh/gl/GrGLInterface.h>

using namespace juce::gl; // Use JUCE's GL bindings

#endif

namespace zenith {

//==============================================================================
// Construction / Destruction
//==============================================================================

SkiaRenderer::SkiaRenderer(juce::Component& component, Backend backend, bool enableVSync)
    : component_(component)
    , backend_(backend)
    , vsyncEnabled_(enableVSync)
{
}

SkiaRenderer::~SkiaRenderer()
{
    shutdown();
}

//==============================================================================
// Initialization
//==============================================================================

bool SkiaRenderer::initialize()
{
#ifdef ZENITH_USE_SKIA
    if (initialized_)
        return true;

    // Create the GPU context
    if (!createGpuContext())
    {
        juce::Logger::writeToLog("SkiaRenderer: Failed to create GPU context. Check GPU drivers.");
        jassertfalse;
        return false;
    }

    juce::Logger::writeToLog("SkiaRenderer: GPU Context Initialized Successfully.");
    initialized_ = true;
    return true;
#else
    juce::Logger::writeToLog("SkiaRenderer: ZENITH_USE_SKIA not defined - Skia disabled.");
    return false;
#endif
}

void SkiaRenderer::shutdown()
{
#ifdef ZENITH_USE_SKIA
    // Release GPU resources in reverse order of creation
    surface_.reset();
    
    if (grContext_)
    {
        // Invalidate the context - required for Wayland where contexts can be lost
        grContext_->abandonContext();
        grContext_.reset();
    }
    
    // Reset cache metadata
    fboCache_ = FboMetadata{};
    
    initialized_ = false;
    juce::Logger::writeToLog("SkiaRenderer: Shutdown complete.");
#endif
}

//==============================================================================
// Rendering
//==============================================================================

void SkiaRenderer::render(std::function<void(SkCanvas*)> drawCallback)
{
#ifdef ZENITH_USE_SKIA
    if (!initialized_)
    {
        if (!initialize())
            return;
    }

    // 1. Context Validation (Wayland/Mac Sleep Handling)
    // CRITICAL: On Wayland, the GL context can be lost at any time (window resize, monitor sleep).
    // We check grContext_->abandoned() every frame and attempt recovery.
    if (!grContext_ || grContext_->abandoned())
    {
        juce::Logger::writeToLog("SkiaRenderer: Context lost or abandoned. Attempting recovery...");
        shutdown();
        if (!initialize())
        {
            juce::Logger::writeToLog("SkiaRenderer: Context recovery FAILED.");
            return;
        }
        juce::Logger::writeToLog("SkiaRenderer: Context recovery successful.");
    }

    // 2. Stats Update
    auto now = juce::Time::getCurrentTime();
    double frameDelta = (now - lastFrameTime_).inMilliseconds();
    lastFrameTime_ = now;
    
    if (frameDelta > 0)
    {
        frameTimes_.push_back(frameDelta);
        if (frameTimes_.size() > 60)
            frameTimes_.erase(frameTimes_.begin());
        
        double sum = 0;
        for (auto t : frameTimes_) sum += t;
        stats_.averageFrameTime = sum / frameTimes_.size();
        stats_.frameTime = frameDelta;
        stats_.fps = (stats_.averageFrameTime > 0) ? (int)(1000.0 / stats_.averageFrameTime) : 0;
    }

    // 3. Delegate to Internal Render Frame
    renderFrame(drawCallback);

#endif
}

void SkiaRenderer::renderFrame(std::function<void(SkCanvas *)>& drawCallback)
{
#ifdef ZENITH_USE_SKIA
    // 4. Handle Resize / Surface Creation
    int width = component_.getWidth();
    int height = component_.getHeight();
    
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int renderWidth = (viewport[2] > 0) ? viewport[2] : width;
    int renderHeight = (viewport[3] > 0) ? viewport[3] : height;

    GLint currentFbo = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFbo);

    // Optimized surface recreation
    if (!surface_ || !fboCache_.matches(currentFbo, renderWidth, renderHeight))
    {
        surface_.reset(); 
        if (!createSurface(renderWidth, renderHeight))
            return;
    }
    
    // 5. Drawing Step
    if (surface_)
    {
        auto* canvas = surface_->getCanvas();
        if (canvas)
        {
            drawCallback(canvas);
        
            // 6. Flush (Zero-allocation path)
            grContext_->flush();
        }
    }
#endif
}

void SkiaRenderer::resize(int width, int height)
{
    // The actual resize logic happens in render() where we act on dimensions.
    (void)width; (void)height;
}

void SkiaRenderer::setTargetFPS(int fps)
{
    targetFPS_ = fps;
}

void SkiaRenderer::setVSyncEnabled(bool enable)
{
    vsyncEnabled_ = enable;
}

const char* SkiaRenderer::getBackendName(Backend backend)
{
    switch (backend)
    {
        case Backend::Auto:     return "Auto";
        case Backend::Direct3D: return "Direct3D 12";
        case Backend::Metal:    return "Metal";
        case Backend::Vulkan:   return "Vulkan";
        case Backend::OpenGL:   return "OpenGL";
        case Backend::Software: return "Software";
        default:                return "Unknown";
    }
}

//==============================================================================
// Internal Methods
//==============================================================================

SkiaRenderer::Backend SkiaRenderer::detectBestBackend() const
{
#if JUCE_WINDOWS
    return Backend::Direct3D; // Or OpenGL if preferred
#elif JUCE_MAC
    return Backend::Metal;
#elif JUCE_LINUX
    // Wayland/X11 usually implies OpenGL or Vulkan
    return Backend::OpenGL; 
#else
    return Backend::OpenGL;
#endif
}

bool SkiaRenderer::createGpuContext()
{
#ifdef ZENITH_USE_SKIA
    // 1. Create the native GL interface
    // Skia needs to abstract over the specific GL driver (Mesa, Nvidia, etc.)
    auto interface = GrGLMakeNativeInterface();
    if (!interface)
    {
        juce::Logger::writeToLog("SkiaRenderer: Failed to create native GL interface. Check GPU drivers.");
        jassertfalse;
        return false;
    }

    // 2. Create the GrDirectContext (The GPU Manager)
    grContext_ = GrDirectContexts::MakeGL(interface);
    if (!grContext_)
    {
        juce::Logger::writeToLog("SkiaRenderer: Failed to create GrDirectContext.");
        jassertfalse;
        return false;
    }

    return true;
#else
    return false;
#endif
}

bool SkiaRenderer::createSurface(int width, int height)
{
#ifdef ZENITH_USE_SKIA
    if (!grContext_) return false;

    // 1. Query Current FBO Metadata
    GLint fboId = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fboId);

    GLint samples = 0;
    glGetIntegerv(GL_SAMPLES, &samples);
    
    GLint stencil = 0;
    glGetIntegerv(GL_STENCIL_BITS, &stencil);

    // 2. Prepare Backend Render Target
    GrGLFramebufferInfo fbInfo;
    fbInfo.fFBOID = (GrGLuint)fboId;
    fbInfo.fFormat = GL_RGBA8; 

    GrBackendRenderTarget backendRT = GrBackendRenderTargets::MakeGL(
        width, height, samples, stencil, fbInfo
    );

    // 3. Create Surface Wrapper
    // Wrap the backend render target in a Skia Surface
    // kBottomLeft_GrSurfaceOrigin: OpenGL uses bottom-left origin
    // kRGBA_8888_SkColorType: Standard byte order for cross-platform compatibility
    surface_ = SkSurfaces::WrapBackendRenderTarget(
        grContext_.get(),
        backendRT,
        kBottomLeft_GrSurfaceOrigin, // OpenGL uses bottom-left origin
        kRGBA_8888_SkColorType,      // Standard byte order
        SkColorSpace::MakeSRGB(),    // sRGB color space
        nullptr
    );
    
    if (!surface_)
    {
        // Fallback: Try RGB8 if RGBA8 fails
        fbInfo.fFormat = GL_RGB8;
        backendRT = GrBackendRenderTargets::MakeGL(width, height, samples, stencil, fbInfo);
        
        surface_ = SkSurfaces::WrapBackendRenderTarget(
            grContext_.get(),
            backendRT,
            kBottomLeft_GrSurfaceOrigin,
            kRGBA_8888_SkColorType,
            SkColorSpace::MakeSRGB(),
            nullptr
        );
    }
    
    if (surface_)
    {
        // Update Metadata Cache (Optimizes surface recreation on subsequent frames)
        fboCache_.fboId = fboId;
        fboCache_.width = width;
        fboCache_.height = height;
        fboCache_.samples = samples;
        fboCache_.stencilBits = stencil;
        fboCache_.format = fbInfo.fFormat;
        return true;
    }
    
    juce::Logger::writeToLog("SkiaRenderer: Critical Error - Failed to create SkSurface.");
    jassertfalse;

    return false;
#else
    return false;
#endif
}

#if JUCE_WINDOWS
bool SkiaRenderer::createD3DContext() { return false; } 
#elif JUCE_MAC
bool SkiaRenderer::createMetalContext() { return false; } 
#elif JUCE_LINUX
bool SkiaRenderer::createVulkanContext() { return false; } 
#endif

void SkiaRenderer::updateStats()
{
    // implemented inside render()
}

} // namespace zenith
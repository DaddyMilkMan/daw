/**
 * @file SkiaContextManager.cpp
 * @brief Implementation of centralized Skia context manager
 */

#include "SkiaContextManager.h"

#ifdef ZENITH_USE_SKIA
#include "include/core/SkBitmap.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkImage.h"
#include "include/core/SkPixmap.h"
#include "include/core/SkSurface.h"
#include "include/gpu/GpuTypes.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"

// OpenGL backend (available in vcpkg)
#ifdef SK_GL
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"
#endif

// Direct3D backend (requires skia[direct3d] feature)
#if defined(SK_DIRECT3D) && JUCE_WINDOWS
#include "include/gpu/ganesh/d3d/GrD3DBackendContext.h"
#include "include/gpu/ganesh/d3d/GrD3DTypes.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#endif

// Metal backend (requires skia[metal] feature)
#if defined(SK_METAL) && JUCE_MAC
#include "include/gpu/ganesh/mtl/GrMtlBackendContext.h"
#include "include/gpu/ganesh/mtl/GrMtlTypes.h"
#endif

// Vulkan backend (requires skia[vulkan] feature)
#if defined(SK_VULKAN) && JUCE_LINUX
#include "include/gpu/ganesh/vk/GrVkBackendContext.h"
#include "include/gpu/ganesh/vk/GrVkTypes.h"
#endif
#endif

namespace zenith {

//==============================================================================
// Singleton Instance
//==============================================================================

SkiaContextManager &SkiaContextManager::getInstance() {
  static SkiaContextManager instance;
  return instance;
}

SkiaContextManager::SkiaContextManager()
    : initialized_(false), currentMode_(RenderMode::Software) {
  DBG("SkiaContextManager: Created");
}

SkiaContextManager::~SkiaContextManager() { shutdown(); }

//==============================================================================
// Initialization
//==============================================================================

bool SkiaContextManager::initialize(RenderMode preferredMode) {
  if (initialized_) {
    DBG("SkiaContextManager: Already initialized");
    return true;
  }

  DBG("SkiaContextManager: Initializing with preferred mode: "
      << (int)preferredMode);

#ifdef ZENITH_USE_SKIA

  // Try requested backend first
  bool success = false;

  switch (preferredMode) {
  case RenderMode::GPU_Direct3D:
    success = initializeGPU_Direct3D();
    break;

  case RenderMode::GPU_Metal:
    success = initializeGPU_Metal();
    break;

  case RenderMode::GPU_Vulkan:
    success = initializeGPU_Vulkan();
    break;

  case RenderMode::GPU_OpenGL:
    success = initializeGPU_OpenGL();
    break;

  case RenderMode::Software:
    success = initializeSoftware();
    break;
  }

  // If preferred backend failed, try fallbacks
  if (!success && preferredMode != RenderMode::GPU_OpenGL) {
    DBG("SkiaContextManager: Preferred backend failed, trying OpenGL...");
    success = initializeGPU_OpenGL();
  }

  // Final fallback: software rendering
  if (!success) {
    DBG("SkiaContextManager: GPU backends failed, falling back to software...");
    success = initializeSoftware();
  }

  if (success) {
    initialized_ = true;
    lastCleanupTime_ = juce::Time::getCurrentTime();
    DBG("SkiaContextManager: Initialized successfully with mode: "
        << (int)currentMode_);
  } else {
    DBG("ERROR: SkiaContextManager: All initialization attempts failed!");
  }

  return success;

#else
  DBG("ERROR: SkiaContextManager: ZENITH_USE_SKIA not defined!");
  return false;
#endif
}

void SkiaContextManager::shutdown() {
  if (!initialized_)
    return;

  DBG("SkiaContextManager: Shutting down...");

  std::lock_guard<std::mutex> lock(surfacesMutex_);

  // Release all surfaces
  surfaces_.clear();

  // Release GPU context
  if (grContext_) {
    grContext_->abandonContext(); // Abandon instead of flush for clean shutdown
    grContext_.reset();
  }

  // Platform-specific cleanup
  if (platformData_) {
    // TODO: Clean up platform-specific data
    platformData_ = nullptr;
  }

  initialized_ = false;
  currentMode_ = RenderMode::Software;

  DBG("SkiaContextManager: Shutdown complete");
}

//==============================================================================
// Backend Initialization
//==============================================================================

bool SkiaContextManager::initializeGPU_OpenGL() {
#if defined(ZENITH_USE_SKIA) && defined(SK_GL)
  DBG("SkiaContextManager: Initializing OpenGL backend...");

  // Create GL interface from current context
  sk_sp<const GrGLInterface> glInterface = GrGLMakeNativeInterface();

  if (!glInterface) {
    DBG("ERROR: Failed to create GrGLInterface (no OpenGL context?)");
    return false;
  }

  // Create Skia GPU context from OpenGL
  grContext_ = GrDirectContexts::MakeGL(glInterface);

  if (!grContext_) {
    DBG("ERROR: Failed to create GrDirectContext from OpenGL");
    return false;
  }

  currentMode_ = RenderMode::GPU_OpenGL;
  DBG("SkiaContextManager: OpenGL backend initialized successfully!");
  return true;

#else
  DBG("WARNING: OpenGL backend not available (SK_GL not defined or Skia "
      "disabled)");
  return false;
#endif
}

bool SkiaContextManager::initializeGPU_Direct3D() {
#if defined(ZENITH_USE_SKIA) && defined(SK_DIRECT3D) && JUCE_WINDOWS
  DBG("SkiaContextManager: Initializing Direct3D backend...");

  // Create D3D12 device
  ComPtr<ID3D12Device> device;
  ComPtr<ID3D12CommandQueue> queue;

  // TODO: Full D3D12 initialization
  // This requires:
  // 1. D3D12CreateDevice()
  // 2. Create command queue
  // 3. Wrap in GrD3DBackendContext
  // 4. GrDirectContext::MakeDirect3D()

  DBG("WARNING: Direct3D backend implementation incomplete");
  return false;

#else
  DBG("WARNING: Direct3D backend not available (SK_DIRECT3D not defined or not "
      "Windows)");
  return false;
#endif
}

bool SkiaContextManager::initializeGPU_Metal() {
#if defined(ZENITH_USE_SKIA) && defined(SK_METAL) && JUCE_MAC
  DBG("SkiaContextManager: Initializing Metal backend...");

  // TODO: Metal initialization
  // Requires:
  // 1. MTLCreateSystemDefaultDevice()
  // 2. Create command queue
  // 3. Wrap in GrMtlBackendContext
  // 4. GrDirectContext::MakeMetal()

  DBG("WARNING: Metal backend implementation incomplete");
  return false;

#else
  DBG("WARNING: Metal backend not available (SK_METAL not defined or not "
      "macOS)");
  return false;
#endif
}

bool SkiaContextManager::initializeGPU_Vulkan() {
#if defined(ZENITH_USE_SKIA) && defined(SK_VULKAN) && JUCE_LINUX
  DBG("SkiaContextManager: Initializing Vulkan backend...");

  // TODO: Vulkan initialization
  // Requires:
  // 1. Create VkInstance
  // 2. Create VkDevice and queues
  // 3. Wrap in GrVkBackendContext
  // 4. GrDirectContext::MakeVulkan()

  DBG("WARNING: Vulkan backend implementation incomplete");
  return false;

#else
  DBG("WARNING: Vulkan backend not available (SK_VULKAN not defined or not "
      "Linux)");
  return false;
#endif
}

bool SkiaContextManager::initializeSoftware() {
  DBG("SkiaContextManager: Initializing software backend...");

  // Software rendering doesn't need GPU context
  // Surfaces will be created with SkSurfaces::Raster()

  currentMode_ = RenderMode::Software;
  DBG("SkiaContextManager: Software backend initialized");
  return true;
}

//==============================================================================
// Surface Management
//==============================================================================

sk_sp<SkSurface>
SkiaContextManager::getOrCreateSurface(juce::Component &component) {
#ifdef ZENITH_USE_SKIA
  std::lock_guard<std::mutex> lock(surfacesMutex_);

  auto it = surfaces_.find(&component);

  int width = component.getWidth();
  int height = component.getHeight();

  // Validate dimensions
  if (width <= 0 || height <= 0) {
    DBG("WARNING: Invalid component dimensions: " << width << "x" << height);
    return nullptr;
  }

  // Check if surface exists and matches size
  if (it != surfaces_.end()) {
    auto &surf = it->second;
    if (surf.surface && surf.width == width && surf.height == height) {
      surf.lastUsed = juce::Time::getCurrentTime();
      return surf.surface;
    }
  }

  // Create new surface
  sk_sp<SkSurface> surface;

  if (currentMode_ == RenderMode::Software) {
    // Software surface
    SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
    surface = SkSurfaces::Raster(info);
  } else if (grContext_) {
    // GPU surface
    SkImageInfo info =
        SkImageInfo::MakeN32Premul(width, height, SkColorSpace::MakeSRGB());
    surface =
        SkSurfaces::RenderTarget(grContext_.get(), skgpu::Budgeted::kNo, info);
  }

  if (!surface) {
    DBG("ERROR: Failed to create SkSurface for component");
    return nullptr;
  }

  // Store in cache
  ComponentSurface &cached = surfaces_[&component];
  cached.surface = surface;
  cached.width = width;
  cached.height = height;
  cached.lastUsed = juce::Time::getCurrentTime();

  stats_.activeSurfaces = (int)surfaces_.size();

  DBG("Created SkSurface for component: "
      << width << "x" << height << " (mode: " << (int)currentMode_ << ")");

  return surface;
#else
  return nullptr;
#endif
}

void SkiaContextManager::cleanupOldSurfaces() {
  // Remove surfaces that haven't been used in 5 seconds
  auto now = juce::Time::getCurrentTime();
  auto threshold = juce::RelativeTime::seconds(5);

  std::lock_guard<std::mutex> lock(surfacesMutex_);

  for (auto it = surfaces_.begin(); it != surfaces_.end();) {
    if ((now - it->second.lastUsed) > threshold) {
      DBG("Cleaning up unused surface for component");
      it = surfaces_.erase(it);
    } else {
      ++it;
    }
  }

  stats_.activeSurfaces = (int)surfaces_.size();
}

//==============================================================================
// Component Rendering
//==============================================================================

void SkiaContextManager::renderToComponent(
    juce::Component &component, std::function<void(SkCanvas *)> drawCallback) {
#ifdef ZENITH_USE_SKIA
  if (!initialized_) {
    DBG("ERROR: SkiaContextManager not initialized!");
    return;
  }

  // Get or create surface for this component
  auto surface = getOrCreateSurface(component);
  if (!surface) {
    DBG("ERROR: Failed to get surface for component");
    return;
  }

  // Render with Skia
  SkCanvas *canvas = surface->getCanvas();

  // Clear
  canvas->clear(SK_ColorTRANSPARENT);

  // Call user drawing code
  if (drawCallback) {
    drawCallback(canvas);
  }

  // Flush GPU commands (if GPU mode)
  if (grContext_) {
    grContext_->flushAndSubmit();
  }

  // Cleanup old surfaces periodically
  auto now = juce::Time::getCurrentTime();
  if ((now - lastCleanupTime_) > juce::RelativeTime::seconds(10)) {
    cleanupOldSurfaces();
    lastCleanupTime_ = now;
  }

  stats_.frameCount++;

#else
  DBG("ERROR: ZENITH_USE_SKIA not defined!");
#endif
}

void SkiaContextManager::renderToComponent(
    juce::Graphics &g, juce::Component &component,
    std::function<void(SkCanvas *)> drawCallback) {
#ifdef ZENITH_USE_SKIA
  if (!initialized_) {
    // Fallback if Skia isn't ready
    g.fillAll(juce::Colours::red);
    return;
  }

  // 1. Get the persistent surface for this component
  auto surface = getOrCreateSurface(component);
  if (!surface)
    return;

  // 2. Draw into the Skia Surface
  SkCanvas *canvas = surface->getCanvas();

  // Important: Save state so the callback doesn't mess up future clips
  canvas->save();
  canvas->clear(SK_ColorTRANSPARENT); // Clear previous frame

  if (drawCallback)
    drawCallback(canvas);

  canvas->restore();

  // 3. Flush GPU commands if we are using hardware acceleration
  if (grContext_) {
    grContext_->flushAndSubmit();
  }

  // 4. Blit the result to the JUCE Graphics context
  blitToJUCE(g, surface.get(), component.getWidth(), component.getHeight());

  // 5. Update stats
  stats_.frameCount++;
#else
  g.fillAll(juce::Colours::grey);
#endif
}

void SkiaContextManager::blitToJUCE(juce::Graphics &g, SkSurface *surface,
                                    int width, int height) {
#ifdef ZENITH_USE_SKIA
  // Create a snapshot of the current state of the surface
  sk_sp<SkImage> skImage = surface->makeImageSnapshot();
  if (!skImage)
    return;

  // Optimization: If we are strictly software, we might peek pixels directly.
  // But generally, we need to map Skia pixels to a JUCE Image.

  // 1. Create a JUCE Image wrapper
  juce::Image juceImage(juce::Image::ARGB, width, height, true);

  // 2. Access the internal pixels of the JUCE Image
  juce::Image::BitmapData destData(juceImage,
                                   juce::Image::BitmapData::writeOnly);

  // 3. Read pixels from Skia Surface directly into JUCE Image memory
  //    Skia uses RGBA or BGRA depending on platform, JUCE usually expects
  //    ARGB/RGB. SkImageInfo specifies the format we WANT to read out as.
  SkImageInfo readInfo = SkImageInfo::Make(
      width, height,
      kBGRA_8888_SkColorType, // JUCE usually matches this on desktop
      kPremul_SkAlphaType);

  bool success = skImage->readPixels(readInfo, destData.getLinePointer(0),
                                     destData.lineStride, 0, 0);

  if (success) {
    // 4. Draw the resulting image to the JUCE context
    g.drawImageAt(juceImage, 0, 0);
  } else {
    // Fallback: If direct read fails, we might need an intermediate bitmap
    // (slower)
    DBG("SkiaContextManager: Failed to read pixels directly to JUCE image");
  }
#endif
}

juce::Image SkiaContextManager::renderToImage(
    int width, int height, std::function<void(SkCanvas *)> drawCallback) {
#ifdef ZENITH_USE_SKIA
  if (!initialized_) {
    DBG("ERROR: SkiaContextManager not initialized!");
    return juce::Image(juce::Image::ARGB, width, height, true);
  }

  // Create temporary surface
  sk_sp<SkSurface> surface;

  SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);

  if (currentMode_ == RenderMode::Software || !grContext_) {
    surface = SkSurfaces::Raster(info);
  } else {
    surface =
        SkSurfaces::RenderTarget(grContext_.get(), skgpu::Budgeted::kNo, info);
  }

  if (!surface) {
    DBG("ERROR: Failed to create temporary surface");
    return juce::Image(juce::Image::ARGB, width, height, true);
  }

  // Render
  SkCanvas *canvas = surface->getCanvas();
  canvas->clear(SK_ColorTRANSPARENT);

  if (drawCallback) {
    drawCallback(canvas);
  }

  if (grContext_) {
    grContext_->flushAndSubmit();
  }

  // Convert to JUCE Image
  sk_sp<SkImage> skImage = surface->makeImageSnapshot();

  juce::Image juceImage(juce::Image::ARGB, width, height, true);

  // Read pixels from Skia to JUCE
  SkPixmap pixmap;
  if (skImage->peekPixels(&pixmap)) {
    juce::Image::BitmapData destData(juceImage,
                                     juce::Image::BitmapData::writeOnly);

    for (int y = 0; y < height; ++y) {
      const uint8_t *src = (const uint8_t *)pixmap.addr(0, y);
      uint8_t *dst = destData.getLinePointer(y);
      memcpy(dst, src, width * 4);
    }
  }

  return juceImage;

#else
  return juce::Image(juce::Image::ARGB, width, height, true);
#endif
}

void SkiaContextManager::componentResized(juce::Component &component) {
  std::lock_guard<std::mutex> lock(surfacesMutex_);

  // Simply remove the surface - it will be recreated with new size on next
  // render
  auto it = surfaces_.find(&component);
  if (it != surfaces_.end()) {
    surfaces_.erase(it);
    stats_.activeSurfaces = (int)surfaces_.size();
  }
}

void SkiaContextManager::componentDestroyed(juce::Component &component) {
  std::lock_guard<std::mutex> lock(surfacesMutex_);

  auto it = surfaces_.find(&component);
  if (it != surfaces_.end()) {
    surfaces_.erase(it);
    stats_.activeSurfaces = (int)surfaces_.size();
    DBG("Released surface for destroyed component");
  }
}

} // namespace zenith

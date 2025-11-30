/**
 * @file SkiaRenderer.cpp
 * @brief Implementation of Skia rendering engine
 */

#include "SkiaRenderer.h"
// Force rebuild for diagnostics

// Skia headers
#include "include/core/SkCanvas.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkSurface.h"
#include "include/gpu/GpuTypes.h"
#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"

#if JUCE_WINDOWS && defined(SK_DIRECT3D) &&                                    \
    0 // Disabled: vcpkg Skia doesn't include D3D backend
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/d3d/GrD3DBackendContext.h"
#include "include/gpu/ganesh/d3d/GrD3DTypes.h"
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

// D3D12 helper structures
struct D3D12Context {
  ComPtr<ID3D12Device> device;
  ComPtr<ID3D12CommandQueue> commandQueue;
  ComPtr<IDXGISwapChain3> swapChain;
  ComPtr<ID3D12DescriptorHeap> rtvHeap;
  ComPtr<ID3D12Resource> renderTargets[2]; // Double buffering
  ComPtr<ID3D12CommandAllocator> commandAllocator;
  ComPtr<ID3D12GraphicsCommandList> commandList;
  ComPtr<ID3D12Fence> fence;
  UINT64 fenceValue = 0;
  HANDLE fenceEvent = nullptr;
  UINT frameIndex = 0;
  UINT rtvDescriptorSize = 0;
};

#elif JUCE_MAC && defined(SK_METAL) &&                                         \
    0 // Disabled: vcpkg Skia doesn't include Metal backend
#include "include/gpu/mtl/GrMtlBackendContext.h"
#include "include/gpu/mtl/GrMtlTypes.h"
#elif JUCE_LINUX && defined(SK_VULKAN) &&                                      \
    0 // Disabled: vcpkg Skia doesn't include Vulkan backend
#include "include/gpu/vk/GrVkBackendContext.h"
#include "include/gpu/vk/GrVkTypes.h"
#endif

// OpenGL backend (enabled and available in vcpkg Skia)
#ifdef SK_GL
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"
#endif

namespace zenith {

//==============================================================================
// Constructor / Destructor
//==============================================================================

SkiaRenderer::SkiaRenderer(juce::Component &component, Backend backend,
                           bool enableVSync)
    : component_(component), backend_(backend), vsyncEnabled_(enableVSync) {
  // Auto-detect best backend if requested
  if (backend_ == Backend::Auto) {
    backend_ = detectBestBackend();
  }

  DBG("SkiaRenderer created with backend: " << getBackendName(backend_));
}

SkiaRenderer::~SkiaRenderer() { shutdown(); }

//==============================================================================
// Initialization
//==============================================================================

bool SkiaRenderer::initialize() {
  if (initialized_) {
    DBG("SkiaRenderer already initialized");
    return true;
  }

  DBG("Initializing SkiaRenderer...");
  DBG("  Step 1: Creating GPU Context...");

  // Create GPU context
  if (!createGpuContext()) {
    DBG("ERROR: Failed to create GPU context");
    return false;
  }
  DBG("  Step 1: GPU Context created successfully.");

  // Create rendering surface
  auto bounds = component_.getLocalBounds();
  DBG("  Step 2: Creating Surface (" << bounds.getWidth() << "x" << bounds.getHeight() << ")...");
  
  if (!createSurface(bounds.getWidth(), bounds.getHeight())) {
    DBG("ERROR: Failed to create surface");
    shutdown();
    return false;
  }
  DBG("  Step 2: Surface created successfully.");

  initialized_ = true;
  lastFrameTime_ = juce::Time::getCurrentTime();

  DBG("SkiaRenderer initialized successfully!");
  DBG("  Size: " << bounds.getWidth() << "x" << bounds.getHeight());
  DBG("  Backend: " << getBackendName(backend_));
  DBG("  VSync: " << (vsyncEnabled_ ? "enabled" : "disabled"));

  return true;
}

void SkiaRenderer::shutdown() {
  if (!initialized_)
    return;

  DBG("Shutting down SkiaRenderer...");

  // Release Skia resources in correct order
  surface_.reset();
  grContext_.reset();

  // Platform-specific cleanup
#if JUCE_WINDOWS && defined(SK_DIRECT3D) &&                                    \
    0 // Disabled: vcpkg Skia doesn't include D3D backend
  if (platformHandle_) {
    auto d3dCtx = static_cast<D3D12Context *>(platformHandle_);

    // Wait for GPU to finish
    if (d3dCtx->fence && d3dCtx->commandQueue) {
      d3dCtx->commandQueue->Signal(d3dCtx->fence.Get(), d3dCtx->fenceValue);
      d3dCtx->fence->SetEventOnCompletion(d3dCtx->fenceValue,
                                          d3dCtx->fenceEvent);
      WaitForSingleObject(d3dCtx->fenceEvent, INFINITE);
    }

    // Close fence event
    if (d3dCtx->fenceEvent) {
      CloseHandle(d3dCtx->fenceEvent);
    }

    // COM objects will auto-release
    delete d3dCtx;
    platformHandle_ = nullptr;

    DBG("D3D12 resources released");
  }
#endif

  initialized_ = false;
  DBG("SkiaRenderer shutdown complete");
}

//==============================================================================
// Rendering
//==============================================================================

void SkiaRenderer::render(std::function<void(SkCanvas *)> drawCallback) {
  if (!initialized_ || !surface_) {
    DBG("ERROR: Cannot render - not initialized");
    return;
  }

  auto startTime = juce::Time::getCurrentTime();

  // Get canvas from surface
  SkCanvas *canvas = surface_->getCanvas();

  // Clear canvas
  // DIAGNOSTIC: Change clear color to MAGENTA to verify renderer is working
  canvas->clear(SkColorSetRGB(255, 0, 255));

  // Execute user drawing code
  if (drawCallback) {
    drawCallback(canvas);
  }

  // Flush to GPU
  if (grContext_) {
    grContext_->flushAndSubmit();
  }

// Platform-specific present
#if JUCE_WINDOWS && defined(SK_DIRECT3D) &&                                    \
    0 // Disabled: vcpkg Skia doesn't include D3D backend
  if (platformHandle_ && backend_ == Backend::Direct3D) {
    auto d3dCtx = static_cast<D3D12Context *>(platformHandle_);

    // Present swap chain
    UINT syncInterval = vsyncEnabled_ ? 1 : 0;
    UINT presentFlags =
        (vsyncEnabled_ || targetFPS_ <= 60) ? 0 : DXGI_PRESENT_ALLOW_TEARING;

    d3dCtx->swapChain->Present(syncInterval, presentFlags);

    // Wait for frame to finish
    const UINT64 currentFenceValue = d3dCtx->fenceValue;
    d3dCtx->commandQueue->Signal(d3dCtx->fence.Get(), currentFenceValue);

    // Update frame index
    d3dCtx->frameIndex = d3dCtx->swapChain->GetCurrentBackBufferIndex();

    // If next frame is not ready, wait
    if (d3dCtx->fence->GetCompletedValue() < d3dCtx->fenceValue) {
      d3dCtx->fence->SetEventOnCompletion(d3dCtx->fenceValue,
                                          d3dCtx->fenceEvent);
      WaitForSingleObject(d3dCtx->fenceEvent, INFINITE);
    }

    d3dCtx->fenceValue = currentFenceValue + 1;
  }
#endif

  // Update statistics
  updateStats();

  // VSync throttling
  if (vsyncEnabled_) {
    auto frameTime = juce::Time::getCurrentTime() - startTime;
    auto targetFrameTime =
        juce::RelativeTime::milliseconds(static_cast<int>(1000.0 / targetFPS_));

    if (frameTime < targetFrameTime) {
      auto sleepTime = targetFrameTime - frameTime;
      juce::Thread::sleep((int)sleepTime.inMilliseconds());
    }
  }
}

void SkiaRenderer::resize(int width, int height) {
  if (!initialized_)
    return;

  DBG("SkiaRenderer resizing to: " << width << "x" << height);

  // Recreate surface with new size
  createSurface(width, height);
}

//==============================================================================
// Configuration
//==============================================================================

void SkiaRenderer::setTargetFPS(int fps) {
  if (fps < 1 || fps > 300) {
    DBG("WARNING: Invalid target FPS: " << fps << ", clamping to 60");
    fps = 60;
  }

  targetFPS_ = fps;
  DBG("Target FPS set to: " << targetFPS_);
}

void SkiaRenderer::setVSyncEnabled(bool enable) {
  vsyncEnabled_ = enable;
  DBG("VSync " << (enable ? "enabled" : "disabled"));
}

const char *SkiaRenderer::getBackendName(Backend backend) {
  switch (backend) {
  case Backend::Auto:
    return "Auto";
  case Backend::Direct3D:
    return "Direct3D";
  case Backend::Metal:
    return "Metal";
  case Backend::Vulkan:
    return "Vulkan";
  case Backend::OpenGL:
    return "OpenGL";
  case Backend::Software:
    return "Software";
  default:
    return "Unknown";
  }
}

//==============================================================================
// Internal Methods
//==============================================================================

bool SkiaRenderer::createGpuContext() {
#if JUCE_WINDOWS && defined(SK_DIRECT3D) &&                                    \
    0 // Disabled: vcpkg Skia doesn't include D3D backend
  if (backend_ == Backend::Direct3D) {
    return createD3DContext();
  }
#elif JUCE_MAC && defined(SK_METAL) &&                                         \
    0 // Disabled: vcpkg Skia doesn't include Metal backend
  if (backend_ == Backend::Metal) {
    return createMetalContext();
  }
#elif JUCE_LINUX && defined(SK_VULKAN) &&                                      \
    0 // Disabled: vcpkg Skia doesn't include Vulkan backend
  if (backend_ == Backend::Vulkan) {
    return createVulkanContext();
  }
#endif

  // Try OpenGL backend (available in vcpkg Skia)
#ifdef SK_GL
  if (backend_ == Backend::OpenGL || backend_ == Backend::Auto) {
    DBG("Attempting to create OpenGL GPU context...");

    // Create OpenGL interface from current context
    // Note: This requires an active OpenGL context to be current
    sk_sp<const GrGLInterface> glInterface = GrGLMakeNativeInterface();

    if (!glInterface) {
      DBG("WARNING: Failed to create GL interface (no active OpenGL context)");
      DBG("Falling back to software rendering");
      backend_ = Backend::Software;
      return true;
    }

    // Create Skia GPU context from OpenGL interface
    grContext_ = GrDirectContexts::MakeGL(glInterface);

    if (!grContext_) {
      DBG("ERROR: Failed to create Skia GPU context from OpenGL");
      DBG("Falling back to software rendering");
      backend_ = Backend::Software;
      return true;
    }

    backend_ = Backend::OpenGL;
    DBG("OpenGL GPU context created successfully!");
    DBG("  GPU acceleration enabled");
    return true;
  }
#endif

  // Fallback to software rendering
  DBG("Using software rendering (CPU-based, no GPU acceleration)");
  backend_ = Backend::Software;

  //  Software rendering doesn't need GPU context
  return true;
}

bool SkiaRenderer::createSurface(int width, int height) {
  if (width <= 0 || height <= 0) {
    DBG("ERROR: Invalid surface size: " << width << "x" << height);
    return false;
  }

  // Release old surface
  surface_.reset();

  if (backend_ == Backend::Software) {
    // Create CPU-backed surface
    SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
    surface_ = SkSurfaces::Raster(info);
  } else {
    // Create GPU-backed surface
    if (!grContext_) {
      DBG("ERROR: Cannot create GPU surface without context");
      return false;
    }

    SkImageInfo info =
        SkImageInfo::MakeN32Premul(width, height, SkColorSpace::MakeSRGB());

    surface_ =
        SkSurfaces::RenderTarget(grContext_.get(), skgpu::Budgeted::kNo, info);
  }

  if (!surface_) {
    DBG("ERROR: Failed to create Skia surface");
    return false;
  }

  DBG("Skia surface created: " << width << "x" << height);
  return true;
}

void SkiaRenderer::updateStats() {
  auto currentTime = juce::Time::getCurrentTime();
  auto frameTime = (currentTime - lastFrameTime_).inMilliseconds();
  lastFrameTime_ = currentTime;

  stats_.frameTime = frameTime;

  // Calculate average (60 frame window)
  frameTimes_.push_back(frameTime);
  if (frameTimes_.size() > 60)
    frameTimes_.erase(frameTimes_.begin());

  double sum = 0.0;
  for (const auto &t : frameTimes_)
    sum += t;
  stats_.averageFrameTime = sum / frameTimes_.size();

  // Calculate FPS
  if (stats_.averageFrameTime > 0.0)
    stats_.fps = (int)(1000.0 / stats_.averageFrameTime);

  // Count dropped frames (frames that took longer than target)
  double targetFrameTime = 1000.0 / targetFPS_;
  if (frameTime > targetFrameTime * 1.5) // 50% over target
    stats_.droppedFrames++;
}

SkiaRenderer::Backend SkiaRenderer::detectBestBackend() const {
  // OpenGL backend is available in vcpkg Skia on all platforms
  // Use it for GPU acceleration
#ifdef SK_GL
  return Backend::OpenGL; // OpenGL available in vcpkg Skia build
#else
  return Backend::Software; // Fallback if GL not enabled
#endif
}

//==============================================================================
// Platform-Specific Implementations
//==============================================================================

#if JUCE_WINDOWS && defined(SK_DIRECT3D) &&                                    \
    0 // Disabled: vcpkg Skia doesn't include D3D backend

bool SkiaRenderer::createD3DContext() {
  DBG("Creating Direct3D 12 context...");

  // Allocate D3D12 context
  auto d3dCtx = std::make_unique<D3D12Context>();

  HRESULT hr;

// Enable debug layer in debug builds
#ifdef DEBUG
  {
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
      debugController->EnableDebugLayer();
      DBG("D3D12 debug layer enabled");
    }
  }
#endif

  // 1. Create DXGI Factory
  ComPtr<IDXGIFactory4> factory;
  hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
  if (FAILED(hr)) {
    DBG("ERROR: Failed to create DXGI factory");
    return false;
  }

  // 2. Create D3D12 Device
  ComPtr<IDXGIAdapter1> adapter;

  // Try to get hardware adapter
  for (UINT adapterIndex = 0;
       SUCCEEDED(factory->EnumAdapters1(adapterIndex, &adapter));
       ++adapterIndex) {
    DXGI_ADAPTER_DESC1 desc;
    adapter->GetDesc1(&desc);

    // Skip software adapter
    if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
      continue;

    // Try to create device
    hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0,
                           IID_PPV_ARGS(&d3dCtx->device));
    if (SUCCEEDED(hr)) {
      DBG("D3D12 device created with adapter: "
          << juce::String(desc.Description));
      break;
    }
  }

  if (!d3dCtx->device) {
    DBG("ERROR: Failed to create D3D12 device");
    return false;
  }

  // 3. Create Command Queue
  D3D12_COMMAND_QUEUE_DESC queueDesc = {};
  queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

  hr = d3dCtx->device->CreateCommandQueue(&queueDesc,
                                          IID_PPV_ARGS(&d3dCtx->commandQueue));
  if (FAILED(hr)) {
    DBG("ERROR: Failed to create command queue");
    return false;
  }

  // 4. Create Swap Chain
  auto hwnd = (HWND)component_.getWindowHandle();
  if (!hwnd) {
    DBG("ERROR: No window handle available");
    return false;
  }

  DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
  swapChainDesc.Width = component_.getWidth();
  swapChainDesc.Height = component_.getHeight();
  swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapChainDesc.Stereo = FALSE;
  swapChainDesc.SampleDesc.Count = 1;
  swapChainDesc.SampleDesc.Quality = 0;
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.BufferCount = 2; // Double buffering
  swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
  swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
  swapChainDesc.Flags = vsyncEnabled_ ? 0 : DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

  ComPtr<IDXGISwapChain1> swapChain1;
  hr = factory->CreateSwapChainForHwnd(d3dCtx->commandQueue.Get(), hwnd,
                                       &swapChainDesc, nullptr, nullptr,
                                       &swapChain1);

  if (FAILED(hr)) {
    DBG("ERROR: Failed to create swap chain");
    return false;
  }

  hr = swapChain1.As(&d3dCtx->swapChain);
  if (FAILED(hr)) {
    DBG("ERROR: Failed to get IDXGISwapChain3 interface");
    return false;
  }

  d3dCtx->frameIndex = d3dCtx->swapChain->GetCurrentBackBufferIndex();

  // 5. Create RTV Descriptor Heap
  D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
  rtvHeapDesc.NumDescriptors = 2;
  rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

  hr = d3dCtx->device->CreateDescriptorHeap(&rtvHeapDesc,
                                            IID_PPV_ARGS(&d3dCtx->rtvHeap));
  if (FAILED(hr)) {
    DBG("ERROR: Failed to create RTV heap");
    return false;
  }

  d3dCtx->rtvDescriptorSize = d3dCtx->device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  // 6. Create Render Target Views
  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(
      d3dCtx->rtvHeap->GetCPUDescriptorHandleForHeapStart());

  for (UINT i = 0; i < 2; i++) {
    hr = d3dCtx->swapChain->GetBuffer(i,
                                      IID_PPV_ARGS(&d3dCtx->renderTargets[i]));
    if (FAILED(hr)) {
      DBG("ERROR: Failed to get swap chain buffer " << i);
      return false;
    }

    d3dCtx->device->CreateRenderTargetView(d3dCtx->renderTargets[i].Get(),
                                           nullptr, rtvHandle);
    rtvHandle.ptr += d3dCtx->rtvDescriptorSize;
  }

  // 7. Create Command Allocator
  hr = d3dCtx->device->CreateCommandAllocator(
      D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&d3dCtx->commandAllocator));

  if (FAILED(hr)) {
    DBG("ERROR: Failed to create command allocator");
    return false;
  }

  // 8. Create Command List
  hr = d3dCtx->device->CreateCommandList(
      0, D3D12_COMMAND_LIST_TYPE_DIRECT, d3dCtx->commandAllocator.Get(),
      nullptr, IID_PPV_ARGS(&d3dCtx->commandList));

  if (FAILED(hr)) {
    DBG("ERROR: Failed to create command list");
    return false;
  }

  d3dCtx->commandList->Close(); // Start closed

  // 9. Create Fence for synchronization
  hr = d3dCtx->device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                   IID_PPV_ARGS(&d3dCtx->fence));
  if (FAILED(hr)) {
    DBG("ERROR: Failed to create fence");
    return false;
  }

  d3dCtx->fenceValue = 1;

  d3dCtx->fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if (!d3dCtx->fenceEvent) {
    DBG("ERROR: Failed to create fence event");
    return false;
  }

  // 10. Create Skia GrDirectContext
  GrD3DBackendContext backendContext;
  backendContext.fAdapter.retain(adapter.Get());
  backendContext.fDevice.retain(d3dCtx->device.Get());
  backendContext.fQueue.retain(d3dCtx->commandQueue.Get());

  grContext_ = GrDirectContext::MakeDirect3D(backendContext);

  if (!grContext_) {
    DBG("ERROR: Failed to create Skia GrDirectContext");
    return false;
  }

  // Store D3D context
  platformHandle_ = d3dCtx.release();

  DBG("D3D12 context created successfully!");
  DBG("  Device: Yes");
  DBG("  Swap Chain: Yes (" << swapChainDesc.Width << "x"
                            << swapChainDesc.Height << ")");
  DBG("  Skia Context: Yes");

  return true;
}

#elif JUCE_MAC

bool SkiaRenderer::createMetalContext() {
  DBG("Creating Metal context...");

  // TODO(zenith-core#1): Metal initialization
  DBG("WARNING: Metal backend not yet implemented, using software rendering");
  backend_ = Backend::Software;
  return true;
}

#elif JUCE_LINUX

bool SkiaRenderer::createVulkanContext() {
  DBG("Creating Vulkan context...");

  // TODO(zenith-core#1): Vulkan initialization
  DBG("WARNING: Vulkan backend not yet implemented, using software rendering");
  backend_ = Backend::Software;
  return true;
}

#endif

} // namespace zenith

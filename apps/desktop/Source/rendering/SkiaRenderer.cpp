/**
 * @file SkiaRenderer.cpp
 * @brief Implementation of Skia rendering engine
 */

#include "SkiaRenderer.h"

// Skia headers
#include <core/SkCanvas.h>
#include <core/SkColorSpace.h>
#include <core/SkSurface.h>
#include <gpu/GpuTypes.h>
#include <gpu/ganesh/GrBackendSurface.h>
#include <gpu/ganesh/GrDirectContext.h>
#include <gpu/ganesh/SkSurfaceGanesh.h>

// Platform-specific headers
#if JUCE_MAC
#define SK_METAL 1
#include <dlfcn.h>
#include <gpu/ganesh/mtl/GrMtlBackendContext.h>
#include <gpu/ganesh/mtl/GrMtlTypes.h>
#include <objc/message.h>
#include <objc/runtime.h>
#elif JUCE_LINUX
#define SK_VULKAN 1
#include <gpu/ganesh/vk/GrVkBackendContext.h>
#include <gpu/ganesh/vk/GrVkTypes.h>
#include <vulkan/vulkan.h>
#elif JUCE_WINDOWS
#include <d3d12.h>
#include <dxgi1_4.h>
#include <gpu/ganesh/d3d/GrD3DBackendContext.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;
#endif

// OpenGL backend (always available as fallback)
#include <gpu/ganesh/gl/GrGLDirectContext.h>
#include <gpu/ganesh/gl/GrGLInterface.h>

namespace zenith {

//==============================================================================
// Constructor / Destructor
//==============================================================================

SkiaRenderer::SkiaRenderer(juce::Component &component, Backend backend,
                           bool enableVSync)
    : component_(component), backend_(backend), vsyncEnabled_(enableVSync) {
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
  if (initialized_)
    return true;

  DBG("Initializing SkiaRenderer...");

  bool contextCreated = createGpuContext();

  // Fallback chain if primary backend fails
  if (!contextCreated) {
    DBG("ERROR: Failed to create GPU context, trying fallback...");

    if (backend_ != Backend::OpenGL) {
      backend_ = Backend::OpenGL;
      contextCreated = createGpuContext();
    }

    if (!contextCreated) {
      backend_ = Backend::Software;
      // Software doesn't need GPU context
    }
  }

  auto bounds = component_.getLocalBounds();
  if (!createSurface(bounds.getWidth(), bounds.getHeight())) {
    DBG("ERROR: Failed to create surface");
    shutdown();
    return false;
  }

  initialized_ = true;
  lastFrameTime_ = juce::Time::getCurrentTime();
  return true;
}

void SkiaRenderer::shutdown() {
  if (!initialized_)
    return;

  surface_.reset();
  grContext_.reset();

  initialized_ = false;
}

//==============================================================================
// Rendering
//==============================================================================

void SkiaRenderer::render(std::function<void(SkCanvas *)> drawCallback) {
  if (!initialized_ || !surface_)
    return;

  auto startTime = juce::Time::getCurrentTime();
  SkCanvas *canvas = surface_->getCanvas();
  canvas->clear(SK_ColorBLACK);

  if (drawCallback)
    drawCallback(canvas);

  if (grContext_)
    grContext_->flushAndSubmit();

#if JUCE_MAC && defined(SK_METAL)
  // Metal present handled by layer automatically (usually)
#endif

  updateStats();

  if (vsyncEnabled_) {
    auto frameTime = juce::Time::getCurrentTime() - startTime;
    auto targetFrameTime =
        juce::RelativeTime::milliseconds((juce::int64)(1000.0 / targetFPS_));
    if (frameTime < targetFrameTime)
      juce::Thread::sleep((int)(targetFrameTime - frameTime).inMilliseconds());
  }
}

void SkiaRenderer::resize(int width, int height) {
  if (initialized_)
    createSurface(width, height);
}

//==============================================================================
// Backend Selection
//==============================================================================

SkiaRenderer::Backend SkiaRenderer::detectBestBackend() const {
#if JUCE_WINDOWS
  return Backend::Direct3D; // Prefer D3D12 on Windows now
#elif JUCE_MAC
  return Backend::Metal;
#elif JUCE_LINUX
  return Backend::Vulkan;
#else
  return Backend::OpenGL;
#endif
}

const char *SkiaRenderer::getBackendName(Backend backend) {
  switch (backend) {
  case Backend::Direct3D:
    return "Direct3D 12";
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

void SkiaRenderer::setTargetFPS(int fps) {
  targetFPS_ = juce::jlimit(1, 300, fps);
}
void SkiaRenderer::setVSyncEnabled(bool enable) { vsyncEnabled_ = enable; }

//==============================================================================
// GPU Context Creation
//==============================================================================

bool SkiaRenderer::createGpuContext() {
  switch (backend_) {
#if JUCE_MAC
  case Backend::Metal:
    return createMetalContext();
#endif
#if JUCE_LINUX
  case Backend::Vulkan:
    return createVulkanContext();
#endif
  case Backend::Direct3D:
#if JUCE_WINDOWS
    return createD3DContext();
#else
    return false;
#endif
  case Backend::OpenGL: {
    auto glInterface = GrGLMakeNativeInterface();
    if (!glInterface)
      return false;
    grContext_ = GrDirectContexts::MakeGL(glInterface);
    return grContext_ != nullptr;
  }
  case Backend::Software:
    return true;
  default:
    return false;
  }
}

bool SkiaRenderer::createSurface(int width, int height) {
  if (width <= 0 || height <= 0)
    return false;
  surface_.reset();

  if (backend_ == Backend::Software) {
    SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
    surface_ = SkSurfaces::Raster(info);
  } else {
    if (!grContext_)
      return false;

    SkImageInfo info =
        SkImageInfo::MakeN32Premul(width, height, SkColorSpace::MakeSRGB());

    // Generic GPU surface (offscreen)
    surface_ =
        SkSurfaces::RenderTarget(grContext_.get(), skgpu::Budgeted::kNo, info);
  }
  return surface_ != nullptr;
}

void SkiaRenderer::updateStats() {
  // Simple stats update
  stats_.frameTime =
      (juce::Time::getCurrentTime() - lastFrameTime_).inMilliseconds();
  lastFrameTime_ = juce::Time::getCurrentTime();
}

//==============================================================================
// Platform Implementations
//==============================================================================

#if JUCE_MAC && defined(SK_METAL)
bool SkiaRenderer::createMetalContext() {
  // Pure C++ Metal Initialization via Obj-C Runtime
  // Avoids needing .mm files

  // id<MTLDevice> device = MTLCreateSystemDefaultDevice();
  void *device =
      ((void *(*)())dlsym(RTLD_DEFAULT, "MTLCreateSystemDefaultDevice"))();
  if (!device)
    return false;

  // id<MTLCommandQueue> queue = [device newCommandQueue];
  void *queue = nullptr;

  // objc_msgSend(device, @selector(newCommandQueue))
  typedef void *(*SendMsgFn)(void *, void *);
  SendMsgFn sendMsg = (SendMsgFn)objc_msgSend;
  SEL newCommandQueueSel = sel_registerName("newCommandQueue");
  queue = sendMsg(device, newCommandQueueSel);

  if (!queue)
    return false;

  GrMtlBackendContext backendContext;
  backendContext.fDevice.retain(device);
  backendContext.fQueue.retain(queue);

  grContext_ = GrDirectContext::MakeMetal(backendContext);
  return grContext_ != nullptr;
}
#endif

#if JUCE_LINUX && defined(SK_VULKAN)
bool SkiaRenderer::createVulkanContext() {
  // Minimal Vulkan Instance
  VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
  appInfo.pApplicationName = "ZenithDAW";
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  createInfo.pApplicationInfo = &appInfo;

  VkInstance instance;
  if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
    return false;

  // Pick physical device (first one)
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
  if (deviceCount == 0)
    return false;

  VkPhysicalDevice physicalDevice;
  vkEnumeratePhysicalDevices(instance, &deviceCount, &physicalDevice);

  // Create logical device
  float queuePriority = 1.0f;
  VkDeviceQueueCreateInfo queueCreateInfo = {
      VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
  queueCreateInfo.queueFamilyIndex =
      0; // Assuming graphics queue at 0 for simplicity
  queueCreateInfo.queueCount = 1;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  VkDeviceCreateInfo deviceInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  deviceInfo.queueCreateInfoCount = 1;
  deviceInfo.pQueueCreateInfos = &queueCreateInfo;

  VkDevice device;
  if (vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device) !=
      VK_SUCCESS)
    return false;

  VkQueue queue;
  vkGetDeviceQueue(device, 0, 0, &queue);

  skgpu::VulkanBackendContext backendContext;
  backendContext.fInstance = instance;
  backendContext.fPhysicalDevice = physicalDevice;
  backendContext.fDevice = device;
  backendContext.fQueue = queue;
  backendContext.fGraphicsQueueIndex = 0;
  backendContext.fGetProc = [](const char *name, VkInstance i, VkDevice d) {
    if (d)
      return vkGetDeviceProcAddr(d, name);
    return vkGetInstanceProcAddr(i, name);
  };

  grContext_ = GrDirectContext::MakeVulkan(backendContext);
  return grContext_ != nullptr;
}
#endif

#if JUCE_WINDOWS
bool SkiaRenderer::createD3DContext()
{
    DBG("SkiaRenderer: Creating D3D12 context...");

    // 1. Enable Debug Layer (Debug builds only)
#if JUCE_DEBUG
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
    }
#endif

    // 2. Create Factory
    ComPtr<IDXGIFactory4> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
        DBG("SkiaRenderer: Failed to create DXGI factory");
        return false;
    }

    // 3. Create Device
    ComPtr<ID3D12Device> device;
    if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)))) {
        DBG("SkiaRenderer: Failed to create D3D12 device");
        return false;
    }

    // 4. Create Command Queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    
    ComPtr<ID3D12CommandQueue> queue;
    if (FAILED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue)))) {
        DBG("SkiaRenderer: Failed to create command queue");
        return false;
    }

    // 5. Create Skia Context
    GrD3DBackendContext backendContext;
    backendContext.fAdapter = nullptr; // Skia will query if needed
    backendContext.fDevice = device;
    backendContext.fQueue = queue;
    
    grContext_ = GrDirectContext::MakeDirect3D(backendContext);
    
    if (grContext_) {
        DBG("SkiaRenderer: D3D12 context created successfully");
        return true;
    }
    
    DBG("SkiaRenderer: Failed to create GrDirectContext from D3D12 device");
    return false;
}
#endif

} // namespace zenith
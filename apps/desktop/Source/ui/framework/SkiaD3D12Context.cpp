/*
  ==============================================================================

    SkiaD3D12Context.cpp
    Created: 2026-01-03
    Author:  Zenith DAW Team

  ==============================================================================
*/

#include "SkiaD3D12Context.h"

#if JUCE_WINDOWS

#include <include/gpu/ganesh/d3d/GrD3DBackendContext.h>
#include <include/core/SkColorSpace.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dcomp.lib")

namespace zenith {

using Microsoft::WRL::ComPtr;

SkiaD3D12Context::SkiaD3D12Context() {
}

SkiaD3D12Context::~SkiaD3D12Context() {
    waitForGpu(); // Ensure GPU is done before destroying resources
    
    // Release Skia objects first
    for (int i = 0; i < kBufferCount; ++i) {
        skiaSurfaces_[i].reset();
    }
    skiaContext_.reset();
    
    // Cleanup simple handles
    if (fenceEvent_) {
        CloseHandle(fenceEvent_);
    }
}

bool SkiaD3D12Context::initialize(void* hwnd, int width, int height) {
    if (initialized_) return true;

    width_ = width;
    height_ = height;

    if (!createDeviceAndQueue()) return false;
    if (!createSwapChain(hwnd)) return false;
    if (!createRtvHeap()) return false;
    if (!createCommandAllocators()) return false;
    if (!createCommandList()) return false;
    if (!createFence()) return false;
    
    if (!initSkia()) {
        DBG("SkiaD3D12Context: Failed to initialize Skia D3D12 backend");
        return false;
    }

    createSurfaces();

    initialized_ = true;
    DBG("SkiaD3D12Context: D3D12 Initialized successfully");
    return true;
}

void SkiaD3D12Context::resize(int width, int height) {
    if (!initialized_ || (width == width_ && height == height_)) return;
    if (width <= 0 || height <= 0) return;

    waitForGpu();

    // Release references to back buffers before resizing
    for (int i = 0; i < kBufferCount; ++i) {
        skiaSurfaces_[i].reset();
        backBuffers_[i].Reset();
    }
    
    // flush context to ensure all gpu resources are released
    if (skiaContext_) {
        skiaContext_->flushAndSubmit();
        // We might want to free gpu resources more aggressively here?
        skiaContext_->freeGpuResources();
    }

    width_ = width;
    height_ = height;

    DXGI_SWAP_CHAIN_DESC desc = {};
    swapChain_->GetDesc(&desc);
    
    HRESULT hr = swapChain_->ResizeBuffers(kBufferCount, width, height, desc.BufferDesc.Format, desc.Flags);
    if (FAILED(hr)) {
        DBG("SkiaD3D12Context: ResizeBuffers failed: " + juce::String::toHexString((int)hr));
        return;
    }

    frameIndex_ = swapChain_->GetCurrentBackBufferIndex();
    
    createSurfaces();
}

SkCanvas* SkiaD3D12Context::beginFrame() {
    if (!initialized_) return nullptr;

    // Wait for the next frame's command allocator to be free
    // This is a simplified synchronization model. 
    // Usually we would wait for the fence associated with THIS frame index.
    
    // Current frame index from SwapChain
    frameIndex_ = swapChain_->GetCurrentBackBufferIndex();
    
    // Wait for this frame resource to be available
    if (fence_->GetCompletedValue() < fenceValues_[frameIndex_]) {
        HRESULT hr = fence_->SetEventOnCompletion(fenceValues_[frameIndex_], fenceEvent_);
        if (SUCCEEDED(hr)) {
            WaitForSingleObjectEx(fenceEvent_, INFINITE, FALSE);
        }
    }

    // Reset command allocator for this frame
    HRESULT hr = commandAllocators_[frameIndex_]->Reset();
    if (FAILED(hr)) return nullptr;

    // Reset command list
    hr = commandList_->Reset(commandAllocators_[frameIndex_].Get(), nullptr);
    if (FAILED(hr)) return nullptr;

    // Transition back buffer to RENDER_TARGET state?
    // Skia handles resource transitions internally usually, but best practice dictates we might need to handle the initial state.
    // However, when wrapping the backend texture in Skia, Skia usually assumes it can manage it.
    
    // IMPORTANT: When wrapping a D3D12 texture in Skia, we need to make sure Skia knows it's wrapped.
    
    return skiaSurfaces_[frameIndex_] ? skiaSurfaces_[frameIndex_]->getCanvas() : nullptr;
}

void SkiaD3D12Context::endFrame() {
    if (!initialized_ || !skiaContext_) return;

    // Flush Skia commands to the Command List
    // We must pass the current GrD3DBackendState
    skiaContext_->flush();
    skiaContext_->submit(true); // Sync CPU? No. boolean is 'syncCpu'

    // NOTE: Skia records commands into our commandList_. We must execute it.
    
    // Transition backbuffer to PRESENT state
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = backBuffers_[frameIndex_].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    
    // Skia likely left it in RenderTarget state.
    // If Skia managed the transition, we might double transition?
    // Let's rely on manual transitions if needed, but Skia often handles this if we tell it the current state.
    // Use raw D3D measure:
    commandList_->ResourceBarrier(1, &barrier);

    HRESULT hr = commandList_->Close();
    if (SUCCEEDED(hr)) {
        ID3D12CommandList* ppCommandLists[] = { commandList_.Get() };
        queue_->ExecuteCommandLists(1, ppCommandLists);
    }

    // Present
    swapChain_->Present(1, 0); // VSync = 1

    // Move to next frame fence
    moveToNextFrame();
}

bool SkiaD3D12Context::createDeviceAndQueue() {
    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif

    ComPtr<IDXGIFactory4> factory;
    if (FAILED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)))) return false;

    ComPtr<IDXGIAdapter1> hardwareAdapter;
    // Find adapter... just pick first usually
    for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterIndex, &hardwareAdapter); ++adapterIndex) {
        DXGI_ADAPTER_DESC1 desc;
        hardwareAdapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
        if (SUCCEEDED(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) {
            break;
        }
    }
    
    if (hardwareAdapter == nullptr) {
        // Fallback to WARP?
        factory->EnumWarpAdapter(IID_PPV_ARGS(&hardwareAdapter));
    }

    if (FAILED(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device_)))) return false;

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    if (FAILED(device_->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue_)))) return false;

    return true;
}

bool SkiaD3D12Context::createSwapChain(void* hwnd) {
    ComPtr<IDXGIFactory4> factory;
    CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = kBufferCount;
    swapChainDesc.Width = width_;
    swapChainDesc.Height = height_;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain1;
    if (FAILED(factory->CreateSwapChainForHwnd(
        queue_.Get(),
        (HWND)hwnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain1
    ))) return false;

    swapChain1.As(&swapChain_);
    frameIndex_ = swapChain_->GetCurrentBackBufferIndex();
    return true;
}

bool SkiaD3D12Context::createRtvHeap() {
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = kBufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // Not shader visible
    if (FAILED(device_->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap_)))) return false;

    rtvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    return true;
}

bool SkiaD3D12Context::createCommandAllocators() {
    for (int i = 0; i < kBufferCount; ++i) {
        if (FAILED(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators_[i]))))
            return false;
    }
    return true;
}

bool SkiaD3D12Context::createCommandList() {
    if (FAILED(device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators_[frameIndex_].Get(), nullptr, IID_PPV_ARGS(&commandList_))))
        return false;
    
    // Command lists are created in the recording state, but our update loop calls Reset().
    // We should close it first or leave it open?
    // Skia needs an open command list.
    // Our beginFrame() calls Reset(), effectively opening it.
    // So here we can just close it to start fresh state.
    commandList_->Close();
    return true;
}

bool SkiaD3D12Context::createFence() {
    if (FAILED(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)))) return false;
    fenceValues_[0] = 0;
    fenceValues_[1] = 0;
    
    fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    return fenceEvent_ != nullptr;
}

bool SkiaD3D12Context::initSkia() {
    GrD3DBackendContext backendContext;
    backendContext.fAdapter = nullptr; // Skia uses this?
    backendContext.fDevice = device_;
    backendContext.fQueue = queue_;
    backendContext.fProtectedContext = false;

    skiaContext_ = GrDirectContexts::MakeDirect3D(backendContext);
    return skiaContext_ != nullptr;
}

void SkiaD3D12Context::createSurfaces() {
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < kBufferCount; i++) {
        if (FAILED(swapChain_->GetBuffer(i, IID_PPV_ARGS(&backBuffers_[i])))) continue;

        device_->CreateRenderTargetView(backBuffers_[i].Get(), nullptr, rtvHandle);

        // Skia Wrap
        GrD3DTextureResourceInfo info(
            backBuffers_[i].Get(),
            nullptr, // resource state tracker
            D3D12_RESOURCE_STATE_PRESENT, 
            DXGI_FORMAT_R8G8B8A8_UNORM,
            1, // samples
            1, // levels
            0 // sample quality
        );
        
        // IMPORTANT: We need to set the state to RENDER_TARGET if we transition it there?
        // Or tell Skia it's currently PRESENT.
        // If we tell Skia it's PRESENT, Skia will transition it to RENDER_TARGET for drawing.

        auto renderTarget = GrBackendRenderTargets::MakeD3D(width_, height_, info);
        
        skiaSurfaces_[i] = SkSurfaces::WrapBackendRenderTarget(
            skiaContext_.get(),
            renderTarget,
            kTopLeft_GrSurfaceOrigin,
            kRGBA_8888_SkColorType,
            nullptr,
            nullptr
        );

        rtvHandle.ptr += rtvDescriptorSize_;
    }
}

void SkiaD3D12Context::waitForGpu() {
    if (!queue_ || !fence_) return;

    // Signal fence
    if (FAILED(queue_->Signal(fence_.Get(), fenceValues_[frameIndex_]))) return;

    if (FAILED(fence_->SetEventOnCompletion(fenceValues_[frameIndex_], fenceEvent_))) return;
    WaitForSingleObjectEx(fenceEvent_, INFINITE, FALSE);
    
    // Increment all fence values just in case
    fenceValues_[frameIndex_]++;
}

void SkiaD3D12Context::moveToNextFrame() {
    const UINT64 currentFenceValue = fenceValues_[frameIndex_];
    queue_->Signal(fence_.Get(), currentFenceValue);

    // Update frame index
    frameIndex_ = swapChain_->GetCurrentBackBufferIndex();

    // Check if new frame is ready
    if (fence_->GetCompletedValue() < fenceValues_[frameIndex_]) {
         HRESULT hr = fence_->SetEventOnCompletion(fenceValues_[frameIndex_], fenceEvent_);
         if (SUCCEEDED(hr)) {
             WaitForSingleObjectEx(fenceEvent_, INFINITE, FALSE);
         }
    }

    // Set next fence value
    fenceValues_[frameIndex_] = currentFenceValue + 1;
}

} // namespace zenith

#endif // JUCE_WINDOWS

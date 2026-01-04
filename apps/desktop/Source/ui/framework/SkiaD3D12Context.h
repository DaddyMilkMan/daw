/*
  ==============================================================================

    SkiaD3D12Context.h
    Created: 2026-01-03
    Author:  Zenith DAW Team

    Manages the Direct3D 12 Device, Command Queue, Swap Chain, and Skia Context.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

#if JUCE_WINDOWS

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <vector>
#include <memory>

// Skia Includes
#include <include/core/SkSurface.h>
#include <include/gpu/ganesh/GrDirectContext.h>
#include <include/gpu/ganesh/d3d/GrD3DBackendContext.h>

namespace zenith {

class SkiaD3D12Context {
public:
    SkiaD3D12Context();
    ~SkiaD3D12Context();

    /**
     * @brief Initialize D3D12 and connect to the given window.
     * @param hwnd The native window handle to render into.
     * @param width Initial width.
     * @param height Initial height.
     * @return true if successful.
     */
    bool initialize(void* hwnd, int width, int height);

    /**
     * @brief Handle window resizing. Recreates swap chain buffers.
     */
    void resize(int width, int height);

    /**
     * @brief Begin a frame. Returns the SkCanvas to draw into.
     * @return Pointer to SkCanvas, or nullptr if failed.
     */
    SkCanvas* beginFrame();

    /**
     * @brief End the frame. Flushes Skia and Presents the Swap Chain.
     */
    void endFrame();

    /**
     * @brief Check if initialized.
     */
    bool isInitialized() const { return initialized_; }

private:
    // D3D12 Setup
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue_;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocators_[2];
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
    Microsoft::WRL::ComPtr<ID3D12Resource> backBuffers_[2];

    HANDLE fenceEvent_ = nullptr;
    uint64_t fenceValues_[2] = {0, 0};
    UINT frameIndex_ = 0;
    UINT rtvDescriptorSize_ = 0;

    static const UINT kBufferCount = 2;

    // Skia
    sk_sp<GrDirectContext> skiaContext_;
    sk_sp<SkSurface> skiaSurfaces_[kBufferCount];

    bool initialized_ = false;
    int width_ = 0;
    int height_ = 0;

    void waitForGpu();
    void moveToNextFrame();
    bool createDeviceAndQueue();
    bool createSwapChain(void* hwnd);
    bool createRtvHeap();
    bool createCommandAllocators();
    bool createCommandList();
    bool createFence();
    bool initSkia();
    void createSurfaces();
    void cleanupSurfaces();
};

} // namespace zenith

#endif // JUCE_WINDOWS

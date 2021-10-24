#pragma once

#include "Render/RenderContext.h"
#include "D3D12Common.h"

#include <array>

#include "Device.h"
#include "Swapchain.h"
#include "DescriptorPool.h"
#include "CommandPool.h"
#include "CommandAllocator.h"

namespace Immortal
{
namespace D3D12
{

static inline DXGI_FORMAT NORMALIZE(Format format)
{
    static DXGI_FORMAT map[] = {
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_R10G10B10A2_UNORM,
        DXGI_FORMAT_R16G16B16A16_FLOAT
    };

    return map[ncast<int>(format)];
}

class RenderContext : public SuperRenderContext
{
public:
    using Super = SuperRenderContext;

    static constexpr int MAX_FRAME_COUNT{ 3 };

public:
    RenderContext(const void *handle);

    RenderContext(Description &desc);

    ~RenderContext();

    void INIT();

    template <class T>
    T Get()
    {
        if constexpr (is_same<T, Swapchain*>())
        {
            return swapchain.get();
        }
        if constexpr (is_same<T, HWND>())
        {
            return hWnd;
        }
        if constexpr (is_same<T, ID3D12Device*>())
        {
            return device->Handle();
        }
        if constexpr (is_same<T, DXGI_FORMAT>())
        {
            return NORMALIZE(desc.format);
        }
        if constexpr (is_same<T, Queue *>())
        {
            return queue.get();
        }
        if constexpr (is_same<T, CommandList *>())
        {
            return commandList.get();
        }
        if constexpr (is_same<T, ID3D12CommandAllocator **>())
        {
            return commandAllocator;
        }
        if constexpr (is_same<T, Window*>())
        {
            return desc.WindowHandle;
        }
        if constexpr (is_same<T, ID3D12Fence *>())
        {
            return fence.Get();
        }
    }

    UINT FrameSize()
    {
        return desc.FrameCount;
    }

    DescriptorPool *ShaderResourceViewDescritorHeap()
    {
        return shaderResourceViewDescriptorHeap.get();
    }

    ID3D12Resource *RenderTarget(UINT backBufferIndex)
    {
        return renderTargets[backBufferIndex];
    }

    Descriptor RenderTargetDescriptor(INT backBufferIndex)
    {
        return Descriptor{
                 renderTargetViewDescriptorHeap->Get<D3D12_CPU_DESCRIPTOR_HANDLE>(),
                 backBufferIndex,
                 renderTargetViewDescriptorSize
            };
    }

    Vector2 Extent()
    {
        return Vector2{
            ncast<float>(desc.Width),
            ncast<float>(desc.Height)
        };
    }

    void CleanUpRenderTarget();

    void CreateRenderTarget();

    void CheckDisplayHDRSupport();

    void EnsureSwapChainColorSpace(Swapchain::BitDepth bitDepth, bool enableST2084);

    void SetHDRMetaData(float MaxOutputNits = 1000.0f, float MinOutputNits = 0.001f, float MaxCLL = 2000.0f, float MaxFALL = 500.0f);

private:
    Super::Description desc{};

    HWND hWnd{ nullptr };

    HMODULE hModule{ nullptr };

    ComPtr<IDXGIFactory4> dxgiFactory{ nullptr };

    std::unique_ptr<Device> device;

    std::unique_ptr<Queue> queue;

    std::unique_ptr<Swapchain> swapchain;

    HANDLE swapchainWritableObject{ nullptr };

    std::unique_ptr<DescriptorPool> renderTargetViewDescriptorHeap;

    std::unique_ptr<DescriptorPool> shaderResourceViewDescriptorHeap;
    
    std::unique_ptr<CommandList> commandList;

    UINT frameIndex{ 0 };

    UINT renderTargetViewDescriptorSize{ 0 };

    ID3D12Resource *renderTargets[MAX_FRAME_COUNT]{ nullptr };

    ID3D12CommandAllocator *commandAllocator[MAX_FRAME_COUNT];

    ID3D12Fence *fence;

    HANDLE fenceEvent{ nullptr };

    UINT64 fenceValues[MAX_FRAME_COUNT]{ 0 };

    UINT64 fenceLastSignaledValue{ 0 };

    struct
    {
        UINT hdrMetaDataPoolIdx{ 0 };

        bool hdrSupport{ false };

        bool enableST2084{ false };

        float referenceWhiteNits{ 80.0f };  

        UINT rootConstants[RootConstantsCount];

        Swapchain::BitDepth bitDepth{ Swapchain::BitDepth::_8 };

        DXGI_COLOR_SPACE_TYPE currentColorSpace{ DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709 };
    } color;

    struct
    {
        int left{ 0 };
        int top{ 0 };
        int right{ 0 };
        int bottom{ 0 };
    } windowBounds;

public:
    ErrorHandle error;

    void WaitForGPU();

    UINT WaitForPreviousFrame();

    void WaitForNextFrameResources();

    void UpdateSwapchain(UINT width, UINT height);
};

}
}

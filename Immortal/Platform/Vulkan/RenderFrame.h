#pragma once

#include "Common.h"
#include "Device.h"
#include "CommandPool.h"
#include "FencePool.h"
#include "SemaphorePool.h"
#include "RenderTarget.h"
#include "DescriptorPool.h"

namespace Immortal
{
namespace Vulkan
{
class RenderFrame
{
public:
    static constexpr UINT32 BUFFER_POOL_BLOCK_SIZE = 256;

    static inline const std::unordered_map<VkBufferUsageFlags, UINT32> supportedUsageMap = {
        { VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 1 },
        { VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 2 },
        { VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,  1 },
        { VK_BUFFER_USAGE_INDEX_BUFFER_BIT,   1 }
    };

public:
    RenderFrame(Device *device, std::unique_ptr<RenderTarget> &&renderTarget, size_t threadCount = 1);

    void Set(std::unique_ptr<RenderTarget> &value)
    {
        renderTarget.reset();
        renderTarget.swap(value);
    }

private:
    Device *device;

    std::unique_ptr<RenderTarget> renderTarget;

    std::map<UINT32, std::vector<std::unique_ptr<CommandPool>>> commandPools;

    FencePool fencePool;

    SemaphorePool semaphorePool;

    size_t threadCount;

private:
    RenderFrame(const RenderFrame &) = delete;

    RenderFrame(RenderFrame &&) = delete;
        
    RenderFrame &operator=(const RenderFrame &) = delete;
        
    RenderFrame &operator=(RenderFrame &&) = delete;
};
}
}

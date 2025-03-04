#include "impch.h"
#include "RenderContext.h"

#include <array>

#include "Common.h"
#include <GLFW/glfw3.h>

#include "Framework/Application.h"
#include "Renderer.h"
#include "Image.h"
#include "RenderFrame.h"
#include "Framebuffer.h"

namespace Immortal
{
namespace Vulkan
{

VkResult RenderContext::Status = VK_NOT_READY;

std::unordered_map<const char *, bool> RenderContext::InstanceExtensions{
    { IMMORTAL_PLATFORM_SURFACE, false }
};

std::unordered_map<const char *, bool> RenderContext::DeviceExtensions{
    { VK_KHR_SWAPCHAIN_EXTENSION_NAME, false }
};

static std::vector<const char *> ValidationLayers = {
    "VK_LAYER_KHRONOS_validation",
    "VK_LAYER_KHRONOS_synchronization2",
    // "VK_LAYER_LUNARG_api_dump",
    // "VK_LAYER_LUNARG_device_simulation",
    // "VK_LAYER_LUNARG_assistant_layer",
    // "VK_LAYER_LUNARG_monitor",
    // "VK_LAYER_LUNARG_screenshot",
    // "VK_LAYER_RENDERDOC_Capture",
};

RenderContext::RenderContext(const RenderContext::Description &desc)
    : handle{ desc.WindowHandle->GetNativeWindow() }
{
    instance = std::make_unique<Instance>(Application::Name(), InstanceExtensions, ValidationLayers);
    if (!instance)
    {
        LOG::ERR("Vulkan Not Supported!");
        return;
    }
    CreateSurface();

    auto &physicalDevice = instance->SuitablePhysicalDevice();
    physicalDevice.HighPriorityGraphicsQueue            = true;
    physicalDevice.RequestedFeatures.samplerAnisotropy  = VK_TRUE;
    physicalDevice.RequestedFeatures.robustBufferAccess = VK_TRUE;

    depthFormat = SuitableDepthFormat(physicalDevice.Handle());

    if (physicalDevice.Features.textureCompressionASTC_LDR)
    {
        physicalDevice.RequestedFeatures.textureCompressionASTC_LDR = VK_TRUE;
    }

    if (instance->IsEnabled(VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME))
    {
        AddDeviceExtension(VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
    }

    device = std::make_unique<Device>(physicalDevice, surface, DeviceExtensions);
    queue  = device->SuitableGraphicsQueuePtr();

    surfaceExtent = VkExtent2D{ Application::Width(), Application::Height() };
    if (surface != VK_NULL_HANDLE)
    {
        VkSurfaceCapabilitiesKHR surfaceProperties{};
        Check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice.Handle(), surface, &surfaceProperties));

        if (surfaceProperties.currentExtent.width == 0xFFFFFFFF)
        {
            swapchain = std::make_unique<Swapchain>(*device, surface, surfaceExtent);
        }
        else
        {
            swapchain = std::make_unique<Swapchain>(*device, surface);
        }
    }
    Prepare();
    EnableGlobal();

    Super::UpdateMeta(physicalDevice.Properties.deviceName, nullptr, nullptr);
}

RenderContext::~RenderContext()
{
    vkDestroySurfaceKHR(instance->Handle(), surface, nullptr);
    instance.release();
}

void RenderContext::CreateSurface()
{
    if (instance->Handle() == VK_NULL_HANDLE && !handle)
    {
        surface = VK_NULL_HANDLE;
        return;
    }

    Check(glfwCreateWindowSurface(instance->Handle(), static_cast<GLFWwindow *>(handle), nullptr, &surface));
}

void RenderContext::Prepare(size_t threadCount)
{
    device->Wait();

    swapchain->Create();

    renderPass.reset(new RenderPass{ device.get(), swapchain->Get<VkFormat>(), depthFormat });
    
    surfaceExtent = swapchain->Get<VkExtent2D>();
    VkExtent3D extent{ surfaceExtent.width, surfaceExtent.height, 1 };
    
    std::vector<ImageView> views;

    for (auto &handle : swapchain->Get<Swapchain::Images>())
    {
        auto image = std::make_unique<Image>(
            device.get(),
            handle,
            extent,
            swapchain->Get<VkFormat>(),
            swapchain->Get<VkImageUsageFlags>()
            );
        auto renderTarget = RenderTarget::Create(std::move(image));
        renderTarget->Set(renderPass);
        present.renderTargets.emplace_back(std::move(renderTarget));
    }
    this->threadCount = threadCount;

    for (auto &buf : present.commandBuffers)
    {
        buf.reset(device->RequestCommandBuffer(Level::Primary));
    }

    SetupDescriptorSetLayout();

    Status = VK_SUCCESS;
}

Swapchain *RenderContext::UpdateSurface()
{
    if (!swapchain)
    {
        goto end;
    }

    VkSurfaceCapabilitiesKHR properties;
    Check(device->GetSurfaceCapabilities(swapchain->Get<Surface>(), &properties));
    
    if (properties.currentExtent.width == 0xFFFFFFFF || (properties.currentExtent.height <= 0 && properties.currentExtent.width <= 0))
    {
        goto end;
    }
    if (properties.currentExtent.width != surfaceExtent.width || properties.currentExtent.height != surfaceExtent.height)
    {
        device->Wait();
        surfaceExtent = properties.currentExtent;
        UpdateSwapchain(surfaceExtent, regisry.preTransform);
    }

end:
    return swapchain.get();
}

void RenderContext::UpdateSwapchain(const VkExtent2D &extent, const VkSurfaceTransformFlagBitsKHR transform)
{
    present.renderTargets.clear();

    if (transform == VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR || transform == VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR)
    {
        swapchain = std::make_unique<Swapchain>(*swapchain, VkExtent2D{ extent.height, extent.width }, transform);
    }
    else
    {
        swapchain = std::make_unique<Swapchain>(*swapchain, extent, transform);;
    }
    regisry.preTransform = transform;

    for (auto &handle : swapchain->Get<Swapchain::Images>())
    {
        auto image = std::make_unique<Image>(
            device.get(),
            handle,
            VkExtent3D{ extent.width, extent.height, 1 },
            swapchain->Get<VkFormat>(),
            swapchain->Get<VkImageUsageFlags>()
        );

        auto renderTarget = RenderTarget::Create(std::move(image));
        renderTarget->Set(renderPass);
        present.renderTargets.emplace_back(std::move(renderTarget));
    }
}

VkDescriptorSetLayout RenderContext::DescriptorSetLayout{ VK_NULL_HANDLE };
void RenderContext::SetupDescriptorSetLayout()
{
    std::array<VkDescriptorSetLayoutBinding, 1> bindings{};
    bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].descriptorCount    = 1;
    bindings[0].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[0].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo info = {};
    info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = U32(bindings.size());
    info.pBindings    = bindings.data();
    Check(device->Create(&info, nullptr, &DescriptorSetLayout));
}

}
}

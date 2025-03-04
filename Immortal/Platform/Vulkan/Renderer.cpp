#include "impch.h"
#include "Renderer.h"
#include <array>
#include "..\D3D12\Renderer.h"

namespace Immortal
{
namespace Vulkan
{

Renderer::Renderer(RenderContext::Super *c) : 
    context{ dcast<RenderContext *>(c) },
    device{ context->Get<Device *>() },
    swapchain{ context->Get<Swapchain *>() },
    semaphorePool{ device }
{
    Setup();
}

Renderer::~Renderer()
{
    for (auto &fence : fences)
    {
        device->Discard(&fence);
    }
}

void Renderer::Setup()
{
    auto frameSize = context->FrameSize();
    for (int i = 0; i < frameSize; i++)
    {
        semaphores[i].acquiredImageReady = semaphorePool.Request();
        semaphores[i].renderComplete     = semaphorePool.Request();
    }

    queue = context->Get<Queue*>();
}

void Renderer::PrepareFrame()
{
    if (!swapchain)
    {
        return;
    }

    auto error = swapchain->AcquireNextImage(&currentBuffer, semaphores[sync].acquiredImageReady, VK_NULL_HANDLE);
    context->MoveToFrame(currentBuffer);
    if (fences[sync] != VK_NULL_HANDLE)
    {
        device->WaitAndReset(&fences[sync]);
    }
    else
    {
        fences[sync] = device->RequestFence();
    }

    if ((error == VK_ERROR_OUT_OF_DATE_KHR) || (error == VK_SUBOPTIMAL_KHR))
    {
        Resize();
    }
    else
    {
        Check(error);
    }
    context->GetCommandBuffer()->Begin();
}

void Renderer::Resize()
{
    swapchain = context->UpdateSurface();
    device->Wait();
}

void Renderer::SubmitFrame()
{
    VkResult error{ VK_SUCCESS };

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext              = nullptr;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &semaphores[sync].renderComplete;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &swapchain->Handle();
    presentInfo.pImageIndices      = &currentBuffer;
    presentInfo.pResults           = nullptr;

    error = queue->Present(presentInfo);
    if (error == VK_ERROR_OUT_OF_DATE_KHR || error == VK_SUBOPTIMAL_KHR)
    {
        Resize();
        return;
    }
    Check(error);
}

void Renderer::SwapBuffers()
{
    context->GetCommandBuffer()->End();

    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = &semaphores[sync].acquiredImageReady;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = &semaphores[sync].renderComplete;
    submitInfo.pWaitDstStageMask    = &submitPipelineStages;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &context->GetCommandBuffer()->Handle();

    queue->Submit(submitInfo, fences[sync]);
    SubmitFrame();

    sync = (sync + 1) % context->FrameSize();
}

void Renderer::Begin(std::shared_ptr<RenderTarget::Super> &renderTarget)
{
    auto nativeRenderTarget = std::dynamic_pointer_cast<RenderTarget>(renderTarget);

    context->Begin([&](CommandBuffer *cmdbuf) {
        auto &desc = nativeRenderTarget->Desc();

        VkRenderPassBeginInfo beginInfo{};
        beginInfo.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        beginInfo.pNext                    = nullptr;
        beginInfo.framebuffer              = nativeRenderTarget->GetFramebuffer();
        beginInfo.clearValueCount          = 2;
        beginInfo.pClearValues             = rcast<VkClearValue*>(&nativeRenderTarget->clearValues);
        beginInfo.renderPass               = nativeRenderTarget->GetRenderPass();
        beginInfo.renderArea.extent.width  = desc.Width;
        beginInfo.renderArea.extent.height = desc.Height;
        beginInfo.renderArea.offset        = { 0, 0 };

        cmdbuf->BeginRenderPass(&beginInfo, VK_SUBPASS_CONTENTS_INLINE);
        cmdbuf->SetViewport(ncast<float>(desc.Width), ncast<float>(desc.Height));
        cmdbuf->SetScissor(desc.Width, desc.Height);
    });
}

void Renderer::End()
{
    context->End([&](CommandBuffer *cmdbuf) {
        cmdbuf->EndRenderPass();
    });
}

void Renderer::Draw(const std::shared_ptr<Pipeline::Super> &pipeline)
{
    context->Submit([&](CommandBuffer *cmdbuf) {
        auto pl = std::dynamic_pointer_cast<Pipeline>(pipeline);
        vkCmdBindDescriptorSets(*cmdbuf, pl->BindPoint(), pl->Layout(), 0, 1, &pl->GetDescriptorSet(), 0, 0);
        vkCmdBindPipeline(*cmdbuf, pl->BindPoint(), *pl);

        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(*cmdbuf, 0, 1, &pl->Get<Buffer::Type::Vertex>()->Handle(), offsets);
        vkCmdBindIndexBuffer(*cmdbuf, pl->Get<Buffer::Type::Index>()->Handle(), 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(*cmdbuf, pl->ElementCount, 1, 0, 0, 0);
    });
}

Descriptor *Renderer::CreateImageDescriptor(uint32_t count)
{
    auto descriptor = new ImageDescriptor[count];
    CleanUpObject(descriptor);

    return descriptor;
}

Descriptor *Renderer::CreateBufferDescriptor(uint32_t count)
{
    auto descriptor = new BufferDescriptor[count];
    CleanUpObject(descriptor);

    return descriptor;
}

}
}

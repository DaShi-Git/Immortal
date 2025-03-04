#pragma once

#include "Render/RenderTarget.h"

#include "Common.h"
#include "Device.h"
#include "Attachment.h"
#include "DescriptorSet.h"
#include "Framebuffer.h"

namespace Immortal
{
namespace Vulkan
{

struct CompareExtent2D
{
    bool operator()(const VkExtent2D &lhs, const VkExtent2D &rhs) const
    {
        return !(lhs.width == rhs.width && lhs.height == rhs.height) && (lhs.width < rhs.width &&lhs.height < rhs.height);
    }
};

class RenderTarget : public SuperRenderTarget
{
public:
    using Super = SuperRenderTarget;

    static std::unique_ptr<RenderTarget> Create(std::unique_ptr<Image> &&image);

public:
    RenderTarget(std::vector<std::unique_ptr<Image>> images);

    RenderTarget(Device *device, const Description &description);

    RenderTarget(RenderTarget &&other);

    ~RenderTarget();

    RenderTarget &operator=(RenderTarget &&other);

    template <class T>
    T *GetAddress()
    {
        if constexpr (IsPrimitiveOf<RenderPass, T>())
        {
            return renderPass.get();
        }
        if constexpr (IsPrimitiveOf<Framebuffer, T>())
        {
            return framebuffer.get();
        }
    }

    VkRenderPass GetRenderPass()
    {
        return *renderPass;
    }

    VkFramebuffer GetFramebuffer()
    {
        return *framebuffer;
    }

    void SetupFramebuffer()
    {
        std::vector<VkImageView> views;

        for (auto &color : attachments.colors)
        {
            views.emplace_back(color.view->Handle());
        }
        views.emplace_back(attachments.depth.view->Handle());

        framebuffer.reset(new Framebuffer{ device, *renderPass, views, VkExtent2D{ desc.Width, desc.Height }});
    }

    void Set(std::shared_ptr<RenderPass> &value)
    {
        renderPass = value;
        SetupFramebuffer();
    }

    uint32_t ColorAttachmentCount()
    {
        return attachments.colors.size();
    }

    const Image &GetColorImage(uint32_t index = 0) const
    {
        return *attachments.colors[index].image;
    }

    void Create();

    void SetupDescriptor();

    void SetupExtent(Extent2D extent)
    {
        desc.Width  = extent.width;
        desc.Height = extent.height;
    }

public:
    virtual operator uint64_t() const override;

    virtual void Map(uint32_t slot = 0) override;

    virtual void Resize(uint32_t x, uint32_t y) override;

private:
    Device *device{ nullptr };

    std::shared_ptr<RenderPass> renderPass;

    std::unique_ptr<Framebuffer> framebuffer;

    std::unique_ptr<DescriptorSet> descriptorSet;

    std::unique_ptr<ImageDescriptor> descriptor;

    Sampler sampler;

    struct
    {
        Attachment depth;

        std::vector<Attachment> colors;
    } attachments;
};

}
}

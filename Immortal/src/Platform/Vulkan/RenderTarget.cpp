#include "impch.h"
#include "RenderTarget.h"

#include "Image.h"

namespace Immortal
{
namespace Vulkan
{

std::unique_ptr<RenderTarget> RenderTarget::Create(Image &&image)
{
    std::vector<Image> images;

    auto device = image.Get<Device *>();

    VkFormat depthFormat = SuitableDepthFormat(device->Get<PhysicalDevice>().Handle());

    Image depthImage{
        device,
        image.Extent(),
        depthFormat,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, VMA_MEMORY_USAGE_GPU_ONLY
    };
    
    images.push_back(std::move(image));
    images.push_back(std::move(depthImage));

    return std::make_unique<RenderTarget>(std::move(images));
};

RenderTarget::RenderTarget(std::vector<Image> &&images) :
    device{ images.back().Get<Device *>() },
    images{ std::move(images) }
{
    SLASSERT(!this->images.empty() && "Should specify at least 1 image");
    std::set<VkExtent2D, CompareExtent2D> uniqueExtent;

    auto ImageExtent = [](const Image &image) -> VkExtent2D
    {
        auto &extent = image.Extent();
        return { extent.width,  extent.height };
    };

    std::transform(this->images.begin(), this->images.end(), std::inserter(uniqueExtent, uniqueExtent.end()), ImageExtent);
    SLASSERT(uniqueExtent.size() == 1 && "Extent size is not unique");

    extent = *uniqueExtent.begin();

    for (auto &image : this->images)
    {
        if (image.Type() != VK_IMAGE_TYPE_2D)
        {
            LOG::ERR("Image type is not 2D");
        }

        views.emplace_back(&image, VK_IMAGE_VIEW_TYPE_2D);
        attachments.emplace_back(Attachment{ image.Format(), image.SampleCount(), image.Usage() });
    }
}

RenderTarget::RenderTarget(RenderTarget &&other) :
    device{ other.device },
    extent{ other.extent },
    attachments{ std::move(other.attachments) },
    images{ std::move(other.images) },
    views{ std::move(other.views) },
    inputAttachments{ std::move(other.inputAttachments) },
    outputAttachments{ std::move(other.outputAttachments) }
{
    other.device = nullptr;
}

RenderTarget::~RenderTarget()
{
    images.clear();
    views.clear();
}

RenderTarget &RenderTarget::operator=(RenderTarget &&other)
{
    LOG::WARN(&other == this && "You are trying to self-assigments, which is not acceptable.");
    device            = other.device;
    extent            = other.extent;
    attachments       = std::move(other.attachments);
    images            = std::move(other.images);
    views             = std::move(other.views);
    inputAttachments  = std::move(other.inputAttachments);
    outputAttachments = std::move(other.outputAttachments);
    
    other.device      = nullptr;

    return *this;
}

}
}

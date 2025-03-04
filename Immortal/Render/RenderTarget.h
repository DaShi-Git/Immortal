#pragma once

#include "Core.h"

#include "Texture.h"
#include "Types.h"

namespace Immortal
{
   
struct Attachment
{
public:
    enum class Type
    {
        Color,
        Depth
    };

    using Description = std::vector<Texture::Description>;

public:
    Attachment() :
        handle{ nullptr }
    {
    
    }

    Attachment(uint32_t handle) :
        handle{ (void*)(uint64_t)(handle) }
    {
        
    }

    operator uint64_t()
    {
        return rcast<uint64_t>(handle);
    }

    void *handle;
};

class RenderTarget
{
public:
    struct Description
    {
        Description() { }

        Description(Vector2 viewport, Attachment::Description attachments) :
            Width{ ncast<uint32_t>(viewport.x) },
            Height{ ncast<uint32_t>(viewport.y) },
            Attachments{ attachments }
        {

        }

        Description(uint32_t width, uint32_t height, Attachment::Description attachments) :
            Width{ width },
            Height{ height },
            Attachments{ attachments }
        {

        }

        uint32_t Width     = 0;
        uint32_t Height    = 1;
        uint32_t Samples   = 1;
        uint32_t Layers    = 1;
        uint32_t MipLevels = 1;

        Attachment::Description Attachments;

        bool SwapChainTarget = false;

        void *context{ nullptr };
    };

public:
    RenderTarget() = default;

    RenderTarget(const Description &description) :
        desc{ description }
    {
    
    }

    virtual ~RenderTarget() = default;

    virtual void Map(uint32_t slot = 0) { }

    virtual void Unmap() { }
         
    virtual void Resize(uint32_t width, uint32_t height) { }

    virtual void Resize(Vector2 size)
    {
        Resize(ncast<uint32_t>(size.x), ncast<uint32_t>(size.y));
    }

    virtual void *ReadPixel(uint32_t attachmentIndex, int x, int y, Format format, int width = 1, int height = 1)
    {
        return nullptr;
    }

    virtual void ClearAttachment(uint32_t attachmentIndex, int value) { }

    template <Attachment::Type T, class ... Args>
    Attachment &Get(Args&& ... args)
    {
        if constexpr (T == Attachment::Type::Color)
        {
            return ColorAttachment(std::forward<Args>(args)...);
        }
        if constexpr (T == Attachment::Type::Depth)
        {
            return DepthAttachment(std::forward<Args>(args)...);
        }
    }

    virtual Attachment ColorAttachment(size_t index = 0)
    {
        return Attachment{};
    }

    virtual Attachment DepthAttachment()
    {
        return Attachment{};
    }

    const Description &Desc() const
    {
        return desc;
    }

    virtual operator uint64_t() const
    {
        return 0;
    }

    void Set(Colour colour)
    {
        clearValues.color = colour;
    }
    
    Colour *ClearColor()
    {
        return &clearValues.color;
    }

    Vector2 ViewportSize()
    {
        return Vector2{ ncast<float>(desc.Width), ncast<float>(desc.Height) };
    }

    uint32_t Width() const
    {
        return desc.Width;
    }

    uint32_t Height() const
    {
        return desc.Height;
    }

public:
    Description desc{};

    struct
    {
        Colour color{ 0.0f, 0.0f, 0.0f, 1.0f };
        DepthStencilValue depthStencil{ 1.0f, 0 };
    } clearValues;
};

using SuperRenderTarget = RenderTarget;

}

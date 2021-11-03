#pragma once

#include "ImmortalCore.h"

#include "Buffer.h"
#include "Shader.h"
#include "Framebuffer.h"

namespace Immortal
{

class IMMORTAL_API Pipeline
{
public:
    enum class DrawType
    {
        None = 0,
        Static,
        Stream,
        Dynamic
    };

    enum class PrimitiveType
    {
        Point,
        Line,
        Triangles
    };

public:
    Pipeline() { }

    Pipeline(std::shared_ptr<Shader> &shader) :
        desc{ shader }
    {
        
    }

    virtual ~Pipeline() { }

    virtual void Map() { }

    virtual void Unmap() { }

    virtual void Set(std::shared_ptr<Buffer> &buffer)
    {

    }

    virtual void Set(const InputElementDescription &description)
    {
        desc.layout = description;
    }

    virtual void Create(std::shared_ptr<Framebuffer> &framebuffer)
    {
        
    }

    template <class T>
    void Update(size_t size, const T *data, int slot = 0)
    {
        desc.vertexBuffers[slot]->Update(sizeof(T) * size, rcast<const void*>(data));
    }

protected:
    struct Description
    {
        std::shared_ptr<Shader> shader{ nullptr };

        std::vector<std::shared_ptr<Buffer>> vertexBuffers;

        std::shared_ptr<Buffer> indexBuffer;

        InputElementDescription layout{};

        DrawType Type{ DrawType::Static };

        PrimitiveType PrimitiveType = PrimitiveType::Triangles;
    } desc;
};

using SuperPipeline = Pipeline;

}

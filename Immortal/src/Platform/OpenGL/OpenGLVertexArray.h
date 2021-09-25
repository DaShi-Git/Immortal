#pragma once

#include "ImmortalCore.h"
#include "Render/VertexArray.h"

namespace Immortal
{

class OpenGLVertexArray : public VertexArray
{
public:
    OpenGLVertexArray();
    ~OpenGLVertexArray();

    void Map() const;
    void Unmap() const;

    void AddVertexBuffer(const Ref<VertexBuffer> &vertexBuffer) override;
    void SetIndexBuffer(const Ref<IndexBuffer> &indexBuffer) override;

    const std::vector<Ref<VertexBuffer> > &GetVertexBuffers() const override
    {
        return mVertexBuffers;
    }

    const Ref<IndexBuffer> &GetIndexBuffer() const override
    {
        return mIndexBuffer;
    }

private:
    uint32_t mHandle;
    uint32_t mVertexBufferIndex = 0;
    std::vector<Ref<VertexBuffer> > mVertexBuffers;
    Ref<IndexBuffer> mIndexBuffer;
};

}

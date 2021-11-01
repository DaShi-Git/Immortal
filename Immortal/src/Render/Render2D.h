#pragma once

#include "ImmortalCore.h"

#include "OrthographicCamera.h"
#include "Camera.h"
#include "Texture.h"
#include "Scene/Component.h"

namespace Immortal
{
class IMMORTAL_API Render2D
{
public:
    static std::shared_ptr<Pipeline> pipeline;

public:
    static void INIT();
    static void Shutdown();

    static void BeginScene(const Camera& camera);
    static void BeginScene(const Camera& camera, const Matrix4& transform);

    static void BeginScene(const OrthographicCamera camera);
    static void EndScene();
    static void Flush();

    static void SetColor(const Vector4 &color, const float brightness, const Vector3 HSV = Vector3(0.0f));

    static void DrawQuad(const Vector2& position, const Vector2& size, const Vector4& color);
    static void DrawQuad(const Vector3& position, const Vector2& size, const Vector4& color);
    static void DrawQuad(const Vector2& position, const Vector2& size, const std::shared_ptr<Texture2D>& texture, float tilingFactor = 1.0f, const Vector4& tintColor = Vector4(1.0f));
    static void DrawQuad(const Vector3& position, const Vector2& size, const std::shared_ptr<Texture2D>& texture, float tilingFactor = 1.0f, const Vector4& tintColor = Vector4(1.0f));

    static void DrawQuad(const Matrix4& transform, const Vector4& color, int entityID = -1);
    static void DrawQuad(const Matrix4& transform, const std::shared_ptr<Texture2D>& texture, float tilingFactor = 1.0f, const Vector4& tintColor = Vector4(1.0f), int entityID = -1);

    static void DrawRotatedQuad(const Vector2& position, const Vector2& size, float rotation, const Vector4& color);
    static void DrawRotatedQuad(const Vector3& position, const Vector2& size, float rotation, const Vector4& color);
    static void DrawRotatedQuad(const Vector2& position, const Vector2& size, float rotation, const std::shared_ptr<Texture2D>& texture, float tilingFactor = 1.0f, const Vector4& tintColor = Vector4(1.0f));
    static void DrawRotatedQuad(const Vector3& position, const Vector2& size, float rotation, const std::shared_ptr<Texture2D>& texture, float tilingFactor = 1.0f, const Vector4& tintColor = Vector4(1.0f));

    static void Render2D::DrawSprite(const Matrix4& transform, SpriteRendererComponent& src, int entityID);

    struct Statistics
    {
        uint32_t DrawCalls = 0;
        uint32_t QuadCount = 0;

        uint32_t TotalVertexCount() const { return QuadCount * 4; }
        uint32_t TotalIndexCount() const { return QuadCount * 6; }
    };

    static void ResetStats();
    static Statistics Stats();

private:
    static void StartBatch();
    static void NextBatch();

};
}


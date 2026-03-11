// src/editor/Utils/RayPicking.hpp
#pragma once
#include "../engine/Math/Math.hpp"
#include "../engine/Graphics/Camera.hpp"
#include "../engine/Core/GameObject.hpp"
#include "../engine/Graphics/RenderBatchSystem.hpp"
#include <vector>

namespace Editor::EditorUtils
{
    using namespace Engine;

    struct RaycastHit
    {
        Core::GameObject* object = nullptr;
        Math::Vector3 point;
        Math::Vector3 normal;
        float distance = 0.0f;
        bool hit = false;
    };

    class RayPicking
    {
    public:
        RayPicking() = default;
        ~RayPicking() = default;

        static void screenToWorldRay(
            float screenX, float screenY,
            float screenWidth, float screenHeight,
            const Graphics::Camera& camera,
            Math::Vector3& outOrigin,
            Math::Vector3& outDirection);

        static bool rayIntersectsAABB(
            const Math::Vector3& rayOrigin,
            const Math::Vector3& rayDirection,
            const Math::Vector3& aabbMin,
            const Math::Vector3& aabbMax,
            float& outDistance);

        static bool rayIntersectsSphere(
            const Math::Vector3& rayOrigin,
            const Math::Vector3& rayDirection,
            const Math::Vector3& sphereCenter,
            float sphereRadius,
            float& outDistance);

        static bool rayIntersectsPlane(
            const Math::Vector3& rayOrigin,
            const Math::Vector3& rayDirection,
            const Math::Vector3& planePoint,
            const Math::Vector3& planeNormal,
            float& outDistance);

        // RenderBatchSystem* を渡すことでレンダラブルタイプに応じたAABBを使用する
        // nullptr の場合はデフォルトサイズ(0.5)にフォールバックする
        static RaycastHit raycast(
            const Math::Vector3& rayOrigin,
            const Math::Vector3& rayDirection,
            const std::vector<std::unique_ptr<Core::GameObject>>& objects,
            const Graphics::RenderBatchSystem* renderBatch = nullptr);

    private:
        static bool getObjectBounds(
            Core::GameObject* object,
            const Graphics::RenderBatchSystem* renderBatch,
            Math::Vector3& outMin,
            Math::Vector3& outMax);
    };
}
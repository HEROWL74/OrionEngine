// editor/Utils/RayPicking.cpp
#define NOMINMAX
#include "RayPicking.hpp"
#include <algorithm>
#include <limits>

namespace Editor::EditorUtils {
void RayPicking::screenToWorldRay(float screenX, float screenY,
                                  float screenWidth, float screenHeight,
                                  const World::Camera &camera,
                                  Math::Vector3 &outOrigin,
                                  Math::Vector3 &outDirection) {
  float ndcX = (2.0f * screenX) / screenWidth - 1.0f;
  float ndcY = 1.0f - (2.0f * screenY) / screenHeight;

  Math::Vector4 rayClipNear(ndcX, ndcY, 0.0f, 1.0f);
  Math::Vector4 rayClipFar(ndcX, ndcY, 1.0f, 1.0f);

  Math::Matrix4 invProj = camera.getProjectionMatrix().inverse();
  Math::Vector4 rayViewNear = invProj * rayClipNear;
  Math::Vector4 rayViewFar = invProj * rayClipFar;

  auto perspDiv = [](Math::Vector4 &v) {
    if (v.w != 0.0f) {
      v.x /= v.w;
      v.y /= v.w;
      v.z /= v.w;
      v.w = 1.0f;
    }
  };
  perspDiv(rayViewNear);
  perspDiv(rayViewFar);

  Math::Matrix4 invView = camera.getViewMatrix().inverse();
  Math::Vector4 rayWorldNear = invView * rayViewNear;
  Math::Vector4 rayWorldFar = invView * rayViewFar;

  outOrigin = Math::Vector3(rayWorldNear.x, rayWorldNear.y, rayWorldNear.z);
  Math::Vector3 farPt(rayWorldFar.x, rayWorldFar.y, rayWorldFar.z);
  outDirection = (farPt - outOrigin).normalized();
}

bool RayPicking::rayIntersectsAABB(const Math::Vector3 &rayOrigin,
                                   const Math::Vector3 &rayDirection,
                                   const Math::Vector3 &aabbMin,
                                   const Math::Vector3 &aabbMax,
                                   float &outDistance) {
  float tMin = 0.0f, tMax = std::numeric_limits<float>::max();

  for (int i = 0; i < 3; ++i) {
    if (std::abs(rayDirection[i]) < 1e-6f) {
      if (rayOrigin[i] < aabbMin[i] || rayOrigin[i] > aabbMax[i])
        return false;
    } else {
      float invD = 1.0f / rayDirection[i];
      float t1 = (aabbMin[i] - rayOrigin[i]) * invD;
      float t2 = (aabbMax[i] - rayOrigin[i]) * invD;
      if (t1 > t2)
        std::swap(t1, t2);
      tMin = std::max(tMin, t1);
      tMax = std::min(tMax, t2);
      if (tMin > tMax)
        return false;
    }
  }
  outDistance = tMin;
  return true;
}

bool RayPicking::rayIntersectsSphere(const Math::Vector3 &rayOrigin,
                                     const Math::Vector3 &rayDirection,
                                     const Math::Vector3 &sphereCenter,
                                     float sphereRadius, float &outDistance) {
  Math::Vector3 oc = rayOrigin - sphereCenter;
  float a = Math::Vector3::dot(rayDirection, rayDirection);
  float b = 2.0f * Math::Vector3::dot(oc, rayDirection);
  float c = Math::Vector3::dot(oc, oc) - sphereRadius * sphereRadius;
  float disc = b * b - 4.0f * a * c;
  if (disc < 0.0f)
    return false;
  float t = (-b - std::sqrt(disc)) / (2.0f * a);
  if (t < 0.0f)
    t = (-b + std::sqrt(disc)) / (2.0f * a);
  if (t < 0.0f)
    return false;
  outDistance = t;
  return true;
}

bool RayPicking::rayIntersectsPlane(const Math::Vector3 &rayOrigin,
                                    const Math::Vector3 &rayDirection,
                                    const Math::Vector3 &planePoint,
                                    const Math::Vector3 &planeNormal,
                                    float &outDistance) {
  float denom = Math::Vector3::dot(planeNormal, rayDirection);
  if (std::abs(denom) < 1e-6f)
    return false;
  float t = Math::Vector3::dot(planePoint - rayOrigin, planeNormal) / denom;
  if (t < 0.0f)
    return false;
  outDistance = t;
  return true;
}

RaycastHit RayPicking::raycast(
    const Math::Vector3 &rayOrigin, const Math::Vector3 &rayDirection,
    const std::vector<std::unique_ptr<Core::GameObject>> &objects,
    const Renderer::RenderBatchSystem *renderBatch) {
  RaycastHit closest;
  closest.hit = false;
  closest.distance = std::numeric_limits<float>::max();

  for (const auto &obj : objects) {
    if (!obj || !obj->isActive())
      continue;

    Math::Vector3 mn, mx;
    if (!getObjectBounds(obj.get(), renderBatch, mn, mx))
      continue;

    float dist;
    if (rayIntersectsAABB(rayOrigin, rayDirection, mn, mx, dist)) {
      if (dist < closest.distance) {
        closest.hit = true;
        closest.object = obj.get();
        closest.distance = dist;
        closest.point = rayOrigin + rayDirection * dist;
        closest.normal = Math::Vector3(0, 1, 0);
      }
    }
  }
  return closest;
}

bool RayPicking::getObjectBounds(Core::GameObject *object,
                                 const Renderer::RenderBatchSystem *renderBatch,
                                 Math::Vector3 &outMin, Math::Vector3 &outMax) {
  if (!object)
    return false;

  auto *transform = object->getTransform();
  if (!transform)
    return false;

  Math::Vector3 position = transform->getPosition();
  Math::Vector3 scale = transform->getScale();

  // RenderBatchSystemからAABBサイズを決定する
  Math::Vector3 halfExtents(0.5f, 0.5f, 0.5f); // デフォルト

  if (renderBatch) {
    const auto *entry = renderBatch->findEntry(object->getId());
    if (entry) {
      switch (entry->type) {
      case Renderer::RenderableType::Cube:
        halfExtents = Math::Vector3(0.5f, 0.5f, 0.5f);
        break;
      default:
        break;
      }
    }
  }

  halfExtents = halfExtents * scale;
  outMin = position - halfExtents;
  outMax = position + halfExtents;
  return true;
}
} // namespace Editor::EditorUtils
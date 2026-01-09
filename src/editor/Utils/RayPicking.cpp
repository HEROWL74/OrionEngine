// src/editor/Utils/RayPicking.cpp
#define NOMINMAX
#include "RayPicking.hpp"
#include <algorithm>
#include <limits>

namespace Editor::Utils
{
	void RayPicking::screenToWorldRay(
		float screenX,
		float screenY,
		float screenWidth,
		float screenHeight,
		const Graphics::Camera& camera,
		Math::Vector3& outOrigin,
		Math::Vector3& outDirection)
	{
		// スクリーン座標を NDC (Normalized Device Coordinates) に変換
		// NDC: x, y は [-1, 1] の範囲
		float ndcX = (2.0f * screenX) / screenWidth - 1.0f;
		float ndcY = 1.0f - (2.0f * screenY) / screenHeight; // Y軸は上が正

		// NDC から クリップ空間へ（near planeとfar plane）
		Math::Vector4 rayClipNear(ndcX, ndcY, -1.0f, 1.0f);
		Math::Vector4 rayClipFar(ndcX, ndcY, 1.0f, 1.0f);

		// プロジェクション行列の逆行列を取得
		Math::Matrix4 invProj = camera.getProjectionMatrix().inverse();

		// ビュー空間に変換
		Math::Vector4 rayViewNear = invProj * rayClipNear;
		Math::Vector4 rayViewFar = invProj * rayClipFar;

		// 透視除算
		if (rayViewNear.w != 0.0f)
		{
			rayViewNear.x /= rayViewNear.w;
			rayViewNear.y /= rayViewNear.w;
			rayViewNear.z /= rayViewNear.w;
			rayViewNear.w = 1.0f;
		}

		if (rayViewFar.w != 0.0f)
		{
			rayViewFar.x /= rayViewFar.w;
			rayViewFar.y /= rayViewFar.w;
			rayViewFar.z /= rayViewFar.w;
			rayViewFar.w = 1.0f;
		}

		// ビュー行列の逆行列を取得してワールド空間に変換
		Math::Matrix4 invView = camera.getViewMatrix().inverse();

		Math::Vector4 rayWorldNear = invView * rayViewNear;
		Math::Vector4 rayWorldFar = invView * rayViewFar;

		// レイの原点と方向を計算
		outOrigin = Math::Vector3(rayWorldNear.x, rayWorldNear.y, rayWorldNear.z);
		Math::Vector3 farPoint(rayWorldFar.x, rayWorldFar.y, rayWorldFar.z);
		outDirection = (farPoint - outOrigin).normalized();
	}

	bool RayPicking::rayIntersectsAABB(
		const Math::Vector3& rayOrigin,
		const Math::Vector3& rayDirection,
		const Math::Vector3& aabbMin,
		const Math::Vector3& aabbMax,
		float& outDistance)
	{
		// Slab法による高速AABB交差判定
		float tMin = 0.0f;
		float tMax = std::numeric_limits<float>::max();

		for (int i = 0; i < 3; ++i)
		{
			if (std::abs(rayDirection[i]) < 1e-6f)
			{
				// レイがこの軸に平行
				if (rayOrigin[i] < aabbMin[i] || rayOrigin[i] > aabbMax[i])
				{
					return false; // レイがAABBの外側
				}
			}
			else
			{
				float invD = 1.0f / rayDirection[i];
				float t1 = (aabbMin[i] - rayOrigin[i]) * invD;
				float t2 = (aabbMax[i] - rayOrigin[i]) * invD;

				if (t1 > t2) std::swap(t1, t2);

				tMin = std::max(tMin, t1);
				tMax = std::min(tMax, t2);

				if (tMin > tMax)
				{
					return false; // 交差なし
				}
			}
		}

		outDistance = tMin;
		return true;
	}

	bool RayPicking::rayIntersectsSphere(
		const Math::Vector3& rayOrigin,
		const Math::Vector3& rayDirection,
		const Math::Vector3& sphereCenter,
		float sphereRadius,
		float& outDistance)
	{
		Math::Vector3 oc = rayOrigin - sphereCenter;
		float a = Math::Vector3::dot(rayDirection, rayDirection);
		float b = 2.0f * Math::Vector3::dot(oc, rayDirection);
		float c = Math::Vector3::dot(oc, oc) - sphereRadius * sphereRadius;

		float discriminant = b * b - 4.0f * a * c;

		if (discriminant < 0.0f)
		{
			return false; // 交差なし
		}

		float t = (-b - std::sqrt(discriminant)) / (2.0f * a);
		if (t < 0.0f)
		{
			t = (-b + std::sqrt(discriminant)) / (2.0f * a);
		}

		if (t < 0.0f)
		{
			return false; // 球がレイの後ろ
		}

		outDistance = t;
		return true;
	}

	bool RayPicking::rayIntersectsPlane(
		const Math::Vector3& rayOrigin,
		const Math::Vector3& rayDirection,
		const Math::Vector3& planePoint,
		const Math::Vector3& planeNormal,
		float& outDistance)
	{
		float denom = Math::Vector3::dot(planeNormal, rayDirection);

		if (std::abs(denom) < 1e-6f)
		{
			return false; // レイが平面に平行
		}

		float t = Math::Vector3::dot(planePoint - rayOrigin, planeNormal) / denom;

		if (t < 0.0f)
		{
			return false; // 交点がレイの後ろ
		}

		outDistance = t;
		return true;
	}

	// unique_ptrのvectorからレイキャスト（オーバーロード）
	RaycastHit RayPicking::raycast(
		const Math::Vector3& rayOrigin,
		const Math::Vector3& rayDirection,
		const std::vector<std::unique_ptr<Core::GameObject>>& objects)
	{
		RaycastHit closestHit;
		closestHit.hit = false;
		closestHit.distance = std::numeric_limits<float>::max();

		for (const auto& obj : objects)
		{
			if (!obj || !obj->isActive())
			{
				continue;
			}

			// オブジェクトの境界ボックスを取得
			Math::Vector3 minBounds, maxBounds;
			if (!getObjectBounds(obj.get(), minBounds, maxBounds))
			{
				continue;
			}

			// レイとの交差判定
			float distance;
			if (rayIntersectsAABB(rayOrigin, rayDirection, minBounds, maxBounds, distance))
			{
				// より近いオブジェクトが見つかった
				if (distance < closestHit.distance)
				{
					closestHit.hit = true;
					closestHit.object = obj.get();
					closestHit.distance = distance;
					closestHit.point = rayOrigin + rayDirection * distance;
					// 法線は簡易的に上方向にしておく
					closestHit.normal = Math::Vector3(0.0f, 1.0f, 0.0f);
				}
			}
		}

		return closestHit;
	}

	bool RayPicking::getObjectBounds(
		Core::GameObject* object,
		Math::Vector3& outMin,
		Math::Vector3& outMax)
	{
		if (!object)
		{
			return false;
		}

		// オブジェクトの位置とスケールを取得
		auto* transform = object->getTransform();
		if (!transform)
		{
			return false;
		}

		Math::Vector3 position = transform->getPosition();
		Math::Vector3 scale = transform->getScale();

		// RenderComponentから実際のサイズを取得
		auto* renderComponent = object->getComponent<Engine::Graphics::RenderComponent>();
		if (renderComponent && renderComponent->isValid())
		{
			auto renderableType = renderComponent->getRenderableType();

			// レンダラブルタイプに応じた境界ボックスを計算
			Math::Vector3 halfExtents;
			switch (renderableType)
			{
			case Engine::Graphics::RenderableType::Triangle:
				// 三角形の場合（概算）
				halfExtents = Math::Vector3(0.5f, 0.5f, 0.1f);
				break;

			case Engine::Graphics::RenderableType::Cube:
				// 立方体の場合
				halfExtents = Math::Vector3(0.5f, 0.5f, 0.5f);
				break;

			default:
				// デフォルトサイズ
				halfExtents = Math::Vector3(0.5f, 0.5f, 0.5f);
				break;
			}

			// スケールを適用
			halfExtents = halfExtents * scale;
			outMin = position - halfExtents;
			outMax = position + halfExtents;
		}
		else
		{
			// RenderComponentがない場合は、デフォルトの境界ボックス
			Math::Vector3 halfExtents = Math::Vector3(0.5f, 0.5f, 0.5f) * scale;
			outMin = position - halfExtents;
			outMax = position + halfExtents;
		}

		return true;
	}
}
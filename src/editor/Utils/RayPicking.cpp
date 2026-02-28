// src/editor/Utils/RayPicking.cpp
#define NOMINMAX
#include "RayPicking.hpp"
#include <algorithm>
#include <limits>

namespace Editor::EditorUtils
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
		// 繧ｹ繧ｯ繝ｪ繝ｼ繝ｳ蠎ｧ讓吶ｒ NDC (Normalized Device Coordinates) 縺ｫ螟画鋤
		// NDC: x, y 縺ｯ [-1, 1] 縺ｮ遽・峇
		float ndcX = (2.0f * screenX) / screenWidth - 1.0f;
		float ndcY = 1.0f - (2.0f * screenY) / screenHeight; // Y霆ｸ縺ｯ荳翫′豁｣

		// NDC 縺九ｉ 繧ｯ繝ｪ繝・・遨ｺ髢薙∈・・ear plane縺ｨfar plane・・
		Math::Vector4 rayClipNear(ndcX, ndcY, -1.0f, 1.0f);
		Math::Vector4 rayClipFar(ndcX, ndcY, 1.0f, 1.0f);

		// 繝励Ο繧ｸ繧ｧ繧ｯ繧ｷ繝ｧ繝ｳ陦悟・縺ｮ騾・｡悟・繧貞叙蠕・
		Math::Matrix4 invProj = camera.getProjectionMatrix().inverse();

		// 繝薙Η繝ｼ遨ｺ髢薙↓螟画鋤
		Math::Vector4 rayViewNear = invProj * rayClipNear;
		Math::Vector4 rayViewFar = invProj * rayClipFar;

		// 騾剰ｦ夜勁邂・
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

		// 繝薙Η繝ｼ陦悟・縺ｮ騾・｡悟・繧貞叙蠕励＠縺ｦ繝ｯ繝ｼ繝ｫ繝臥ｩｺ髢薙↓螟画鋤
		Math::Matrix4 invView = camera.getViewMatrix().inverse();

		Math::Vector4 rayWorldNear = invView * rayViewNear;
		Math::Vector4 rayWorldFar = invView * rayViewFar;

		// 繝ｬ繧､縺ｮ蜴溽せ縺ｨ譁ｹ蜷代ｒ險育ｮ・
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
		// Slab豕輔↓繧医ｋ鬮倬蘗ABB莠､蟾ｮ蛻､螳・
		float tMin = 0.0f;
		float tMax = std::numeric_limits<float>::max();

		for (int i = 0; i < 3; ++i)
		{
			if (std::abs(rayDirection[i]) < 1e-6f)
			{
				// 繝ｬ繧､縺後％縺ｮ霆ｸ縺ｫ蟷ｳ陦・
				if (rayOrigin[i] < aabbMin[i] || rayOrigin[i] > aabbMax[i])
				{
					return false; // 繝ｬ繧､縺窟ABB縺ｮ螟門・
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
					return false; // 莠､蟾ｮ縺ｪ縺・
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
			return false; // 莠､蟾ｮ縺ｪ縺・
		}

		float t = (-b - std::sqrt(discriminant)) / (2.0f * a);
		if (t < 0.0f)
		{
			t = (-b + std::sqrt(discriminant)) / (2.0f * a);
		}

		if (t < 0.0f)
		{
			return false; // 逅・′繝ｬ繧､縺ｮ蠕後ｍ
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
			return false; // 繝ｬ繧､縺悟ｹｳ髱｢縺ｫ蟷ｳ陦・
		}

		float t = Math::Vector3::dot(planePoint - rayOrigin, planeNormal) / denom;

		if (t < 0.0f)
		{
			return false; // 莠､轤ｹ縺後Ξ繧､縺ｮ蠕後ｍ
		}

		outDistance = t;
		return true;
	}

	// unique_ptr縺ｮvector縺九ｉ繝ｬ繧､繧ｭ繝｣繧ｹ繝茨ｼ医が繝ｼ繝舌・繝ｭ繝ｼ繝会ｼ・
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

			// 繧ｪ繝悶ず繧ｧ繧ｯ繝医・蠅・阜繝懊ャ繧ｯ繧ｹ繧貞叙蠕・
			Math::Vector3 minBounds, maxBounds;
			if (!getObjectBounds(obj.get(), minBounds, maxBounds))
			{
				continue;
			}

			// 繝ｬ繧､縺ｨ縺ｮ莠､蟾ｮ蛻､螳・
			float distance;
			if (rayIntersectsAABB(rayOrigin, rayDirection, minBounds, maxBounds, distance))
			{
				// 繧医ｊ霑代＞繧ｪ繝悶ず繧ｧ繧ｯ繝医′隕九▽縺九▲縺・
				if (distance < closestHit.distance)
				{
					closestHit.hit = true;
					closestHit.object = obj.get();
					closestHit.distance = distance;
					closestHit.point = rayOrigin + rayDirection * distance;
					// 豕慕ｷ壹・邁｡譏鍋噪縺ｫ荳頑婿蜷代↓縺励※縺翫￥
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

		// 繧ｪ繝悶ず繧ｧ繧ｯ繝医・菴咲ｽｮ縺ｨ繧ｹ繧ｱ繝ｼ繝ｫ繧貞叙蠕・
		auto* transform = object->getTransform();
		if (!transform)
		{
			return false;
		}

		Math::Vector3 position = transform->getPosition();
		Math::Vector3 scale = transform->getScale();

		// RenderComponent縺九ｉ螳滄圀縺ｮ繧ｵ繧､繧ｺ繧貞叙蠕・
		auto* renderComponent = object->getComponent<Engine::Graphics::RenderComponent>();
		if (renderComponent && renderComponent->isValid())
		{
			auto renderableType = renderComponent->getRenderableType();

			// 繝ｬ繝ｳ繝繝ｩ繝悶Ν繧ｿ繧､繝励↓蠢懊§縺溷｢・阜繝懊ャ繧ｯ繧ｹ繧定ｨ育ｮ・
			Math::Vector3 halfExtents;
			switch (renderableType)
			{
			case Engine::Graphics::RenderableType::Triangle:
				// 荳芽ｧ貞ｽ｢縺ｮ蝣ｴ蜷茨ｼ域ｦらｮ暦ｼ・
				halfExtents = Math::Vector3(0.5f, 0.5f, 0.1f);
				break;

			case Engine::Graphics::RenderableType::Cube:
				// 遶区婿菴薙・蝣ｴ蜷・
				halfExtents = Math::Vector3(0.5f, 0.5f, 0.5f);
				break;

			default:
				// 繝・ヵ繧ｩ繝ｫ繝医し繧､繧ｺ
				halfExtents = Math::Vector3(0.5f, 0.5f, 0.5f);
				break;
			}

			// 繧ｹ繧ｱ繝ｼ繝ｫ繧帝←逕ｨ
			halfExtents = halfExtents * scale;
			outMin = position - halfExtents;
			outMax = position + halfExtents;
		}
		else
		{
			// RenderComponent縺後↑縺・ｴ蜷医・縲√ョ繝輔か繝ｫ繝医・蠅・阜繝懊ャ繧ｯ繧ｹ
			Math::Vector3 halfExtents = Math::Vector3(0.5f, 0.5f, 0.5f) * scale;
			outMin = position - halfExtents;
			outMax = position + halfExtents;
		}

		return true;
	}
}


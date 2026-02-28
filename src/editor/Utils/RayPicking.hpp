// src/editor/Utils/RayPicking.hpp
#pragma once
#include "engine/Math/Math.hpp"
#include "engine/Graphics/Camera.hpp"
#include "engine/Core/GameObject.hpp"
#include "engine/Graphics/RenderComponent.hpp"
#include <vector>

namespace Editor::EditorUtils
{
	using namespace Engine;

	// 繝ｬ繧､繧ｭ繝｣繧ｹ繝・ぅ繝ｳ繧ｰ縺ｮ邨先棡
	struct RaycastHit
	{
		Core::GameObject* object = nullptr;
		Math::Vector3 point;           // 莠､轤ｹ縺ｮ蠎ｧ讓・
		Math::Vector3 normal;          // 莠､轤ｹ縺ｮ豕慕ｷ・
		float distance = 0.0f;         // 繝ｬ繧､縺ｮ蜴溽せ縺九ｉ縺ｮ霍晞屬
		bool hit = false;              // 繝偵ャ繝医＠縺溘°縺ｩ縺・°
	};

	// 繧ｹ繧ｯ繝ｪ繝ｼ繝ｳ蠎ｧ讓吶°繧峨Ρ繝ｼ繝ｫ繝臥ｩｺ髢薙∈縺ｮ繝ｬ繧､繧堤函謌舌☆繧九け繝ｩ繧ｹ
	class RayPicking
	{
	public:
		RayPicking() = default;
		~RayPicking() = default;

		// 繧ｹ繧ｯ繝ｪ繝ｼ繝ｳ蠎ｧ讓吶°繧峨Ξ繧､繧堤函謌・
		// screenX, screenY: 繧ｹ繧ｯ繝ｪ繝ｼ繝ｳ蠎ｧ讓・(0,0 縺悟ｷｦ荳・
		// screenWidth, screenHeight: 繧ｹ繧ｯ繝ｪ繝ｼ繝ｳ縺ｮ繧ｵ繧､繧ｺ
		// camera: 繧ｫ繝｡繝ｩ
		// outOrigin: 繝ｬ繧､縺ｮ蜴溽せ (繝ｯ繝ｼ繝ｫ繝臥ｩｺ髢・
		// outDirection: 繝ｬ繧､縺ｮ譁ｹ蜷・(繝ｯ繝ｼ繝ｫ繝臥ｩｺ髢薙∵ｭ｣隕丞喧貂医∩)
		static void screenToWorldRay(
			float screenX,
			float screenY,
			float screenWidth,
			float screenHeight,
			const Graphics::Camera& camera,
			Math::Vector3& outOrigin,
			Math::Vector3& outDirection);

		// 繝ｬ繧､縺ｨ繝舌え繝ｳ繝・ぅ繝ｳ繧ｰ繝懊ャ繧ｯ繧ｹ縺ｮ莠､蟾ｮ蛻､螳・
		static bool rayIntersectsAABB(
			const Math::Vector3& rayOrigin,
			const Math::Vector3& rayDirection,
			const Math::Vector3& aabbMin,
			const Math::Vector3& aabbMax,
			float& outDistance);

		// 繝ｬ繧､縺ｨ逅・・莠､蟾ｮ蛻､螳・
		static bool rayIntersectsSphere(
			const Math::Vector3& rayOrigin,
			const Math::Vector3& rayDirection,
			const Math::Vector3& sphereCenter,
			float sphereRadius,
			float& outDistance);

		// 繝ｬ繧､縺ｨ蟷ｳ髱｢縺ｮ莠､蟾ｮ蛻､螳・
		static bool rayIntersectsPlane(
			const Math::Vector3& rayOrigin,
			const Math::Vector3& rayDirection,
			const Math::Vector3& planePoint,
			const Math::Vector3& planeNormal,
			float& outDistance);

		// 隍・焚縺ｮGameObject縺ｫ蟇ｾ縺励※繝ｬ繧､繧ｭ繝｣繧ｹ繝・
		// 譛繧りｿ代＞繧ｪ繝悶ず繧ｧ繧ｯ繝医ｒ霑斐☆
		static RaycastHit raycast(
			const Math::Vector3& rayOrigin,
			const Math::Vector3& rayDirection,
			const std::vector<std::unique_ptr<Core::GameObject>>& objects);

	private:
		// GameObject縺ｮ蠅・阜繝懊ャ繧ｯ繧ｹ繧貞叙蠕暦ｼ育ｰ｡譏鍋沿・・
		static bool getObjectBounds(
			Core::GameObject* object,
			Math::Vector3& outMin,
			Math::Vector3& outMax);
	};
}


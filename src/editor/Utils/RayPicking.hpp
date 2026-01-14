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

	// レイキャスティングの結果
	struct RaycastHit
	{
		Core::GameObject* object = nullptr;
		Math::Vector3 point;           // 交点の座標
		Math::Vector3 normal;          // 交点の法線
		float distance = 0.0f;         // レイの原点からの距離
		bool hit = false;              // ヒットしたかどうか
	};

	// スクリーン座標からワールド空間へのレイを生成するクラス
	class RayPicking
	{
	public:
		RayPicking() = default;
		~RayPicking() = default;

		// スクリーン座標からレイを生成
		// screenX, screenY: スクリーン座標 (0,0 が左上)
		// screenWidth, screenHeight: スクリーンのサイズ
		// camera: カメラ
		// outOrigin: レイの原点 (ワールド空間)
		// outDirection: レイの方向 (ワールド空間、正規化済み)
		static void screenToWorldRay(
			float screenX,
			float screenY,
			float screenWidth,
			float screenHeight,
			const Graphics::Camera& camera,
			Math::Vector3& outOrigin,
			Math::Vector3& outDirection);

		// レイとバウンディングボックスの交差判定
		static bool rayIntersectsAABB(
			const Math::Vector3& rayOrigin,
			const Math::Vector3& rayDirection,
			const Math::Vector3& aabbMin,
			const Math::Vector3& aabbMax,
			float& outDistance);

		// レイと球の交差判定
		static bool rayIntersectsSphere(
			const Math::Vector3& rayOrigin,
			const Math::Vector3& rayDirection,
			const Math::Vector3& sphereCenter,
			float sphereRadius,
			float& outDistance);

		// レイと平面の交差判定
		static bool rayIntersectsPlane(
			const Math::Vector3& rayOrigin,
			const Math::Vector3& rayDirection,
			const Math::Vector3& planePoint,
			const Math::Vector3& planeNormal,
			float& outDistance);

		// 複数のGameObjectに対してレイキャスト
		// 最も近いオブジェクトを返す
		static RaycastHit raycast(
			const Math::Vector3& rayOrigin,
			const Math::Vector3& rayDirection,
			const std::vector<std::unique_ptr<Core::GameObject>>& objects);

	private:
		// GameObjectの境界ボックスを取得（簡易版）
		static bool getObjectBounds(
			Core::GameObject* object,
			Math::Vector3& outMin,
			Math::Vector3& outMax);
	};
}
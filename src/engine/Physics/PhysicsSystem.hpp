// src/engine/Physics/PhysicsSystem.hpp
#pragma once

#include "../Graphics/Scene.hpp"
#include "BoxCollider.hpp"
#include <unordered_set>
#include <utility>

namespace Engine::Physics
{
	// 衝突ペアを管理するためのハッシュ関数
	struct CollisionPairHash
	{
		std::size_t operator()(const std::pair<Core::GameObject*, Core::GameObject*>& pair)const
		{
			auto h1 = std::hash<Core::GameObject*>{}(pair.first);
			auto h2 = std::hash<Core::GameObject*>{}(pair.second);
			return h1 ^ (h2 << 1);
		}
	};

	class PhysicsSystem
	{
	public:
		static PhysicsSystem& get()
		{
			static PhysicsSystem instance;
			return instance;
		}

		// 毎フレーム呼ぶ
		void update(Graphics::Scene& scene);
		// 参照を消し、jsonに影響を及ぼさないための処理
		void clear();
	private:
		PhysicsSystem() = default;

		// 前フレームで衝突していたペア
		std::unordered_set<
			std::pair<Core::GameObject*, Core::GameObject*>,
			CollisionPairHash
		> m_previousCollisions;

		// 衝突イベントを発火
		void triggerCollisionEnter(Core::GameObject* a, Core::GameObject* b);
		void triggerCollisionExit(Core::GameObject* a, Core::GameObject* b);

		// ペアを正規化
		static std::pair<Core::GameObject*, Core::GameObject*> makePair(
			Core::GameObject* a, Core::GameObject* b
		)
		{
			return a < b ? std::make_pair(a, b) : std::make_pair(b, a);
		}
	}; 
}
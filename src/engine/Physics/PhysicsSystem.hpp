// src/engine/Physics/PhysicsSystem.hpp
#pragma once

#include "../Graphics/Scene.hpp"
#include "BoxCollider.hpp"
#include <unordered_set>
#include <utility>

namespace Engine::Physics
{
	// 陦晉ｪ√・繧｢繧堤ｮ｡逅・☆繧九◆繧√・繝上ャ繧ｷ繝･髢｢謨ｰ
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

		// 豈弱ヵ繝ｬ繝ｼ繝蜻ｼ縺ｶ
		void update(Graphics::Scene& scene);
		// 蜿ら・繧呈ｶ医＠縲）son縺ｫ蠖ｱ髻ｿ繧貞所縺ｼ縺輔↑縺・◆繧√・蜃ｦ逅・
		void clear();
	private:
		PhysicsSystem() = default;

		// 蜑阪ヵ繝ｬ繝ｼ繝縺ｧ陦晉ｪ√＠縺ｦ縺・◆繝壹い
		std::unordered_set<
			std::pair<Core::GameObject*, Core::GameObject*>,
			CollisionPairHash
		> m_previousCollisions;

		// 陦晉ｪ√う繝吶Φ繝医ｒ逋ｺ轣ｫ
		void triggerCollisionEnter(Core::GameObject* a, Core::GameObject* b);
		void triggerCollisionExit(Core::GameObject* a, Core::GameObject* b);

		// 繝壹い繧呈ｭ｣隕丞喧
		static std::pair<Core::GameObject*, Core::GameObject*> makePair(
			Core::GameObject* a, Core::GameObject* b
		)
		{
			return a < b ? std::make_pair(a, b) : std::make_pair(b, a);
		}
	}; 
}


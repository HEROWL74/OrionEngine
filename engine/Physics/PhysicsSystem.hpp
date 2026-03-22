// engine/Physics/PhysicsSystem.hpp
#pragma once

#include "../World/Scene.hpp"
#include "BoxCollider.hpp"
#include <unordered_set>
#include <utility>

namespace Engine::Physics
{
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

		void update(World::Scene& scene);
		void clear();
	private:
		PhysicsSystem() = default;

		std::unordered_set<
			std::pair<Core::GameObject*, Core::GameObject*>,
			CollisionPairHash
		> m_previousCollisions;

		void triggerCollisionEnter(Core::GameObject* a, Core::GameObject* b);
		void triggerCollisionExit(Core::GameObject* a, Core::GameObject* b);

		static std::pair<Core::GameObject*, Core::GameObject*> makePair(
			Core::GameObject* a, Core::GameObject* b
		)
		{
			return a < b ? std::make_pair(a, b) : std::make_pair(b, a);
		}
	}; 
}


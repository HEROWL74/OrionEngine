// src/engine/Physics/PhysicsSystem.cpp
#include "PhysicsSystem.hpp"
#include "../Scripting/LuaScriptComponent.hpp"
#include "../Utils/Common.hpp"

namespace Engine::Physics
{
	void PhysicsSystem::update(World::Scene& scene)
	{
		auto& gameObjects = scene.getGameObjects();

		static int frameCount = 0;
		if (frameCount < 5) {
			Utils::log_info(std::format("=== PhysicsSystem Update Frame {} === GameObject count: {}",
				frameCount, gameObjects.size()));
			frameCount++;
		}

		std::unordered_set<
			std::pair<Core::GameObject*, Core::GameObject*>,
			CollisionPairHash
		> currentCollisions;

		for (size_t i = 0; i < gameObjects.size(); ++i)
		{
			auto* objA = gameObjects[i].get();
			if (!objA || !objA->isActive()) continue;

			auto* colliderA = objA->getComponent<BoxCollider>();
			if (!colliderA || !colliderA->isEnabled()) continue;

			if (frameCount < 5) {
				Math::Vector3 minA, maxA;
				colliderA->getWorldAABB(minA, maxA);
				Utils::log_info(std::format("  [{}] enabled={} AABB: ({:.2f},{:.2f},{:.2f}) to ({:.2f},{:.2f},{:.2f})",
					objA->getName(), colliderA->isEnabled(),
					minA.x, minA.y, minA.z, maxA.x, maxA.y, maxA.z));
			}

			for (size_t j = i + 1; j < gameObjects.size(); ++j)
			{
				auto* objB = gameObjects[j].get();
				if (!objB || !objB->isActive())continue;

				auto* colliderB = objB->getComponent<BoxCollider>();
				if (!colliderB || !colliderB->isEnabled())continue;

				if (colliderA->intersects(colliderB))
				{
					Utils::log_info(std::format("Collision detected: {} <-> {}",
						objA->getName(), objB->getName()));

					auto pair = makePair(objA, objB);
					currentCollisions.insert(pair);

					if (m_previousCollisions.find(pair) == m_previousCollisions.end())
					{
						Utils::log_info(std::format("Calling triggerCollisionEnter"));
						triggerCollisionEnter(objA, objB);
					}
				}
			}
		}

		for (const auto& pair : m_previousCollisions)
		{
			if (currentCollisions.find(pair) == currentCollisions.end())
			{
				Utils::log_info(std::format("Collision exit: {} <-> {}",
					pair.first->getName(), pair.second->getName()));
				triggerCollisionExit(pair.first, pair.second);
			}
		}

		m_previousCollisions = std::move(currentCollisions);
	}

	void PhysicsSystem::triggerCollisionEnter(Core::GameObject* a, Core::GameObject* b)
	{
		Utils::log_info(std::format("triggerCollisionEnter: {} vs {}",
			a->getName(), b->getName()));

		auto* scriptA = a->getComponent<Scripting::LuaScriptComponent>();
		if (scriptA && scriptA->isEnabled())
		{
			Utils::log_info(std::format("{} has LuaScript: {}",
				a->getName(), scriptA->getScriptPath()));

			auto& mgr = Scripting::ScriptManager::get();
			auto func = mgr.getFunction(scriptA->getScriptPath(), "onCollisionEnter");
			if (func.valid())
			{
				Utils::log_info("Calling Lua onCollisionEnter");
				try
				{
					func(a, b);
				}
				catch (const sol::error& e)
				{
					Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
						std::format("onCollisionEnter error: {}", e.what())));
				}
			}
			else
			{
				Utils::log_warning("onCollisionEnter function not found in Lua script");
			}
		}
		else
		{
			Utils::log_info(std::format("} has no LuaScript", a->getName()));
		}

		auto* scriptB = b->getComponent<Scripting::LuaScriptComponent>();
		if (scriptB && scriptB->isEnabled())
		{
			Utils::log_info(std::format("} has LuaScript: {}",
				b->getName(), scriptB->getScriptPath()));

			auto& mgr = Scripting::ScriptManager::get();
			auto func = mgr.getFunction(scriptB->getScriptPath(), "onCollisionEnter");
			if (func.valid())
			{
				Utils::log_info("Calling Lua onCollisionEnter");
				try
				{
					func(b, a);
				}
				catch (const sol::error& e)
				{
					Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
						std::format("onCollisionEnter error: {}", e.what())));
				}
			}
			else
			{
				Utils::log_warning("onCollisionEnter function not found in Lua script");
			}
		}
		else
		{
			Utils::log_info(std::format("{} has no LuaScript", b->getName()));
		}
	}

	void PhysicsSystem::triggerCollisionExit(Core::GameObject* a, Core::GameObject* b)
	{
		auto* scriptA = a->getComponent<Scripting::LuaScriptComponent>();
		if (scriptA && scriptA->isEnabled())
		{
			auto& mgr = Scripting::ScriptManager::get();
			auto func = mgr.getFunction(scriptA->getScriptPath(), "onCollisionExit");
			if (func.valid())
			{
				try
				{
					func(a, b);
				}
				catch (const sol::error& e)
				{
					Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
						std::format("onCollisionExit error: {}", e.what())));
				}
			}
		}

		auto* scriptB = b->getComponent<Scripting::LuaScriptComponent>();
		if (scriptB && scriptB->isEnabled())
		{
			auto& mgr = Scripting::ScriptManager::get();
			auto func = mgr.getFunction(scriptB->getScriptPath(), "onCollisionExit");
			if (func.valid())
			{
				try
				{
					func(b, a);
				}
				catch (const sol::error& e)
				{
					Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
						std::format("onCollisionExit error: {}", e.what())));
				}
			}
		}
	}

	void PhysicsSystem::clear()
	{
		m_previousCollisions.clear();
	}
}


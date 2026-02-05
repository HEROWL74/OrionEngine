#include "Scene.hpp"
#include "../Physics/PhysicsSystem.hpp"

namespace Engine::Graphics
{
	Utils::VoidResult Scene::initialize(Device* device)
	{
		CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
		CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");

		m_device = device;

		m_initialized = true;
		return {};
	}

	Core::GameObject* Scene::createGameObject(const std::string& name)
	{
		auto gameObject = std::make_unique<Core::GameObject>(name);
		Core::GameObject* ptr = gameObject.get();
		m_gameObjects.push_back(std::move(gameObject));
		return ptr;
	}

	void Scene::destroyGameObject(Core::GameObject* gameObject)
	{
		if (!gameObject || gameObject->isDestroyed())
		{
			return;
		}

		std::string objectName = gameObject->getName();
		Utils::log_info(std::format("Scene::destroyGameObject() called for '{}'", objectName));

		// 削除対象が選択中のオブジェクトなら選択解除
		if (m_selectedObject == gameObject)
		{
			Utils::log_info("Clearing selection (object being deleted)");
			m_selectedObject = nullptr;
		}
		gameObject->destroy();

		m_pendingDestroy.push_back(gameObject);

		Utils::log_info(std::format("GameObject '{}' marked for deletion (pending destroy list size: {})",
			objectName, m_pendingDestroy.size()));
	}

	void Scene::clear()
	{
		Utils::log_info("Scene::clear() called");
		Utils::log_info(std::format("  Current m_gameObjects.size() = {}", m_gameObjects.size()));

		// GPU同期
		if (m_device)
		{
			Utils::log_info("Waiting for GPU before clearing scene...");
			m_device->waitForGpu();
			Utils::log_info("GPU synchronized");
		}

		// Physics から完全解除
		Physics::PhysicsSystem::get().clear();

		m_selectedObject = nullptr;

		// GameObject を全破棄
		Utils::log_info(std::format("Destroying {} GameObjects", m_gameObjects.size()));
		for (auto& obj : m_gameObjects)
		{
			if (obj)
			{
				Utils::log_info(std::format("  Destroying: {}", obj->getName()));
				obj->destroy();
			}
		}

		Utils::log_info("Calling m_gameObjects.clear()...");
		m_gameObjects.clear();

		Utils::log_info(std::format("  After clear: m_gameObjects.size() = {}", m_gameObjects.size()));

		m_pendingDestroy.clear();

		Utils::log_info("Scene cleared completely");
	}

	Core::GameObject* Scene::findGameObject(const std::string& name) const
	{
		for (const auto& gameObject : m_gameObjects)
		{
			if (gameObject && gameObject->getName() == name && !gameObject->isDestroyed())
			{
				return gameObject.get();
			}
		}
		return nullptr;
	}

	void Scene::start()
	{
		Utils::log_info("=== Scene::start() called ===");
		Utils::log_info(std::format("  m_gameObjects.size() = {}", m_gameObjects.size()));

		for (auto& gameObject : m_gameObjects)
		{
			if (!gameObject)
			{
				Utils::log_warning("  Found NULL GameObject in m_gameObjects!");
				continue;
			}

			Utils::log_info(std::format("  Starting GameObject: {} (destroyed: {})",
				gameObject->getName(), gameObject->isDestroyed()));

			if (gameObject->isActive() && !gameObject->isDestroyed())
			{
				gameObject->start();
			}
		}

		Utils::log_info("=== Scene::start() completed ===");
	}

	void Scene::update(float deltaTime)
	{
		// 通常の更新（破棄予定のオブジェクトはスキップ）
		for (auto& gameObject : m_gameObjects)
		{
			if (gameObject->isActive() && !gameObject->isDestroyed())
			{
				gameObject->update(deltaTime);
			}
		}

		Physics::PhysicsSystem::get().update(*this);

		processPendingDestroy();
	}

	void Scene::lateUpdate(float deltaTime)
	{
		for (auto& gameObject : m_gameObjects)
		{
			if (gameObject->isActive() && !gameObject->isDestroyed())
			{
				gameObject->lateUpdate(deltaTime);
			}
		}
	}

	void Scene::render(ID3D12GraphicsCommandList* commandList, const Camera& camera, UINT frameIndex)
	{
		if (!m_initialized)
		{
			return;
		}

		for (auto& gameObject : m_gameObjects)
		{
			if (gameObject->isActive() && !gameObject->isDestroyed())
			{
				auto* renderComponent = gameObject->getComponent<RenderComponent>();
				if (renderComponent && renderComponent->isEnabled() && renderComponent->isVisible())
				{
					renderComponent->render(commandList, camera, frameIndex);
				}
			}
		}
	}

	Core::GameObject* Scene::findObjectByName(const std::string& name)
	{
		for (auto& obj : m_gameObjects)
		{
			if (obj && obj->getName() == name && !obj->isDestroyed())
			{
				return obj.get();
			}
		}
		return nullptr;
	}

	void Scene::processPendingDestroy()
	{
		if (m_pendingDestroy.empty())
		{
			return;
		}

		Utils::log_info(std::format("=== Processing {} pending deletions ===", m_pendingDestroy.size()));

		// ★ GPU同期（最も重要！）
		if (m_device)
		{
			Utils::log_info("Waiting for GPU before deleting objects...");
			m_device->waitForGpu();
			Utils::log_info("GPU synchronized - safe to delete resources");
		}

		// 削除実行
		size_t deletedCount = 0;
		for (auto* gameObject : m_pendingDestroy)
		{
			// 念のため選択チェック
			if (m_selectedObject == gameObject)
			{
				m_selectedObject = nullptr;
			}

			// リストから検索
			auto it = std::find_if(m_gameObjects.begin(), m_gameObjects.end(),
				[gameObject](const auto& obj) { return obj.get() == gameObject; });

			if (it != m_gameObjects.end())
			{
				std::string name = (*it)->getName();

				Utils::log_info(std::format("  [{}] Deleting GameObject '{}'", deletedCount + 1, name));

				m_gameObjects.erase(it);

				Utils::log_info(std::format("  [{}] GameObject '{}' deleted successfully", deletedCount + 1, name));
				deletedCount++;
			}
			else
			{
				Utils::log_warning("GameObject not found in list (already deleted?)");
			}
		}

		m_pendingDestroy.clear();
		Utils::log_info(std::format("=== All {} pending deletions completed ===", deletedCount));
	}

	void Scene::setSelectedObject(Core::GameObject* object)
	{
		// 破棄済みは選択不可
		if (object && object->isDestroyed())
		{
			Utils::log_warning("Attempted to select destroyed object");
			return;
		}

		m_selectedObject = object;

		if (object)
		{
			Utils::log_info(std::format("Selected object: {}", object->getName()));
		}
		else
		{
			Utils::log_info("Selection cleared");
		}
	}

	Core::GameObject* Scene::getSelectedObject() const
	{
		// 念のため安全チェック
		if (m_selectedObject && m_selectedObject->isDestroyed())
		{
			return nullptr;
		}

		return m_selectedObject;
	}

	Core::GameObject* Scene::findGameObjectById(Core::GameObject::ObjectID id) const
	{
		for (const auto& obj : m_gameObjects)
		{
			if (obj && !obj->isDestroyed() && obj->getId() == id)
				return obj.get();
		}
		return nullptr;
	}


	void Scene::clearSelection()
	{
		Utils::log_info("Scene::clearSelection() called");
		m_selectedObject = nullptr;
	}
}
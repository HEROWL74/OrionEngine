#include "Scene.hpp"

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
		if (!gameObject)
		{
			Utils::log_warning("Attempted to destroy null GameObject");
			return;
		}

		// すぐに削除せず、リストに追加
		m_pendingDestroy.push_back(gameObject);
		gameObject->setActive(false);  // すぐに無効化

		Utils::log_info(std::format("GameObject '{}' marked for destruction", gameObject->getName()));
	}

	Core::GameObject* Scene::findGameObject(const std::string& name) const
	{
		for (const auto& gameObject : m_gameObjects)
		{
			if (gameObject->getName() == name)
			{
				return gameObject.get();
			}
		}
		return nullptr;
	}

	void Scene::start()
	{
		for (auto& gameObject : m_gameObjects)
		{
			if (gameObject->isActive())
			{
				gameObject->start();
			}
		}
	}

	void Scene::update(float deltaTime)
	{
		for (auto& gameObject : m_gameObjects)
		{
			if (gameObject->isActive())
			{
				gameObject->update(deltaTime);
			}
		}

		// フレームの最後に遅延削除を実行
		processPendingDestroy();
	}

	void Scene::lateUpdate(float deltaTime)
	{
		for (auto& gameObject : m_gameObjects)
		{
			if (gameObject->isActive())
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
			if (gameObject->isActive())
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
			if (obj && obj->getName() == name)
				return obj.get();
		}
		return nullptr;
	}

	void Scene::processPendingDestroy()
	{
		if (m_pendingDestroy.empty())
		{
			return;
		}

		// GPU同期
		if (m_device)
		{
			m_device->waitForGpu();
		}

		// 削除実行
		for (auto* gameObject : m_pendingDestroy)
		{
			auto it = std::find_if(m_gameObjects.begin(), m_gameObjects.end(),
				[gameObject](const auto& obj) { return obj.get() == gameObject; });

			if (it != m_gameObjects.end())
			{
				std::string name = (*it)->getName();
				(*it)->destroy();
				m_gameObjects.erase(it);
				Utils::log_info(std::format("GameObject '{}' destroyed", name));
			}
		}

		m_pendingDestroy.clear();
	}
}
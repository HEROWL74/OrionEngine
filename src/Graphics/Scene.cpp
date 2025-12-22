#include "Scene.hpp"

namespace Engine::Graphics
{
	//==========================================================================================
//Scene実装
//==========================================================================================
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

		std::string objectName = gameObject->getName();

		auto it = m_gameObjects.begin();
		while (it != m_gameObjects.end())
		{
			if (it->get() == gameObject)
			{
				(*it)->setActive(false);

				(*it)->destroy();

				it = m_gameObjects.erase(it);

				Utils::log_info(std::format("GameObject '{}' destroyed successfully", objectName));
				return;
			}
			else
			{
				++it;
			}
		}

		Utils::log_warning(std::format("GameObject '{}' not found in scene", objectName));
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
}

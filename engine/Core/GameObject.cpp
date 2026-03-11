//src/Core/GameObject.cpp
#include "GameObject.hpp"
#include "EntityRegistry.hpp"
#include <algorithm>
#include "../Scripting/IScript.hpp"
#include "../Scripting/LuaScriptComponent.hpp"

namespace Engine::Core
{
	//============================================
	//GameObject
	//============================================

	static EntityRegistry s_registry;

	GameObject::GameObject(const std::string& name)
		:m_name(name)
		,m_id(s_registry.create())
	{
		//
		m_transform = addComponent<Transform>();
	}

	GameObject::~GameObject()
	{
		if (s_registry.isAlive(m_id))
		{
			s_registry.destroy(m_id);
		}
	}

	void GameObject::start()
	{
		if (m_started || !m_active) return;

		//
		for (auto& [type, component] : m_components)
		{
			if (component->isEnabled())
			{
				component->start();
			}
		}

		//
		for (auto& child : m_children)
		{
			if (child->isActive())
			{
				child->start();
			}
		}

		m_started = true;
	}

	void GameObject::update(float deltaTime)
	{
		if (!m_active) return;

		//
		if (!m_started)
		{
			start();
		}

		//
		for (auto& [type, component] : m_components)
		{
			if (component->isEnabled())
			{
				component->update(deltaTime);
			}
		}


		//
		for (auto& child : m_children)
		{
			if (child->isActive())
			{
				child->update(deltaTime);
			}
		}
	}

	void GameObject::lateUpdate(float deltaTime)
	{
		if (!m_active) return;

		//
		for (auto& [type, component] : m_components)
		{
			if (component->isEnabled())
			{
				component->lateUpdate(deltaTime);
			}
		}

		//
		for (auto& child : m_children)
		{
			if (child->isActive())
			{
				child->lateUpdate(deltaTime);
			}
		}
	}

	void GameObject::destroy()
	{
		if (m_destroyed) return;
		m_destroyed = true;

		m_active = false;

		for (auto& [type, component] : m_components)
		{
			if (component)
			{
				component->setEnabled(false);
				component->onDestroy();
			}
		}
	}

	bool GameObject::hasComponent(std::type_index type) const
	{
		return m_components.find(type) != m_components.end();
	}

	void GameObject::addChild(std::unique_ptr<GameObject> child)
	{
		if (child)
		{
			child->m_parent = this;
			m_children.push_back(std::move(child));
		}
	}

	void GameObject::removeChild(GameObject* child)
	{
		auto it = std::find_if(m_children.begin(), m_children.end(),
			[child](const std::unique_ptr<GameObject>& ptr)
			{
				return ptr.get() == child;
			});

		if (it != m_children.end())
		{
			this->m_parent = nullptr;
			m_children.erase(it);
		}
	}

	GameObject* GameObject::findChild(const std::string& name) const
	{
		for (const auto& child : m_children)
		{
			if (child->getName() == name)
			{
				return child.get();
			}
		}

		return nullptr;
	}

} 


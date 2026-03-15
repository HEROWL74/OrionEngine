//src/Core/GameObject.hpp
#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <typeindex>  //ECSでよく使う
#include "EntityID.hpp"
#include "TransformStorage.hpp"
#include "../Math/Math.hpp"
#include "../Utils/Common.hpp"

namespace Engine::Core
{
	// 前方宣言
	class GameObject;
	class Component;
	class Transform;

	//==========================================================
	//Component class
	//==========================================================
	class Component
	{
	public:
		Component() = default;
		virtual ~Component() = default;

		// コピー&ムーブ禁止
		Component(const Component&) = delete;
		Component& operator=(const Component&) = delete;
		Component(Component&&) = delete;
		Component& operator=(Component&&) = delete;

		// ライフサイクル
		virtual void start() {}                    // start
		virtual void update(float deltaTime) {}    // update
		virtual void lateUpdate(float deltaTime) {}// late Update
		virtual void onDestroy() {}                // onDestroy

		// GameObjectゲッター
		GameObject* getGameObject() const { return m_gameObject; }

		// 有効チェックと有効セット
		bool isEnabled() const { return m_enabled; }
		void setEnabled(bool enabled) { m_enabled = enabled; }
		void setGameObject(GameObject* go) { m_gameObject = go; }
	protected:
		GameObject* m_gameObject = nullptr;
		bool m_enabled = true;

		friend class GameObject;
	};

	//=========================================================
	// Transform
	//=========================================================
	class Transform : public Component
	{
	public:
		Transform() = default;
		~Transform() = default;

		void bind(TransformStorage* storage, EntityID id)
		{
			m_storage = storage;
			m_id = id;
		}

		// Object Transform getter
		const Math::Vector3& getPosition() const { return m_storage->getPosition(m_id); }
		const Math::Vector3& getRotation() const { return m_storage->getRotation(m_id); }
		const Math::Vector3& getScale() const { return m_storage->getScale(m_id); }
		const Math::Matrix4& getWorldMatrix() { return m_storage->getWorldMatrix(m_id); }


		void setPosition(const Math::Vector3& v) { m_storage->setPosition(m_id, v); }
		void setRotation(const Math::Vector3& v) { m_storage->setRotation(m_id, v); }
		void setScale(const Math::Vector3& v) { m_storage->setScale(m_id, v); }

		// translate rotate
		void translate(const Math::Vector3& delta) { m_storage->translate(m_id, delta); }
		void rotate(const Math::Vector3& delta) { m_storage->rotate(m_id, delta); }

		Math::Vector3 getForward() const
		{
			const auto& rot = getRotation();
			float pitch = Math::radians(rot.x);
			float yaw = Math::radians(rot.y);
			return Math::Vector3(
				std::sin(yaw) * std::cos(pitch),
				-std::sin(pitch),
				std::cos(yaw) * std::cos(pitch)
			).normalized();
		}
		Math::Vector3 getRight() const
		{
			return Math::Vector3::cross(getForward(), Math::Vector3::up()).normalized();
		}
		Math::Vector3 getUp() const
		{
			return Math::Vector3::cross(getRight(), getForward());
		}

	private:
		TransformStorage* m_storage = nullptr;
		EntityID m_id = INVALID_ENTITY;
	};

	//=========================================================
	// GameObject class
	//=========================================================
	class GameObject
	{
	public:
		explicit GameObject(const std::string& name = "GameObject");

		~GameObject();

		// コピー&ムーブ禁止
		GameObject(const GameObject&) = delete;
		GameObject& operator=(const GameObject&) = delete;
		GameObject(GameObject&&) = delete;
		GameObject& operator=(GameObject&&) = delete;

		// Name getter & setter
		const std::string& getName() const { return m_name; }
		void setName(const std::string& name) { m_name = name; }

		bool isActive() const { return m_active; }
		void setActive(bool active) { m_active = active; }

		EntityID getId() const { return m_id; }

		//Transform getter
		Transform* getTransform() const { return m_transform; }

		void bindTransformStorage(TransformStorage* storage)
		{
			if (m_transform)
				m_transform->bind(storage, m_id);
		}

		template<typename T, typename... Args>
		T* addComponent(Args&&... args);

		template<typename T>
		T* getComponent() const;

		template<typename T>
		void removeComponent();

		bool hasComponent(std::type_index type) const;

		template<typename T>
		bool hasComponent() const { return hasComponent(std::type_index(typeid(T))); }

		// ライフサイクル
		void start();
		void update(float deltaTime);
		void lateUpdate(float deltaTime);
		void destroy();

		void addChild(std::unique_ptr<GameObject> child);
		void removeChild(GameObject* child);
		GameObject* findChild(const std::string& name) const;
		const std::vector<std::unique_ptr<GameObject>>& getChildren() const { return m_children; }

		GameObject* getParent() const { return m_parent; }
		bool isDestroyed() const { return m_destroyed; }
	private:
		std::string m_name;
		bool m_active = true;
		bool m_started = false;
		bool m_destroyed = false;

		EntityID m_id;

		Transform* m_transform = nullptr;

		std::unordered_map<std::type_index, std::unique_ptr<Component>> m_components;

		GameObject* m_parent = nullptr;
		std::vector<std::unique_ptr<GameObject>> m_children;
	};

	//=============================================================
	//addComponent
	//=============================================================
	template<typename T, typename... Args>
	T* GameObject::addComponent(Args&&... args)
	{
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");

		std::type_index typeIndex(typeid(T));

		auto it = m_components.find(typeIndex);
		if (it != m_components.end())
		{
			return static_cast<T*>(it->second.get());
		}

		T* rawPtr = new T(std::forward<Args>(args)...);
		rawPtr->m_gameObject = this;

		m_components[typeIndex] = std::unique_ptr<Component>(rawPtr);

		return rawPtr;
	}

	template<typename T>
	T* GameObject::getComponent() const
	{
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");

		std::type_index typeIndex(typeid(T));
		auto it = m_components.find(typeIndex);

		if (it != m_components.end())
		{
			return static_cast<T*>(it->second.get());
		}

		return nullptr;
	}

	template<typename T>
	void GameObject::removeComponent()
	{
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");

		std::type_index typeIndex(typeid(T));
		auto it = m_components.find(typeIndex);

		if (it != m_components.end())
		{
			it->second->onDestroy();
			m_components.erase(it);
		}
	}
}





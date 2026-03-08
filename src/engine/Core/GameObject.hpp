//src/Core/GameObject.hpp
#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <typeindex>  //ECSでよく使う
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

	protected:
		GameObject* m_gameObject = nullptr;
		bool m_enabled = true;

		friend class GameObject;
	};

	//=========================================================
	//Transformclass
	//=========================================================
	class Transform : public Component
	{
	public:
		Transform() = default;
		~Transform() = default;

		// Object Transform getter
		const Math::Vector3& getPosition() const { return m_position; }
		const Math::Vector3& getRotation() const { return m_rotation; }
		const Math::Vector3& getScale() const { return m_scale; }

		void setPosition(const Math::Vector3& position) { m_position = position; m_isDirty = true; }
		void setRotation(const Math::Vector3& rotation) { m_rotation = rotation; m_isDirty = true; }
		void setScale(const Math::Vector3& scale) { m_scale = scale; m_isDirty = true; }

		// translate rotate
		void translate(const Math::Vector3& transition) { m_position += transition; m_isDirty = true; }
		void rotate(const Math::Vector3& rotation) { m_rotation += rotation; m_isDirty = true; }

		const Math::Matrix4& getWorldMatrix();

		Math::Vector3 getForward() const;
		Math::Vector3 getRight() const;
		Math::Vector3 getUp() const;
	private:
		Math::Vector3 m_position = Math::Vector3::zero();
		Math::Vector3 m_rotation = Math::Vector3::zero();
		Math::Vector3 m_scale = Math::Vector3::one();

		mutable Math::Matrix4 m_worldMatrix;
		mutable bool m_isDirty = true;

		void updateWorldMatrix() const;
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

		using ObjectID = uint64_t;

		ObjectID getId() const { return m_id; }

		//Transform getter
		Transform* getTransform() const { return m_transform; }

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

		ObjectID m_id;

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





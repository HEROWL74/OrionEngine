//src/Core/GameObject.hpp
#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <typeindex>  //ECS縺ｧ繧医￥菴ｿ縺・
#include "../Math/Math.hpp"
#include "../Utils/Common.hpp"

namespace Engine::Core
{
	//蜑肴婿螳｣險
	class GameObject;
	class Component;
	class Transform;

	//==========================================================
	//Component繝吶・繧ｹ繧ｯ繝ｩ繧ｹ
	//==========================================================
	class Component
	{
	public:
		Component() = default;
		virtual ~Component() = default;

		//繧ｳ繝斐・繝ｻ繝繝ｼ繝也ｦ∵ｭ｢
		Component(const Component&) = delete;
		Component& operator=(const Component&) = delete;
		Component(Component&&) = delete;
		Component& operator=(Component&&) = delete;

		//繝ｩ繧､繝輔し繧､繧ｯ繝ｫ
		virtual void start() {}                    //蛻晄悄蛹悶・譎ゅ↓荳蠎ｦ縺縺大他縺ｶ
		virtual void update(float deltaTime) {}    //豈弱ヵ繝ｬ繝ｼ繝蜻ｼ縺ｶ
		virtual void lateUpdate(float deltaTime) {}//update縺ｨ縺ゅ→縺ｫ蜻ｼ縺ｶ
		virtual void onDestroy() {}                //遐ｴ譽・凾縺ｫ蜻ｼ縺ｶ

		//繧ｲ繝ｼ繝繧ｪ繝悶ず繧ｧ繧ｯ繝医・蜿門ｾ・
		GameObject* getGameObject() const { return m_gameObject; }

		//譛牙柑縺狗┌蜉ｹ縺・
		bool isEnabled() const { return m_enabled; }
		void setEnabled(bool enabled) { m_enabled = enabled; }

	protected:
		GameObject* m_gameObject = nullptr;
		bool m_enabled = true;

		friend class GameObject;
	};

	//=========================================================
	//Transform繧ｳ繝ｳ繝昴・繝阪Φ繝茨ｼ亥ｿ・医さ繝ｳ繝昴・繝阪Φ繝茨ｼ・
	//=========================================================
	class Transform : public Component
	{
	public:
		Transform() = default;
		~Transform() = default;

		//菴咲ｽｮ繝ｻ蝗櫁ｻ｢繝ｻ繧ｹ繧ｱ繝ｼ繝ｫ
		const Math::Vector3& getPosition() const { return m_position; }
		const Math::Vector3& getRotation() const { return m_rotation; }
		const Math::Vector3& getScale() const { return m_scale; }

		void setPosition(const Math::Vector3& position) { m_position = position; m_isDirty = true; }
		void setRotation(const Math::Vector3& rotation) { m_rotation = rotation; m_isDirty = true; }
		void setScale(const Math::Vector3& scale) { m_scale = scale; m_isDirty = true; }

		//遘ｻ蜍輔・蝗櫁ｻ｢
		void translate(const Math::Vector3& transition) { m_position += transition; m_isDirty = true; }
		void rotate(const Math::Vector3& rotation) { m_rotation += rotation; m_isDirty = true; }

		//繝ｯ繝ｼ繝ｫ繝芽｡悟・縺ｮ蜿門ｾ・
		const Math::Matrix4& getWorldMatrix();

		//譁ｹ蜷代・繧ｯ繝医Ν
		Math::Vector3 getForward() const;
		Math::Vector3 getRight() const;
		Math::Vector3 getUp() const;
	private:
		Math::Vector3 m_position = Math::Vector3::zero();
		Math::Vector3 m_rotation = Math::Vector3::zero();
		Math::Vector3 m_scale = Math::Vector3::one();

		mutable Math::Matrix4 m_worldMatrix;
		mutable bool m_isDirty = true; //isDirty縺ｨ譖ｸ縺・◆縺ｮ縺ｯ縲∫憾諷九′譛譁ｰ縺倥ｃ縺ｪ縺・ｴ蜷医↓蜃ｦ逅・ｒ霑ｽ蜉縺吶ｋ縺溘ａ

		void updateWorldMatrix() const;
	};

	//=========================================================
	//繧ｲ繝ｼ繝繧ｪ繝悶ず繧ｧ繧ｯ繝医け繝ｩ繧ｹ
	//=========================================================
	class GameObject
	{
	public:
		explicit GameObject(const std::string& name = "GameObject");

		~GameObject();

		//繧ｳ繝斐・繝ｻ繝繝ｼ繝也ｦ∵ｭ｢
		GameObject(const GameObject&) = delete;
		GameObject& operator=(const GameObject&) = delete;
		GameObject(GameObject&&) = delete;
		GameObject& operator=(GameObject&&) = delete;

		//蝓ｺ譛ｬ諠・ｱ
		const std::string& getName() const { return m_name; }
		void setName(const std::string& name) { m_name = name; }

		bool isActive() const { return m_active; }
		void setActive(bool active) { m_active = active; }

		using ObjectID = uint64_t;

		ObjectID getId() const { return m_id; }

		//Transform・亥ｿ・医・繧ｳ繝ｳ繝昴・繝阪Φ繝茨ｼ・
		Transform* getTransform() const { return m_transform; }

		//繧ｳ繝ｳ繝昴・繝阪Φ繝医・霑ｽ蜉繝ｻ蜿門ｾ励・蜑企勁
		template<typename T, typename... Args>
		T* addComponent(Args&&... args);

		template<typename T>
		T* getComponent() const;

		template<typename T>
		void removeComponent();

		bool hasComponent(std::type_index type) const;

		template<typename T>
		bool hasComponent() const { return hasComponent(std::type_index(typeid(T))); }

		//繝ｩ繧､繝輔し繧､繧ｯ繝ｫ
		void start();
		void update(float deltaTime);
		void lateUpdate(float deltaTime);
		void destroy();

		//蟄舌が繝悶ず繧ｧ繧ｯ繝育ｮ｡逅・
		void addChild(std::unique_ptr<GameObject> child);
		void removeChild(GameObject* child);
		GameObject* findChild(const std::string& name) const;
		const std::vector<std::unique_ptr<GameObject>>& getChildren() const { return m_children; }

		//隕ｪ繧ｪ繝悶ず繧ｧ繧ｯ繝・
		GameObject* getParent() const { return m_parent; }
		bool isDestroyed() const { return m_destroyed; }
	private:
		std::string m_name;
		bool m_active = true;
		bool m_started = false;
		bool m_destroyed = false;

		ObjectID m_id;

		//蠢・医さ繝ｳ繝昴・繝阪Φ繝・
		Transform* m_transform = nullptr;

		//繧ｳ繝ｳ繝昴・繝阪Φ繝育ｮ｡逅・
		std::unordered_map<std::type_index, std::unique_ptr<Component>> m_components;

		//髫主ｱ､讒矩
		GameObject* m_parent = nullptr;
		std::vector<std::unique_ptr<GameObject>> m_children;
	};

	//=============================================================
	//繝・Φ繝励Ξ繝ｼ繝亥ｮ溯｣・
	//=============================================================
	template<typename T, typename... Args>
	T* GameObject::addComponent(Args&&... args)
	{
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");

		std::type_index typeIndex(typeid(T));

		// 譌｢縺ｫ蟄伜惠縺吶ｋ蝣ｴ蜷医・譌｢蟄倥・繧ゅ・繧定ｿ斐☆
		auto it = m_components.find(typeIndex);
		if (it != m_components.end())
		{
			return static_cast<T*>(it->second.get());
		}

		// 譁ｰ縺励＞繧ｳ繝ｳ繝昴・繝阪Φ繝医ｒ菴懈・
		T* rawPtr = new T(std::forward<Args>(args)...);
		rawPtr->m_gameObject = this;

		// unique_ptr縺ｫ譬ｼ邏・
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





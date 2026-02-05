// src/engine/Physics/BoxCollider.hpp
#pragma once

#include "../Core/GameObject.hpp"
#include "../Math/Math.hpp"

namespace Engine::Physics
{
	class BoxCollider : public Core::Component
	{
	public:
		BoxCollider() = default;
		explicit BoxCollider(const Math::Vector3& size)
			:m_size(size) {}

		// サイズとオフセット
		void setSize(const Math::Vector3& size) { m_size = size; }
		const Math::Vector3& getSize()const { return m_size; }

		void setCenter(const Math::Vector3& center)  { m_center = center; }
		const Math::Vector3& getCenter() const { return m_center; }

		// トリガー
		void setTrigger(bool trigger) { m_isTrigger = trigger; }
		bool isTrigger()const { return m_isTrigger; }

		// ワールド空間でのAABBを取得
		void getWorldAABB(Math::Vector3& outMin, Math::Vector3& outMax)const;

		// 他のコライダーと衝突判定
		bool intersects(const BoxCollider* other);

	private:
		Math::Vector3 m_center = Math::Vector3::zero();
		Math::Vector3 m_size = Math::Vector3::one();
		bool m_isTrigger = false;
	}; 
}
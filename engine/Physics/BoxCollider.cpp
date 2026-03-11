// src/engine/Physics/BoxCollider.cpp
#include "BoxCollider.hpp"

namespace Engine::Physics
{
	void BoxCollider::getWorldAABB(Math::Vector3& outMin, Math::Vector3& outMax)const
	{
		if (!m_gameObject)
		{
			outMin = Math::Vector3::zero();
			outMax = Math::Vector3::zero();
			return;
		}

		auto* transform = m_gameObject->getTransform();
		if (!transform)
		{
			outMin = Math::Vector3::zero();
			outMax = Math::Vector3::zero();
			return;
		}

		Math::Vector3 worldCenter = transform->getPosition() + m_center;

		Math::Vector3 scale = transform->getScale();
		Math::Vector3 halfSize(
			m_size.x * scale.x * 0.5f,
			m_size.y * scale.y * 0.5f,
			m_size.z * scale.z * 0.5f
		);

		outMin = Math::Vector3(
			worldCenter.x - halfSize.x,
			worldCenter.y - halfSize.y,
			worldCenter.z - halfSize.z
		);

		outMax = Math::Vector3(
			worldCenter.x + halfSize.x,
			worldCenter.y + halfSize.y,
			worldCenter.z + halfSize.z
		);
	}

	bool BoxCollider::intersects(const BoxCollider* other)
	{
		if (!other || !m_gameObject || !other->getGameObject())
		{
			return false;
		}

		Math::Vector3 minA, maxA, minB, maxB;
		getWorldAABB(minA, maxA);
		other->getWorldAABB(minB, maxB);

		return (minA.x <= maxB.x && maxA.x >= minB.x) &&
			(minA.y <= maxB.y && maxA.y >= minB.y) &&
			(minA.z <= maxB.z && maxA.z >= minB.z);
	}
}


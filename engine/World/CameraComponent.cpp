// engine/World/CameraComponent.cpp
#include "CameraComponent.hpp"

namespace Engine::World
{
	void CameraComponent::update(float deltaTime)
	{
		syncFromTransform();
	}

	void CameraComponent::syncFromTransform()
	{
		auto* go = getGameObject();
		if (!go) return;

		auto* t = go->getTransform();
		if (!t) return;

		m_camera.setPosition(t->getPosition());
		m_camera.setRotation(t->getRotation());
	}
}
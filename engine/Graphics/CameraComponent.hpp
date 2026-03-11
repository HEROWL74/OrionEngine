// src/engine/Graphics/CameraComponent.hpp
#pragma once

#include "../Core/GameObject.hpp"
#include "Camera.hpp"

namespace Engine::Graphics
{
	// ======================================
	// CameraComponent
	// ======================================
	class CameraComponent : public Core::Component
	{
	public:
		CameraComponent() = default;
		~CameraComponent() override = default;

		// ライフサイクル
		void start() override {};
		void update(float deltaTime) override;

		// 内部Cameraへのアクセス
		Camera& getCamera() { return m_camera; }
		const Camera& getCamera() const { return m_camera; }

		// 投影設定
		void setPerspective(float fov, float aspect, float nearPlane, float farPlane)
		{
			m_camera.setPerspective(fov, aspect, nearPlane, farPlane);
		}

		void updateAspect(float aspect)
		{
			m_camera.updateAspect(aspect);
		}

		void syncFromTransform();

		// プロパティ
		float getFov() const { return m_camera.getFov(); }
		float getNearPlane() const { return m_camera.getNearPlane(); }
		float getFarPlane() const { return m_camera.getFarPlane(); }
		float getAspect() const { return m_camera.getAspect(); }
		
		// セッター類
		void setFov(float fov)
		{
			m_camera.setPerspective(fov, m_camera.getAspect(), m_camera.getNearPlane(), m_camera.getFarPlane());
		}

		void setNearPlane(float nearPlane)
		{
			m_camera.setPerspective(m_camera.getFov(), m_camera.getAspect(), nearPlane, m_camera.getFarPlane());
		}

		void setFarPlane(float farPlane)
		{
			m_camera.setPerspective(m_camera.getFov(), m_camera.getAspect(), m_camera.getNearPlane(), farPlane);
		}
	private:
		Camera m_camera;
	};
}
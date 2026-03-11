#pragma once
#include "ImGuiManager.hpp"
#include "../Views/EditorView.hpp"
#include "../Views/Gizmo.hpp"
#include "../engine/Input/InputManager.hpp"
#include "../engine/Graphics/Scene.hpp"

namespace Editor::UI
{
	class EditorViewWindow
	{
	public:
		void initialize(ImGuiManager* imgui, EditorView* view, Engine::Input::InputManager* inputManager);
		void draw();

		bool isFocused() const { return m_isFocused; }
		bool isHovered() const { return m_isHovered; }
		bool isCameraControlRequested() const { return m_cameraControlRequested; }

		void setCamera(Graphics::Camera* camera) { m_camera = camera; }
		void setScene(Graphics::Scene* scene) { m_scene = scene; }
		void processResize();

		// カメラ感度設定
		void setCameraSensitivity(float sensitivity) { m_cameraSensitivity = sensitivity; }
		float getCameraSensitivity() const { return m_cameraSensitivity; }

	private:
		ImGuiManager* m_imgui = nullptr;
		EditorView* m_view = nullptr;
		Graphics::Camera* m_camera = nullptr;
		Graphics::Scene* m_scene = nullptr;
		Engine::Input::InputManager* m_inputManager = nullptr;
		ImTextureID m_texture = {};

		ImVec2 m_lastSize = { 0, 0 };
		ImVec2 m_viewportPos = { 0, 0 };  // ビューポートの位置（スクリーン座標）
		bool m_needsResize = false;
		uint32_t m_pendingWidth = 0;
		uint32_t m_pendingHeight = 0;

		bool m_isFocused = false;
		bool m_isHovered = false;
		bool m_cameraControlRequested = false;

		// カメラ操作関連
		float m_cameraSensitivity = 0.15f;

		// Gizmoドラッグ関連
		bool m_isDraggingGizmo = false;
		GizmoAxis m_draggedAxis = GizmoAxis::None;
		Math::Vector3 m_dragStartObjectPosition;

		void drawOverlay();
		void startCameraControl();
		void endCameraControl();
		//void updateCameraControl();

		// オブジェクト選択
		void handleObjectSelection();

		// マウス入力処理
		void handleMouseInput();
		void getRayFromMouse(Math::Vector3& outOrigin, Math::Vector3& outDirection);

		// Gizmo操作
		void startGizmoDrag(GizmoAxis axis, const Math::Vector3& rayOrigin, const Math::Vector3& rayDirection);
		void updateGizmoDrag();
		void endGizmoDrag();
	};
}


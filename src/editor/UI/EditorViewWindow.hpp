#pragma once
#include "ImGuiManager.hpp"
#include "../Views/EditorView.hpp"

namespace Editor::UI
{
	class EditorViewWindow
	{
	public:
		void initialize(ImGuiManager* imgui, EditorView* view);
		void draw();

		bool isFocused() const { return m_isFocused; }
		bool isHovered() const { return m_isHovered; }
		bool isCameraControlRequested() const { return m_cameraControlRequested; }

		void setCamera(Graphics::Camera* camera) { m_camera = camera; }
		void processResize();

	private:
		ImGuiManager* m_imgui = nullptr;
		EditorView* m_view = nullptr;
		Graphics::Camera* m_camera = nullptr;
		ImTextureID m_texture = {};

		ImVec2 m_lastSize = { 0, 0 };
		bool m_needsResize = false;
		uint32_t m_pendingWidth = 0;
		uint32_t m_pendingHeight = 0;

		bool m_isFocused = false;
		bool m_isHovered = false;
		bool m_cameraControlRequested = false;

		void drawOverlay();
	};
}
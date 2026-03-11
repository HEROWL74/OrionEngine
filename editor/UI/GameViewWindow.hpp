#pragma once
#include "ImGuiManager.hpp"
#include "../Views/GameView.hpp"

namespace Editor::UI
{
	class GameViewWindow
	{
	public:
		void initialize(ImGuiManager* imgui, GameView* view);
		void draw();
		void setCamera(World::Camera* camera) { m_camera = camera; }
		void processResize();

	private:
		ImGuiManager* m_imgui = nullptr;
		GameView* m_view = nullptr;
		World::Camera* m_camera = nullptr;
		ImTextureID m_texture = {};

		ImVec2 m_lastSize = { 0, 0 };
		bool m_needsResize = false;
		uint32_t m_pendingWidth = 0;
		uint32_t m_pendingHeight = 0;
	};
}


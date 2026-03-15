#pragma once
#include "ImGuiManager.hpp"
#include "../Views/GameView.hpp"
#include "../engine/World/CameraComponent.hpp"
#include "../engine/Core/GameObject.hpp"

namespace Editor::UI
{
	class GameViewWindow
	{
	public:
		void initialize(ImGuiManager* imgui, GameView* view);
		void draw();

		// GameCameraのGameObjectを受け取ることで、リサイズ時にCameraComponentにもアスペクトを伝える
		void setGameCameraObject(Engine::Core::GameObject* go) { m_gameCameraObject = go; }

		void processResize();

	private:
		ImGuiManager* m_imgui = nullptr;
		GameView* m_view = nullptr;
		// GameCameraがCameraComponentを持つGameObjectの場合、リサイズ時に同期するための参照
		Engine::Core::GameObject* m_gameCameraObject = nullptr;
		ImTextureID m_texture = {};

		ImVec2 m_lastSize = { 0, 0 };
		bool m_needsResize = false;
		uint32_t m_pendingWidth = 0;
		uint32_t m_pendingHeight = 0;
	};
}
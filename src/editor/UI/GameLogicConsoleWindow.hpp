// src/editor/UI/GameLogicConsoleWindow.hpp
#pragma once
#include "engine/Utils/Common.hpp"
#include "ImGuiManager.hpp"

namespace Editor::UI
{
	using namespace Engine;
	class GameObject;
	struct GameLogicType
	{
		GameObject* gameObject = nullptr;
		float ms = 0.0f;
	};

	class GameLogicConsoleWindow
	{
		GameLogicConsoleWindow();
		~GameLogicConsoleWindow() = default;

	private:

	};
}
// src/runtime/GameApp.cpp
#include "GameApp.hpp"
#include "engine/Core/Window.hpp"

namespace Runtime
{
	static Engine::Core::Window g_window;

	Engine::Utils::VoidResult GameApp::initialize(HINSTANCE hInstance, int nCmdShow)
	{
		Engine::Core::WindowSettings settings{};
		settings.title = L"Orion Game";
		settings.width = 1280;
		settings.height = 720;
		settings.resizable = true;

		auto result = g_window.create(hInstance, settings);
		if (!result)return result;

		g_window.setCloseCallback([this]()
			{
				m_running = false;
			});


		g_window.show(nCmdShow);
		return {};
	}

	int GameApp::run()
	{
		while (m_running)
		{
			if (!g_window.processMessages())
				break;

			Sleep(1);
		}
		return 0;
	}

}
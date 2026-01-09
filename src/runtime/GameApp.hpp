// src/runtime/GameApp.hpp
#pragma once

#include <Windows.h>
#include "engine/Utils/Common.hpp"

namespace Runtime
{
	class GameApp
	{
	public:
		Engine::Utils::VoidResult initialize(HINSTANCE hInstance, int nCmdShow);
		int run();

	private:
		bool m_running = true;
	};
}
// src/engine/Core/Window/WindowFactory.hpp
#pragma once
#include "IWindow.hpp"
#include <memory>

#ifdef _WIN32
#include "Window_Windows.hpp"
#else 
#include "Window_Linux.hpp"
#endif

namespace Engine::Window
{
	class WindowFactory
	{
	public:
		[[nodiscard]] static std::unique_ptr<IWindow> Create()
		{
#ifdef _WIN32
			return std::make_unique<Window_Windows>();
#else
			return std::make_unique<Window_Linux>();
#endif
		}

		// コンストラクタ禁止
		WindowFactory() = delete;
	};
}
// src/editor/LinuxApp.hpp
#pragma once

#ifndef _WIN32

#include <filesystem>
#include <memory>
#include "../engine/Core/Window/IWindow.hpp"

namespace Editor
{
	// LinuxApp
	// Linux 向けの最小アプリクラス

	class LinuxApp
	{
	public:
		LinuxApp() = default;
		~LinuxApp() = default;

		LinuxApp(const LinuxApp&) = delete;
		LinuxApp& operator=(const LinuxApp&) = delete;

		// ウィンドウ生成
		[[nodiscard]] bool initialize(const std::filesystem::path& projectPath = {});

		// メインループ 0 = 正常終了
		[[nodiscard]] int run();

	private:
		std::unique_ptr<Engine::Window::IWindow> m_window;

		void update(float deltaTime);
		void render();
	};
}

#endif
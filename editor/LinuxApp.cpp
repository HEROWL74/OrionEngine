// editor/LinuxApp.cpp

#ifndef _WIN32

#include "LinuxApp.hpp"
#include "../engine/Core/Window/WindowFactory.hpp"

#include <chrono>
#include <cstdio>

namespace Editor
{
	// =======================================
	// Initialize
	// =======================================
	bool LinuxApp::initialize(const std::filesystem::path& projectPath)
	{
		std::fprintf(stdout, "[LinuxApp] Initializing...\n");

		// ウィンドウ作成
		m_window = Engine::Window::WindowFactory::Create();

		Engine::Window::WindowDesc desc;
		desc.width = 1280;
		desc.height = 720;
		desc.title = "OrionEngine (Linux)";
		desc.resizable = true;

		if (!m_window->Create(desc))
		{
			std::fprintf(stderr, "[LinuxApp] Failed to create window\n");
			return false;
		}

		// Callback 登録
		m_window->SetResizeCallback([](uint32_t w, uint32_t h) {
			std::fprintf(stdout, "[LinuxApp] Window resized: %u x %u\n", w, h);
		});

		m_window->SetDropCallback([](const std::vector<std::filesystem::path>& paths)
			{
				for (const auto& p : paths)
				{
					std::fprintf(stdout, "[LinuxApp] Dropped: %s\n", p.string().c_str());
				}
			});

		std::fprintf(stdout, "[LinuxApp] Window created: %u x %u\n",
			m_window->GetWidth(), m_window->GetHeight());

		std::fprintf(stdout, "[LinuxApp] Initialization complete\n");
		return true;
	}

	int LinuxApp::run()
	{
		std::fprintf(stdout, "[LinuxApp] Entering main loop\n");

		using Clock = std::chrono::high_resolution_clock;
		using FloatSecs = std::chrono::duration<float>;

		auto lastTime = Clock::now();

		while (!m_window->ShouldClose())
		{
			m_window->PollEvents();

			auto now = Clock::now();
			float deltaTime = std::chrono::duration_cast<FloatSecs>(now - lastTime).count();
			lastTime = now;

			// ゲームロジック更新
			update(deltaTime);

			// 描画
			render();
		}

		std::fprintf(stdout, "[LinuxApp] Main loop exited\n");

		// クリーンアップ
		m_window->Destroy();

		return 0;
	}

	void LinuxApp::update(float deltaTime)
	{
		(void)deltaTime;
	}

	void LinuxApp::render()
	{

	}
}

#endif
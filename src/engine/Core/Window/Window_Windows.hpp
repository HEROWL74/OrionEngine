//src/engine/Core/Window/Window_Windows.hpp
#pragma once
#include "IWindow.hpp"

#ifdef _WIN32
#include <Windows.h>

namespace Engine::Window
{
	class Window_Windows final : public IWindow
	{
	public:
		Window_Windows() = default;
		~Window_Windows() override = default;

		bool Create(const WindowDesc& desc) override;
		void Destroy() override;

		void PollEvents() override;
		bool ShouldClose() const override;

		uint32_t GetWidth() const override;
		uint32_t GetHeight()const override;

		void* GetNativeHandle() const override;

		void SetTitle(const std::string& title) override;

	private:
		HWND m_hwnd;
		uint32_t m_width = 0;
		uint32_t m_height = 0;
		bool m_shouldClose = false;
	};
}
#endif


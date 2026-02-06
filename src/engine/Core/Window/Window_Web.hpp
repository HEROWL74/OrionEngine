// src/engine/Core/Window/Window_Web.hpp
#pragma once
#include "IWindow.hpp"

namespace Engine::Window
{
	class Window_Web final : public IWindow
	{
	public:
		Window_Web() = default;
		~Window_Web() override = default;

		bool Create(const WindowDesc& desc) override;
		void Destroy() override;

		void PollEvents() override;
		bool ShouldClose() const override;

		uint32_t GetWidth() const override;
		uint32_t GetHeight()const override;

		void* GetNativeHandle() const override;

		void SetTitle(const std::string& title) override;

	private:
		uint32_t m_width = 0;
		uint32_t m_height = 0;
		bool m_shouldClose = false;
	};
}

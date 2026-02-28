// src/engine/Core/Window/Window_Window.cpp
#include "Window_Windows.hpp"

#ifdef _WIN32

namespace Engine::Window
{
	bool Window_Windows::Create(const WindowDesc& desc)
	{
		m_width = desc.width;
		m_height = desc.height;
		m_shouldClose = false;

		return true;
	}
	void Window_Windows::Destroy()
	{
		m_hwnd = nullptr;
	}

	void Window_Windows::PollEvents()
	{
		// TODO: PeekMessage / DispatchMessage
	}
	bool Window_Windows::ShouldClose() const
	{
		return m_shouldClose;
	}

	uint32_t Window_Windows::GetWidth() const
	{
		return m_width;
	}
	uint32_t Window_Windows::GetHeight()const
	{
		return m_height;
	}

	void* Window_Windows::GetNativeHandle() const
	{
		return m_hwnd;
	}

	void Window_Windows::SetTitle(const std::string& title)
	{
		if (m_hwnd)
		{
			SetWindowTextA(m_hwnd, title.c_str());
		}
	}
}

#endif


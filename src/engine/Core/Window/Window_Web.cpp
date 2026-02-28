// src/engine/Core/Window/Window_Web.cpp
#include "Window_Web.hpp"

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
namespace Engine::Window
{
	bool Window_Web::Create(const WindowDesc& desc)
	{
		m_width = desc.width;
		m_height = desc.height;
		m_shouldClose = false;

		return true;
	}
	void Window_Web::Destroy()
	{
	}

	void Window_Web::PollEvents()
	{
	}
	bool Window_Web::ShouldClose() const
	{
		return m_shouldClose;
	}

	uint32_t Window_Web::GetWidth() const
	{
		return m_width;
	}
	uint32_t Window_Web::GetHeight()const
	{
		return m_height;
	}

	void* Window_Web::GetNativeHandle() const
	{
		return nullptr;
	}

	void Window_Web::SetTitle(const std::string& title)
	{

	}
}

#endif


// src/engine/Core/Window/IWindow.hpp
#pragma once
#include <cstdint>
#include <string>

namespace Engine::Window
{
	struct WindowDesc
	{
		uint32_t width = 1280;
		uint32_t height = 720;
		std::string title = "OrionEngine";
		bool resizable = true;
	};

	class IWindow
	{
	public:
		virtual ~IWindow() = default;

		// Initialize / Shutdown
		virtual bool Create(const WindowDesc& desc) = 0;
		virtual void Destroy() = 0;

		// Every Frame
		virtual void PollEvents() = 0;
		virtual bool ShouldClose()const = 0;

		// GetSize
		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		// Platform Native Handle
		virtual void* GetNativeHandle() const = 0;

		// Utility
		virtual void SetTitle(const std::string& title) = 0;
	};
}
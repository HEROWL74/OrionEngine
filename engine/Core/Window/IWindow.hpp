// engine/Core/Window/IWindow.hpp
#pragma once
#include <cstdint>
#include <string>
#include <functional>
#include <filesystem>
#include <vector>

namespace Engine::Window
{
	struct WindowDesc
	{
		uint32_t width = 1280;
		uint32_t height = 720;
		std::string title = "OrionEngine";
		bool resizable = true;
		bool fullScreen = false;
		int x = -1;
		int y = -1;
	};

	// Callback型
	using ResizeCallback = std::function<void(uint32_t width, uint32_t)>;
	using CloseCallback = std::function<void()>;
	using DropCallback = std::function<void(const std::vector<std::filesystem::path>&)>;

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

		// Frame buffer size
		virtual uint32_t GetFramebufferWidth() const { return GetWidth(); }
		virtual uint32_t GetFramebufferHeight() const { return GetHeight(); }

		// Platform Native Handle
		virtual void* GetNativeHandle() const = 0;

		// Vulkan Surface Create function
		virtual bool CreateVulkanSurface(void* vkInstance, void* outSurface) const { return false; }

		// Utility
		virtual void SetTitle(const std::string& title) = 0;

		// Callback登録
		virtual void SetResizeCallback(ResizeCallback cb) { m_resizeCallback = std::move(cb); }
		virtual void SetCloseCallback(CloseCallback cb) { m_closeCallback = std::move(cb); }
		virtual void SetDropCallback(DropCallback cb) { m_dropCallback = std::move(cb); }
	protected:
		ResizeCallback m_resizeCallback;
		CloseCallback m_closeCallback;
		DropCallback m_dropCallback;
	};
}


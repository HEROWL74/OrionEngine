// engine/Core/Window/Window_Linux.cpp
#include "Window_Linux.hpp"

#ifndef _WIN32

#include <cstdio>
#include <cassert>

namespace Engine::Window
{
	// ====================================
	// デストラクタ
	// ====================================
	Window_Linux::~Window_Linux()
	{
		Destroy();
	}

	// ====================================
	// Create
	// ====================================
	bool Window_Linux::Create(const WindowDesc& desc)
	{
		if (!glfwInit())
		{
			fprintf(stderr, "[Window_Linux] glfwInit() failed");
			return false;
		}

		// Vulkanを使うので、OpenGL コンテキストは不要
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		// Resize 拒否
		glfwWindowHint(GLFW_RESIZABLE, desc.resizable ? GLFW_TRUE : GLFW_FALSE);

		// フルスクリーン対応
		GLFWmonitor* monitor = nullptr;
		if (desc.fullScreen)
		{
			monitor = glfwGetPrimaryMonitor();
		}

		// ウィンドウ作成
		m_window = glfwCreateWindow(
			static_cast<int>(desc.width),
			static_cast<int>(desc.height),
			desc.title.c_str(),
			monitor,
			nullptr
		);

		if (!m_window)
		{
			fprintf(stderr, "[Window_Linux] glfwCreateWindow() failed\n");
			glfwTerminate();
			return false;
		}

		// ウィンドウ位置の設定
		if (desc.x >= 0 && desc.y >= 0)
		{
			glfwSetWindowPos(m_window, desc.x, desc.y);
		}

		// ウィンドウサイズを取得
		int w = 0, h = 0;
		glfwGetWindowSize(m_window, &w, &h);
		m_width = static_cast<uint32_t>(w);
		m_height = static_cast<uint32_t>(h);

		int fbw = 0, fbh = 0;
		glfwGetFramebufferSize(m_window, &fbw, &fbh);
		m_fbWidth = static_cast<uint32_t>(fbw);
		m_fbHeight = static_cast<uint32_t>(fbh);

		// Callback登録
		glfwSetWindowUserPointer(m_window, this);

		glfwSetWindowSizeCallback(m_window, GlfwWindowSizeCallback);
		glfwSetFramebufferSizeCallback(m_window, GlfwFramebufferSizeCallback);
		glfwSetWindowCloseCallback(m_window,GlfwWindowCloseCallback);
		glfwSetDropCallback(m_window, GlfwDropCallback);

		return true;
	}

	void Window_Linux::Destroy()
	{
		if (m_window)
		{
			glfwDestroyWindow(m_window);
			m_window = nullptr;
		}

		glfwTerminate();
	}

	void Window_Linux::PollEvents()
	{
		glfwPollEvents();
	}

	bool Window_Linux::ShouldClose() const
	{
		if (!m_window) return true;
		return glfwWindowShouldClose(m_window);
	}

	uint32_t Window_Linux::GetWidth() const
	{
		return m_width;
	}
	uint32_t Window_Linux::GetHeight() const
	{
		return m_height;
	}
	uint32_t Window_Linux::GetFramebufferWidth() const
	{
		return m_fbWidth;
	}
	uint32_t Window_Linux::GetFramebufferHeight() const
	{
		return m_fbHeight;
	}

	void* Window_Linux::GetNativeHandle() const
	{
		return static_cast<void*>(m_window);
	}

	// Vulkan Surface 作成
	bool Window_Linux::CreateVulkanSurface(void* vkInstance, void* outSurface) const
	{
		if (!m_window || vkInstance || !outSurface)
		{
			fprintf(stderr, "[Window_Linux] CreateVulkanSurface: invalid argument\n");
			return false;
		}

		VkInstance instance = static_cast<VkInstance>(vkInstance);
		VkSurfaceKHR* pSurface = static_cast<VkSurfaceKHR*>(outSurface);

		VkResult result = glfwCreateWindowSurface(instance, m_window, nullptr, pSurface);
		if (result != VK_SUCCESS)
		{
			fprintf(stderr, "[Window_Linux] glfwCreateWindowSurface failed: %d\n", result);
			return false;
		}
		return true;
	}

	void Window_Linux::SetTitle(const std::string& title)
	{
		if (m_window)
		{
			glfwSetWindowTitle(m_window, title.c_str());
		}
	}
	// GLFW Callback
    void Window_Linux::GlfwWindowSizeCallback(GLFWwindow* w, int width, int height)
	{
		auto* self = static_cast<Window_Linux*>(glfwGetWindowUserPointer(w));
		if (!self) return;

		self->m_width = static_cast<uint32_t>(width);
		self->m_height = static_cast<uint32_t>(height);

		if (self->m_resizeCallback)
		{
			self->m_resizeCallback(self->m_width, self->m_height);
		}
	}

	void Window_Linux::GlfwFramebufferSizeCallback(GLFWwindow* w, int width, int height)
	{
		auto* self = static_cast<Window_Linux*>(glfwGetWindowUserPointer(w));
		if (!self) return;

		self->m_fbWidth = static_cast<uint32_t>(width);
		self->m_fbHeight = static_cast<uint32_t>(height);
	}

	void Window_Linux::GlfwWindowCloseCallback(GLFWwindow* w)
	{
		auto* self = static_cast<Window_Linux*>(glfwGetWindowUserPointer(w));
		if (!self) return;

		if (self->m_closeCallback)
		{
			self->m_closeCallback();
		}
	}
	void Window_Linux::GlfwDropCallback(GLFWwindow* w, int count, const char** paths)
	{
		auto* self = static_cast<Window_Linux*>(glfwGetWindowUserPointer(w));
		if (!self || !self->m_dropCallback) return;

		std::vector<std::filesystem::path> filePaths;
		filePaths.reserve(static_cast<size_t>(count));
		for (int i = 0; i < count; ++i)
		{
			filePaths.emplace_back(paths[i]);
		}
		self->m_dropCallback(filePaths);
	}
}
#endif
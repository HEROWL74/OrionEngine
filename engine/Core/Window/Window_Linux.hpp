// src/engine/Core/Window/Window_Linux.hpp
#pragma once
#include "IWindow.hpp"

#ifndef _WIN32
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

namespace Engine::Window
{
	// Window_Linux class
	class Window_Linux final : public IWindow
	{
	public:
		Window_Linux() = default;
		~Window_Linux() override;

		// IWindow 実装
		bool Create(const WindowDesc& desc) override;
		void Destroy() override;

		void PollEvents() override;
		bool ShouldClose() const override;


		uint32_t GetWidth() const override;
		uint32_t GetHeight() const override;
		// HiDPI対応 フレームバッファの実ピクセル数を返す
		uint32_t GetFramebufferWidth() const override;
		uint32_t GetFramebufferHeight() const override;

		void* GetNativeHandle() const override;

		// Vulkan Surface 作成
		bool CreateVulkanSurface(void* vkInstance, void* outSurface) const override;

		void SetTitle(const std::string& title) override;

		// GLFW Callback
		static void GlfwWindowSizeCallback(GLFWwindow* w, int width, int height);
		static void GlfwFramebufferSizeCallback(GLFWwindow* w, int width, int height);
		static void GlfwWindowCloseCallback(GLFWwindow* w);
		static void GlfwDropCallback(GLFWwindow* w, int count, const char** paths);

	private:
		GLFWwindow* m_window = nullptr;

		uint32_t m_width = 0;
		uint32_t m_height = 0;
		uint32_t m_fbWidth = 0; //フレームバッファ(Width)
		uint32_t m_fbHeight = 0; // フレームバッファ(Height)
	};
}
#endif
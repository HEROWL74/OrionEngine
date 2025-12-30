//GameView.hpp
#pragma once
#include <d3d12.h>
#include <memory>
#include "../Graphics/Camera.hpp"
#include "../Graphics/Scene.hpp"
#include "../Graphics/RenderComponent.hpp"
#include "../Graphics/RenderTarget.hpp"
#include "../Graphics/Device.hpp"
#include "../UI/ImGuiManager.hpp"

namespace Engine::Graphics
{
	class GameView
	{
	public:
		[[nodiscard]] Utils::VoidResult initialize(Device* device,
			uint32_t width,
			uint32_t height);

		void render(Scene& scene,
			ID3D12GraphicsCommandList* commandList,
			const Camera& camera,
			UINT frameIndex);
		void registerToImGui(UI::ImGuiManager* imgui);

		[[nodiscard]]
		ImTextureID getOutputTexture() const;

		void resize(uint32_t width, uint32_t height);

		bool isInitialized() const { return m_initialized; }

	private:
		Device* m_device = nullptr;
		UI::ImGuiManager* m_imguiManager = nullptr;
		std::unique_ptr<RenderTarget> m_renderTarget;
		bool m_initialized = false;
		uint32_t m_width = 0;
		uint32_t m_height = 0;
	};
}
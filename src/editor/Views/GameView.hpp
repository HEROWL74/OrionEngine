//GameView.hpp
#pragma once
#include <d3d12.h>
#include <memory>
#include "engine/Graphics/Camera.hpp"
#include "engine/Graphics/Scene.hpp"
#include "engine/Graphics/RenderComponent.hpp"
#include "engine/Graphics/RenderTarget.hpp"
#include "engine/Graphics/Device.hpp"
#include "engine/Graphics/Skybox.hpp"
#include "editor/UI/ImGuiManager.hpp"

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

		void setSkybox(Skybox* skybox) { m_skybox = skybox; }
	private:
		Device* m_device = nullptr;
		UI::ImGuiManager* m_imguiManager = nullptr;
		std::unique_ptr<RenderTarget> m_renderTarget;
		Skybox* m_skybox = nullptr;
		bool m_initialized = false;
		uint32_t m_width = 0;
		uint32_t m_height = 0;
	};
}
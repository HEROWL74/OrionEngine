#pragma once
#include <memory>
#include <d3d12.h>

#include "../renderer/Device.hpp"
#include "../renderer/RenderTarget.hpp"
#include "../renderer/PipelineStateCache.hpp"
#include "../engine/World/Scene.hpp"
#include "../engine/World/Camera.hpp"
#include "../renderer/Skybox.hpp"
#include "../engine/Utils/Common.hpp"
#include "../renderer/RenderContext.hpp"
#include "../engine/UI/UIComponent.hpp"
#include "../renderer/UITextRenderer.hpp"

namespace Editor::UI
{
	using namespace Engine;
	class GameView
	{
	public:
		Utils::VoidResult initialize(Renderer::Device* device, uint32_t width, uint32_t height, Renderer::PipelineStateCache* psoCache);
		void render(World::Scene& scene,
			ID3D12GraphicsCommandList* commandList,
			const World::Camera& camera,
			UINT frameIndex);

		void resize(uint32_t width, uint32_t height);

		Renderer::RenderTarget* getRenderTarget() const;

		void setPSOCache(Renderer::PipelineStateCache* psoCache) { m_psoCache = psoCache; }
		void setSkybox(Renderer::Skybox* skybox) { m_skybox = skybox; }
		void setScene(World::Scene* scene) { m_scene = scene; }
		void setUITextRenderer(Engine::EngineUI::UITextRenderer* renderer)
		{
			m_uiTextRenderer = renderer;
		}

	private:
		Renderer::Device* m_device = nullptr;
		std::unique_ptr<Renderer::RenderTarget> m_renderTarget;
		Renderer::Skybox* m_skybox = nullptr;
		Renderer::PipelineStateCache* m_psoCache;
		Engine::EngineUI::UITextRenderer* m_uiTextRenderer = nullptr;

		World::Scene* m_scene;

		uint32_t m_width = 0;
		uint32_t m_height = 0;
		bool m_initialized = false;
	};
}


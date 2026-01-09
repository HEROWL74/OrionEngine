#pragma once
#include <memory>
#include <d3d12.h>

#include "engine/Graphics/Device.hpp"
#include "engine/Graphics/RenderTarget.hpp"
#include "engine/Graphics/Scene.hpp"
#include "engine/Graphics/Camera.hpp"
#include "engine/Graphics/Skybox.hpp"
#include "engine/Utils/Common.hpp"
#include "engine/Utils/RenderContext.hpp"

namespace Editor::UI
{
	using namespace Engine;
	class GameView
	{
	public:
		Utils::VoidResult initialize(Graphics::Device* device, uint32_t width, uint32_t height);
		void render(Graphics::Scene& scene,
			ID3D12GraphicsCommandList* commandList,
			const Graphics::Camera& camera,
			UINT frameIndex);

		void resize(uint32_t width, uint32_t height);

		Graphics::RenderTarget* getRenderTarget() const;

		void setSkybox(Graphics::Skybox* skybox) { m_skybox = skybox; }

	private:
		Graphics::Device* m_device = nullptr;
		std::unique_ptr<Graphics::RenderTarget> m_renderTarget;
		Graphics::Skybox* m_skybox = nullptr;

		uint32_t m_width = 0;
		uint32_t m_height = 0;
		bool m_initialized = false;
	};
}

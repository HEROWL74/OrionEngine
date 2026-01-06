#include "GameView.hpp"

namespace Editor::UI
{
	Utils::VoidResult GameView::initialize(Graphics::Device* device, uint32_t width, uint32_t height)
	{
		CHECK_CONDITION(device, Utils::ErrorType::Unknown, "Device is null");

		m_device = device;
		m_width = width;
		m_height = height;

		m_renderTarget = std::make_unique<Graphics::RenderTarget>();
		auto result = m_renderTarget->initialize(
			device, width, height, DXGI_FORMAT_R8G8B8A8_UNORM);

		if (!result)
			return result;

		m_initialized = true;
		return {};
	}

	void GameView::render(Graphics::Scene& scene,
		ID3D12GraphicsCommandList* commandList,
		const Graphics::Camera& camera,
		UINT frameIndex)
	{
		if (!m_initialized) return;

		m_renderTarget->transitionTo(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);

		auto rtv = m_renderTarget->getRTV();
		auto dsv = m_renderTarget->getDSV();
		commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

		float clear[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
		commandList->ClearRenderTargetView(rtv, clear, 0, nullptr);
		commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		if (m_skybox)
			m_skybox->render(commandList, camera);

		Utils::RenderContext context;
		context.commandList = commandList;
		context.camera = &camera;
		context.viewType = Utils::RenderViewType::Game;
		context.frameIndex = frameIndex;


		for (auto& obj : scene.getGameObjects())
		{
			if (!obj->isActive()) continue;
			if (auto rc = obj->getComponent<Graphics::RenderComponent>())
				rc->render(context);
		}

		m_renderTarget->transitionTo(commandList,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	void GameView::resize(uint32_t width, uint32_t height)
	{
		if (width == 0 || height == 0) return;

		m_width = width;
		m_height = height;

		m_renderTarget.reset();
		m_renderTarget = std::make_unique<Graphics::RenderTarget>();
		m_renderTarget->initialize(
			m_device, width, height, DXGI_FORMAT_R8G8B8A8_UNORM);
	}

	Graphics::RenderTarget* GameView::getRenderTarget() const
	{
		return m_renderTarget.get();
	}
}

// src/editor/Views/GameView.cpp
#include "GameView.hpp"

namespace Editor::UI
{
    Utils::VoidResult GameView::initialize(
        Renderer::Device* device, uint32_t width, uint32_t height,
        Renderer::PipelineStateCache* psoCache)
    {
        CHECK_CONDITION(device, Utils::ErrorType::Unknown, "Device is null");
        m_device = device;
        m_width = width;
        m_height = height;
        m_psoCache = psoCache;

        m_renderTarget = std::make_unique<Renderer::RenderTarget>();
        auto result = m_renderTarget->initialize(device, width, height, DXGI_FORMAT_R8G8B8A8_UNORM);
        if (!result) return result;

        m_initialized = true;
        return {};
    }

    void GameView::render(
        World::Scene& scene,
        ID3D12GraphicsCommandList* commandList,
        const World::Camera& camera,
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

        // ---- 3Dオブジェクト描画 (RenderBatchSystemに委譲) ----
        Utils::RenderContext context;
        context.commandList = commandList;
        context.camera = &camera;
        context.viewType = Utils::RenderViewType::Game;
        context.frameIndex = frameIndex;
        context.psoCache = m_psoCache;

        scene.renderBatch(context);

        // ---- UIText描画 ----
        if (m_uiTextRenderer && m_uiTextRenderer->isInitialized())
        {
            m_uiTextRenderer->beginFrame();
            auto allTexts = scene.getUITexts();
            int renderedCount = 0;
            for (auto* text : allTexts)
            {
                if (!text) continue;
                if (text->isVisible())
                {
                    m_uiTextRenderer->draw(context, *text, m_width, m_height, &camera);
                    ++renderedCount;
                }
            }
            Utils::log_info(std::format("GameView: Rendered {} UITexts", renderedCount));
        }

        m_renderTarget->transitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    void GameView::resize(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0) return;
        m_width = width;
        m_height = height;
        m_renderTarget.reset();
        m_renderTarget = std::make_unique<Renderer::RenderTarget>();
        m_renderTarget->initialize(m_device, width, height, DXGI_FORMAT_R8G8B8A8_UNORM);
    }

    Renderer::RenderTarget* GameView::getRenderTarget() const
    {
        return m_renderTarget.get();
    }
}
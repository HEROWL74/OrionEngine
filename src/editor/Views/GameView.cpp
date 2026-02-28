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

    // GameView.cpp
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

        // Skybox描画
        if (m_skybox)
            m_skybox->render(commandList, camera);

        Utils::RenderContext context;
        context.commandList = commandList;
        context.camera = &camera;
        context.viewType = Utils::RenderViewType::Game;
        context.frameIndex = frameIndex;

        // 3Dオブジェクトの描画
        for (auto& obj : scene.getGameObjects())
        {
            if (!obj->isActive()) continue;
            if (auto rc = obj->getComponent<Graphics::RenderComponent>())
                rc->render(context);
        }

        if (m_uiTextRenderer && m_uiTextRenderer->isInitialized())
        {
            Utils::RenderContext uiContext;
            uiContext.commandList = commandList;
            uiContext.camera = &camera;
            uiContext.viewType = Utils::RenderViewType::Game;
            uiContext.frameIndex = frameIndex;

            const auto& allTexts = scene.getUITexts();

            Utils::log_info("========================================");
            Utils::log_info("GameView: Starting UIText rendering");
            Utils::log_info(std::format("Total UIText count: {}", allTexts.size()));

            int renderedCount = 0;
            for (auto* text : allTexts)
            {
                if (!text)
                {
                    Utils::log_warning("GameView: Null UIText pointer encountered");
                    continue;
                }

                Utils::log_info(std::format("GameView: UIText '{}' - Visible: {}, Enabled: {}",
                    text->getName(),
                    text->isVisible(),
                    text->isEnabled()));

                if (text->isVisible())
                {
                    m_uiTextRenderer->draw(
                        uiContext,
                        *text,
                        m_width,
                        m_height,
                        &camera
                    );
                    renderedCount++;
                }
            }

            Utils::log_info(std::format("GameView: Rendered {} out of {} UITexts",
                renderedCount, allTexts.size()));
            Utils::log_info("========================================\n");
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


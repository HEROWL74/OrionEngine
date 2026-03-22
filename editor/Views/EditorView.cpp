// editor/Views/EditorView.cpp
#include "EditorView.hpp"
#include "../engine/Core/ProjectSettings.hpp"
#include <directx/d3dx12.h>

namespace Editor::UI
{
    using namespace Engine;

    Utils::VoidResult EditorView::initialize(
        Renderer::Device* device, uint32_t width, uint32_t height,
        Renderer::ShaderManager* shaderManager,
        Renderer::PipelineStateCache* psoCache)
    {
        CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
        CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");
        CHECK_CONDITION(width > 0 && height > 0, Utils::ErrorType::Unknown, "Invalid dimensions");

        m_device = device;
        m_width = width;
        m_height = height;
        m_psoCache = psoCache;

        Utils::log_info(std::format("EditorView::initialize - Creating RenderTarget {}x{}", width, height));

        m_renderTarget = std::make_unique<Renderer::RenderTarget>();
        auto result = m_renderTarget->initialize(device, width, height, DXGI_FORMAT_R8G8B8A8_UNORM);
        if (!result)
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
                "Failed to create EditorView render target"));

        m_fxaaRenderer = std::make_unique<Renderer::FXAARenderer>();
        if (!m_fxaaRenderer->initialize(m_device, shaderManager, width, height))
        {
            Utils::log_warning("Failed to initialize FXAA - antialiasing disabled");
            m_fxaaRenderer.reset();
        }

        m_fxaaOutputTarget = std::make_unique<Renderer::RenderTarget>();
        if (!m_fxaaOutputTarget->initialize(device, width, height, DXGI_FORMAT_R8G8B8A8_UNORM))
        {
            Utils::log_warning("Failed to initialize FXAA output target");
            m_fxaaOutputTarget.reset();
            m_fxaaRenderer.reset();
        }

        if (shaderManager)
        {
            if (!initializeGrid(shaderManager))
                Utils::log_warning("Failed to initialize grid");

            m_gizmo = std::make_unique<Gizmo>();
            if (!m_gizmo->initialize(device, shaderManager))
            {
                Utils::log_warning("Failed to initialize Gizmo");
                m_gizmo.reset();
            }
            else
            {
                m_gizmo->setType(GizmoType::Translation);
            }
        }

        m_initialized = true;
        Utils::log_info("EditorView initialized successfully");
        return {};
    }

    void EditorView::render(
        World::Scene& scene,
        ID3D12GraphicsCommandList* commandList,
        const World::Camera& camera,
        UINT frameIndex)
    {
        if (!m_initialized || !m_renderTarget || !commandList) return;

        m_renderTarget->transitionTo(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);

        auto rtv = m_renderTarget->getRTV();
        auto dsv = m_renderTarget->getDSV();
        commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

        float clearColor[4] = { 0.2f, 0.3f, 0.4f, 1.0f };
        commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
        commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        D3D12_VIEWPORT viewport{};
        viewport.Width = static_cast<float>(m_width);
        viewport.Height = static_cast<float>(m_height);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;

        D3D12_RECT scissorRect{ 0, 0,
            static_cast<LONG>(m_width), static_cast<LONG>(m_height) };

        commandList->RSSetViewports(1, &viewport);
        commandList->RSSetScissorRects(1, &scissorRect);

        if (m_skybox)
            m_skybox->render(commandList, camera);

        if (m_showGrid)
            renderGrid(commandList, camera);

        // ---- 3Dオブジェクト描画 (RenderBatchSystemに委譲) ----
        Utils::RenderContext context;
        context.commandList = commandList;
        context.camera = &camera;
        context.viewType = Utils::RenderViewType::Editor;
        context.frameIndex = frameIndex;
        context.psoCache = m_psoCache;

        scene.renderBatch(context);

        // ---- UIText描画 ----
        if (m_uiTextRenderer && m_uiTextRenderer->isInitialized() && m_scene)
        {
            m_uiTextRenderer->beginFrame();
            Utils::RenderContext uiContext = context;

            auto allTexts = m_scene->getUITexts();
            int renderedCount = 0;
            for (auto* text : allTexts)
            {
                if (!text) continue;
                if (text->isVisible())
                {
                    m_uiTextRenderer->draw(uiContext, *text, m_width, m_height, &camera);
                    ++renderedCount;
                }
            }
            Utils::log_info(std::format("EditorView: Rendered {} UITexts", renderedCount));
        }

        renderEditorElements(commandList, camera, frameIndex);

        // ---- FXAA ----
        if (m_enableFXAA && m_fxaaRenderer && m_fxaaOutputTarget)
        {
            m_renderTarget->transitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            m_fxaaRenderer->apply(commandList,
                m_renderTarget->getColorResource(),
                m_renderTarget->getColorSRV(),
                m_fxaaOutputTarget.get());
            m_fxaaOutputTarget->transitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }
        else
        {
            m_renderTarget->transitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }
    }

    void EditorView::renderEditorElements(
        ID3D12GraphicsCommandList* commandList,
        const World::Camera& camera, UINT frameIndex)
    {
        if (!commandList) return;
        if (m_showGizmos && m_selectedObject && m_gizmo)
            m_gizmo->render(commandList, camera, m_selectedObject);
        if (m_showGizmos && m_selectedObject)
            renderSelectionOutline(commandList, camera, m_selectedObject);
    }

    void EditorView::renderGrid(
        ID3D12GraphicsCommandList* commandList,
        const World::Camera& camera)
    {
        if (!m_gridInitialized || !m_gridVertexBuffer || !m_gridCameraBuffer) return;
        if (m_gridCameraMapped)
        {
            GridCameraConstants c;
            c.viewProjection = camera.getViewProjectionMatrix();
            memcpy(m_gridCameraMapped, &c, sizeof(c));
        }
        commandList->SetPipelineState(m_gridPipelineState.Get());
        commandList->SetGraphicsRootSignature(m_gridRootSignature.Get());
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
        commandList->IASetVertexBuffers(0, 1, &m_gridVertexBufferView);
        commandList->SetGraphicsRootConstantBufferView(0, m_gridCameraBuffer->GetGPUVirtualAddress());
        commandList->DrawInstanced(m_gridVertexCount, 1, 0, 0);
    }

    void EditorView::renderSelectionOutline(
        ID3D12GraphicsCommandList* commandList,
        const World::Camera& camera,
        Core::GameObject* object) {
    }

    void EditorView::resize(uint32_t width, uint32_t height)
    {
        if (!m_device || width == 0 || height == 0) return;
        if (m_width == width && m_height == height) return;

        Utils::log_info(std::format("EditorView::resize {}x{}", width, height));
        m_width = width;
        m_height = height;

        if (m_renderTarget) { m_renderTarget->release(); m_renderTarget.reset(); }
        if (m_fxaaRenderer) m_fxaaRenderer->resize(width, height);
        if (m_fxaaOutputTarget)
        {
            m_fxaaOutputTarget->release();
            m_fxaaOutputTarget->initialize(m_device, width, height, DXGI_FORMAT_R8G8B8A8_UNORM);
        }

        m_renderTarget = std::make_unique<Renderer::RenderTarget>();
        m_renderTarget->initialize(m_device, width, height, DXGI_FORMAT_R8G8B8A8_UNORM);
    }

    Utils::VoidResult EditorView::initializeGrid(Renderer::ShaderManager* shaderManager)
    {
        if (!m_device || !shaderManager)
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Device or ShaderManager is null"));
        auto r = createGridGeometry(); if (!r) return r;
        r = createGridRootSignature(); if (!r) return r;
        r = createGridPipelineState(shaderManager); if (!r) return r;
        m_gridInitialized = true;
        Utils::log_info("Grid initialized successfully");
        return {};
    }

    Utils::VoidResult EditorView::createGridGeometry()
    {
        auto dev = m_device->getDevice();
        const float gridSize = 50.0f, gridStep = 1.0f;
        const int gridLines = static_cast<int>(gridSize / gridStep);
        std::vector<GridVertex> vertices;

        for (int i = -gridLines; i <= gridLines; ++i) {
            float pos = i * gridStep;
            Math::Vector4 color = (i == 0) ?
                Math::Vector4(0, 0, 1, 1) : Math::Vector4(0.3f, 0.3f, 0.3f, 1);
            vertices.push_back({ {-gridSize,0,pos}, color });
            vertices.push_back({ { gridSize,0,pos}, color });
        }
        for (int i = -gridLines; i <= gridLines; ++i) {
            float pos = i * gridStep;
            Math::Vector4 color = (i == 0) ?
                Math::Vector4(1, 0, 0, 1) : Math::Vector4(0.3f, 0.3f, 0.3f, 1);
            vertices.push_back({ {pos,0,-gridSize}, color });
            vertices.push_back({ {pos,0, gridSize}, color });
        }

        m_gridVertexCount = static_cast<UINT>(vertices.size());
        const UINT vbSize = static_cast<UINT>(sizeof(GridVertex) * vertices.size());

        auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto bufDesc = CD3DX12_RESOURCE_DESC::Buffer(vbSize);
        CHECK_HR(dev->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_gridVertexBuffer)),
            Utils::ErrorType::ResourceCreation, "Failed to create grid vertex buffer");

        void* pData = nullptr; D3D12_RANGE rr{ 0,0 };
        m_gridVertexBuffer->Map(0, &rr, &pData);
        memcpy(pData, vertices.data(), vbSize);
        m_gridVertexBuffer->Unmap(0, nullptr);

        m_gridVertexBufferView.BufferLocation = m_gridVertexBuffer->GetGPUVirtualAddress();
        m_gridVertexBufferView.StrideInBytes = sizeof(GridVertex);
        m_gridVertexBufferView.SizeInBytes = vbSize;

        const UINT cbSize = (sizeof(GridCameraConstants) + 255) & ~255;
        auto cbDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);
        CHECK_HR(dev->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &cbDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_gridCameraBuffer)),
            Utils::ErrorType::ResourceCreation, "Failed to create grid camera buffer");

        CHECK_HR(m_gridCameraBuffer->Map(0, &rr, &m_gridCameraMapped),
            Utils::ErrorType::ResourceCreation, "Failed to map grid camera buffer");
        return {};
    }

    Utils::VoidResult EditorView::createGridRootSignature()
    {
        auto dev = m_device->getDevice();
        CD3DX12_ROOT_PARAMETER1 rp[1]{};
        rp[0].InitAsConstantBufferView(0, 0,
            D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc{};
        desc.Init_1_1(1, rp, 0, nullptr,
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> sig, err;
        CHECK_HR(D3DX12SerializeVersionedRootSignature(&desc,
            D3D_ROOT_SIGNATURE_VERSION_1_1, &sig, &err),
            Utils::ErrorType::ResourceCreation, "Failed to serialize grid root signature");
        CHECK_HR(dev->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(),
            IID_PPV_ARGS(&m_gridRootSignature)),
            Utils::ErrorType::ResourceCreation, "Failed to create grid root signature");
        return {};
    }

    Utils::VoidResult EditorView::createGridPipelineState(Renderer::ShaderManager* shaderManager)
    {
        auto dev = m_device->getDevice();
        auto& settings = Engine::Core::ProjectSettings::get();

        Renderer::ShaderCompileDesc vsDesc;
        vsDesc.filePath = settings.getEngineAssetPath("shaders/GridVS.cso").string();
        vsDesc.entryPoint = "main";
        vsDesc.type = Renderer::ShaderType::Vertex;
        vsDesc.enableDebug = true;
        auto vs = shaderManager->loadShader(vsDesc);
        if (!vs) return std::unexpected(Utils::make_error(
            Utils::ErrorType::ShaderCompilation, "Failed to load grid vertex shader"));

        Renderer::ShaderCompileDesc psDesc;
        psDesc.filePath = settings.getEngineAssetPath("shaders/GridPS.cso").string();
        psDesc.entryPoint = "main";
        psDesc.type = Renderer::ShaderType::Pixel;
        psDesc.enableDebug = true;
        auto ps = shaderManager->loadShader(psDesc);
        if (!ps) return std::unexpected(Utils::make_error(
            Utils::ErrorType::ShaderCompilation, "Failed to load grid pixel shader"));

        D3D12_INPUT_ELEMENT_DESC layout[] = {
            {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,   0, 0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
            {"COLOR",   0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.pRootSignature = m_gridRootSignature.Get();
        psoDesc.VS = { vs->getBytecode(), vs->getBytecodeSize() };
        psoDesc.PS = { ps->getBytecode(), ps->getBytecodeSize() };
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = TRUE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        psoDesc.InputLayout = { layout, _countof(layout) };
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleDesc.Count = 1;

        CHECK_HR(dev->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_gridPipelineState)),
            Utils::ErrorType::ResourceCreation, "Failed to create grid pipeline state");
        return {};
    }
}
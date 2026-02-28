// src/Graphics/SplashScreen.cpp
#include "SplashScreen.hpp"
#include <format>

namespace Engine::Graphics
{
    struct SplashVertex
    {
        float position[3];
        float texcoord[2];
    };

    Utils::VoidResult SplashScreen::initialize(
        Device* device,
        ShaderManager* shaderManager,
        TextureManager* textureManager)
    {
        CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
        CHECK_CONDITION(shaderManager != nullptr, Utils::ErrorType::Unknown, "ShaderManager is null");
        CHECK_CONDITION(textureManager != nullptr, Utils::ErrorType::Unknown, "TextureManager is null");

        m_device = device;
        m_shaderManager = shaderManager;

        Utils::log_info("Initializing Splash Screen...");

        // Load logo texture
        m_logoTexture = textureManager->loadTexture("engine-assets/images/OrionEngineIcon.png", false, false);
        if (!m_logoTexture)
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::ResourceCreation,
                "Failed to load splash screen logo"));
        }

        auto rootSigResult = createRootSignature();
        if (!rootSigResult) return rootSigResult;

        auto pipelineResult = createPipelineState();
        if (!pipelineResult) return pipelineResult;

        auto vertexBufferResult = createVertexBuffer();
        if (!vertexBufferResult) return vertexBufferResult;

        auto constantBufferResult = createConstantBuffer();
        if (!constantBufferResult) return constantBufferResult;

        Utils::log_info("Splash Screen initialized successfully");
        return {};
    }

    bool SplashScreen::update(float deltaTime)
    {
        if (isFinished())
        {
            return false;
        }

        m_currentTime += deltaTime;
        updateConstants();

        return !isFinished();
    }

    void SplashScreen::render(ID3D12GraphicsCommandList* commandList)
    {
        if (isFinished())
        {
            return;
        }

        commandList->SetGraphicsRootSignature(m_rootSignature.Get());
        commandList->SetPipelineState(m_pipelineState.Get());

        // Set constant buffer
        commandList->SetGraphicsRootConstantBufferView(0,
            m_constantBuffer->GetGPUVirtualAddress());

        // Set SRV heap for texture
        ID3D12DescriptorHeap* heaps[] = { m_device->getSrvHeap() };
        commandList->SetDescriptorHeaps(1, heaps);

        // Set logo texture
        if (m_logoTexture)
        {
            commandList->SetGraphicsRootDescriptorTable(1, m_logoTexture->getSRVHandle());
        }

        // Draw fullscreen quad
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
        commandList->DrawInstanced(6, 1, 0, 0);
    }

    Utils::VoidResult SplashScreen::createRootSignature()
    {
        D3D12_ROOT_PARAMETER rootParameters[2];

        // Constant buffer (b0)
        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[0].Descriptor.ShaderRegister = 0;
        rootParameters[0].Descriptor.RegisterSpace = 0;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // Texture (t0)
        D3D12_DESCRIPTOR_RANGE textureRange{};
        textureRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        textureRange.NumDescriptors = 1;
        textureRange.BaseShaderRegister = 0;
        textureRange.RegisterSpace = 0;
        textureRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
        rootParameters[1].DescriptorTable.pDescriptorRanges = &textureRange;
        rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // Static sampler
        D3D12_STATIC_SAMPLER_DESC samplerDesc{};
        samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samplerDesc.MipLODBias = 0.0f;
        samplerDesc.MaxAnisotropy = 1;
        samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        samplerDesc.MinLOD = 0.0f;
        samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
        samplerDesc.ShaderRegister = 0;
        samplerDesc.RegisterSpace = 0;
        samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
        rootSignatureDesc.NumParameters = _countof(rootParameters);
        rootSignatureDesc.pParameters = rootParameters;
        rootSignatureDesc.NumStaticSamplers = 1;
        rootSignatureDesc.pStaticSamplers = &samplerDesc;
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        CHECK_HR(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
            &signature, &error), Utils::ErrorType::ResourceCreation, "Failed to serialize splash screen root signature");

        CHECK_HR(m_device->getDevice()->CreateRootSignature(0, signature->GetBufferPointer(),
            signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)),
            Utils::ErrorType::ResourceCreation, "Failed to create splash screen root signature");

        return {};
    }

    Utils::VoidResult SplashScreen::createPipelineState()
    {
        // Load shaders
        ShaderCompileDesc vsDesc;
        vsDesc.filePath = "engine-assets/shaders/SplashVertex.cso";
        vsDesc.entryPoint = "main";
        vsDesc.type = ShaderType::Vertex;
        vsDesc.enableDebug = true;

        auto vertexShader = m_shaderManager->loadShader(vsDesc);
        if (!vertexShader)
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::ShaderCompilation,
                "Failed to load splash screen vertex shader"));
        }

        ShaderCompileDesc psDesc;
        psDesc.filePath = "engine-assets/shaders/SplashPixel.cso";
        psDesc.entryPoint = "main";
        psDesc.type = ShaderType::Pixel;
        psDesc.enableDebug = true;

        auto pixelShader = m_shaderManager->loadShader(psDesc);
        if (!pixelShader)
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::ShaderCompilation,
                "Failed to load splash screen pixel shader"));
        }

        // Input layout
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
              D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12,
              D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // Pipeline state
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = { vertexShader->getBytecode(), vertexShader->getBytecodeSize() };
        psoDesc.PS = { pixelShader->getBytecode(), pixelShader->getBytecodeSize() };

        // Rasterizer state
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
        psoDesc.RasterizerState.DepthBias = 0;
        psoDesc.RasterizerState.DepthBiasClamp = 0.0f;
        psoDesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
        psoDesc.RasterizerState.DepthClipEnable = TRUE;
        psoDesc.RasterizerState.MultisampleEnable = FALSE;
        psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
        psoDesc.RasterizerState.ForcedSampleCount = 0;
        psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        // Blend state (alpha blending for fade effect)
        psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
        psoDesc.BlendState.IndependentBlendEnable = FALSE;
        D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc{};
        renderTargetBlendDesc.BlendEnable = TRUE;
        renderTargetBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        renderTargetBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        renderTargetBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
        renderTargetBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
        renderTargetBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
        renderTargetBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
        {
            psoDesc.BlendState.RenderTarget[i] = renderTargetBlendDesc;
        }

        // Depth stencil state (no depth test)
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;

        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;

        CHECK_HR(m_device->getDevice()->CreateGraphicsPipelineState(&psoDesc,
            IID_PPV_ARGS(&m_pipelineState)),
            Utils::ErrorType::ResourceCreation, "Failed to create splash screen pipeline state");

        return {};
    }

    Utils::VoidResult SplashScreen::createVertexBuffer()
    {
        // Fullscreen quad vertices (NDC space: -1 to 1)
        SplashVertex vertices[] = {
            // First triangle
            { {-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f} },  // Bottom-left
            { {-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f} },  // Top-left
            { { 1.0f,  1.0f, 0.0f}, {1.0f, 0.0f} },  // Top-right

            // Second triangle
            { {-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f} },  // Bottom-left
            { { 1.0f,  1.0f, 0.0f}, {1.0f, 0.0f} },  // Top-right
            { { 1.0f, -1.0f, 0.0f}, {1.0f, 1.0f} }   // Bottom-right
        };

        const UINT vertexBufferSize = sizeof(vertices);

        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = vertexBufferSize;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        CHECK_HR(m_device->getDevice()->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_vertexBuffer)),
            Utils::ErrorType::ResourceCreation, "Failed to create splash screen vertex buffer");

        // Upload vertex data
        UINT8* pVertexDataBegin;
        D3D12_RANGE readRange{ 0, 0 };
        CHECK_HR(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)),
            Utils::ErrorType::ResourceCreation, "Failed to map splash screen vertex buffer");

        memcpy(pVertexDataBegin, vertices, sizeof(vertices));
        m_vertexBuffer->Unmap(0, nullptr);

        // Initialize vertex buffer view
        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.StrideInBytes = sizeof(SplashVertex);
        m_vertexBufferView.SizeInBytes = vertexBufferSize;

        return {};
    }

    Utils::VoidResult SplashScreen::createConstantBuffer()
    {
        const UINT constantBufferSize = (sizeof(SplashConstants) + 255) & ~255;

        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = constantBufferSize;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        CHECK_HR(m_device->getDevice()->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_constantBuffer)),
            Utils::ErrorType::ResourceCreation, "Failed to create splash screen constant buffer");

        return {};
    }

    float SplashScreen::calculateFadeAlpha() const
    {
        if (m_currentTime < m_fadeInDuration)
        {
            // Fade in
            return m_currentTime / m_fadeInDuration;
        }
        else if (m_currentTime < m_fadeInDuration + m_displayDuration)
        {
            // Full display
            return 1.0f;
        }
        else if (m_currentTime < m_totalDuration)
        {
            // Fade out
            float fadeOutProgress = (m_currentTime - m_fadeInDuration - m_displayDuration) / m_fadeOutDuration;
            return 1.0f - fadeOutProgress;
        }

        return 0.0f;
    }

    void SplashScreen::updateConstants()
    {
        SplashConstants constants;
        constants.fadeAlpha = calculateFadeAlpha();
        constants.logoScale = m_logoScale;
        constants.screenAspect = (m_screenHeight > 0.0f) ? (m_screenWidth / m_screenHeight) : 1.0f;
        constants.padding = 0.0f;

        // Map and update constant buffer
        UINT8* pConstantDataBegin = {};
        D3D12_RANGE readRange{ 0, 0 };

        if (SUCCEEDED(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pConstantDataBegin))))
        {
            memcpy(pConstantDataBegin, &constants, sizeof(SplashConstants));
            m_constantBuffer->Unmap(0, nullptr);
        }
    }
}


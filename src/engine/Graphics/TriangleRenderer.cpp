// src/Graphics/TriangleRenderer.cpp
#include "TriangleRenderer.hpp"
#include <format>

using Engine::Graphics::ShaderType;

namespace Engine::Graphics
{
    Utils::VoidResult TriangleRenderer::initialize(Device* device, ShaderManager* shaderManager)
    {
        Utils::log_info("TriangleRenderer::initialize START");

        // 繧医ｊ隧ｳ邏ｰ縺ｪnull繝√ぉ繝・け
        if (!device) {
            Utils::log_warning("Device is null in TriangleRenderer::initialize");
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Device is null"));
        }

        if (!device->isValid()) {
            Utils::log_warning("Device is not valid in TriangleRenderer::initialize");
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Device is not valid"));
        }

        if (!shaderManager) {
            Utils::log_warning("ShaderManager is null in TriangleRenderer::initialize");
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "ShaderManager is null"));
        }

        m_device = device;
        m_shaderManager = shaderManager;

        Utils::log_info("Initializing Triangle Renderer...");

        // 螳壽焚繝舌ャ繝輔ぃ繝槭ロ繝ｼ繧ｸ繝｣繝ｼ繧貞・譛溷喧
        Utils::log_info("Initializing constant buffer manager...");
        auto constantBufferResult = m_constantBufferManager.initialize(device);
        if (!constantBufferResult) return constantBufferResult;

        // 荳芽ｧ貞ｽ｢縺ｮ鬆らせ繝・・繧ｿ繧定ｨｭ螳・
        Utils::log_info("Setting up triangle vertices...");
        setupTriangleVertices();

        // 繝ｯ繝ｼ繝ｫ繝芽｡悟・繧貞・譛溷喧
        Utils::log_info("Updating world matrix...");
        updateWorldMatrix();

        // 蜷・さ繝ｳ繝昴・繝阪Φ繝医ｒ鬆・ｬ｡蛻晄悄蛹・
        Utils::log_info("Creating root signature...");
        auto rootSigResult = createRootSignature();
        if (!rootSigResult) return rootSigResult;

        Utils::log_info("Creating pipeline state (with shader loading)...");
        auto pipelineResult = createPipelineState();
        if (!pipelineResult) return pipelineResult;

        Utils::log_info("Creating vertex buffer...");
        auto vertexBufferResult = createVertexBuffer();
        if (!vertexBufferResult) return vertexBufferResult;

        Utils::log_info("Triangle Renderer initialized successfully!");
        return {};
    }

    void TriangleRenderer::render(ID3D12GraphicsCommandList* commandList, const Camera& camera, UINT frameIndex)
    {
        //デフォルトマテリアル設定
        if (!m_material && m_materialManager)
        {
            m_material = m_materialManager->getDefaultMaterial();
        }

        // CB設定
        CameraConstants cameraConstants{};
        cameraConstants.viewMatrix = camera.getViewMatrix();
        cameraConstants.projectionMatrix = camera.getProjectionMatrix();
        cameraConstants.viewProjectionMatrix = camera.getViewProjectionMatrix();
        cameraConstants.cameraPosition = camera.getPosition();

        ObjectConstants objectConstants{};
        objectConstants.worldMatrix = m_worldMatrix;
        objectConstants.worldViewProjectionMatrix = camera.getViewProjectionMatrix() * m_worldMatrix;
        objectConstants.objectPosition = m_position;

        m_constantBufferManager.updateCameraConstants(frameIndex, cameraConstants);
        m_constantBufferManager.updateObjectConstants(frameIndex, objectConstants);


        commandList->SetGraphicsRootSignature(m_rootSignature.Get());
        commandList->SetPipelineState(m_pipelineState.Get());

        ID3D12DescriptorHeap* heap[] = { m_device->getSrvHeap() };
        commandList->SetDescriptorHeaps(1, heap);

        if (m_material && m_material->hasTexture(TextureType::Albedo))
        {
            auto tex = m_material->getSRVGpuHandle();
            D3D12_GPU_DESCRIPTOR_HANDLE base = tex;

            commandList->SetGraphicsRootDescriptorTable(3, base);
        }
 
        commandList->SetGraphicsRootConstantBufferView(0, m_constantBufferManager.getCameraConstantsGPUAddress(frameIndex));
        commandList->SetGraphicsRootConstantBufferView(1, m_constantBufferManager.getObjectConstantsGPUAddress(frameIndex));

        // 笘・繝槭ユ繝ｪ繧｢繝ｫ螳壽焚繝舌ャ繝輔ぃ繧偵ヰ繧､繝ｳ繝・
        if (m_material && m_material->getConstantBuffer())
        {
            commandList->SetGraphicsRootConstantBufferView(2,
                m_material->getConstantBuffer()->GetGPUVirtualAddress());
        }
        else
        {
            Utils::log_warning("Material constant buffer is null in TriangleRenderer");
        }

        //プリミティブトポロジー設定
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // 頂点バッファセット
        commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);

        // 描画
        commandList->DrawInstanced(6, 1, 0, 0);
    }

    Utils::VoidResult TriangleRenderer::createRootSignature()
    {
        D3D12_ROOT_PARAMETER rootParameters[4]{};

        // バッファー定義1(b0)
        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[0].Descriptor.ShaderRegister = 0;
        rootParameters[0].Descriptor.RegisterSpace = 0;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        // バッファー定義2 (b1)
        rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[1].Descriptor.ShaderRegister = 1;
        rootParameters[1].Descriptor.RegisterSpace = 0;
        rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        // バッファー定義3 (b2)
        rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[2].Descriptor.ShaderRegister = 2;
        rootParameters[2].Descriptor.RegisterSpace = 0;
        rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        //t0～t5: SRVテーブル
        D3D12_DESCRIPTOR_RANGE srvRange{};
        srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        srvRange.NumDescriptors = 1;
        srvRange.BaseShaderRegister = 0;
        srvRange.RegisterSpace = 0;
        srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_DESCRIPTOR_TABLE srvTable{};
        srvTable.NumDescriptorRanges = 1;
        srvTable.pDescriptorRanges = &srvRange;

        rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[3].DescriptorTable = srvTable;
        rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;



        // Static Sampler
        D3D12_STATIC_SAMPLER_DESC samplerDesc{};
        samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.MipLODBias = 0.0f;
        samplerDesc.MaxAnisotropy = 1;
        samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        samplerDesc.MinLOD = 0.0f;
        samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
        samplerDesc.ShaderRegister = 0; // s0
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

        HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
        if (FAILED(hr))
        {
            std::string errorMsg = "Failed to serialize root signature";
            if (error)
            {
                errorMsg += std::format(": {}", static_cast<char*>(error->GetBufferPointer()));
            }
            return std::unexpected(Utils::make_error(Utils::ErrorType::ResourceCreation, errorMsg, hr));
        }

        CHECK_HR(m_device->getDevice()->CreateRootSignature(0, signature->GetBufferPointer(),
            signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)),
            Utils::ErrorType::ResourceCreation, "Failed to create root signature");

        return {};
    }

    Utils::VoidResult TriangleRenderer::createShaders()
    {
        // ShaderCompileDesc
        ShaderCompileDesc vsDesc;
        vsDesc.filePath = "engine-assets/shaders/BasicVertex.hlsl";
        vsDesc.entryPoint = "main";
        vsDesc.type = ShaderType::Vertex;
        vsDesc.enableDebug = true;

        auto vertexShader = m_shaderManager->loadShader(vsDesc);
        if (!vertexShader)
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::ShaderCompilation, "Failed to load vertex shader"));
        }

        ShaderCompileDesc psDesc;
        psDesc.filePath = "engine-assets/shaders/PBR_Lite_PS.hlsl";
        psDesc.entryPoint = "main";
        psDesc.type = ShaderType::Pixel;
        psDesc.enableDebug = true;

        auto pixelShader = m_shaderManager->loadShader(psDesc);
        if (!pixelShader)
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::ShaderCompilation, "Failed to load pixel shader"));
        }

        return {};
    }

    Utils::VoidResult TriangleRenderer::createPipelineState()
    {
        // シェーダーセット
        ShaderCompileDesc vsDesc;
        vsDesc.filePath = "engine-assets/shaders/BasicVertex.hlsl";
        vsDesc.entryPoint = "main";
        vsDesc.type = ShaderType::Vertex;
        vsDesc.enableDebug = true;

        auto vertexShaderResult = m_shaderManager->loadShader(vsDesc);
        if (!vertexShaderResult)
        {
            Utils::log_warning("Failed to load vertex shader for TriangleRenderer");
            return std::unexpected(Utils::make_error(Utils::ErrorType::ShaderCompilation, "Failed to load vertex shader"));
        }

        ShaderCompileDesc psDesc;
        psDesc.filePath = "engine-assets/shaders/PBR_Lite_PS.hlsl";
        psDesc.entryPoint = "main";
        psDesc.type = ShaderType::Pixel;
        psDesc.enableDebug = true;

        auto pixelShaderResult = m_shaderManager->loadShader(psDesc);
        if (!pixelShaderResult)
        {
            Utils::log_warning("Failed to load pixel shader for TriangleRenderer");
            return std::unexpected(Utils::make_error(Utils::ErrorType::ShaderCompilation, "Failed to load pixel shader"));
        }

        //型推論で代入
        auto& vertexShader = vertexShaderResult;
        auto& pixelShader = pixelShaderResult;

        CHECK_CONDITION(vertexShader != nullptr, Utils::ErrorType::ShaderCompilation,
            "Vertex shader is null");
        CHECK_CONDITION(pixelShader != nullptr, Utils::ErrorType::ShaderCompilation,
            "Pixel shader is null");

        // 入力レイアウト作成
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
             { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // PSO生成
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = { vertexShader->getBytecode(), vertexShader->getBytecodeSize() };
        psoDesc.PS = { pixelShader->getBytecode(), pixelShader->getBytecodeSize() };

        // psoラスタライザステート作成
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
        psoDesc.RasterizerState.DepthBias = 0;
        psoDesc.RasterizerState.DepthBiasClamp = 0.0f;
        psoDesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
        psoDesc.RasterizerState.DepthClipEnable = TRUE;
        psoDesc.RasterizerState.MultisampleEnable = FALSE;
        psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
        psoDesc.RasterizerState.ForcedSampleCount = 0;
        psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        //psoのブレンド設定
        psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
        psoDesc.BlendState.IndependentBlendEnable = FALSE;
        const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc = {
            FALSE, FALSE,
            D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
            D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
            D3D12_LOGIC_OP_NOOP,
            D3D12_COLOR_WRITE_ENABLE_ALL,
        };
        for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
        {
            psoDesc.BlendState.RenderTarget[i] = defaultRenderTargetBlendDesc;
        }

        // 深度ステンシル設定
        psoDesc.DepthStencilState.DepthEnable = TRUE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
        psoDesc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

        const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp = {
            D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS
        };
        psoDesc.DepthStencilState.FrontFace = defaultStencilOp;
        psoDesc.DepthStencilState.BackFace = defaultStencilOp;

        // RTV&DSV設定
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleDesc.Count = 1;

        CHECK_HR(m_device->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)),
            Utils::ErrorType::ResourceCreation, "Failed to create graphics pipeline state");

        return {};
    }

    Utils::VoidResult TriangleRenderer::createVertexBuffer()
    {
        const UINT vertexBufferSize = sizeof(m_triangleVertices);

        // heapProps作成
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        // RC作成
        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = vertexBufferSize;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        CHECK_HR(m_device->getDevice()->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_vertexBuffer)),
            Utils::ErrorType::ResourceCreation, "Failed to create vertex buffer");

        // 
        UINT8* pVertexDataBegin{};
        D3D12_RANGE readRange{ 0, 0 }; // CPU書き込み

        CHECK_HR(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)),
            Utils::ErrorType::ResourceCreation, "Failed to map vertex buffer");

        memcpy(pVertexDataBegin, m_triangleVertices.data(), sizeof(m_triangleVertices));
        m_vertexBuffer->Unmap(0, nullptr);

        // VBV作成
        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.StrideInBytes = sizeof(Vertex);
        m_vertexBufferView.SizeInBytes = vertexBufferSize;

        return {};
    }

    void TriangleRenderer::setupTriangleVertices()
    {
        //頂点位置生成
        m_triangleVertices = { {
                // 正面（三角形を反時計回り CCW）
                {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}, // 左下 v0
                {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}}, // 右下 v1
                {{ 0.0f,  0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.5f, 0.0f}}, // 上    v2

                // 裏面（三角形を時計回り CW に反転）
                {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}, // 左下 (v0)
                {{ 0.0f,  0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.5f, 0.0f}}, // 上    (v2)
                {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}}  // 右下 (v1)
            } };

    }

    void TriangleRenderer::updateWorldMatrix()
    {
      //マトリックス更新処理
        Math::Matrix4 scaleMatrix = Math::Matrix4::scaling(m_scale);
        Math::Matrix4 rotationMatrix = Math::Matrix4::rotationX(Math::radians(m_rotation.x)) *
            Math::Matrix4::rotationY(Math::radians(m_rotation.y)) *
            Math::Matrix4::rotationZ(Math::radians(m_rotation.z));
        Math::Matrix4 translationMatrix = Math::Matrix4::translation(m_position);

        m_worldMatrix = translationMatrix * rotationMatrix * scaleMatrix;
    }
}

// src/Graphics/CubeRenderer.cpp
#include "CubeRenderer.hpp"
#include <format>

namespace Engine::Graphics
{
    Utils::VoidResult CubeRenderer::initialize(Device* device, ShaderManager* shaderManager)
    {
       
        CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
        CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");

        m_device = device;
        m_shaderManager = shaderManager;
        Utils::log_info("Initializing Cube Renderer...");

        auto constantBufferResult = m_constantBufferManager.initialize(device);
        if (!constantBufferResult) {
            Utils::log_error(constantBufferResult.error());
            return constantBufferResult;
        }

    
        setupCubeVertices();

      
        updateWorldMatrix();

    
        auto rootSigResult = createPBRRootSignature();
        if (!rootSigResult) {
            Utils::log_error(rootSigResult.error());
            return rootSigResult;
        }

        auto shaderResult = createShaders();
        if (!shaderResult) {
            Utils::log_error(shaderResult.error());
            return shaderResult;
        }

        auto pipelineResult = createPipelineState();
        if (!pipelineResult) {
            Utils::log_error(pipelineResult.error());
            return pipelineResult;
        }

        auto vertexBufferResult = createVertexBuffer();
        if (!vertexBufferResult) {
            Utils::log_error(vertexBufferResult.error());
            return vertexBufferResult;
        }

        auto indexBufferResult = createIndexBuffer();
        if (!indexBufferResult) {
            Utils::log_error(indexBufferResult.error());
            return indexBufferResult;
        }

        Utils::log_info("Cube Renderer initialized successfully!");
        return {};
    }

    void CubeRenderer::render(ID3D12GraphicsCommandList* commandList, const Camera& camera, UINT frameIndex)
    {
        if (!m_material && m_materialManager)
        {
            m_material = m_materialManager->getDefaultMaterial();
        }

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

        commandList->SetGraphicsRootConstantBufferView(0, m_constantBufferManager.getCameraConstantsGPUAddress(frameIndex));
        commandList->SetGraphicsRootConstantBufferView(1, m_constantBufferManager.getObjectConstantsGPUAddress(frameIndex));

        if (m_material && m_material->getConstantBuffer())
        {
            commandList->SetGraphicsRootConstantBufferView(2, m_material->getConstantBuffer()->GetGPUVirtualAddress());
        }

        //SRVヒープをセット
        ID3D12DescriptorHeap* heaps[] = { m_device->getSrvHeap() };
        commandList->SetDescriptorHeaps(1, heaps);

        //Albedoテクスチャをバインド (RootParam=3)
        if (m_material)
        {
            auto tex = m_material->getTexture(TextureType::Albedo);
            if (tex)
            {
                commandList->SetGraphicsRootDescriptorTable(3, tex->getSRVHandle());
            }
        }

        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
        commandList->IASetIndexBuffer(&m_indexBufferView);

        commandList->DrawIndexedInstanced(36, 1, 0, 0, 0);
    }

    Utils::VoidResult CubeRenderer::createRootSignature()
    {
        // 3Root Parameters camera, Object, Material・・
        D3D12_ROOT_PARAMETER rootParameters[3];

        // \ (b0)
        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[0].Descriptor.ShaderRegister = 0;
        rootParameters[0].Descriptor.RegisterSpace = 0;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        // \ (b1)
        rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[1].Descriptor.ShaderRegister = 1;
        rootParameters[1].Descriptor.RegisterSpace = 0;
        rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        //  (b2)
        rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[2].Descriptor.ShaderRegister = 2;
        rootParameters[2].Descriptor.RegisterSpace = 0;
        rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

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

    Utils::VoidResult CubeRenderer::createPBRRootSignature()
    {
        // 4つのRootParameter定義
        D3D12_ROOT_PARAMETER rootParameters[4];

        // (b0)
        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[0].Descriptor.ShaderRegister = 0;
        rootParameters[0].Descriptor.RegisterSpace = 0;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        // (b1)
        rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[1].Descriptor.ShaderRegister = 1;
        rootParameters[1].Descriptor.RegisterSpace = 0;
        rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        // (b2)
        rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[2].Descriptor.ShaderRegister = 2;
        rootParameters[2].Descriptor.RegisterSpace = 0;
        rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // テクスチャをセット(t0-t5)
        static D3D12_DESCRIPTOR_RANGE textureRange{};
        textureRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        textureRange.NumDescriptors = 6;
        textureRange.BaseShaderRegister = 0;
        textureRange.RegisterSpace = 0;
        textureRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[3].DescriptorTable.NumDescriptorRanges = 1;
        rootParameters[3].DescriptorTable.pDescriptorRanges = &textureRange;
        rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;


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
        HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);

        if (FAILED(hr))
        {
            std::string errorMsg = "Failed to serialize PBR root signature";
            if (error)
            {
                errorMsg += std::format(": {}", static_cast<char*>(error->GetBufferPointer()));
            }
            return std::unexpected(Utils::make_error(Utils::ErrorType::ResourceCreation, errorMsg, hr));
        }

        CHECK_HR(m_device->getDevice()->CreateRootSignature(0, signature->GetBufferPointer(),
            signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)),
            Utils::ErrorType::ResourceCreation, "Failed to create PBR root signature");

        return {};
    }

    Utils::VoidResult CubeRenderer::createShaders()
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
        psDesc.filePath = "engine-assets/shaders/BasicPixel.hlsl";
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

    Utils::VoidResult CubeRenderer::createPipelineState()
    {
        ShaderCompileDesc vsDesc;
        vsDesc.filePath = "engine-assets/shaders/BasicVertex.hlsl";
        vsDesc.entryPoint = "main";
        vsDesc.type = ShaderType::Vertex;
        vsDesc.enableDebug = true;

        auto vertexShaderResult = m_shaderManager->loadShader(vsDesc);
        if (!vertexShaderResult)
        {
            Utils::log_warning("Failed to load vertex shader for CubeRenderer");
            return std::unexpected(Utils::make_error(Utils::ErrorType::ShaderCompilation, "Failed to load vertex shader"));
        }

        ShaderCompileDesc psDesc;
        psDesc.filePath = "engine-assets/shaders/BasicPixel.hlsl";
        psDesc.entryPoint = "main";
        psDesc.type = ShaderType::Pixel;
        psDesc.enableDebug = true;

        auto pixelShaderResult = m_shaderManager->loadShader(psDesc);
        if (!pixelShaderResult)
        {
            Utils::log_warning("Failed to load pixel shader for CubeRenderer");
            return std::unexpected(Utils::make_error(Utils::ErrorType::ShaderCompilation, "Failed to load pixel shader"));
        }

        auto& vertexShader = vertexShaderResult;
        auto& pixelShader = pixelShaderResult;

        CHECK_CONDITION(vertexShader != nullptr, Utils::ErrorType::ShaderCompilation,
            "Vertex shader is null");
        CHECK_CONDITION(pixelShader != nullptr, Utils::ErrorType::ShaderCompilation,
            "Pixel shader is null");

        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // パイプラインステートオブジェクト設定
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = { vertexShader->getBytecode(), vertexShader->getBytecodeSize() };
        psoDesc.PS = { pixelShader->getBytecode(), pixelShader->getBytecodeSize() };

  
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

    Utils::VoidResult CubeRenderer::createVertexBuffer()
    {
        const UINT vertexBufferSize = sizeof(m_cubeVertices);

    
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

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

        
        UINT8* pVertexDataBegin;
        D3D12_RANGE readRange{ 0, 0 };

        CHECK_HR(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)),
            Utils::ErrorType::ResourceCreation, "Failed to map vertex buffer");

        memcpy(pVertexDataBegin, m_cubeVertices.data(), sizeof(m_cubeVertices));
        m_vertexBuffer->Unmap(0, nullptr);

 
        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.StrideInBytes = sizeof(Vertex);
        m_vertexBufferView.SizeInBytes = vertexBufferSize;

        return {};
    }

    Utils::VoidResult CubeRenderer::createIndexBuffer()
    {
        const UINT indexBufferSize = sizeof(m_cubeIndices);

        
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;


        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = indexBufferSize;
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
            IID_PPV_ARGS(&m_indexBuffer)),
            Utils::ErrorType::ResourceCreation, "Failed to create index buffer");

        UINT8* pIndexDataBegin;
        D3D12_RANGE readRange{ 0, 0 };

        CHECK_HR(m_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)),
            Utils::ErrorType::ResourceCreation, "Failed to map index buffer");

        memcpy(pIndexDataBegin, m_cubeIndices.data(), sizeof(m_cubeIndices));
        m_indexBuffer->Unmap(0, nullptr);

       
        m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
        m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
        m_indexBufferView.SizeInBytes = indexBufferSize;

        return {};
    }

    void CubeRenderer::setupCubeVertices()
    {
        m_cubeVertices = { {
                // Front (+Z)
                { {-0.5f, -0.5f,  0.5f}, {1,0,0}, {0,0} }, // 左下
                { { 0.5f, -0.5f,  0.5f}, {1,0,0}, {1,0} }, // 右下
                { { 0.5f,  0.5f,  0.5f}, {1,0,0}, {1,1} }, // 右上
                { {-0.5f,  0.5f,  0.5f}, {1,0,0}, {0,1} }, // 左上

                // Back (-Z)
                { {-0.5f, -0.5f, -0.5f}, {0,1,0}, {0,0} }, // 左下
                { { 0.5f, -0.5f, -0.5f}, {0,1,0}, {1,0} }, // 右下
                { { 0.5f,  0.5f, -0.5f}, {0,1,0}, {1,1} }, // 右上
                { {-0.5f,  0.5f, -0.5f}, {0,1,0}, {0,1} }, // 左上

                // Left (-X)
                { {-0.5f, -0.5f, -0.5f}, {0,0,1}, {0,0} },
                { {-0.5f, -0.5f,  0.5f}, {0,0,1}, {1,0} },
                { {-0.5f,  0.5f,  0.5f}, {0,0,1}, {1,1} },
                { {-0.5f,  0.5f, -0.5f}, {0,0,1}, {0,1} },

                // Right (+X)
                { { 0.5f, -0.5f,  0.5f}, {1,1,0}, {0,0} },
                { { 0.5f, -0.5f, -0.5f}, {1,1,0}, {1,0} },
                { { 0.5f,  0.5f, -0.5f}, {1,1,0}, {1,1} },
                { { 0.5f,  0.5f,  0.5f}, {1,1,0}, {0,1} },

                // Top (+Y)
                { {-0.5f,  0.5f,  0.5f}, {1,0,1}, {0,0} },
                { { 0.5f,  0.5f,  0.5f}, {1,0,1}, {1,0} },
                { { 0.5f,  0.5f, -0.5f}, {1,0,1}, {1,1} },
                { {-0.5f,  0.5f, -0.5f}, {1,0,1}, {0,1} },

                // Bottom (-Y)
                { {-0.5f, -0.5f, -0.5f}, {0,1,1}, {0,0} },
                { { 0.5f, -0.5f, -0.5f}, {0,1,1}, {1,0} },
                { { 0.5f, -0.5f,  0.5f}, {0,1,1}, {1,1} },
                { {-0.5f, -0.5f,  0.5f}, {0,1,1}, {0,1} }
            } };

        m_cubeIndices = { {
                // Front (CCW: 左下, 右上, 右下) 
                0, 2, 1,  0, 3, 2,
                // Back
                4, 5, 6,  4, 6, 7,
                // Left
                8, 10, 9,  8, 11, 10,
                // Right
                12, 14, 13,  12, 15, 14,
                // Top
                16, 18, 17,  16, 19, 18,
                // Bottom
                20, 22, 21,  20, 23, 22
            } };
    }



    void CubeRenderer::updateWorldMatrix()
    {
        Math::Matrix4 scaleMatrix = Math::Matrix4::scaling(m_scale);
        Math::Matrix4 rotationMatrix = Math::Matrix4::rotationX(Math::radians(m_rotation.x)) *
            Math::Matrix4::rotationY(Math::radians(m_rotation.y)) *
            Math::Matrix4::rotationZ(Math::radians(m_rotation.z));
        Math::Matrix4 translationMatrix = Math::Matrix4::translation(m_position);

        m_worldMatrix = translationMatrix * rotationMatrix * scaleMatrix;
    }
}

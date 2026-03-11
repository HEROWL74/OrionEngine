// src/engine/UI/UITextRenderer.cpp
#include "UITextRenderer.hpp"
#include "../Core/ProjectSettings.hpp"
#include <directx/d3dx12.h>
#include <fstream>
#include <format>
#include <algorithm>
#define NOMINMAX
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace Engine::EngineUI
{
    UITextRenderer::~UITextRenderer()
    {
        release();
    }

    Utils::VoidResult UITextRenderer::initialize(
        Graphics::Device* device,
        Graphics::ShaderManager* shaderManager)
    {
        CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
        CHECK_CONDITION(shaderManager != nullptr, Utils::ErrorType::Unknown, "ShaderManager is null");

        m_device = device;
        m_shaderManager = shaderManager;

        Utils::log_info("Initializing UITextRenderer...");

        auto& settings = Engine::Core::ProjectSettings::get();
        auto fontResult = loadFont(settings.getEngineAssetPath("fonts/arial.ttf").string());
        if (!fontResult) return fontResult;

        auto heapResult = createDescriptorHeap();
        if (!heapResult) return heapResult;

        auto rootSigResult = createRootSignature();
        if (!rootSigResult) return rootSigResult;

        auto psoResult = createPipelineState();
        if (!psoResult) return psoResult;

        auto vbResult = createVertexBuffer();
        if (!vbResult) return vbResult;

        auto cbResult = createConstantBuffer();
        if (!cbResult) return cbResult;

        auto atlasResult = createFontAtlas(24.0f);
        if (!atlasResult)
        {
            Utils::log_warning("Failed to create default font atlas");
        }

        m_initialized = true;
        Utils::log_info("UITextRenderer initialized successfully");
        return {};
    }

    void UITextRenderer::draw(
        Utils::RenderContext& context,
        const UIText& text,
        uint32_t screenWidth,
        uint32_t screenHeight,
        const Graphics::Camera* camera)
    {
        if (!m_initialized || !context.commandList) return;
        if (!text.isVisible()) return;
        if (screenWidth == 0 || screenHeight == 0) return;
        if (!camera) return;

        auto* mutableText = const_cast<UIText*>(&text);
        auto* go = mutableText ? mutableText->getGameObject() : nullptr;
           
        if (go)
        {
            if (go->isDestroyed()) return;  // destroyされていたらスキップ
            mutableText->syncFromGameObjectTransform();  // 生存確認後に同期
        }
        FontAtlas* atlas = getOrCreateAtlas(text.getFontSize());
        if (!atlas) {
            Utils::log_warning("Failed to get font atlas");
            return;
        }

        const uint32_t frameIndex = context.frameIndex;

        // 繝代う繝励Λ繧､繝ｳ險ｭ螳・
        context.commandList->SetPipelineState(m_pipelineState.Get());
        context.commandList->SetGraphicsRootSignature(m_rootSignature.Get());
        context.commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context.commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);

        // 繝・け繧ｹ繝√Ε險ｭ螳・
        ID3D12DescriptorHeap* heaps[] = { m_srvHeap.Get() };
        context.commandList->SetDescriptorHeaps(_countof(heaps), heaps);
        context.commandList->SetGraphicsRootDescriptorTable(1, atlas->srvGpuHandle);

        // 鬆らせ繝・・繧ｿ逕滓・
        uint32_t vertexCount = 0;
        uint32_t startVertex = m_currentVertexOffset;

        Math::Vector4 color(text.getColor(), text.getAlpha());
        const std::string& str = text.getText();

        // 繝ｭ繝ｼ繧ｫ繝ｫ蠎ｧ讓咏ｳｻ縺ｧ鬆らせ繧堤函謌・
        float x = 0.0f;
        float y = 0.0f;

        for (size_t i = 0; i < str.size(); ++i)
        {
            if (str[i] == '\n')
            {
                x = 0.0f;
                y += atlas->fontSize * 1.2f;
                continue;
            }

            int codepoint = static_cast<int>(str[i]);
            auto it = atlas->glyphs.find(codepoint);
            if (it == atlas->glyphs.end()) continue;

            const auto& glyph = it->second;

            Math::Vector3 glyphPos(x + glyph.xoff, y + glyph.yoff, 0.0f);
            Math::Vector2 glyphSize(glyph.width, glyph.height);
            Math::Vector2 uvMin(glyph.x0, glyph.y0);
            Math::Vector2 uvMax(glyph.x1, glyph.y1);

            if (m_currentVertexOffset + vertexCount + 6 <= m_maxVertices && m_vertexBufferMapped)
            {
                TextVertex* v = &m_vertexBufferMapped[m_currentVertexOffset + vertexCount];

                v[0] = { Math::Vector3(glyphPos.x, glyphPos.y, 0.0f), uvMin, color };
                v[1] = { Math::Vector3(glyphPos.x + glyphSize.x, glyphPos.y, 0.0f),
                         Math::Vector2(uvMax.x, uvMin.y), color };
                v[2] = { Math::Vector3(glyphPos.x, glyphPos.y + glyphSize.y, 0.0f),
                         Math::Vector2(uvMin.x, uvMax.y), color };
                v[3] = { Math::Vector3(glyphPos.x + glyphSize.x, glyphPos.y, 0.0f),
                         Math::Vector2(uvMax.x, uvMin.y), color };
                v[4] = { Math::Vector3(glyphPos.x + glyphSize.x, glyphPos.y + glyphSize.y, 0.0f),
                         uvMax, color };
                v[5] = { Math::Vector3(glyphPos.x, glyphPos.y + glyphSize.y, 0.0f),
                         Math::Vector2(uvMin.x, uvMax.y), color };

                vertexCount += 6;
            }

            x += glyph.xadvance;
        }

        // 謠冗判螳溯｡・
        if (vertexCount > 0)
        {
            TextConstants constants{};

            constants.world = text.getWorldMatrix();
            constants.viewProjection = camera->getViewProjectionMatrix();
            constants.color = Math::Vector4(text.getColor(), text.getAlpha());

            auto& cb = m_frameCBs[frameIndex][m_currentUITextIndex];
            memcpy(cb.mapped, &constants, sizeof(TextConstants));

            context.commandList->SetGraphicsRootConstantBufferView(0, cb.buffer->GetGPUVirtualAddress());
            context.commandList->DrawInstanced(vertexCount, 1, startVertex, 0);

            m_currentVertexOffset += vertexCount;
            m_currentUITextIndex++;
        }
    }

    void UITextRenderer::release()
    {
        if (m_vertexBuffer && m_vertexBufferMapped)
        {
            m_vertexBuffer->Unmap(0, nullptr);
            m_vertexBufferMapped = nullptr;
        }

        for (auto& frameCBs : m_frameCBs)
        {
            for (auto& cb : frameCBs)
            {
                if (cb.buffer && cb.mapped)
                {
                    cb.buffer->Unmap(0, nullptr);
                    cb.mapped = nullptr;
                }
            }
        }

        m_fontAtlases.clear();
        m_vertexBuffer.Reset();
        m_rootSignature.Reset();
        m_pipelineState.Reset();
        m_srvHeap.Reset();

        m_initialized = false;
    }

    Utils::VoidResult UITextRenderer::loadFont(const std::string& fontPath)
    {
        std::ifstream file(fontPath, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            return std::unexpected(Utils::make_error(
                Utils::ErrorType::FileI0,
                std::format("Font file not found: {}", fontPath)));
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        file.seekg(0, std::ios::beg);

        m_fontData.resize(fileSize);
        file.read(reinterpret_cast<char*>(m_fontData.data()), fileSize);
        file.close();

        Utils::log_info(std::format("Font loaded: {} ({} bytes)", fontPath, fileSize));
        return {};
    }

    FontAtlas* UITextRenderer::getOrCreateAtlas(float fontSize)
    {
        int key = static_cast<int>(fontSize);
        auto it = m_fontAtlases.find(key);
        if (it != m_fontAtlases.end())
        {
            return it->second.get();
        }

        auto result = createFontAtlas(fontSize);
        if (!result) return nullptr;

        return m_fontAtlases[key].get();
    }

    Utils::VoidResult UITextRenderer::createFontAtlas(float fontSize)
    {
        if (m_fontData.empty())
        {
            return std::unexpected(Utils::make_error(
                Utils::ErrorType::Unknown, "Font data not loaded"));
        }

        auto atlas = std::make_unique<FontAtlas>();
        atlas->fontSize = fontSize;

        stbtt_fontinfo font;
        if (!stbtt_InitFont(&font, m_fontData.data(), 0))
        {
            return std::unexpected(Utils::make_error(
                Utils::ErrorType::Unknown, "Failed to initialize font"));
        }

        float scale = stbtt_ScaleForPixelHeight(&font, fontSize);

        int ascent, descent, lineGap;
        stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);
        atlas->ascent = ascent * scale;
        atlas->descent = descent * scale;
        atlas->lineGap = lineGap * scale;

        const int atlasWidth = 512;
        const int atlasHeight = 512;
        std::vector<uint8_t> atlasData(atlasWidth * atlasHeight, 0);

        int x = 0, y = 0;
        int rowHeight = 0;

        for (int c = 32; c < 127; ++c)
        {
            int w, h, xoff, yoff;
            uint8_t* bitmap = stbtt_GetCodepointBitmap(&font, 0, scale, c, &w, &h, &xoff, &yoff);
            if (!bitmap) continue;

            if (x + w >= atlasWidth)
            {
                x = 0;
                y += rowHeight;
                rowHeight = 0;
            }

            if (y + h >= atlasHeight)
            {
                stbtt_FreeBitmap(bitmap, nullptr);
                break;
            }

            for (int by = 0; by < h; ++by)
            {
                for (int bx = 0; bx < w; ++bx)
                {
                    atlasData[(y + by) * atlasWidth + (x + bx)] = bitmap[by * w + bx];
                }
            }

            FontAtlas::GlyphInfo glyph;
            glyph.x0 = static_cast<float>(x) / atlasWidth;
            glyph.y0 = static_cast<float>(y) / atlasHeight;
            glyph.x1 = static_cast<float>(x + w) / atlasWidth;
            glyph.y1 = static_cast<float>(y + h) / atlasHeight;
            glyph.xoff = static_cast<float>(xoff);
            glyph.yoff = static_cast<float>(yoff);
            glyph.width = static_cast<float>(w);
            glyph.height = static_cast<float>(h);

            int advance, lsb;
            stbtt_GetCodepointHMetrics(&font, c, &advance, &lsb);
            glyph.xadvance = advance * scale;

            atlas->glyphs[c] = glyph;
            stbtt_FreeBitmap(bitmap, nullptr);

            x += w + 1;
            rowHeight = (std::max)(rowHeight, h);
        }

        atlas->width = atlasWidth;
        atlas->height = atlasHeight;

        auto dev = m_device->getDevice();

        D3D12_RESOURCE_DESC texDesc{};
        texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Width = atlasWidth;
        texDesc.Height = atlasHeight;
        texDesc.DepthOrArraySize = 1;
        texDesc.MipLevels = 1;
        texDesc.Format = DXGI_FORMAT_R8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        CHECK_HR(dev->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
            IID_PPV_ARGS(&atlas->texture)),
            Utils::ErrorType::ResourceCreation, "Failed to create font atlas texture");

        const UINT64 uploadSize = GetRequiredIntermediateSize(atlas->texture.Get(), 0, 1);
        auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadSize);

        ComPtr<ID3D12Resource> uploadBuffer;
        CHECK_HR(dev->CreateCommittedResource(
            &uploadHeapProps, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&uploadBuffer)),
            Utils::ErrorType::ResourceCreation, "Failed to create upload buffer");

        ComPtr<ID3D12CommandAllocator> allocator;
        ComPtr<ID3D12GraphicsCommandList> cmdList;
        ComPtr<ID3D12CommandQueue> tempQueue;

        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        CHECK_HR(dev->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&tempQueue)),
            Utils::ErrorType::ResourceCreation, "Failed to create temp queue");

        CHECK_HR(dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)),
            Utils::ErrorType::ResourceCreation, "Failed to create temp allocator");

        CHECK_HR(dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&cmdList)),
            Utils::ErrorType::ResourceCreation, "Failed to create temp cmdlist");

        D3D12_SUBRESOURCE_DATA textureData{};
        textureData.pData = atlasData.data();
        textureData.RowPitch = atlasWidth;
        textureData.SlicePitch = atlasWidth * atlasHeight;

        UpdateSubresources(cmdList.Get(), atlas->texture.Get(), uploadBuffer.Get(), 0, 0, 1, &textureData);

        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            atlas->texture.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        cmdList->ResourceBarrier(1, &barrier);

        cmdList->Close();

        ID3D12CommandList* cmdLists[] = { cmdList.Get() };
        tempQueue->ExecuteCommandLists(1, cmdLists);

        ComPtr<ID3D12Fence> fence;
        CHECK_HR(dev->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)),
            Utils::ErrorType::ResourceCreation, "Failed to create fence");

        HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        CHECK_CONDITION(fenceEvent != nullptr, Utils::ErrorType::ResourceCreation, "Failed to create fence event");

        const UINT64 fenceValue = 1;
        tempQueue->Signal(fence.Get(), fenceValue);
        fence->SetEventOnCompletion(fenceValue, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
        CloseHandle(fenceEvent);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = DXGI_FORMAT_R8_UNORM;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_srvHeap->GetCPUDescriptorHandleForHeapStart();
        cpuHandle.ptr += m_currentSrvIndex * m_srvDescriptorSize;

        dev->CreateShaderResourceView(atlas->texture.Get(), &srvDesc, cpuHandle);

        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_srvHeap->GetGPUDescriptorHandleForHeapStart();
        gpuHandle.ptr += m_currentSrvIndex * m_srvDescriptorSize;
        atlas->srvGpuHandle = gpuHandle;

        m_currentSrvIndex++;

        int fontSizeKey = static_cast<int>(fontSize);
        m_fontAtlases[fontSizeKey] = std::move(atlas);

        Utils::log_info(std::format("Font atlas created: size={}", fontSize));
        return {};
    }

    Utils::VoidResult UITextRenderer::createRootSignature()
    {
        auto dev = m_device->getDevice();

        CD3DX12_DESCRIPTOR_RANGE1 ranges[1]{};
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

        CD3DX12_ROOT_PARAMETER1 rootParams[2]{};
        rootParams[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
        rootParams[1].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

        D3D12_STATIC_SAMPLER_DESC sampler{};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.MipLODBias = 0.0f;
        sampler.MaxAnisotropy = 0;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        sampler.MinLOD = 0.0f;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = 0;
        sampler.RegisterSpace = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc;
        rootSigDesc.Init_1_1(_countof(rootParams), rootParams, 1, &sampler,
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> sig, err;
        CHECK_HR(D3DX12SerializeVersionedRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &sig, &err),
            Utils::ErrorType::ResourceCreation, "Failed to serialize root signature");

        CHECK_HR(dev->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)),
            Utils::ErrorType::ResourceCreation, "Failed to create root signature");

        return {};
    }

    Utils::VoidResult UITextRenderer::createPipelineState()
    {
        auto dev = m_device->getDevice();

        auto& settings = Engine::Core::ProjectSettings::get();

        Graphics::ShaderCompileDesc vsDesc;
        vsDesc.filePath = settings.getEngineAssetPath("shaders/UITextVS.cso").string();
        vsDesc.entryPoint = "main";
        vsDesc.type = Graphics::ShaderType::Vertex;

        auto vsResult = m_shaderManager->loadShader(vsDesc);
        if (!vsResult)
        {
            return std::unexpected(Utils::make_error(
                Utils::ErrorType::ShaderCompilation, "Failed to load UIText vertex shader"));
        }

        Graphics::ShaderCompileDesc psDesc;
        psDesc.filePath = settings.getEngineAssetPath("shaders/UITextPS.cso").string();
        psDesc.entryPoint = "main";
        psDesc.type = Graphics::ShaderType::Pixel;

        auto psResult = m_shaderManager->loadShader(psDesc);
        if (!psResult)
        {
            return std::unexpected(Utils::make_error(
                Utils::ErrorType::ShaderCompilation, "Failed to load UIText pixel shader"));
        }

        D3D12_INPUT_ELEMENT_DESC inputLayout[] =
        {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = { vsResult->getBytecode(), vsResult->getBytecodeSize() };
        psoDesc.PS = { psResult->getBytecode(), psResult->getBytecodeSize() };
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
        psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

        psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleDesc.Count = 1;

        CHECK_HR(dev->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)),
            Utils::ErrorType::ResourceCreation, "Failed to create PSO");

        return {};
    }

    Utils::VoidResult UITextRenderer::createVertexBuffer()
    {
        const UINT bufferSize = sizeof(TextVertex) * m_maxVertices;

        auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

        CHECK_HR(m_device->getDevice()->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&m_vertexBuffer)),
            Utils::ErrorType::ResourceCreation, "Failed to create vertex buffer");

        D3D12_RANGE readRange{ 0, 0 };
        CHECK_HR(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_vertexBufferMapped)),
            Utils::ErrorType::ResourceCreation, "Failed to map vertex buffer");

        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.SizeInBytes = bufferSize;
        m_vertexBufferView.StrideInBytes = sizeof(TextVertex);

        return {};
    }

    Utils::VoidResult UITextRenderer::createConstantBuffer()
    {
        const UINT cbSize = (sizeof(TextConstants) + 255) & ~255;
        auto dev = m_device->getDevice();

        for (uint32_t frameIdx = 0; frameIdx < kFrameCount; ++frameIdx)
        {
            for (uint32_t textIdx = 0; textIdx < kMaxUITexts; ++textIdx)
            {
                auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
                auto desc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);

                CHECK_HR(dev->CreateCommittedResource(
                    &heapProps,
                    D3D12_HEAP_FLAG_NONE,
                    &desc,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_PPV_ARGS(&m_frameCBs[frameIdx][textIdx].buffer)
                ), Utils::ErrorType::ResourceCreation, "Failed to create UIText CB");

                D3D12_RANGE readRange{ 0, 0 };
                CHECK_HR(
                    m_frameCBs[frameIdx][textIdx].buffer->Map(
                        0, &readRange,
                        reinterpret_cast<void**>(&m_frameCBs[frameIdx][textIdx].mapped)),
                    Utils::ErrorType::ResourceCreation,
                    "Failed to map UIText CB");
            }
        }

        return {};
    }

    Utils::VoidResult UITextRenderer::createDescriptorHeap()
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
        heapDesc.NumDescriptors = 100;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        CHECK_HR(m_device->getDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_srvHeap)),
            Utils::ErrorType::ResourceCreation, "Failed to create SRV heap");

        m_srvDescriptorSize = m_device->getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        return {};
    }
}


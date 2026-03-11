// src/Graphics/CubeRenderer.cpp
#include "CubeRenderer.hpp"
#include "../engine/Core/ProjectSettings.hpp"
#include <format>

namespace Renderer
{
    Utils::VoidResult CubeRenderer::initialize(Device* device)
    {
       
        CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
        CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");

        m_device = device;

        Utils::log_info("Initializing Cube Renderer...");

        auto constantBufferResult = m_constantBufferManager.initialize(device, Utils::GetRequiredConstantBufferCount());
        if (!constantBufferResult) {
            Utils::log_error(constantBufferResult.error());
            return constantBufferResult;
        }
    
        setupCubeVertices();

        updateWorldMatrix();

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

    void CubeRenderer::render(const Utils::RenderContext& context)
    {
        if (!context.psoCache) return;
        auto* pso = context.psoCache->getDefaultPBR();
        if (!pso) return;

        if (!m_material && m_materialManager)
            m_material = m_materialManager->getDefaultMaterial();

        // デバッグログ（120フレームに1回）
        static int renderCounter = 0;
        if (renderCounter++ % 120 == 0)
        {
            Utils::log_info(std::format(
                "CubeRenderer - View: {}, BufferIndex: {}, Camera: ({:.2f}, {:.2f}, {:.2f})",
                context.getViewTypeName(),
                context.getConstantBufferIndex(),
                context.camera->getPosition().x,
                context.camera->getPosition().y,
                context.camera->getPosition().z));
        }

        const uint32_t bufferIndex = context.getConstantBufferIndex();

        // 定数バッファ更新
        CameraConstants cameraConstants{};
        cameraConstants.viewMatrix = context.camera->getViewMatrix();
        cameraConstants.projectionMatrix = context.camera->getProjectionMatrix();
        cameraConstants.viewProjectionMatrix = context.camera->getViewProjectionMatrix();
        cameraConstants.cameraPosition = context.camera->getPosition();
        m_constantBufferManager.updateCameraConstants(bufferIndex, cameraConstants);

        ObjectConstants objectConstants{};
        objectConstants.worldMatrix = m_worldMatrix;
        objectConstants.worldViewProjectionMatrix = context.camera->getViewProjectionMatrix() * m_worldMatrix;
        objectConstants.objectPosition = m_position;
        m_constantBufferManager.updateObjectConstants(bufferIndex, objectConstants);

        // PSO・RootSignature をキャッシュから借りてセット
        context.commandList->SetGraphicsRootSignature(pso->getRootSignature());
        context.commandList->SetPipelineState(pso->getPipelineState());

        // スロット番号は PBRRootSlots 定数で指定
        context.commandList->SetGraphicsRootConstantBufferView(
            PBRRootSlots::Camera,
            m_constantBufferManager.getCameraConstantsGPUAddress(bufferIndex));
        context.commandList->SetGraphicsRootConstantBufferView(
            PBRRootSlots::Object,
            m_constantBufferManager.getObjectConstantsGPUAddress(bufferIndex));

        if (m_material && m_material->getConstantBuffer())
        {
            context.commandList->SetGraphicsRootConstantBufferView(
                PBRRootSlots::Material,
                m_material->getConstantBuffer()->GetGPUVirtualAddress());
        }

        ID3D12DescriptorHeap* heaps[] = { m_device->getSrvHeap() };
        context.commandList->SetDescriptorHeaps(1, heaps);

        if (m_material)
        {
            auto tex = m_material->getTexture(TextureType::Albedo);
            if (tex)
            {
                context.commandList->SetGraphicsRootDescriptorTable(
                    PBRRootSlots::Textures,
                    tex->getSRVHandle());
            }
        }

        context.commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context.commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
        context.commandList->IASetIndexBuffer(&m_indexBufferView);
        context.commandList->DrawIndexedInstanced(36, 1, 0, 0, 0);
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


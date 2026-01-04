// src/Graphics/ConstantBuffer.hpp
#pragma once

#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include "Device.hpp"
#include "../Math/Math.hpp"
#include "../Utils/Common.hpp"

using Microsoft::WRL::ComPtr;

namespace Engine::Graphics
{
    // カメラ用定数バッファ
    struct CameraConstants
    {
        Math::Matrix4 viewMatrix;
        Math::Matrix4 projectionMatrix;
        Math::Matrix4 viewProjectionMatrix;
        Math::Vector3 cameraPosition;
        float padding;  // 16バイト境界にアライン
    };

    // オブジェクト用定数バッファ
    struct ObjectConstants
    {
        Math::Matrix4 worldMatrix;
        Math::Matrix4 worldViewProjectionMatrix;
        Math::Vector3 objectPosition;
        float padding;
    };

    // 定数バッファ管理クラス
    template<typename T>
    class ConstantBuffer
    {
    public:
        ConstantBuffer() = default;
        ~ConstantBuffer() = default;

        // コピー・ムーブ禁止
        ConstantBuffer(const ConstantBuffer&) = delete;
        ConstantBuffer& operator=(const ConstantBuffer&) = delete;
        ConstantBuffer(ConstantBuffer&&) = delete;
        ConstantBuffer& operator=(ConstantBuffer&&) = delete;

        // 初期化
        [[nodiscard]] Utils::VoidResult initialize(Device* device, UINT frameCount = 2)
        {
            m_device = device;
            m_frameCount = frameCount;

            // 定数バッファのサイズ（256バイト境界にアライン）
            m_alignedSize = (sizeof(T) + 255) & ~255;

            // フレーム数分の定数バッファを作成
            m_constantBuffers.resize(frameCount);
            m_mappedData.resize(frameCount);

            for (UINT i = 0; i < frameCount; ++i)
            {
                auto result = createConstantBuffer(i);
                if (!result) return result;
            }

            return {};
        }

        // データを更新
        void updateData(UINT frameIndex, const T& data)
        {
            if (frameIndex < m_frameCount && m_mappedData[frameIndex])
            {
                memcpy(m_mappedData[frameIndex], &data, sizeof(T));
            }
        }

        // GPUアドレスを取得
        D3D12_GPU_VIRTUAL_ADDRESS getGPUAddress(UINT frameIndex) const
        {
            if (frameIndex < m_frameCount && m_constantBuffers[frameIndex])
            {
                return m_constantBuffers[frameIndex]->GetGPUVirtualAddress();
            }
            return 0;
        }

        // 有効かチェック
        [[nodiscard]] bool isValid() const
        {
            return !m_constantBuffers.empty() && m_constantBuffers[0] != nullptr;
        }

    private:
        Device* m_device = nullptr;
        UINT m_frameCount = 0;
        UINT m_alignedSize = 0;

        std::vector<ComPtr<ID3D12Resource>> m_constantBuffers;
        std::vector<void*> m_mappedData;

        [[nodiscard]] Utils::VoidResult createConstantBuffer(UINT index)
        {
            // アップロードヒープのプロパティ
            D3D12_HEAP_PROPERTIES heapProps{};
            heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
            heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            heapProps.CreationNodeMask = 1;
            heapProps.VisibleNodeMask = 1;

            // リソース記述子
            D3D12_RESOURCE_DESC resourceDesc{};
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resourceDesc.Alignment = 0;
            resourceDesc.Width = m_alignedSize;
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
                IID_PPV_ARGS(&m_constantBuffers[index])),
                Utils::ErrorType::ResourceCreation, "Failed to create constant buffer");

            // メモリマッピング
            D3D12_RANGE readRange{ 0, 0 };  // CPUから読み取らない
            CHECK_HR(m_constantBuffers[index]->Map(0, &readRange, &m_mappedData[index]),
                Utils::ErrorType::ResourceCreation, "Failed to map constant buffer");

            return {};
        }
    };

    // 定数バッファマネージャー
    class ConstantBufferManager
    {
    public:
        ConstantBufferManager() = default;
        ~ConstantBufferManager() = default;

        // コピー・ムーブ禁止
        ConstantBufferManager(const ConstantBufferManager&) = delete;
        ConstantBufferManager& operator=(const ConstantBufferManager&) = delete;
        ConstantBufferManager(ConstantBufferManager&&) = delete;
        ConstantBufferManager& operator=(ConstantBufferManager&&) = delete;

        // 初期化
        [[nodiscard]] Utils::VoidResult initialize(Device* device, UINT frameCount = 2);

        // カメラ定数を更新
        void updateCameraConstants(UINT frameIndex, const CameraConstants& constants);

        // オブジェクト定数を更新
        void updateObjectConstants(UINT frameIndex, const ObjectConstants& constants);

        // GPUアドレスを取得
        D3D12_GPU_VIRTUAL_ADDRESS getCameraConstantsGPUAddress(UINT frameIndex) const;
        D3D12_GPU_VIRTUAL_ADDRESS getObjectConstantsGPUAddress(UINT frameIndex) const;

        // 有効かチェック
        [[nodiscard]] bool isValid() const;

    private:
        ConstantBuffer<CameraConstants> m_cameraConstants;
        ConstantBuffer<ObjectConstants> m_objectConstants;
    };
}

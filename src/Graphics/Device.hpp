// src/Graphics/Device.hpp
#pragma once

#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <string>
#include "../Utils/Common.hpp"

using Microsoft::WRL::ComPtr;

namespace Engine::Graphics
{
    // =============================================================================
    // アダプター情報構造体
    // =============================================================================

    //GPUアダプターの情報
    struct AdapterInfo
    {
        std::wstring description;       // アダプター名
        size_t dedicatedVideoMemory;    // 専用ビデオメモリ (バイト)
        size_t dedicatedSystemMemory;   // 専用システムメモリ (バイト)
        size_t sharedSystemMemory;      // 共有システムメモリ (バイト)
        bool isHardware;                // ハードウェアアダプターか
        UINT vendorId;                  // ベンダーID
        UINT deviceId;                  // デバイスID

        /// @brief メモリ情報を文字列で取得
        [[nodiscard]] std::string getMemoryInfoString() const noexcept;
    };

    // =============================================================================
    // デバイス設定構造体
    // =============================================================================

    //デバイス作成時の設定
    struct DeviceSettings
    {
        bool enableDebugLayer = true;           // デバッグレイヤーを有効にするか（デバッグビルドのみ）
        bool enableGpuValidation = false;      // GPU検証を有効にするか（重い）
        D3D_FEATURE_LEVEL minFeatureLevel = D3D_FEATURE_LEVEL_11_0;  // 最小機能レベル
        bool preferHighPerformanceAdapter = true;  // 高性能アダプターを優先するか
    };

    // =============================================================================
    // DescriptorHandlePair
    //==============================================================================
    struct DescriptorHandlePair
    {
        D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle;
        UINT index;
    };

    // =============================================================================
    // Deviceクラス
    // =============================================================================

    //DirectX 12デバイスの作成と管理を行うクラス
    class Device
    {
    public:
        //コンストラクタ
        Device() = default;

        //デストラクタ
        ~Device() = default;

        // コピー禁止
        Device(const Device&) = delete;
        Device& operator=(const Device&) = delete;

        // ムーブ許可
        Device(Device&&) noexcept = default;
        Device& operator=(Device&&) noexcept = default;

        //デバイスを初期化
        [[nodiscard]] Utils::VoidResult initialize(const DeviceSettings& settings = {});

        //利用可能なアダプターを列挙
        [[nodiscard]] std::vector<AdapterInfo> enumerateAdapters() const;

        //D3D12デバイスを取得
        [[nodiscard]] ID3D12Device* getDevice() const noexcept { return m_device.Get(); }

        //DXGIファクトリを取得
        [[nodiscard]] IDXGIFactory4* getDXGIFactory() const noexcept { return m_dxgiFactory.Get(); }

        //デバイスが有効かチェック
        [[nodiscard]] bool isValid() const noexcept { return m_device != nullptr; }

        //現在のアダプター情報を取得
        [[nodiscard]] const AdapterInfo& getCurrentAdapterInfo() const noexcept { return m_currentAdapterInfo; }

        //サポートされている機能レベルを取得
        [[nodiscard]] D3D_FEATURE_LEVEL getFeatureLevel() const noexcept { return m_featureLevel; }

        //デバッグレイヤーが有効かチェック
        [[nodiscard]] bool isDebugLayerEnabled() const noexcept { return m_debugLayerEnabled; }

        //デスクリプタのインクリメントサイズを取得
        [[nodiscard]] UINT getDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE heapType) const noexcept;

        //機能サポートチェック
        [[nodiscard]] bool checkFeatureSupport(D3D12_FEATURE feature, void* pFeatureSupportData, UINT featureSupportDataSize) const noexcept;

        // SRVヒープのハンドル取得
        ID3D12DescriptorHeap* getSrvHeap() const noexcept { return m_srvHeap.Get(); }
        D3D12_CPU_DESCRIPTOR_HANDLE getSrvCpuStart() const noexcept {
            return m_srvHeap ? m_srvHeap->GetCPUDescriptorHandleForHeapStart() : D3D12_CPU_DESCRIPTOR_HANDLE{};
        }
        D3D12_GPU_DESCRIPTOR_HANDLE getSrvGpuStart() const noexcept {
            return m_srvHeap ? m_srvHeap->GetGPUDescriptorHandleForHeapStart() : D3D12_GPU_DESCRIPTOR_HANDLE{};
        }
        // 1つSRVを確保して、そのインデックスを返す（超簡易アロケータ）
        UINT allocateSrvIndex() noexcept { return m_srvAllocated++; }

        // SRVディスクリプタを1つ確保し、CPUとGPUのハンドルペアを返す
        [[nodiscard]] DescriptorHandlePair allocateSrvDescriptor() noexcept
        {
            UINT index = m_srvAllocated++;
            UINT descriptorSize = getDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            DescriptorHandlePair pair;
            pair.index = index;
            pair.CpuHandle = getSrvCpuStart();
            pair.CpuHandle.ptr += index * descriptorSize;
            pair.GpuHandle = getSrvGpuStart();
            pair.GpuHandle.ptr += index * descriptorSize;

            return pair;
        }

        // グラフィックスキュー
        ID3D12CommandQueue* getGraphicsQueue() const noexcept { return m_graphicsQueue.Get(); }

        void waitForGpu(); // 簡易同期

    private:
        // =============================================================================
        // メンバ変数
        // =============================================================================

        ComPtr<ID3D12Device> m_device;              // D3D12デバイス
        ComPtr<IDXGIFactory4> m_dxgiFactory;        // DXGIファクトリ
        ComPtr<IDXGIAdapter1> m_adapter;            // 選択されたアダプター
        ComPtr<ID3D12DescriptorHeap> m_srvHeap;

        AdapterInfo m_currentAdapterInfo{};         // 現在のアダプター情報
        D3D_FEATURE_LEVEL m_featureLevel = D3D_FEATURE_LEVEL_11_0;  // サポート機能レベル
        bool m_debugLayerEnabled = false;           // デバッグレイヤーが有効か

        // デスクリプタサイズのキャッシュ
        UINT m_rtvDescriptorSize = 0;
        UINT m_dsvDescriptorSize = 0;
        UINT m_cbvSrvUavDescriptorSize = 0;
        UINT m_samplerDescriptorSize = 0;

        UINT m_srvAllocated = 0; // SRVを何個割り当てたかの簡易カウンタ

        ComPtr<ID3D12CommandQueue> m_graphicsQueue;
        ComPtr<ID3D12Fence>        m_fence;
        UINT64 m_fenceValue = 0;
        HANDLE m_fenceEvent = nullptr;

        // =============================================================================
        // プライベートメソッド
        // =============================================================================

        //デバッグレイヤーを初期化
        [[nodiscard]] Utils::VoidResult initializeDebugLayer(const DeviceSettings& settings);

        /// DXGIファクトリを作成
        [[nodiscard]] Utils::VoidResult createDXGIFactory();

        //最適なアダプターを選択
        [[nodiscard]] Utils::VoidResult selectBestAdapter(const DeviceSettings& settings);

        //D3D12デバイスを作成
        [[nodiscard]] Utils::VoidResult createDevice(const DeviceSettings& settings);

        //デスクリプタサイズをキャッシュ
        void cacheDescriptorSizes();

        //アダプター情報を取得
        [[nodiscard]] AdapterInfo getAdapterInfo(IDXGIAdapter1* adapter) const;

        //アダプターがD3D12対応かチェック
        [[nodiscard]] bool isAdapterCompatible(IDXGIAdapter1* adapter, D3D_FEATURE_LEVEL minFeatureLevel) const;

        [[nodiscard]] Utils::VoidResult createSrvHeap(UINT numDescriptors = 1024);

        [[nodiscard]] Utils::VoidResult createGraphicsQueue();

        
    };
}

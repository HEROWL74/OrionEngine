//src/Graphics/Device.cpp
#include "Device.hpp"
#include <format>
#include <algorithm>

namespace Engine::Graphics
{
    // =============================================================================
   // AdapterInfo螳溯｣・
   // =============================================================================

    std::string AdapterInfo::getMemoryInfoString() const noexcept
    {
        const auto toMB = [](size_t bytes) {return bytes / (1024 * 1024); };

        return std::format(
            "Video: {}MB, System: {}MB, Shared: {}MB",
            toMB(dedicatedVideoMemory),
            toMB(dedicatedSystemMemory),
            toMB(sharedSystemMemory)
        );
    }
    // =============================================================================
    //Device螳溯｣・
    // =============================================================================

    Utils::VoidResult Device::initialize(const DeviceSettings& settings)
    {
        Utils::log_info("Initializing Graphics Device...");

        //繝・ヰ繝・げ繝ｬ繧､繝､繝ｼ縺ｮ蛻晄悄蛹・
        auto debugResult = initializeDebugLayer(settings);
        if (!debugResult)
        {
            return debugResult;
        }

        //DXGI繝輔ぃ繧ｯ繝医Μ縺ｮ菴懈・
        auto factoryResult = createDXGIFactory();
        if (!factoryResult)
        {
            return factoryResult;
        }

        //譛驕ｩ縺ｪ繧｢繝繝励ち繝ｼ縺ｮ驕ｸ謚・
        auto adapterResult = selectBestAdapter(settings);
        if (!adapterResult)
        {
            return adapterResult;
        }

        //D3D12繝・ヰ繧､繧ｹ縺ｮ菴懈・
        auto deviceResult = createDevice(settings);
        if (!deviceResult)
        {
            return deviceResult;
        }

      
        cacheDescriptorSizes();

        auto queueResult = createGraphicsQueue();
        if (!queueResult) { return queueResult; }
        auto srvHeapResult = createSrvHeap(1024);
        if (!srvHeapResult) { return srvHeapResult; }

        Utils::log_info(std::format("Graphics Device initialized successfully"));
        Utils::log_info(std::format("Selected Adapter: {}",
            std::string(m_currentAdapterInfo.description.begin(),
                m_currentAdapterInfo.description.end())));
        Utils::log_info(std::format("Memory: {}", m_currentAdapterInfo.getMemoryInfoString()));
        Utils::log_info(std::format("Feature Level: 0x{:04X}", static_cast<unsigned>(m_featureLevel)));

        return {};
    }

    std::vector<AdapterInfo> Device::enumerateAdapters() const
    {
        std::vector<AdapterInfo> adapters;

        if (!m_dxgiFactory)
        {
            return adapters;
        }

        ComPtr<IDXGIAdapter1> adapter;
        for (UINT i = 0; SUCCEEDED(m_dxgiFactory->EnumAdapters1(i, &adapter)); ++i) 
        {
            adapters.push_back(getAdapterInfo(adapter.Get()));
            adapter.Reset();
        }

        return adapters;
    }

    UINT Device::getDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE heapType) const noexcept
    {
        if (!m_device)
        {
            return 0;
        }

        switch (heapType)
        {
        case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
            return m_rtvDescriptorSize;
        case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
            return m_dsvDescriptorSize;
        case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
            return m_cbvSrvUavDescriptorSize;
        case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
            return m_samplerDescriptorSize;
        default:
            return m_device->GetDescriptorHandleIncrementSize(heapType);
        }
    }

    bool Device::checkFeatureSupport(D3D12_FEATURE feature, void* pFeatureSupportData, UINT featureSupportDataSize) const noexcept
    {
        if (!m_device)
        {
            return false;
        }

        return SUCCEEDED(m_device->CheckFeatureSupport(feature, pFeatureSupportData, featureSupportDataSize));
    }

    //======================================================================
    //繝励Λ繧､繝吶・繝医Γ繧ｽ繝・ラ螳溯｣・

    Utils::VoidResult Device::initializeDebugLayer(const DeviceSettings& settings)
    {
        #ifdef _DEBUG
        if (settings.enableDebugLayer)
        {
            ComPtr<ID3D12Debug> debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
            {
                debugController->EnableDebugLayer();
                m_debugLayerEnabled = true;
                Utils::log_info("D3D12 Debug Layer enabled");

                //GPU讀懆ｨｼ縺ｮ譛牙柑蛹・
                if (settings.enableGpuValidation)
                {
                    ComPtr<ID3D12Debug1> debugController1;
                    if (SUCCEEDED(debugController.As(&debugController1)))
                    {
                        debugController1->SetEnableGPUBasedValidation(TRUE);
                        Utils::log_info("GPU-based validation enabled");
                    }
                }
            }
            else
            {
                Utils::log_warning("Failed to enable D3D12 Debug Layer");
            }
        }
        #else
        //繝ｪ繝ｪ繝ｼ繧ｹ繝薙Ν繝峨〒縺ｯ縲√ョ繝舌ャ繧ｰ繝ｬ繧､繝､繝ｼ繧堤┌蜉ｹ縺ｫ縺吶ｋ
        UNREFERENCED_PARAMETER(settings);
        #endif

        return {};
    }

    Utils::VoidResult Device::createDXGIFactory()
    {
        UINT dxgiFactoryFlags = 0;

        #ifdef _DEBUG
        if (m_debugLayerEnabled)
        {
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
        #endif

        CHECK_HR(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_dxgiFactory)),
            Utils::ErrorType::DeviceCreation, "Failed to create DXGI factory");

        return {};
    }

    Utils::VoidResult Device::selectBestAdapter(const DeviceSettings& settings)
    {
        ComPtr<IDXGIAdapter1> bestAdapter;
        size_t maxVideoMemory = 0;
        bool foundHardwareAdapter = false;

        ComPtr<IDXGIAdapter1> adapter;
        for (UINT i = 0; SUCCEEDED(m_dxgiFactory->EnumAdapters1(i, &adapter)); ++i)
        {
            AdapterInfo info = getAdapterInfo(adapter.Get());

            //D3D12蟇ｾ蠢懊メ繧ｧ繝・け
            if (!isAdapterCompatible(adapter.Get(), settings.minFeatureLevel))
            {
                Utils::log_info(std::format("Skipping incompatible adapter: {}",
                    std::string(info.description.begin(), info.description.end())));

                adapter.Reset();
                continue;
            }

            Utils::log_info(std::format("Found compatible adapter: {} ({})",
                std::string(info.description.begin(), info.description.end()),
                info.getMemoryInfoString()));

            //繧｢繝繝励ち繝ｼ驕ｸ謚槭Ο繧ｸ繝・け
            bool shouldSelect = false;

            if (settings.preferHighPerformanceAdapter)
            {
                // 鬮俶ｧ閭ｽ蜆ｪ蜈茨ｼ壹ワ繝ｼ繝峨え繧ｧ繧｢繧｢繝繝励ち繝ｼ縺ｧ譛螟ｧ繝薙ョ繧ｪ繝｡繝｢繝ｪ
                if (info.isHardware && info.dedicatedVideoMemory > maxVideoMemory)
                {
                    shouldSelect = true;
                }
                else if (!foundHardwareAdapter && info.isHardware)
                {
                    shouldSelect = true;
                }
            }
            else
            {
                //譛蛻昴↓隕九▽縺九▲縺溷ｯｾ蠢懊い繝繝励ち繝ｼ繧剃ｽｿ逕ｨ
                if (!bestAdapter)
                {
                    shouldSelect = true;
                }
            }

            if (shouldSelect)
            {
                bestAdapter = adapter;
                m_currentAdapterInfo = info;
                maxVideoMemory = info.dedicatedVideoMemory;
                foundHardwareAdapter = info.isHardware;
            }

            adapter.Reset();
        }

        CHECK_CONDITION(bestAdapter != nullptr, Utils::ErrorType::DeviceCreation,
            "No compatible D3D12 adapter found");

        m_adapter = bestAdapter;
        return {};
    }

    Utils::VoidResult Device::createDevice(const DeviceSettings& settings)
    {
        //繧ｵ繝昴・繝医＆繧後※縺・ｋ譛鬮倥・讖溯・繝ｬ繝吶Ν繧定ｩｦ陦・
        const D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_12_2,
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0
        };

        HRESULT hr = E_FAIL;
        for (const auto& level : featureLevels)
        {
            if (level < settings.minFeatureLevel)
            {
                continue;
            }

            hr = D3D12CreateDevice(m_adapter.Get(), level, IID_PPV_ARGS(&m_device));
            if (SUCCEEDED(hr))
            {
                m_featureLevel = level;
                break;
            }
        }

        CHECK_HR(hr, Utils::ErrorType::DeviceCreation,
            "Failed to create D3D12 device with required feature level");

        return {};
    }

    void Device::cacheDescriptorSizes()
    {
        if (!m_device)
        {
            return;
        }

        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        m_cbvSrvUavDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        m_samplerDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    }

    AdapterInfo Device::getAdapterInfo(IDXGIAdapter1* adapter) const
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        AdapterInfo info{};
        info.description = desc.Description;
        info.dedicatedVideoMemory = desc.DedicatedVideoMemory;
        info.dedicatedSystemMemory = desc.DedicatedSystemMemory;
        info.sharedSystemMemory = desc.SharedSystemMemory;
        info.isHardware = !(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE);
        info.vendorId = desc.VendorId;
        info.deviceId = desc.DeviceId;

        return info;
    }

    bool Device::isAdapterCompatible(IDXGIAdapter1* adapter, D3D_FEATURE_LEVEL minFeatureLevel) const
    {
        ComPtr<ID3D12Device> testDevice;
        HRESULT hr = D3D12CreateDevice(adapter, minFeatureLevel, IID_PPV_ARGS(&testDevice));

        return SUCCEEDED(hr);
    }

    Utils::VoidResult Device::createGraphicsQueue()
    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

        CHECK_HR(m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_graphicsQueue)),
            Utils::ErrorType::DeviceCreation, "Failed to create graphics command queue");

        // フェンス & イベント
        CHECK_HR(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)),
            Utils::ErrorType::DeviceCreation, "Failed to create fence");
        m_fenceValue = 1;
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

        return {};
    }

    Utils::VoidResult Device::createSrvHeap(UINT numDescriptors)
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = numDescriptors;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        CHECK_HR(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_srvHeap)),
            Utils::ErrorType::ResourceCreation, "Failed to create SRV descriptor heap");

        m_srvAllocated = 0;
        return {};
    }

    void Device::waitForGpu()
    {
        if (!m_graphicsQueue || !m_fence) return;

        const UINT64 fenceToWait = m_fenceValue++;
        m_graphicsQueue->Signal(m_fence.Get(), fenceToWait);
        if (m_fence->GetCompletedValue() < fenceToWait)
        {
            m_fence->SetEventOnCompletion(fenceToWait, m_fenceEvent);
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }
    }
}

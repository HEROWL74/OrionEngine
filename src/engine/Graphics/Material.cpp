//src/Graphics/Material.cpp
#include "Material.hpp"
//#include "Texture.hpp" 
#include <format>

namespace Engine::Graphics
{
    //=========================================================================
    // Material実装
    //=========================================================================

    Material::Material(const std::string& name)
        : m_name(name)
    { }

    Utils::VoidResult Material::initialize(Device* device)
    {
        Utils::log_info(std::format("=== Material::initialize START for '{}' ===", m_name));
        Utils::log_info(std::format("Current m_initialized state: {}", m_initialized));

        if (m_initialized)
        {
            Utils::log_info(std::format("Material '{}' already initialized, returning", m_name));
            return {};
        }

        CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
        CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");

        m_device = device;
        Utils::log_info(std::format("Device assigned to material '{}'", m_name));

        Utils::log_info(std::format("Calling createConstantBuffer for '{}'", m_name));
        auto cbResult = createConstantBuffer();
        if (!cbResult)
        {
            Utils::log_warning(std::format("createConstantBuffer failed for '{}': {}", m_name, cbResult.error().message));
            return cbResult;
        }
        Utils::log_info(std::format("createConstantBuffer succeeded for '{}'", m_name));

        Utils::log_info(std::format("Calling createDescriptors for '{}'", m_name));
        auto descResult = createDescriptors();
        if (!descResult)
        {
            Utils::log_warning(std::format("createDescriptors failed for '{}': {}", m_name, descResult.error().message));
            return descResult;
        }
        Utils::log_info(std::format("createDescriptors succeeded for '{}'", m_name));


        Utils::log_info(std::format("Setting m_initialized = true for '{}'", m_name));
        m_initialized = true;
        Utils::log_info(std::format("m_initialized is now: {} for '{}'", m_initialized, m_name));

        Utils::log_info(std::format("=== Material::initialize COMPLETE for '{}' ===", m_name));
        return {};
    }

    void Material::setTexture(TextureType type, std::shared_ptr<Texture> texture)
    {
        if (texture)
        {
            m_textures[type] = texture;
        }
        else
        {
            removeTexture(type);
        }
        m_isDirty = true;
    }

    std::shared_ptr<Texture> Material::getTexture(TextureType type) const
    {
        auto it = m_textures.find(type);
        return (it != m_textures.end()) ? it->second : nullptr;
    }

    bool Material::hasTexture(TextureType type) const
    {
        return m_textures.find(type) != m_textures.end();
    }

    void Material::removeTexture(TextureType type)
    {
        auto it = m_textures.find(type);
        if (it != m_textures.end())
        {
            m_textures.erase(it);
            if (type == TextureType::Albedo)
            {
                m_properties.useAlbedoTex = 0;
            }
            m_isDirty = true;
            updateConstantBuffer();
        }
    }

    Utils::VoidResult Material::updateConstantBuffer()
    {
        Utils::log_info(std::format("=== updateConstantBuffer called for '{}' ===", m_name));
        Utils::log_info(std::format("m_initialized: {}", m_initialized));
        Utils::log_info(std::format("m_device: {}", m_device ? "not null" : "null"));
        Utils::log_info(std::format("m_constantBufferData: {}", m_constantBufferData ? "not null" : "null"));

   
        if (!m_initialized) {
            Utils::log_warning(std::format("Material '{}' not initialized (m_initialized = {})", m_name, m_initialized));
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
                std::format("Material '{}' not initialized", m_name)));
        }

        if (!m_constantBufferData) {
            Utils::log_warning(std::format("Material '{}' constant buffer data is null", m_name));
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
                std::format("Material '{}' constant buffer data is null", m_name)));
        }

        if (!m_device) {
            Utils::log_warning(std::format("Material '{}' device is null", m_name));
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
                std::format("Material '{}' device is null", m_name)));
        }

        if (!m_device->isValid()) {
            Utils::log_warning(std::format("Material '{}' device is not valid", m_name));
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
                std::format("Material '{}' device is not valid", m_name)));
        }

        Utils::log_info(std::format("All checks passed, updating constant buffer for '{}'", m_name));

        MaterialConstantBuffer cbData{};

   
        cbData.albedo = Math::Vector4(
            m_properties.albedo.x,
            m_properties.albedo.y,
            m_properties.albedo.z,
            m_properties.metallic
        );

        cbData.roughnessAoEmissiveStrength = Math::Vector4(
            m_properties.roughness,
            m_properties.ao,
            m_properties.emissiveStrength,
            0.0f
        );

        cbData.emissive = Math::Vector4(
            m_properties.emissive.x,
            m_properties.emissive.y,
            m_properties.emissive.z,
            m_properties.normalStrength
        );

        cbData.alphaParams = Math::Vector4(
            m_properties.alpha,
            m_properties.useAlphaTest ? 1.0f : 0.0f,
            m_properties.alphaTestThreshold,
            m_properties.heightScale
        );

        cbData.uvTransform = Math::Vector4(
            m_properties.uvScale.x,
            m_properties.uvScale.y,
            m_properties.uvOffset.x,
            m_properties.uvOffset.y
        );

        cbData.hasAlbedoTexture = (m_properties.useAlbedoTex != 0) ? 1 : 0;  // intで送る


       
        memcpy(m_constantBufferData, &cbData, sizeof(MaterialConstantBuffer));

        m_isDirty = false;
        Utils::log_info(std::format("updateConstantBuffer completed successfully for '{}'", m_name));
        return {};
    }

    void Material::bind(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex) const
    {
        if (!m_initialized)
        {
            Utils::log_warning(std::format("Attempting to bind uninitialized material '{}'", m_name));
            return;
        }

        if (!m_constantBuffer)
        {
            Utils::log_warning(std::format("No constant buffer available for material '{}'", m_name));
            return;
        }

   
        commandList->SetGraphicsRootConstantBufferView(
            rootParameterIndex,
            m_constantBuffer->GetGPUVirtualAddress()
        );

     
        /*
        if (m_cbvDescriptorHeap)
        {
            ID3D12DescriptorHeap* heaps[] = { m_cbvDescriptorHeap.Get() };
            commandList->SetDescriptorHeaps(1, heaps);
            commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, m_cbvGpuHandle);
        }
        */
    }
    Utils::VoidResult Material::createConstantBuffer()
    {
        if (!m_device) {
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
                std::format("Device is null for material '{}'", m_name)));
        }

        if (!m_device->isValid()) {
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
                std::format("Device is not valid for material '{}'", m_name)));
        }

        if (!m_device->getDevice()) {
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
                std::format("D3D12 device is null for material '{}'", m_name)));
        }

        const UINT constantBufferSize = (sizeof(MaterialConstantBuffer) + 255) & ~255;

        Utils::log_info(std::format("Creating constant buffer of size {} bytes for material '{}'",
            constantBufferSize, m_name));

  
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

       
        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = constantBufferSize;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        HRESULT hr = m_device->getDevice()->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_constantBuffer));

        if (FAILED(hr)) {
            return std::unexpected(Utils::make_error(Utils::ErrorType::ResourceCreation,
                std::format("Failed to create constant buffer for material '{}': HRESULT=0x{:X}",
                    m_name, static_cast<unsigned>(hr)), hr));
        }

        Utils::log_info(std::format("D3D12 constant buffer resource created for material '{}'", m_name));

        D3D12_RANGE readRange{ 0, 0 };
        hr = m_constantBuffer->Map(0, &readRange, &m_constantBufferData);

        if (FAILED(hr)) {
            m_constantBuffer.Reset();
            return std::unexpected(Utils::make_error(Utils::ErrorType::ResourceCreation,
                std::format("Failed to map constant buffer for material '{}': HRESULT=0x{:X}",
                    m_name, static_cast<unsigned>(hr)), hr));
        }

        Utils::log_info(std::format("Constant buffer mapped successfully for material '{}'", m_name));
        return {};
    }
    Utils::VoidResult Material::createDescriptors()
    {
        Utils::log_info(std::format("Creating descriptors for material '{}'", m_name));

        // CBV + SRV作成
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.NumDescriptors = 2; // CBV(1) + SRV(1)
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        srvHeapDesc.NodeMask = 0;

        HRESULT hr = m_device->getDevice()->CreateDescriptorHeap(
            &srvHeapDesc,
            IID_PPV_ARGS(&m_srvDescriptorHeap));

        if (FAILED(hr))
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::ResourceCreation,
                std::format("Failed to create SRV descriptor heap for material '{}': HRESULT=0x{:X}",
                    m_name, static_cast<unsigned>(hr)), hr));
        }

        // CBV作成
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
        cbvDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = (sizeof(MaterialConstantBuffer) + 255) & ~255; // 256アラインメント

        D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle = m_srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        m_device->getDevice()->CreateConstantBufferView(&cbvDesc, cbvHandle);

        // SRV
        UINT descriptorSize = m_device->getDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = cbvHandle;
        srvHandle.ptr += descriptorSize;

        // CPUハンドル取得
        // ptrにディスクリプタサイズを＋
        m_srvGpuHandle = m_srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
        m_srvGpuHandle.ptr += descriptorSize;

        Utils::log_info(std::format("Descriptors created successfully for material '{}'", m_name));
        return {};
    }

    Utils::VoidResult Material::saveToFile(const std::string& filePath) const
    {
        return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Not implemented yet"));
    }

    Utils::VoidResult Material::loadFromFile(const std::string& filePath)
    {
        return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Not implemented yet"));
    }

    //=========================================================================
    // MaterialManager実装
    //=========================================================================
    Utils::VoidResult MaterialManager::initialize(Device* device)
    {
        if (m_initialized) {
            Utils::log_warning("MaterialManager already initialized");
            return {};
        }

        CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
        CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");

        m_device = device;

        //マテリアル初期化
        auto defaultResult = createDefaultMaterial();
        if (!defaultResult) {
            m_device = nullptr;
            Utils::log_warning(std::format("Failed to create default material: {}", defaultResult.error().message));
            return defaultResult;
        }


        m_initialized = true;

        Utils::log_info("MaterialManager initialized successfully");
        return {};
    }


    std::shared_ptr<Material> MaterialManager::createMaterial(const std::string& name)
    {
        if (!m_initialized)
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "MaterialManager not initialized"));
            return nullptr;
        }


        std::string uniqueName = name;
        int counter = 1;
        while (hasMaterial(uniqueName))
        {
            uniqueName = name + "_" + std::to_string(counter);
            counter++;
        }

        if (uniqueName != name)
        {
            Utils::log_info(std::format("Material '{}' already exists, creating '{}' instead", name, uniqueName));
        }

        auto material = std::make_shared<Material>(uniqueName);
        auto initResult = material->initialize(m_device);
        if (!initResult)
        {
            Utils::log_error(initResult.error());
            return nullptr;
        }

        m_materials[uniqueName] = material;
        Utils::log_info(std::format("Material '{}' created successfully", uniqueName));
        return material;
    }

    std::shared_ptr<Material> MaterialManager::getMaterial(const std::string& name) const
    {
        auto it = m_materials.find(name);
        return (it != m_materials.end()) ? it->second : nullptr;
    }

    bool MaterialManager::hasMaterial(const std::string& name) const
    {
        return m_materials.find(name) != m_materials.end();
    }

    void MaterialManager::removeMaterial(const std::string& name)
    {
        auto it = m_materials.find(name);
        if (it != m_materials.end())
        {
            m_materials.erase(it);
            Utils::log_info(std::format("Material '{}' removed", name));
        }
    }

    void MaterialManager::updateAllMaterials()
    {
        for (auto& [name, material] : m_materials)
        {
            material->updateConstantBuffer();
        }
    }

    Utils::VoidResult MaterialManager::createDefaultMaterial()
    {
        m_defaultMaterial = std::make_shared<Material>("DefaultMaterial");


        if (!m_device || !m_device->isValid()) {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
                "Device is invalid when creating default material"));
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
                "Device is invalid"));
        }


        auto initResult = m_defaultMaterial->initialize(m_device);
        if (!initResult) {
            Utils::log_warning(std::format("Failed to initialize default material: {}",
                initResult.error().message));
            m_defaultMaterial.reset();
            return initResult;
        }


        MaterialProperties defaultProps;
        defaultProps.albedo = Math::Vector3(0.8f, 0.8f, 0.8f);
        defaultProps.metallic = 0.0f;
        defaultProps.roughness = 0.5f;
        defaultProps.ao = 1.0f;

        // setPropertiesをセット
        m_defaultMaterial->setProperties(defaultProps);


        m_materials["DefaultMaterial"] = m_defaultMaterial;

        Utils::log_info("Default material created successfully");
        return {};
    }

    void Material::setProperties(const MaterialProperties& properties)
    {
        Utils::log_info(std::format("=== setProperties called for '{}' ===", m_name));
        Utils::log_info(std::format("Current m_initialized: {}", m_initialized));
        Utils::log_info(std::format("Device valid: {}", (m_device && m_device->isValid())));

        m_properties = properties;
        m_isDirty = true;

        if (m_initialized && m_device && m_device->isValid()) {
            Utils::log_info(std::format("Calling updateConstantBuffer for '{}'", m_name));
            auto result = updateConstantBuffer();
            if (!result) {
                Utils::log_warning(std::format("Failed to update constant buffer for material '{}': {}",
                    m_name, result.error().message));
            }
            else {
                Utils::log_info(std::format("updateConstantBuffer succeeded for '{}'", m_name));
            }
        }
        else {
            Utils::log_info(std::format("Skipping updateConstantBuffer for '{}' - not ready (init:{}, device:{})",
                m_name, m_initialized, (m_device && m_device->isValid())));
        }

        Utils::log_info(std::format("=== setProperties completed for '{}' ===", m_name));
    }


    //=========================================================================
    //テクスチャタイプ変換実装
    //=========================================================================

    std::string textureTypeToString(TextureType type)
    {
        switch (type)
        {
        case TextureType::Albedo:    return "Albedo";
        case TextureType::Normal:    return "Normal";
        case TextureType::Metallic:  return "Metallic";
        case TextureType::Roughness: return "Roughness";
        case TextureType::AO:        return "AO";
        case TextureType::Emissive:  return "Emissive";
        case TextureType::Height:    return "Height";
        default:                     return "Unknown";
        }
    }

    TextureType stringToTextureType(const std::string& str)
    {
        if (str == "Albedo")    return TextureType::Albedo;
        if (str == "Normal")    return TextureType::Normal;
        if (str == "Metallic")  return TextureType::Metallic;
        if (str == "Roughness") return TextureType::Roughness;
        if (str == "AO")        return TextureType::AO;
        if (str == "Emissive")  return TextureType::Emissive;
        if (str == "Height")    return TextureType::Height;

        Utils::log_warning(std::format("Unknown texture type: {}", str));
        return TextureType::Albedo; //デフォでは、Albedoを返す
    }
}

//src/Graphics/ShaderManager.cpp
#include "ShaderManager.hpp"
#include <format>
#include <fstream>
#include <filesystem>
#include <sstream>

namespace Engine::Graphics
{
    //=========================================================================
    // Shader螳溯｣・
    //=========================================================================

    Utils::Result<std::shared_ptr<Shader>> Shader::compileFromFile(const ShaderCompileDesc& desc)
    {
        // 繝輔ぃ繧､繝ｫ隱ｭ縺ｿ霎ｼ縺ｿ
        auto codeResult = readShaderFile(desc.filePath);
        if (!codeResult)
        {
            return std::unexpected(codeResult.error());
        }

        // 繧､繝ｳ繧ｯ繝ｫ繝ｼ繝牙・逅・
        std::string baseDir = std::filesystem::path(desc.filePath).parent_path().string();
        std::string processedCode = processIncludes(*codeResult, baseDir);

        auto shader = std::make_shared<Shader>();
        auto initResult = shader->initialize(processedCode, desc.entryPoint, desc.type, desc.macros, desc.enableDebug, desc.filePath);
        if (!initResult)
        {
            return std::unexpected(initResult.error());
        }

        return shader;
    }

    Utils::Result<std::shared_ptr<Shader>> Shader::compileFromString(
        const std::string& shaderCode,
        const std::string& entryPoint,
        ShaderType type,
        const std::vector<ShaderMacro>& macros,
        bool enableDebug)
    {
        auto shader = std::make_shared<Shader>();
        auto initResult = shader->initialize(shaderCode, entryPoint, type, macros, enableDebug);
        if (!initResult)
        {
            return std::unexpected(initResult.error());
        }

        return shader;
    }

    Utils::VoidResult Shader::initialize(
        const std::string& shaderCode,
        const std::string& entryPoint,
        ShaderType type,
        const std::vector<ShaderMacro>& macros,
        bool enableDebug,
        const std::string& filePath)
    {
        m_type = type;
        m_entryPoint = entryPoint;
        m_filePath = filePath;

        // 繧ｳ繝ｳ繝代う繝ｫ繝輔Λ繧ｰ
        UINT compileFlags = 0;
        if (enableDebug)
        {
            compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
        }
        else
        {
            compileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
        }

        // 繝槭け繝ｭ螟画鋤
        auto d3dMacros = convertMacros(macros);

        // 繧ｷ繧ｧ繝ｼ繝繝ｼ繧ｿ繝ｼ繧ｲ繝・ヨ
        std::string target = shaderTypeToTarget(type);

        ComPtr<ID3DBlob> errorBlob;
        HRESULT hr = D3DCompile(
            shaderCode.c_str(),
            shaderCode.size(),
            filePath.empty() ? nullptr : filePath.c_str(),
            d3dMacros.empty() ? nullptr : d3dMacros.data(),
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entryPoint.c_str(),
            target.c_str(),
            compileFlags,
            0,
            &m_bytecode,
            &errorBlob
        );

        if (FAILED(hr))
        {
            std::string errorMsg = "Shader compilation failed";
            if (errorBlob)
            {
                errorMsg += std::format(": {}", static_cast<const char*>(errorBlob->GetBufferPointer()));
            }

            return std::unexpected(Utils::make_error(Utils::ErrorType::ShaderCompilation, errorMsg, hr));
        }

        return {};
    }

    std::string Shader::shaderTypeToTarget(ShaderType type)
    {
        switch (type)
        {
        case ShaderType::Vertex:   return "vs_5_1";
        case ShaderType::Pixel:    return "ps_5_1";
        case ShaderType::Geometry: return "gs_5_1";
        case ShaderType::Hull:     return "hs_5_1";
        case ShaderType::Domain:   return "ds_5_1";
        case ShaderType::Compute:  return "cs_5_1";
        default:                   return "vs_5_1";
        }
    }

    std::vector<D3D_SHADER_MACRO> Shader::convertMacros(const std::vector<ShaderMacro>& macros)
    {
        std::vector<D3D_SHADER_MACRO> d3dMacros;
        d3dMacros.reserve(macros.size() + 1);

        for (const auto& macro : macros)
        {
            D3D_SHADER_MACRO d3dMacro;
            d3dMacro.Name = macro.name.c_str();
            d3dMacro.Definition = macro.definition.c_str();
            d3dMacros.push_back(d3dMacro);
        }

        // 邨らｫｯ
        D3D_SHADER_MACRO endMacro = { nullptr, nullptr };
        d3dMacros.push_back(endMacro);

        return d3dMacros;
    }

    //=========================================================================
    // PipelineState螳溯｣・
    //=========================================================================

    Utils::Result<std::shared_ptr<PipelineState>> PipelineState::create(
        Device* device,
        const PipelineStateDesc& desc)
    {
        auto pipelineState = std::make_shared<PipelineState>();
        auto initResult = pipelineState->initialize(device, desc);
        if (!initResult)
        {
            return std::unexpected(initResult.error());
        }

        return pipelineState;
    }

    void PipelineState::setDebugName(const std::string& name)
    {
        if (m_pipelineState)
        {
            std::wstring wname(name.begin(), name.end());
            m_pipelineState->SetName(wname.c_str());
        }
        if (m_rootSignature)
        {
            std::wstring wname(name.begin(), name.end());
            wname += L"_RootSignature";
            m_rootSignature->SetName(wname.c_str());
        }
    }

    Utils::VoidResult PipelineState::initialize(Device* device, const PipelineStateDesc& desc)
    {
        CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
        CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");

        m_device = device;
        m_desc = desc;

        auto rootSigResult = createRootSignature();
        if (!rootSigResult)
        {
            return rootSigResult;
        }

        auto pipelineResult = createPipelineState();
        if (!pipelineResult)
        {
            return pipelineResult;
        }

        if (!m_desc.debugName.empty())
        {
            setDebugName(m_desc.debugName);
        }

        return {};
    }

    Utils::VoidResult PipelineState::createRootSignature()
    {
        std::vector<D3D12_ROOT_PARAMETER1> rootParams;
        rootParams.reserve(m_desc.rootParameters.size());

        // DescriptorTable逕ｨ縺ｮ繝ｬ繝ｳ繧ｸ繧剃ｿ晄戟縺吶ｋ繝吶け繧ｿ繝ｼ
        std::vector<std::vector<D3D12_DESCRIPTOR_RANGE1>> descriptorRanges;
        descriptorRanges.resize(m_desc.rootParameters.size());

        for (size_t i = 0; i < m_desc.rootParameters.size(); ++i)
        {
            const auto& param = m_desc.rootParameters[i];
            D3D12_ROOT_PARAMETER1 rootParam = {};
            rootParam.ShaderVisibility = param.visibility;

            switch (param.type)
            {
            case RootParameterDesc::ConstantBufferView:
                rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
                rootParam.Descriptor.ShaderRegister = param.shaderRegister;
                rootParam.Descriptor.RegisterSpace = param.registerSpace;
                break;

            case RootParameterDesc::ShaderResourceView:
                rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
                rootParam.Descriptor.ShaderRegister = param.shaderRegister;
                rootParam.Descriptor.RegisterSpace = param.registerSpace;
                break;

            case RootParameterDesc::UnorderedAccessView:
                rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
                rootParam.Descriptor.ShaderRegister = param.shaderRegister;
                rootParam.Descriptor.RegisterSpace = param.registerSpace;
                break;

            case RootParameterDesc::Constants:
                rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
                rootParam.Constants.ShaderRegister = param.shaderRegister;
                rootParam.Constants.RegisterSpace = param.registerSpace;
                rootParam.Constants.Num32BitValues = param.numConstants;
                break;

            case RootParameterDesc::DescriptorTable:
                rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                // 繝ｬ繝ｳ繧ｸ繧偵さ繝斐・
                descriptorRanges[i] = param.ranges;
                rootParam.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(descriptorRanges[i].size());
                rootParam.DescriptorTable.pDescriptorRanges = descriptorRanges[i].data();
                break;

            default:
                return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Unsupported root parameter type"));
            }

            rootParams.push_back(rootParam);
        }

        // 繧ｹ繧ｿ繝・ぅ繝・け繧ｵ繝ｳ繝励Λ繝ｼ蜃ｦ逅・
        std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;
        staticSamplers.reserve(m_desc.staticSamplers.size());

        for (const auto& sampler : m_desc.staticSamplers)
        {
            D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
            samplerDesc.Filter = sampler.filter;
            samplerDesc.AddressU = sampler.addressModeU;
            samplerDesc.AddressV = sampler.addressModeV;
            samplerDesc.AddressW = sampler.addressModeW;
            samplerDesc.MipLODBias = 0.0f;
            samplerDesc.MaxAnisotropy = 1;
            samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
            samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
            samplerDesc.MinLOD = 0.0f;
            samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
            samplerDesc.ShaderRegister = sampler.shaderRegister;
            samplerDesc.RegisterSpace = sampler.registerSpace;
            samplerDesc.ShaderVisibility = sampler.visibility;

            staticSamplers.push_back(samplerDesc);
        }

        // 繝ｫ繝ｼ繝医す繧ｰ繝阪メ繝｣險倩ｿｰ
        D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc = {};
        rootSigDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
        rootSigDesc.Desc_1_1.NumParameters = static_cast<UINT>(rootParams.size());
        rootSigDesc.Desc_1_1.pParameters = rootParams.empty() ? nullptr : rootParams.data();
        rootSigDesc.Desc_1_1.NumStaticSamplers = static_cast<UINT>(staticSamplers.size());
        rootSigDesc.Desc_1_1.pStaticSamplers = staticSamplers.empty() ? nullptr : staticSamplers.data();
        rootSigDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        ComPtr<ID3DBlob> serializedRootSig;
        ComPtr<ID3DBlob> errorBlob;
        HRESULT hr = D3D12SerializeVersionedRootSignature(&rootSigDesc, &serializedRootSig, &errorBlob);
        if (FAILED(hr))
        {
            std::string errorMsg = "Failed to serialize root signature";
            if (errorBlob)
            {
                errorMsg += std::format(": {}", static_cast<const char*>(errorBlob->GetBufferPointer()));
            }
            return std::unexpected(Utils::make_error(Utils::ErrorType::ResourceCreation, errorMsg, hr));
        }

        CHECK_HR(m_device->getDevice()->CreateRootSignature(
            0,
            serializedRootSig->GetBufferPointer(),
            serializedRootSig->GetBufferSize(),
            IID_PPV_ARGS(&m_rootSignature)),
            Utils::ErrorType::ResourceCreation,
            "Failed to create root signature");

        return {};
    }

    Utils::VoidResult PipelineState::createPipelineState()
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

        // 繧ｷ繧ｧ繝ｼ繝繝ｼ險ｭ螳・
        if (m_desc.vertexShader && m_desc.vertexShader->isValid())
        {
            psoDesc.VS.pShaderBytecode = m_desc.vertexShader->getBytecode();
            psoDesc.VS.BytecodeLength = m_desc.vertexShader->getBytecodeSize();
        }

        if (m_desc.pixelShader && m_desc.pixelShader->isValid())
        {
            psoDesc.PS.pShaderBytecode = m_desc.pixelShader->getBytecode();
            psoDesc.PS.BytecodeLength = m_desc.pixelShader->getBytecodeSize();
        }

        if (m_desc.geometryShader && m_desc.geometryShader->isValid())
        {
            psoDesc.GS.pShaderBytecode = m_desc.geometryShader->getBytecode();
            psoDesc.GS.BytecodeLength = m_desc.geometryShader->getBytecodeSize();
        }

        // 繝ｫ繝ｼ繝医す繧ｰ繝阪メ繝｣
        psoDesc.pRootSignature = m_rootSignature.Get();

        // 蜈･蜉帙Ξ繧､繧｢繧ｦ繝・
        psoDesc.InputLayout.pInputElementDescs = m_desc.inputLayout.empty() ? nullptr : m_desc.inputLayout.data();
        psoDesc.InputLayout.NumElements = static_cast<UINT>(m_desc.inputLayout.size());

        // 繝励Μ繝溘ユ繧｣繝悶ヨ繝昴Ο繧ｸ繝ｼ
        psoDesc.PrimitiveTopologyType = m_desc.primitiveTopology;

        // 繝ｬ繝ｳ繝繝ｼ繧ｿ繝ｼ繧ｲ繝・ヨ
        psoDesc.NumRenderTargets = static_cast<UINT>(m_desc.rtvFormats.size());
        for (size_t i = 0; i < m_desc.rtvFormats.size() && i < 8; ++i)
        {
            psoDesc.RTVFormats[i] = m_desc.rtvFormats[i];
        }
        psoDesc.DSVFormat = m_desc.dsvFormat;

        // 繧ｵ繝ｳ繝励Ν險ｭ螳・
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleDesc.Quality = 0;
        psoDesc.SampleMask = UINT_MAX;

        // 繝悶Ξ繝ｳ繝峨せ繝・・繝・
        psoDesc.BlendState.RenderTarget[0].BlendEnable = m_desc.enableBlending;
        psoDesc.BlendState.RenderTarget[0].SrcBlend = m_desc.srcBlend;
        psoDesc.BlendState.RenderTarget[0].DestBlend = m_desc.destBlend;
        psoDesc.BlendState.RenderTarget[0].BlendOp = m_desc.blendOp;
        psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        // 繝ｩ繧ｹ繧ｿ繝ｩ繧､繧ｶ繝ｼ繧ｹ繝・・繝・
        psoDesc.RasterizerState.FillMode = m_desc.fillMode;
        psoDesc.RasterizerState.CullMode = m_desc.cullMode;
        psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
        psoDesc.RasterizerState.DepthBias = 0;
        psoDesc.RasterizerState.DepthBiasClamp = 0.0f;
        psoDesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
        psoDesc.RasterizerState.DepthClipEnable = m_desc.enableDepthClip;
        psoDesc.RasterizerState.MultisampleEnable = FALSE;
        psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
        psoDesc.RasterizerState.ForcedSampleCount = 0;
        psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        // 豺ｱ蠎ｦ繧ｹ繝・Φ繧ｷ繝ｫ繧ｹ繝・・繝・
        psoDesc.DepthStencilState.DepthEnable = m_desc.enableDepthTest;
        psoDesc.DepthStencilState.DepthWriteMask = m_desc.enableDepthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
        psoDesc.DepthStencilState.DepthFunc = m_desc.depthFunc;
        psoDesc.DepthStencilState.StencilEnable = FALSE;

        CHECK_HR(m_device->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)),
            Utils::ErrorType::ResourceCreation,
            std::format("Failed to create graphics pipeline state: {}", m_desc.debugName));

        return {};
    }

    //=========================================================================
    // ShaderManager螳溯｣・
    //=========================================================================

    Utils::VoidResult ShaderManager::initialize(Device* device)
    {
        CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
        CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");

        m_device = device;

        auto defaultShadersResult = createDefaultShaders();
        if (!defaultShadersResult)
        {
            return defaultShadersResult;
        }

        auto defaultPipelinesResult = createDefaultPipelines();
        if (!defaultPipelinesResult)
        {
            return defaultPipelinesResult;
        }

        m_initialized = true;
        Utils::log_info("ShaderManager initialized successfully");
        return {};
    }

    std::shared_ptr<Shader> ShaderManager::loadShader(const ShaderCompileDesc& desc)
    {
        // 繝・ヰ繝・げ: this繝昴う繝ｳ繧ｿ縺ｮ遒ｺ隱・
        Utils::log_info(std::format("ShaderManager::loadShader called, this = {}",
            static_cast<void*>(this)));

        if (this == nullptr)
        {
            Utils::log_warning("ShaderManager::loadShader - this is nullptr!");
            return nullptr;
        }

        Utils::log_info(std::format("Checking m_initialized: {}", m_initialized));

        if (!m_initialized)
        {
            Utils::log_warning("ShaderManager not initialized");
            return nullptr;
        }

        Utils::log_info(std::format("Generating shader key for: {}", desc.filePath));
        std::string key = generateShaderKey(desc);

        if (hasShader(key))
        {
            Utils::log_info(std::format("Shader already exists: {}", key));
            return getShader(key);
        }

        Utils::log_info(std::format("Compiling new shader: {}", desc.filePath));
        auto shaderResult = Shader::compileFromFile(desc);
        if (!shaderResult)
        {
            Utils::log_warning(std::format("Failed to compile shader '{}': {}", desc.filePath, shaderResult.error().message));
            return nullptr;
        }

        m_shaders[key] = *shaderResult;
        Utils::log_info(std::format("Shader compiled and cached: {}", desc.filePath));
        return *shaderResult;
    }

    std::shared_ptr<Shader> ShaderManager::getShader(const std::string& name) const
    {
        auto it = m_shaders.find(name);
        return (it != m_shaders.end()) ? it->second : nullptr;
    }

    bool ShaderManager::hasShader(const std::string& name) const
    {
        return m_shaders.find(name) != m_shaders.end();
    }

    void ShaderManager::removeShader(const std::string& name)
    {
        auto it = m_shaders.find(name);
        if (it != m_shaders.end())
        {
            m_shaders.erase(it);
        }
    }

    std::shared_ptr<PipelineState> ShaderManager::createPipelineState(
        const std::string& name,
        const PipelineStateDesc& desc)
    {
        if (!m_initialized)
        {
            Utils::log_warning("ShaderManager not initialized");
            return nullptr;
        }

        if (hasPipelineState(name))
        {
            Utils::log_warning(std::format("Pipeline state '{}' already exists", name));
            return getPipelineState(name);
        }

        auto pipelineResult = PipelineState::create(m_device, desc);
        if (!pipelineResult)
        {
            Utils::log_warning(std::format("Failed to create pipeline state '{}': {}", name, pipelineResult.error().message));
            return nullptr;
        }

        m_pipelineStates[name] = *pipelineResult;
        Utils::log_info(std::format("Pipeline state created: {}", name));
        return *pipelineResult;
    }

    std::shared_ptr<PipelineState> ShaderManager::getPipelineState(const std::string& name) const
    {
        auto it = m_pipelineStates.find(name);
        return (it != m_pipelineStates.end()) ? it->second : nullptr;
    }

    bool ShaderManager::hasPipelineState(const std::string& name) const
    {
        return m_pipelineStates.find(name) != m_pipelineStates.end();
    }

    void ShaderManager::removePipelineState(const std::string& name)
    {
        auto it = m_pipelineStates.find(name);
        if (it != m_pipelineStates.end())
        {
            m_pipelineStates.erase(it);
        }
    }

    Utils::VoidResult ShaderManager::createDefaultShaders()
    {
        // HLSL繝輔ぃ繧､繝ｫ縺九ｉ繧ｷ繧ｧ繝ｼ繝繝ｼ繧偵さ繝ｳ繝代う繝ｫ
        ShaderCompileDesc vsDesc;
        vsDesc.filePath = "engine-assets/shaders/PBR_VS.hlsl";
        vsDesc.entryPoint = "main";
        vsDesc.type = ShaderType::Vertex;

        auto vsResult = Shader::compileFromFile(vsDesc);
        if (!vsResult)
        {
            return std::unexpected(vsResult.error());
        }

        ShaderCompileDesc psDesc;
        psDesc.filePath = "engine-assets/shaders/PBR_PS.hlsl";
        psDesc.entryPoint = "main";
        psDesc.type = ShaderType::Pixel;

        auto psResult = Shader::compileFromFile(psDesc);
        if (!psResult)
        {
            return std::unexpected(psResult.error());
        }

        m_shaders["DefaultPBR_VS"] = *vsResult;
        m_shaders["DefaultPBR_PS"] = *psResult;

        Utils::log_info("Default PBR shaders loaded successfully");
        return {};
    }

    std::shared_ptr<Shader> ShaderManager::compileFromString(
        const std::string& shaderCode,
        const std::string& entryPoint,
        ShaderType type,
        const std::string& shaderName)
    {
        if (!m_initialized)
        {
            Utils::log_warning("ShaderManager not initialized");
            return nullptr;
        }

        auto shaderResult = Shader::compileFromString(shaderCode, entryPoint, type);
        if (!shaderResult)
        {
            Utils::log_warning(std::format("Failed to compile inline shader '{}': {}", shaderName, shaderResult.error().message));
            return nullptr;
        }

        return *shaderResult;
    }

    Utils::VoidResult ShaderManager::createDefaultPipelines()
    {
        // PBR繝代う繝励Λ繧､繝ｳ菴懈・
        PipelineStateDesc pbrDesc;
        pbrDesc.vertexShader = getShader("DefaultPBR_VS");
        pbrDesc.pixelShader = getShader("DefaultPBR_PS");
        pbrDesc.inputLayout = StandardInputLayouts::PBRVertex;
        pbrDesc.debugName = "DefaultPBR";

        // Scene螳壽焚繝舌ャ繝輔ぃ (b0)
        RootParameterDesc sceneConstants;
        sceneConstants.type = RootParameterDesc::ConstantBufferView;
        sceneConstants.shaderRegister = 0;
        sceneConstants.visibility = D3D12_SHADER_VISIBILITY_ALL;

        // Object螳壽焚繝舌ャ繝輔ぃ (b1)
        RootParameterDesc objectConstants;
        objectConstants.type = RootParameterDesc::ConstantBufferView;
        objectConstants.shaderRegister = 1;
        objectConstants.visibility = D3D12_SHADER_VISIBILITY_VERTEX;

        // Material螳壽焚繝舌ャ繝輔ぃ (b2)
        RootParameterDesc materialConstants;
        materialConstants.type = RootParameterDesc::ConstantBufferView;
        materialConstants.shaderRegister = 2;
        materialConstants.visibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // PBR繝・け繧ｹ繝√Ε逕ｨ繝・ぅ繧ｹ繧ｯ繝ｪ繝励ち繝・・繝悶Ν
        RootParameterDesc textureTable;
        textureTable.type = RootParameterDesc::DescriptorTable;
        textureTable.visibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // 6縺､縺ｮ繝・け繧ｹ繝√Ε縺吶∋縺ｦ繧・縺､縺ｮ繝ｬ繝ｳ繧ｸ縺ｧ螳夂ｾｩ (t0-t5)
        std::vector<D3D12_DESCRIPTOR_RANGE1> srvRanges;

        D3D12_DESCRIPTOR_RANGE1 pbrTextureRange{};
        pbrTextureRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        pbrTextureRange.NumDescriptors = 6;  // t0, t1, t2, t3, t4, t5 縺ｮ6縺､
        pbrTextureRange.BaseShaderRegister = 0;  // t0縺九ｉ髢句ｧ・
        pbrTextureRange.RegisterSpace = 0;
        pbrTextureRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
        pbrTextureRange.OffsetInDescriptorsFromTableStart = 0;

        srvRanges.push_back(pbrTextureRange);
        textureTable.ranges = srvRanges;

        // 繝ｫ繝ｼ繝医ヱ繝ｩ繝｡繝ｼ繧ｿ繧定ｨｭ螳・
        pbrDesc.rootParameters = { sceneConstants, objectConstants, materialConstants, textureTable };

        // 繧ｹ繧ｿ繝・ぅ繝・け繧ｵ繝ｳ繝励Λ繝ｼ (s0)
        StaticSamplerDesc linearSampler;
        linearSampler.shaderRegister = 0;
        linearSampler.filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        linearSampler.addressModeU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        linearSampler.addressModeV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        linearSampler.visibility = D3D12_SHADER_VISIBILITY_PIXEL;

        pbrDesc.staticSamplers = { linearSampler };

        // 繝代う繝励Λ繧､繝ｳ菴懈・
        auto pbrPipelineResult = PipelineState::create(m_device, pbrDesc);
        if (!pbrPipelineResult)
        {
            return std::unexpected(pbrPipelineResult.error());
        }

        m_defaultPBRPipeline = *pbrPipelineResult;
        m_pipelineStates["DefaultPBR"] = m_defaultPBRPipeline;

        Utils::log_info("PBR pipeline created successfully");
        return {};
    }

    std::string ShaderManager::generateShaderKey(const ShaderCompileDesc& desc) const
    {
        std::string key = desc.filePath + "_" + desc.entryPoint + "_" + std::to_string(static_cast<int>(desc.type));

        for (const auto& macro : desc.macros)
        {
            key += "_" + macro.name + "=" + macro.definition;
        }

        if (desc.enableDebug)
        {
            key += "_DEBUG";
        }

        return key;
    }

    //=========================================================================
    // 讓呎ｺ門・蜉帙Ξ繧､繧｢繧ｦ繝亥ｮ夂ｾｩ
    //=========================================================================

    namespace StandardInputLayouts
    {
        const std::vector<D3D12_INPUT_ELEMENT_DESC> Position =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        const std::vector<D3D12_INPUT_ELEMENT_DESC> PositionUV =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        const std::vector<D3D12_INPUT_ELEMENT_DESC> PositionNormalUV =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        const std::vector<D3D12_INPUT_ELEMENT_DESC> PBRVertex =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };
    }

    //=========================================================================
    // 繝ｦ繝ｼ繝・ぅ繝ｪ繝・ぅ髢｢謨ｰ螳溯｣・
    //=========================================================================

    Utils::Result<std::string> readShaderFile(const std::string& filePath)
    {
        std::ifstream file(filePath);
        if (!file.is_open())
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::FileI0,
                std::format("Cannot open shader file: {}", filePath)));
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    std::string processIncludes(const std::string& shaderCode, const std::string& baseDir)
    {
        // 邁｡蜊倥↑繧､繝ｳ繧ｯ繝ｫ繝ｼ繝牙・逅・ｼ・include "filename"縺ｮ蠖｢蠑上・縺ｿ蟇ｾ蠢懶ｼ・
        std::string result = shaderCode;
        std::string includePattern = "#include \"";

        size_t pos = 0;
        while ((pos = result.find(includePattern, pos)) != std::string::npos)
        {
            size_t startQuote = pos + includePattern.length();
            size_t endQuote = result.find("\"", startQuote);

            if (endQuote != std::string::npos)
            {
                std::string includeFile = result.substr(startQuote, endQuote - startQuote);
                std::string fullPath = baseDir.empty() ? includeFile : baseDir + "/" + includeFile;

                auto includeResult = readShaderFile(fullPath);
                if (includeResult)
                {
                    // 繧､繝ｳ繧ｯ繝ｫ繝ｼ繝画枚繧貞ｮ滄圀縺ｮ繝輔ぃ繧､繝ｫ蜀・ｮｹ縺ｧ鄂ｮ謠・
                    size_t lineEnd = result.find("\n", pos);
                    if (lineEnd == std::string::npos) lineEnd = result.length();

                    result.replace(pos, lineEnd - pos, *includeResult);
                    pos += includeResult->length();
                }
                else
                {
                    Utils::log_warning(std::format("Failed to include file: {}", fullPath));
                    pos = endQuote + 1;
                }
            }
            else
            {
                pos += includePattern.length();
            }
        }

        return result;
    }
}

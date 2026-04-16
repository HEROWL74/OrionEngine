#include "PipelineStateCache.hpp"
#include "../engine/Core/ProjectSettings.hpp"
#include "RootSignature.hpp"
#include <format>

namespace Renderer
{
	Utils::VoidResult PipelineStateCache::initialize(Device* device, ShaderManager* shaderManager)
	{
		CHECK_CONDITION(device != nullptr, Utils::ErrorType::DeviceCreation, "Device is null");
		CHECK_CONDITION(shaderManager != nullptr, Utils::ErrorType::DeviceCreation, "ShaderManager is null");
		CHECK_CONDITION(shaderManager->isValid(), Utils::ErrorType::DeviceCreation,
			"ShaderManager is not initialized");

		m_device = device;
		m_shaderManager = shaderManager;

		// デフォルトPSOを生成・登録
		auto result = createDefaultPBR();
		if (!result)
		{
			return result;
		}

		Utils::log_info("PipelineStateCache is initialized");
		return {};
	}

	// PSOの登録・取得
	Utils::VoidResult PipelineStateCache::registerPSO(
		const std::string& name,
		std::unique_ptr<GraphicsPipelineState> pso
	)
	{
		if (has(name))
		{
			return std::unexpected(Utils::make_error(Utils::ErrorType::ResourceCreation,
				std::format("PSO '{}' is already registered", name)));
		}

		m_cache[name] = std::move(pso);
		return {};
	}

	// 名前でPSOを取得する。存在しない場合は nullptr を返す
	GraphicsPipelineState* PipelineStateCache::get(const std::string& name) const
	{
		auto it = m_cache.find(name);
		return (it != m_cache.end()) ? it->second.get() : nullptr;
	}

	// 存在確認
	bool PipelineStateCache::has(const std::string& name) const
	{
		return m_cache.find(name) != m_cache.end();
	}

	Utils::VoidResult PipelineStateCache::createDefaultPBR()
	{
		auto& settings = Core::ProjectSettings::get();

		// シェーダーロード
		ShaderCompileDesc vsDesc;
		vsDesc.filePath = settings.getEngineAssetPath("shaders/BasicVertex.dxil").string();
		vsDesc.entryPoint = "main";
		vsDesc.type = ShaderType::Vertex;

		auto vs = m_shaderManager->loadShader(vsDesc);
		if (!vs)
		{
			return std::unexpected(Utils::make_error(Utils::ErrorType::ShaderCompilation,
				"PipelineStateCache: Failed to load BasicVertex.dxil"));
		}

		ShaderCompileDesc psDesc;
		psDesc.filePath = settings.getEngineAssetPath("shaders/BasicPixel.dxil").string();
		psDesc.entryPoint = "main";
		psDesc.type = ShaderType::Pixel;

		auto ps = m_shaderManager->loadShader(psDesc);
		if (!ps)
		{
			return std::unexpected(Utils::make_error(Utils::ErrorType::ShaderCompilation,
				"PipelineStateCache: Failed to load BasicPixel.dxil"));
		}

		// RootSignature 生成
		auto rootSigResult = RootSignatureFactory::createPBR(m_device);
		if (!rootSigResult)
			return std::unexpected(rootSigResult.error());

		// Desc 組み立て
		GraphicsPipelineStateDesc desc;
		desc.vertexShader = vs;
		desc.pixelShader = ps;
		desc.inputLayout = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
		};
		desc.debugName = "DefaultPBR";
		desc.depthFunc = D3D12_COMPARISON_FUNC_LESS;

		// GraphicsPipelineState 作成
		auto pso = std::make_unique<GraphicsPipelineState>();
		auto initResult = pso->initialize(m_device, std::move(*rootSigResult), desc);
		if (!initResult)
			return initResult;

		// キャッシュに登録
		m_cache["DefaultPBR"] = std::move(pso);

		Utils::log_info("DefaultPBR PSO created and cached");
		return {};
	}
}

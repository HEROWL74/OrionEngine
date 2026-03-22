// renderer/PipelineStateCache.hpp
#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include "../engine/Utils/Common.hpp"
#include "Device.hpp"
#include "ShaderManager.hpp"
#include "GraphicsPipelineState.hpp"

namespace Renderer
{
	// ============================
	// PipelineStateCache
	// ============================
	class PipelineStateCache
	{
	public:
		PipelineStateCache() = default;
		~PipelineStateCache() = default;

		// Delete Copy & Move (キャッシュは一つだけしか存在させない)
		PipelineStateCache(const PipelineStateCache&) = delete;
		PipelineStateCache& operator=(const PipelineStateCache&) = delete;
		PipelineStateCache(PipelineStateCache&&) = delete;
		PipelineStateCache& operator=(PipelineStateCache&&) = delete;
        
		// ShaderManager初期化後に呼ぶ デフォルトPSOをここで生成
		[[nodiscard]] Utils::VoidResult initialize(Device* device, ShaderManager* shaderManager);


		// PSOの登録・取得
		[[nodiscard]] Utils::VoidResult registerPSO(
			const std::string& name,
			std::unique_ptr<GraphicsPipelineState> pso
		);

		// 名前でPSOを取得する。存在しない場合は nullptr を返す
		GraphicsPipelineState* get(const std::string& name) const;

		// 存在確認
		bool has(const std::string& name) const;

		// PBR標準PSO
		GraphicsPipelineState* getDefaultPBR() const { return get("DefaultPBR"); }

	private:
		Device* m_device = nullptr;
		ShaderManager* m_shaderManager = nullptr;

		std::unordered_map<std::string, std::unique_ptr<GraphicsPipelineState>> m_cache;

		//起動時に登録するデフォルトPSOの生成
		[[nodiscard]] Utils::VoidResult createDefaultPBR();
	};
}
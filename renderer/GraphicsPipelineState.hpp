// renderer/GraphicsPipelineState.hpp
#pragma once
#include "RootSignature.hpp"

namespace Renderer
{
	// =================================
	// GraphicsPipelineStateDesc
	// =================================
	struct GraphicsPipelineStateDesc
	{
		// シェーダー
		std::shared_ptr<Shader> vertexShader;
		std::shared_ptr<Shader> pixelShader;

		// 入力レイアウト
		std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;

		// レンダーターゲット
		std::vector<DXGI_FORMAT> rtvFormats = { DXGI_FORMAT_R8G8B8A8_UNORM };
		DXGI_FORMAT dsvFormat = DXGI_FORMAT_D32_FLOAT;

		// ラスタライザ
		D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_BACK;
		D3D12_FILL_MODE fillMode = D3D12_FILL_MODE_SOLID;

		// 深度
		bool enableDepthTest = true;
		bool enableDepthWrite = true;
		D3D12_COMPARISON_FUNC depthFunc;

		// ブレンド
		bool enableBlending = false;
		D3D12_BLEND srcBlend = D3D12_BLEND_ONE;
		D3D12_BLEND destBlend = D3D12_BLEND_ZERO;
		D3D12_BLEND_OP blendOp = D3D12_BLEND_OP_ADD;

		// デバッグ用Name
		std::string debugName;
	};

    // =================================
	// GraphicsPipelineState PSO Settings
	// =================================
	class GraphicsPipelineState
	{
	public:
		GraphicsPipelineState() = default;
		~GraphicsPipelineState() = default;

		// コピー禁止・ムーブ許可
		GraphicsPipelineState(const GraphicsPipelineState&) = delete;
		GraphicsPipelineState& operator=(const GraphicsPipelineState&) = delete;
		GraphicsPipelineState(GraphicsPipelineState&&) = default;
		GraphicsPipelineState& operator=(GraphicsPipelineState&&) = default;

        // RootSignature と Desc を受け取って初期化する
		[[nodiscard]] Utils::VoidResult initialize(
			Device* device,
			RootSignature&& rootSignature,
			const GraphicsPipelineStateDesc& desc);

		// D3D12オブジェクトの取得
		ID3D12RootSignature* getRootSignature() const { return m_rootSignature.get() ; }
		ID3D12PipelineState* getPipelineState() const { return m_pipelineState.Get(); }

		// 有効性チェック
		bool isValid() const
		{
			return m_rootSignature.isValid() && m_pipelineState != nullptr;
		}

	private:
		RootSignature m_rootSignature;
		ComPtr<ID3D12PipelineState> m_pipelineState;

		// PSO作成
		[[nodiscard]] Utils::VoidResult createPSO(
			Device* device,
			const GraphicsPipelineStateDesc& desc
		);
 	};
}

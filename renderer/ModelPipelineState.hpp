// renderer/ModelPipelineState.hpp
#pragma once
#include <memory>
#include "RootSignature.hpp"
#include "ShaderManager.hpp"


namespace Renderer
{
    // ==========================
    // ModelPipelineStateDesc
    // ==========================
    struct ModelPipelineStateDesc
    {
        std::shared_ptr<Shader> vertexShader;
        std::shared_ptr<Shader> pixelShader;
        
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
    class ModelPipelineState
    {
    public:
        ModelPipelineState() = default;
        ~ModelPipelineState() = default;
        
        // コピー禁止・ムーブ許可
        ModelPipelineState(const ModelPipelineState&) = delete;
        ModelPipelineState& operator=(const ModelPipelineState&) = delete;
        ModelPipelineState(ModelPipelineState&&) = default;
        ModelPipelineState& operator=(ModelPipelineState&&) = default;
        
        [[nodiscard]] Utils::VoidResult initialize(
            Device* device,
            RootSignature&& rootSignature,
            const ModelPipelineStateDesc& desc);
    private:
        RootSignature m_rootSignature;
        ComPtr<ID3D12PipelineState> m_pipelineState;
        
        // PSO作成
        [[nodiscard]] Utils::VoidResult createPSO(
            Device* device,
            const ModelPipelineStateDesc& desc
        );
    };    
}

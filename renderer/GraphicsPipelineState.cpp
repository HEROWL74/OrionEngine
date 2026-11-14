#include "GraphicsPipelineState.hpp"

namespace Renderer {
// RootSignature と Desc を受け取って初期化する
[[nodiscard]] Utils::VoidResult
GraphicsPipelineState::initialize(Device *device, RootSignature &&rootSignature,
                                  const GraphicsPipelineStateDesc &desc) {
  CHECK_CONDITION(device != nullptr, Utils::ErrorType::DeviceCreation,
                  "Device is null");
  CHECK_CONDITION(rootSignature.isValid(), Utils::ErrorType::ResourceCreation,
                  "RootSignature is not initialized");

  // RootSignatureの所有権を受け取る
  m_rootSignature = std::move(rootSignature);

  return createPSO(device, desc);
}

[[nodiscard]] Utils::VoidResult
GraphicsPipelineState::createPSO(Device *device,
                                 const GraphicsPipelineStateDesc &desc) {
  CHECK_CONDITION(desc.vertexShader && desc.vertexShader->isValid(),
                  Utils::ErrorType::ShaderCompilation,
                  "VertexShader is null or invaild");
  CHECK_CONDITION(desc.pixelShader && desc.pixelShader->isValid(),
                  Utils::ErrorType::ShaderCompilation,
                  "PixelShader is null or invaild");

  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};

  // ルートシグネチャ
  psoDesc.pRootSignature = m_rootSignature.get();

  // シェーダー
  psoDesc.VS = {desc.vertexShader->getBytecode(),
                desc.vertexShader->getBytecodeSize()};
  psoDesc.PS = {desc.pixelShader->getBytecode(),
                desc.pixelShader->getBytecodeSize()};

  // 入力レイアウト
  psoDesc.InputLayout.pInputElementDescs =
      desc.inputLayout.empty() ? nullptr : desc.inputLayout.data();
  psoDesc.InputLayout.NumElements = static_cast<UINT>(desc.inputLayout.size());

  // ラスタライザー
  psoDesc.RasterizerState.FillMode = desc.fillMode;
  psoDesc.RasterizerState.CullMode = desc.cullMode;
  psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
  psoDesc.RasterizerState.DepthBias = 0;
  psoDesc.RasterizerState.DepthBiasClamp = 0.0f;
  psoDesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
  psoDesc.RasterizerState.DepthClipEnable = TRUE;
  psoDesc.RasterizerState.MultisampleEnable = FALSE;
  psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
  psoDesc.RasterizerState.ForcedSampleCount = 0;
  psoDesc.RasterizerState.ConservativeRaster =
      D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

  // ブレンドステート
  psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
  psoDesc.BlendState.IndependentBlendEnable = FALSE;
  D3D12_RENDER_TARGET_BLEND_DESC &rtBlend = psoDesc.BlendState.RenderTarget[0];
  rtBlend.BlendEnable = desc.enableBlending;
  rtBlend.SrcBlend = desc.srcBlend;
  rtBlend.DestBlend = desc.destBlend;
  rtBlend.BlendOp = desc.blendOp;
  rtBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
  rtBlend.DestBlendAlpha = D3D12_BLEND_ZERO;
  rtBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
  rtBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
  // 残りのRTはデフォルト
  for (UINT i = 1; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
    psoDesc.BlendState.RenderTarget[i].RenderTargetWriteMask =
        D3D12_COLOR_WRITE_ENABLE_ALL;
  }

  // 深度ステンシル
  psoDesc.DepthStencilState.DepthEnable = desc.enableDepthTest ? TRUE : FALSE;
  psoDesc.DepthStencilState.DepthWriteMask = desc.enableDepthWrite
                                                 ? D3D12_DEPTH_WRITE_MASK_ALL
                                                 : D3D12_DEPTH_WRITE_MASK_ZERO;
  psoDesc.DepthStencilState.DepthFunc = desc.depthFunc;
  psoDesc.DepthStencilState.StencilEnable = FALSE;
  psoDesc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
  psoDesc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
  const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp = {
      D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP,
      D3D12_COMPARISON_FUNC_ALWAYS};
  psoDesc.DepthStencilState.FrontFace = defaultStencilOp;
  psoDesc.DepthStencilState.BackFace = defaultStencilOp;

  // レンダーターゲット
  psoDesc.NumRenderTargets = static_cast<UINT>(desc.rtvFormats.size());
  for (size_t i = 0; i < desc.rtvFormats.size() && i < 8; ++i) {
    psoDesc.RTVFormats[i] = desc.rtvFormats[i];
  }
  psoDesc.DSVFormat = desc.dsvFormat;

  // その他の設定
  psoDesc.SampleMask = UINT_MAX;
  psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  psoDesc.SampleDesc.Count = 1;
  psoDesc.SampleDesc.Quality = 0;

  CHECK_HR(device->getDevice()->CreateGraphicsPipelineState(
               &psoDesc, IID_PPV_ARGS(&m_pipelineState)),
           Utils::ErrorType::ResourceCreation,
           std::format("Failed to create PSO: {}", desc.debugName));

  // デバッグ名を設定
  if (!desc.debugName.empty() && m_pipelineState) {
    std::wstring wname(desc.debugName.begin(), desc.debugName.end());
    m_pipelineState->SetName(wname.c_str());
  }

  return {};
}
} // namespace Renderer

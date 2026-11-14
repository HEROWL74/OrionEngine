#include "RootSignature.hpp"

namespace Renderer {
// ---------------------------------
// Builder Method
// ---------------------------------

RootSignature &RootSignature::addCBV(uint32_t shaderRegister,
                                     D3D12_SHADER_VISIBILITY visiblity) {
  D3D12_ROOT_PARAMETER param{};
  param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
  param.Descriptor.ShaderRegister = shaderRegister;
  param.Descriptor.RegisterSpace = 0;
  param.ShaderVisibility = visiblity;

  m_params.push_back(param);
  return *this;
}

RootSignature &RootSignature::addSRVTable(uint32_t count, uint32_t baseRegister,
                                          D3D12_SHADER_VISIBILITY visiblity) {
  /*[MEMO]
pDescriptorRanges はポインタで参照される
    vector は push_back のたびに再確保が起きアドレスが変わる。
　 そのためここではポインタを設定せず、m_ranges 内のインデックスを
　 NumDescriptorRanges に仮置きしておく。
finalize() で vector の push_back が完全に終わった後に
安定したアドレスを使ってポインタを確定させる。
  */

  D3D12_DESCRIPTOR_RANGE range{};
  range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  range.NumDescriptors = count;
  range.BaseShaderRegister = baseRegister;
  range.RegisterSpace = 0;
  range.OffsetInDescriptorsFromTableStart =
      D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

  m_ranges.push_back(range);

  D3D12_ROOT_PARAMETER param{};
  param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  param.DescriptorTable.NumDescriptorRanges =
      static_cast<UINT>(m_ranges.size() - 1);        // Index 仮置き
  param.DescriptorTable.pDescriptorRanges = nullptr; // finalize()で確定
  param.ShaderVisibility = visiblity;

  m_params.push_back(param);
  return *this;
}

RootSignature &
RootSignature::addLinearSampler(uint32_t shaderRegister,
                                D3D12_SHADER_VISIBILITY visiblity) {
  D3D12_STATIC_SAMPLER_DESC sampler{};
  sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
  sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  sampler.MipLODBias = 0.0f;
  sampler.MaxAnisotropy = 1;
  sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
  sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
  sampler.MinLOD = 0.0f;
  sampler.MaxLOD = D3D12_FLOAT32_MAX;
  sampler.ShaderRegister = shaderRegister;
  sampler.RegisterSpace = 0;
  sampler.ShaderVisibility = visiblity;

  m_samplers.push_back(sampler);
  return *this;
}

// finalize()
// Descriptortableのrangeポインタを確定させる
// vectorはこれ以降 resize/push_backしてはいけない
Utils::VoidResult RootSignature::finalize(Device *device) {
  CHECK_CONDITION(device != nullptr, Utils::ErrorType::DeviceCreation,
                  "Device is null");
  CHECK_CONDITION(!m_rootSignature, Utils::ErrorType::ResourceCreation,
                  "RootSignature::finalize() called twice");

  // DescriptorTableパラメータのrangeポインタを確定
  // addSRVTable()でインデックスを仮置きしていたものを本物のポインタに差し替える
  for (auto &param : m_params) {
    if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
      UINT rangeIndex = param.DescriptorTable.NumDescriptorRanges;
      param.DescriptorTable.NumDescriptorRanges = 1;
      param.DescriptorTable.pDescriptorRanges = &m_ranges[rangeIndex];
    }
  }

  D3D12_ROOT_SIGNATURE_DESC desc{};
  desc.NumParameters = static_cast<UINT>(m_params.size());
  desc.pParameters = m_params.empty() ? nullptr : m_params.data();
  desc.NumStaticSamplers = static_cast<UINT>(m_samplers.size());
  desc.pStaticSamplers = m_samplers.empty() ? nullptr : m_samplers.data();
  desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

  ComPtr<ID3DBlob> serialized;
  ComPtr<ID3DBlob> error;
  CHECK_HR(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1,
                                       &serialized, &error),
           Utils::ErrorType::ResourceCreation,
           "Failed to serialize root signature");

  CHECK_HR(device->getDevice()->CreateRootSignature(
               0, serialized->GetBufferPointer(), serialized->GetBufferSize(),
               IID_PPV_ARGS(&m_rootSignature)),
           Utils::ErrorType::ResourceCreation,
           "Failed to create root signature");

  return {};
}

namespace RootSignatureFactory {
Utils::Result<RootSignature> createPBR(Device *device) {
  RootSignature sig;
  sig.addCBV(0, D3D12_SHADER_VISIBILITY_ALL);             // Slot0: Camera
  sig.addCBV(1, D3D12_SHADER_VISIBILITY_ALL);          // Slot1: Object
  sig.addCBV(2, D3D12_SHADER_VISIBILITY_ALL);           // Slot2: Material
  sig.addSRVTable(6, 0, D3D12_SHADER_VISIBILITY_ALL);   // Slot3: t0-t5
  sig.addLinearSampler(0, D3D12_SHADER_VISIBILITY_ALL); // s0

  auto result = sig.finalize(device);
  if (!result) {
    return std::unexpected(result.error());
  }
  return sig;
}

// ------------------------------------------------------------------
// createModel()
//
// ModelRenderer 用ルートシグネチャ。
// PBR との違いは SRV テーブルのスロット数のみ（6→7）。
//
//   Slot 0 (b0): CameraConstants CBV  (ALL)
//   Slot 1 (b1): ObjectConstants CBV  (VS)
//   Slot 2 (b2): MaterialCB      CBV  (PS)
//   Slot 3 (t0-t6): 7 textures   SRV Table (PS)
//   s0: Linear Wrap Sampler      (PS)
// ------------------------------------------------------------------
Utils::Result<RootSignature> createModel(Device *device) {
  RootSignature sig;
  sig.addCBV(0, D3D12_SHADER_VISIBILITY_ALL);    // Slot 0: Camera
  sig.addCBV(1, D3D12_SHADER_VISIBILITY_VERTEX); // Slot 1: Object
  sig.addCBV(2, D3D12_SHADER_VISIBILITY_PIXEL);  // Slot 2: Material
  sig.addSRVTable(7, 0,
                  D3D12_SHADER_VISIBILITY_PIXEL); // Slot 3: t0-t6 (7 textures)
  sig.addLinearSampler(0, D3D12_SHADER_VISIBILITY_PIXEL); // s0

  auto result = sig.finalize(device);
  if (!result) {
    return std::unexpected(result.error());
  }
  return sig;
}
} // namespace RootSignatureFactory
} // namespace Renderer
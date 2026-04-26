#include "ModelPipelineState.hpp"

namespace Renderer
{
   Utils::VoidResult ModelPipelineState::initialize(Device* device, RootSignature&& rootSignature, const ModelPipelineStateDesc& desc)
   {
      CHECK_CONDITION(device != nullptr, Utils::ErrorType::DeviceCreation,
                 "Device is null");
      CHECK_CONDITION(rootSignature.isValid(), Utils::ErrorType::ResourceCreation,
                      "RootSignature is not initialized");
       
       m_rootSignature = std::move(rootSignature);
      return createPSO(device, desc);
   }

   Utils::VoidResult ModelPipelineState::createPSO(Device* device, const ModelPipelineStateDesc& desc)
   {
       CHECK_CONDITION(desc.vertexShader && desc.vertexShader->isValid(),
                Utils::ErrorType::ShaderCompilation,
                "VertexShader is null or invaild");
       CHECK_CONDITION(desc.pixelShader && desc.pixelShader->isValid(),
                       Utils::ErrorType::ShaderCompilation,
                       "PixelShader is null or invaild");

       D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};

       // RootSignature set
       psoDesc.pRootSignature = m_rootSignature.get();
       
       psoDesc.VS = {desc.vertexShader->getBytecode(),
               desc.vertexShader->getBytecodeSize()};
       psoDesc.PS = {desc.pixelShader->getBytecode(),
                     desc.pixelShader->getBytecodeSize()};
       
       // Input Layout
       psoDesc.InputLayout.pInputElementDescs =
           desc.inputLayout.empty() ? nullptr : desc.inputLayout.data();
       psoDesc.InputLayout.NumElements = static_cast<UINT>(desc.inputLayout.size());
       
       
       
       return {};
   }
}

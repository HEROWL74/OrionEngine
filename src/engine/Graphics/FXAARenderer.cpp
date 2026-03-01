#include "FXAARenderer.hpp"
#include "../Core/ProjectSettings.hpp"
#include <directx/d3dx12.h>
#include <format>

namespace Engine::Graphics
{
	Utils::VoidResult FXAARenderer::initialize(
		Device* device,
		ShaderManager* shaderManager,
		uint32_t width,
		uint32_t height
	)
	{
		CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
		CHECK_CONDITION(shaderManager != nullptr, Utils::ErrorType::Unknown, "ShaderManager is null");

		m_device = device;
		m_width = width;
		m_height = height;


		// Default Settings
		m_fxaaConstants.rcpFrame = Math::Vector2(1.0 / width, 1.0f / height);
		m_fxaaConstants.fxaaQualitySubpix = 0.75f;
		m_fxaaConstants.fxaaQualityEdgeThreshold = 0.125f;

		auto rootSigResult = createRootSignature();
		if (!rootSigResult) return rootSigResult;

		auto psoResult = createPipelineState(shaderManager);
		if (!psoResult) return psoResult;

		auto cbResult = createConstantBuffer();
		if (!cbResult) return cbResult;

		updateCBs();

		Utils::log_info("FXAA Renderer initialized successfully");
		return{};
	}

	void FXAARenderer::apply(
		ID3D12GraphicsCommandList* commandList,
		ID3D12Resource* sourceTexture,
		D3D12_GPU_DESCRIPTOR_HANDLE sourceSRV,
		RenderTarget* outputTarget
	)
	{
		if (!commandList || !sourceTexture || !outputTarget)return;

		Utils::log_info("FXAA apply called");

		// 出力ターゲットに遷移
		outputTarget->transitionTo(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);

		D3D12_CPU_DESCRIPTOR_HANDLE rtv = outputTarget->getRTV();
		commandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

		D3D12_VIEWPORT viewport{};
		viewport.Width = static_cast<float>(m_width);
		viewport.Height = static_cast<float>(m_height);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		D3D12_RECT scissor{};
		scissor.right = static_cast<LONG>(m_width);
		scissor.bottom = static_cast<LONG>(m_height);

		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissor);

		// FXAA適用
		commandList->SetPipelineState(m_pipelineState.Get());
		commandList->SetGraphicsRootSignature(m_rootSignature.Get());

		ID3D12DescriptorHeap* heaps[] = { m_device->getSrvHeap() };
		commandList->SetDescriptorHeaps(1, heaps);

		commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());
		commandList->SetGraphicsRootDescriptorTable(1, sourceSRV);

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->DrawInstanced(3, 1, 0, 0);
	}

	void FXAARenderer::resize(uint32_t width, uint32_t height)
	{
		m_width = width;
		m_height = height;
		m_fxaaConstants.rcpFrame = Math::Vector2(1.0f / width, 1.0f / height);
		updateCBs();
	}
	void FXAARenderer::setQuality(float subpix, float edgeThresHold, float fxaaQualityEdgeThresholdMin)
	{
		if (!m_constantBufferMapped)
		{
			Utils::log_warning("FXAA CB not mapped!");
			return;
		}

		m_fxaaConstants.fxaaQualitySubpix = subpix;
		m_fxaaConstants.fxaaQualityEdgeThreshold = edgeThresHold;
		m_fxaaConstants.fxaaQualityEdgeThresholdMin = fxaaQualityEdgeThresholdMin;
		updateCBs();
	}

	Utils::VoidResult FXAARenderer::createRootSignature()
	{
		auto dev = m_device->getDevice();

		CD3DX12_ROOT_PARAMETER1 rootParams[2] = {};

		// CB (b0)
		rootParams[0].InitAsConstantBufferView(0, 0,
			D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);

		// SRV (t0)
		CD3DX12_DESCRIPTOR_RANGE1 srvRange{};
		srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
		rootParams[1].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);

		// Sampler
		CD3DX12_STATIC_SAMPLER_DESC samplerDesc(
			0,
			D3D12_FILTER_MIN_MAG_MIP_LINEAR,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP
		);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc{};
		rootSigDesc.Init_1_1(2, rootParams, 1, &samplerDesc,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> sig, err;
		CHECK_HR(D3DX12SerializeVersionedRootSignature(&rootSigDesc,
			D3D_ROOT_SIGNATURE_VERSION_1_1, &sig, &err),
			Utils::ErrorType::ResourceCreation, "Failed to serialize FXAA root signature");

		CHECK_HR(dev->CreateRootSignature(0, sig->GetBufferPointer(),
			sig->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)),
			Utils::ErrorType::ResourceCreation, "Failed to create FXAA root signature");

		return {};
	}

	Utils::VoidResult FXAARenderer::createPipelineState(ShaderManager* shaderManager)
	{
		auto dev = m_device->getDevice();
		auto& settings = Engine::Core::ProjectSettings::get();

		ShaderCompileDesc vsDesc;
		vsDesc.filePath = settings.getEngineAssetPath("shaders/FXAA_VS.cso").string();
		vsDesc.entryPoint = "main";
		vsDesc.type = ShaderType::Vertex;
		vsDesc.enableDebug = true;
	    
		auto vsResult = shaderManager->loadShader(vsDesc);
		if (!vsResult)return std::unexpected(Utils::make_error(
			Utils::ErrorType::ShaderCompilation, "Failed to load FXAA Vertex Shader"
		));

		ShaderCompileDesc psDesc;
		psDesc.filePath = settings.getEngineAssetPath("shaders/FXAA_PS.cso").string();
		psDesc.entryPoint = "main";
		psDesc.type = ShaderType::Pixel;
		psDesc.enableDebug = true;

		auto psResult = shaderManager->loadShader(psDesc);
		if (!psResult)return std::unexpected(Utils::make_error(
			Utils::ErrorType::ShaderCompilation, "Failed to load Pixel Vertex Shader"
		));

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = m_rootSignature.Get();
		psoDesc.VS = { vsResult->getBytecode(), vsResult->getBytecodeSize() };
		psoDesc.PS = { psResult->getBytecode(), psResult->getBytecodeSize() };
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;

		CHECK_HR(dev->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)), Utils::ErrorType::ResourceCreation, "Failed to create FXAA pipeline state");

		return {};
	}
	Utils::VoidResult FXAARenderer::createConstantBuffer()
	{
		auto dev = m_device->getDevice();
		const UINT cbSize = (sizeof(FXAAConstants) + 255) & ~255;

		auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);

		CHECK_HR(dev->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
			&bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(&m_constantBuffer)),
			Utils::ErrorType::ResourceCreation, "Failed to map FXAA constant buffer");

		D3D12_RANGE readRange{ 0,0 };
		CHECK_HR(m_constantBuffer->Map(0, &readRange, &m_constantBufferMapped),
			Utils::ErrorType::ResourceCreation, "Failed to map FXAA constant buffer");

		return {};
	}
	void FXAARenderer::updateCBs()
	{
		if (m_constantBufferMapped)
		{
			memcpy(m_constantBufferMapped, &m_fxaaConstants, sizeof(FXAAConstants));
		}
	}
}


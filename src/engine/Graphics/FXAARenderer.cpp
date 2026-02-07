#include "FXAARenderer.hpp"
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

	}

	void FXAARenderer::resize(uint32_t width, uint32_t height)
	{

	}
	void FXAARenderer::setQuality(float subpix, float edgeThresHold)
	{

	}

	Utils::VoidResult FXAARenderer::createRootSignature()
	{

	}
	Utils::VoidResult FXAARenderer::createPipelineState(ShaderManager* shaderManager)
	{

	}
	Utils::VoidResult FXAARenderer::createConstantBuffer()
	{

	}
	void FXAARenderer::updateCBs()
	{

	}
}
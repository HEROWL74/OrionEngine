// src/Graphics/FXAARenderer.hpp
#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "Device.hpp"
#include "ShaderManager.hpp"
#include "RenderTarget.hpp"
#include "../Math/Math.hpp"
#include "../Utils/Common.hpp"

using Microsoft::WRL::ComPtr;

namespace Engine::Graphics
{
	struct FXAAConstants
	{
		Math::Vector2 rcpFrame;
		float fxaaQualitySubpix;
		float fxaaQualityEdgeThreshold;
		float fxaaQualityEdgeThresholdMin;
	};

	class FXAARenderer
	{
	public:
		FXAARenderer() = default;
		~FXAARenderer() = default;

		[[nodiscatd]] Utils::VoidResult initialize(
			Device* device,
			ShaderManager* shaderManager,
			uint32_t width,
			uint32_t height
		);

		void apply(
			ID3D12GraphicsCommandList* commandList,
			ID3D12Resource* sourceTexture,
			D3D12_GPU_DESCRIPTOR_HANDLE sourceSRV,
			RenderTarget* outputTarget
		);

		void resize(uint32_t width, uint32_t height);
		void setQuality(float subpix, float edgeThresHold, float fxaaQualityEdgeThresholdMin);

	private:
		Device* m_device = nullptr;
		uint32_t m_width = 0;
		uint32_t m_height = 0;

		// FXAA RootSig &  PSO,  Resource CB
		ComPtr<ID3D12RootSignature> m_rootSignature;
		ComPtr<ID3D12PipelineState> m_pipelineState;
		ComPtr<ID3D12Resource> m_constantBuffer;
		void* m_constantBufferMapped = nullptr;

		FXAAConstants m_fxaaConstants{};

		[[nodiscard]] Utils::VoidResult createRootSignature();
		[[nodiscard]] Utils::VoidResult createPipelineState(ShaderManager* shaderManager);
		[[nodiscard]] Utils::VoidResult createConstantBuffer();
		void updateCBs();
	};
}


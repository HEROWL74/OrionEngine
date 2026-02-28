// src/Graphics/RootSignature.hpp
#pragma once
#include <d3d12.h>
#include "../Utils/Common.hpp"
#include "Device.hpp"
#include "ShaderManager.hpp"

namespace Engine::Graphics
{
	class RootParameter
	{
	public:
		RootParameter()
		{
			m_RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		}
	private:
		D3D12_ROOT_PARAMETER m_RootParam;
	};

	class RootSignature
	{
	public:
		RootSignature() = default;
		~RootSignature() = default;

		[[nodiscard]] Utils::VoidResult initialize(Device* device, ShaderManager* shaderManager);

	private:
		Device* m_device = nullptr;
		ShaderManager* m_shaderManager = nullptr;

		UINT m_NumParaneters{};
		UINT m_NumSamplers{};
		UINT m_NumInitializedStaticSamplers{};
		uint32_t m_DescriptorTableBitMap{};
		uint32_t m_SamplerTableBitMap;
		uint32_t m_DescriptorTableSize[16]{};

	};
}

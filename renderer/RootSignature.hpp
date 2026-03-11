// src/Graphics/RootSignature.hpp
#pragma once
#include <d3d12.h>
#include "../engine/Utils/Common.hpp"
#include "Device.hpp"
#include "ShaderManager.hpp"

namespace Renderer
{
	using namespace Engine;

	// ===============================================
	// PBR Layout Slot CB
	// ===============================================
	namespace PBRRootSlots
	{
		constexpr UINT Camera = 0;    // b0: Camera
		constexpr UINT Object = 1;    // b1: Object
		constexpr UINT Material = 2;  // b2: Material 
		constexpr UINT Textures = 3;  // t0-t5: Textures
	}

	// ===============================================
	// RootSignature Layout
	// ===============================================

	class RootSignature
	{
	public:
		RootSignature() = default;
		~RootSignature() = default;

		/* Delete copy & default move
		*理由: 使うファイルによって、使いまわすRootSignatureは
		* それぞれ違うため、ムーブのみ許可し、コピーを禁止
		*/ 
		RootSignature(const RootSignature&) = delete;
		RootSignature& operator=(const RootSignature&) = delete;
		RootSignature(RootSignature&&) = default;
		RootSignature& operator=(RootSignature&&) = default;

		// --------------
		// Builder Method (to call before finalize())
		// --------------

		// Add CBV (b0, b1, b2, ...)
		RootSignature& addCBV(
			uint32_t shaderRegister,
			D3D12_SHADER_VISIBILITY visiblity = D3D12_SHADER_VISIBILITY_ALL);

		// Add SRV Descriptor Table
		// count: How many textures to bundle
		// baseRegister t0, t1 ... leading number
		RootSignature& addSRVTable(
			uint32_t count,
			uint32_t baseRegister,
			D3D12_SHADER_VISIBILITY visiblity = D3D12_SHADER_VISIBILITY_PIXEL);

		// Linear Wrap Sample (Static Samplerを追加)
		RootSignature& addLinearSampler(
			uint32_t shaderRegister,
			D3D12_SHADER_VISIBILITY visiblity = D3D12_SHADER_VISIBILITY_PIXEL);

		// パラメータを確定してGPUオブジェクトを生成
		[[nodiscard]] Utils::VoidResult finalize(Device* device);

		// D3D12Object取得
		ID3D12RootSignature* get() const { return m_rootSignature.Get(); }

		// 有効性チェック
		bool isValid() const { return m_rootSignature != nullptr; }
	private:
		// finalize()前のパラメータ蓄積用
		// DescriptorRangeはポインタで参照されるため
		// vectorに保持しておいてアドレスを安定させる
		std::vector<D3D12_ROOT_PARAMETER> m_params;
		std::vector<D3D12_DESCRIPTOR_RANGE> m_ranges;
		std::vector<D3D12_STATIC_SAMPLER_DESC> m_samplers;

		ComPtr<ID3D12RootSignature> m_rootSignature;
	};

	// =========================================
	// 使いまわすようのレイアウトファクトリ関数
	// =========================================
	namespace RootSignatureFactory
	{
		// PBR標準レイアウト
		// Slot0(b0): Camera CBV (ALL)
		// Slot0(b1): Object CBV (VS)
		// Slot0(b2): Material CBV (PS)
		// Slot3(t0-t5): SRV Table (PS)
		// s0: Linear Wrap Sampler (PS)
		[[nodiscard]] Utils::Result<RootSignature> createPBR(Device* device);
	}
}


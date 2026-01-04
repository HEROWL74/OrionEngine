// RenderTarget.hpp
#pragma once
#include "Device.hpp"
#include "editor/UI/ImGuiManager.hpp"

namespace Engine::Graphics
{
	using Microsoft::WRL::ComPtr;

	class RenderTarget
	{
	public:
		RenderTarget() = default;
		~RenderTarget() { release(); }

		Utils::VoidResult initialize(Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format);
		void registerToImGui(UI::ImGuiManager* imguiManager);
		void release();
		void transitionTo(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES newState);

		ID3D12Resource* getTexture() const noexcept { return m_texture.Get(); }
		D3D12_CPU_DESCRIPTOR_HANDLE getRTV() const noexcept { return m_rtv; }
		D3D12_CPU_DESCRIPTOR_HANDLE getDSV() const noexcept { return m_dsv; }
		ImTextureID getImGuiTextureID() const noexcept { return m_imguiTextureID; }
		D3D12_RESOURCE_STATES getCurrentState() const noexcept { return m_currentState; }

		uint32_t getWidth() const noexcept { return m_width; }
		uint32_t getHeight() const noexcept { return m_height; }

	private:
		Device* m_device = nullptr;
		ComPtr<ID3D12Resource> m_texture;
		ComPtr<ID3D12Resource> m_depthStencilBuffer;
		ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
		ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
		D3D12_CPU_DESCRIPTOR_HANDLE m_rtv{};
		D3D12_CPU_DESCRIPTOR_HANDLE m_dsv{};
		ImTextureID m_imguiTextureID = {};
		D3D12_RESOURCE_STATES m_currentState = D3D12_RESOURCE_STATE_RENDER_TARGET;

		uint32_t m_width = 0;
		uint32_t m_height = 0;
		DXGI_FORMAT m_format = DXGI_FORMAT_UNKNOWN;
	};
}
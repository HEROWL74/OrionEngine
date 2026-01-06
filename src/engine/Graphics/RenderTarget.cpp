// RenderTarget.cpp
#include "RenderTarget.hpp"

namespace Engine::Graphics
{
	Utils::VoidResult RenderTarget::initialize(Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format)
	{
		CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Invalid RenderTarget parameters - device is null");
		CHECK_CONDITION(width > 0 && height > 0, Utils::ErrorType::Unknown, "Invalid RenderTarget parameters - invalid dimensions");

		m_device = device;
		m_width = width;
		m_height = height;
		m_format = format;

		auto dev = device->getDevice();

		D3D12_HEAP_PROPERTIES heapProps{};
		heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_RESOURCE_DESC textureDesc{};
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		textureDesc.Width = width;
		textureDesc.Height = height;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.MipLevels = 1;
		textureDesc.Format = format;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		D3D12_CLEAR_VALUE clearValue{};
		clearValue.Format = format;
		clearValue.Color[0] = 0.15f;
		clearValue.Color[1] = 0.15f;
		clearValue.Color[2] = 0.15f;
		clearValue.Color[3] = 1.0f;

		CHECK_HR(dev->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&textureDesc,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			&clearValue,
			IID_PPV_ARGS(&m_texture)
		), Utils::ErrorType::ResourceCreation, "Failed to create render target texture");

		m_currentState = D3D12_RESOURCE_STATE_RENDER_TARGET;

		Utils::log_info(std::format("RenderTarget texture created: 0x{:016X}",
			reinterpret_cast<uintptr_t>(m_texture.Get())));

		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.NumDescriptors = 1;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		CHECK_HR(dev->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)),
			Utils::ErrorType::ResourceCreation, "Failed to create RTV heap");

		m_rtv = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.Format = format;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		dev->CreateRenderTargetView(m_texture.Get(), &rtvDesc, m_rtv);

		D3D12_RESOURCE_DESC depthDesc{};
		depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthDesc.Width = width;
		depthDesc.Height = height;
		depthDesc.DepthOrArraySize = 1;
		depthDesc.MipLevels = 1;
		depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthDesc.SampleDesc.Count = 1;
		depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE depthClearValue{};
		depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		depthClearValue.DepthStencil.Depth = 1.0f;
		depthClearValue.DepthStencil.Stencil = 0;

		CHECK_HR(dev->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&depthDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&depthClearValue,
			IID_PPV_ARGS(&m_depthStencilBuffer)
		), Utils::ErrorType::ResourceCreation, "Failed to create depth stencil buffer");

		Utils::log_info(std::format("RenderTarget depth buffer created: 0x{:016X}",
			reinterpret_cast<uintptr_t>(m_depthStencilBuffer.Get())));

		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		CHECK_HR(dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)),
			Utils::ErrorType::ResourceCreation, "Failed to create DSV heap");

		m_dsv = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

		dev->CreateDepthStencilView(m_depthStencilBuffer.Get(), &dsvDesc, m_dsv);

		Utils::log_info(std::format("RenderTarget initialized: {}x{}", width, height));
		return {};
	}

	void RenderTarget::transitionTo(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES newState)
	{
		if (m_currentState == newState || !m_texture || !commandList)
		{
			return;
		}

		Utils::log_info(std::format("RenderTarget transition: 0x{:016X} from state {} to state {}",
			reinterpret_cast<uintptr_t>(m_texture.Get()),
			static_cast<int>(m_currentState),
			static_cast<int>(newState)));

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = m_texture.Get();
		barrier.Transition.StateBefore = m_currentState;
		barrier.Transition.StateAfter = newState;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		commandList->ResourceBarrier(1, &barrier);
		m_currentState = newState;
	}

	void RenderTarget::release()
	{
		if (m_texture)
		{
			Utils::log_info(std::format("Releasing RenderTarget texture: 0x{:016X}",
				reinterpret_cast<uintptr_t>(m_texture.Get())));
		}

		if (m_depthStencilBuffer)
		{
			Utils::log_info(std::format("Releasing RenderTarget depth buffer: 0x{:016X}",
				reinterpret_cast<uintptr_t>(m_depthStencilBuffer.Get())));
		}

		m_texture.Reset();
		m_depthStencilBuffer.Reset();
		m_rtvHeap.Reset();
		m_dsvHeap.Reset();
		m_rtv = {};
		m_dsv = {};
		m_currentState = D3D12_RESOURCE_STATE_RENDER_TARGET;

		Utils::log_info("RenderTarget released");
	}
}
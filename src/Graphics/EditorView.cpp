// EditorView.cpp
#include "EditorView.hpp"

namespace Engine::Graphics
{
	Utils::VoidResult EditorView::initialize(Device* device, uint32_t width, uint32_t height)
	{
		CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
		CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");
		CHECK_CONDITION(width > 0 && height > 0, Utils::ErrorType::Unknown, "Invalid dimensions");

		m_device = device;
		m_width = width;
		m_height = height;

		Utils::log_info(std::format("EditorView::initialize - Creating RenderTarget {}x{}", width, height));

		m_renderTarget = std::make_unique<RenderTarget>();
		auto result = m_renderTarget->initialize(device, width, height, DXGI_FORMAT_R8G8B8A8_UNORM);
		if (!result)
		{
			Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "Failed to create EditorView render target"));
			return std::unexpected(Utils::make_error(
				Utils::ErrorType::Unknown, "Failed to create EditorView render target"));
		}

		m_initialized = true;
		Utils::log_info("EditorView initialized successfully");
		return {};
	}

	void EditorView::render(Scene& scene, ID3D12GraphicsCommandList* commandList, const Camera& camera, UINT frameIndex)
	{
		if (!m_initialized || !m_renderTarget || !commandList)
		{
			Utils::log_warning("EditorView::render - Skipping render (not initialized or invalid resources)");
			return;
		}

		Utils::log_info(std::format("EditorView::render - Using texture: 0x{:016X}, Current state: {}",
			reinterpret_cast<uintptr_t>(m_renderTarget->getTexture()),
			static_cast<int>(m_renderTarget->getCurrentState())));

		m_renderTarget->transitionTo(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);

		D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_renderTarget->getRTV();
		D3D12_CPU_DESCRIPTOR_HANDLE dsv = m_renderTarget->getDSV();
		commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

		float clearColor[4] = { 0.15f, 0.15f, 0.15f, 1.0f };
		commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
		commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		D3D12_VIEWPORT viewport{};
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		viewport.Width = static_cast<float>(m_width);
		viewport.Height = static_cast<float>(m_height);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		D3D12_RECT scissorRect{};
		scissorRect.left = 0;
		scissorRect.top = 0;
		scissorRect.right = static_cast<LONG>(m_width);
		scissorRect.bottom = static_cast<LONG>(m_height);

		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);

		for (auto& gameObject : scene.getGameObjects())
		{
			if (gameObject->isActive())
			{
				auto* renderComponent = gameObject->getComponent<RenderComponent>();
				if (renderComponent && renderComponent->isEnabled() && renderComponent->isVisible())
				{
					renderComponent->render(commandList, camera, frameIndex);
				}
			}
		}

		renderEditorElements(commandList, camera, frameIndex);

		m_renderTarget->transitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		Utils::log_info("EditorView::render - Completed");
	}

	void EditorView::renderEditorElements(ID3D12GraphicsCommandList* commandList, const Camera& camera, UINT frameIndex)
	{
		if (!commandList)
		{
			return;
		}

		if (m_showGrid)
		{
			renderGrid(commandList, camera);
		}

		if (m_showGizmos && m_selectedObject)
		{
			renderSelectionOutline(commandList, camera, m_selectedObject);
		}
	}

	void EditorView::renderGrid(ID3D12GraphicsCommandList* commandList, const Camera& camera)
	{
	}

	void EditorView::renderSelectionOutline(ID3D12GraphicsCommandList* commandList, const Camera& camera, Core::GameObject* object)
	{
		if (!object)
		{
			return;
		}
	}

	ImTextureID EditorView::getOutputTexture() const
	{
		if (!m_renderTarget)
		{
			return {};
		}

		return m_renderTarget->getImGuiTextureID();
	}

	void EditorView::registerToImGui(UI::ImGuiManager* imguiManager)
	{
		if (!m_renderTarget || !imguiManager)
		{
			Utils::log_warning("Cannot register EditorView to ImGui - invalid state");
			return;
		}

		m_imguiManager = imguiManager;

		Utils::log_info("EditorView::registerToImGui - Starting registration");
		m_renderTarget->registerToImGui(imguiManager);
		Utils::log_info("EditorView registered to ImGui");
	}

	void EditorView::resize(uint32_t width, uint32_t height)
	{
		if (!m_device || width == 0 || height == 0)
		{
			return;
		}

		if (m_width == width && m_height == height)
		{
			return;
		}

		Utils::log_info(std::format("EditorView::resize - {}x{} -> {}x{}", m_width, m_height, width, height));

		m_width = width;
		m_height = height;

		if (m_renderTarget)
		{
			Utils::log_info("EditorView::resize - Releasing old RenderTarget");
			m_renderTarget->release();
			m_renderTarget.reset();
			Utils::log_info("EditorView::resize - Old RenderTarget released");
		}

		Utils::log_info("EditorView::resize - Creating new RenderTarget");
		m_renderTarget = std::make_unique<RenderTarget>();
		auto result = m_renderTarget->initialize(m_device, width, height, DXGI_FORMAT_R8G8B8A8_UNORM);

		if (!result)
		{
			Utils::log_error(result.error());
			Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "Failed to resize EditorView render target"));
		}
		else
		{
			if (m_imguiManager)
			{
				m_renderTarget->registerToImGui(m_imguiManager);
				Utils::log_info("EditorView re-registered to ImGui after resize");
			}
			Utils::log_info(std::format("EditorView resized successfully to {}x{}", width, height));
		}
	}
}
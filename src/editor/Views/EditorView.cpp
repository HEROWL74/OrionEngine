// EditorView.cpp
#include "EditorView.hpp"
#include "engine/ThirdParty/d3dx12.h"

namespace Engine::Graphics
{
	Utils::VoidResult EditorView::initialize(Device* device, uint32_t width, uint32_t height, ShaderManager* shaderManager)
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

		if (shaderManager)
		{
			auto gridResult = initializeGrid(shaderManager);
			if (!gridResult)
			{
				Utils::log_warning("Failed to initialize grid - grid will not be rendered");
			}
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

		m_renderTarget->transitionTo(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);

		D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_renderTarget->getRTV();
		D3D12_CPU_DESCRIPTOR_HANDLE dsv = m_renderTarget->getDSV();
		commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

		float clearColor[4] = { 1.0f, 0.0f, 1.0f, 1.0f };
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

		if (m_skybox)
		{
			m_skybox->render(commandList, camera);
		}

		if (m_showGrid)
		{
			renderGrid(commandList, camera);
		}


		//  EditorView 専用の frameIndex を使用（0, 1）
		UINT editorFrameIndex = frameIndex % 2;  // 0 または 1

		for (auto& gameObject : scene.getGameObjects())
		{
			if (gameObject->isActive())
			{
				auto* renderComponent = gameObject->getComponent<RenderComponent>();
				if (renderComponent && renderComponent->isEnabled() && renderComponent->isVisible())
				{
					renderComponent->render(commandList, camera, editorFrameIndex);
				}
			}
		}

		renderEditorElements(commandList, camera, editorFrameIndex);

		m_renderTarget->transitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	void EditorView::renderEditorElements(ID3D12GraphicsCommandList* commandList, const Camera& camera, UINT frameIndex)
	{
		if (!commandList)
		{
			return;
		}

		if (m_showGizmos && m_selectedObject)
		{
			renderSelectionOutline(commandList, camera, m_selectedObject);
		}
	}

	void EditorView::renderGrid(ID3D12GraphicsCommandList* commandList, const Camera& camera)
	{
		if (!m_gridInitialized || !m_gridVertexBuffer || !m_gridCameraBuffer)
		{
			return;
		}

		// CB更新 通常のビュー行列を使用（平行移動を含む）
		if (m_gridCameraMapped)
		{
			GridCameraConstants constants;
			constants.viewProjection = camera.getViewProjectionMatrix();
			memcpy(m_gridCameraMapped, &constants, sizeof(GridCameraConstants));
		}

		commandList->SetPipelineState(m_gridPipelineState.Get());
		commandList->SetGraphicsRootSignature(m_gridRootSignature.Get());

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		commandList->IASetVertexBuffers(0, 1, &m_gridVertexBufferView);
		commandList->SetGraphicsRootConstantBufferView(0, m_gridCameraBuffer->GetGPUVirtualAddress());

		commandList->DrawInstanced(m_gridVertexCount, 1, 0, 0);
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

	Utils::VoidResult EditorView::initializeGrid(ShaderManager* shaderManager)
	{
		if (!m_device || !shaderManager)
		{
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Device or ShaderManager is null"));
		}

		if (!shaderManager)
		{
			Utils::log_warning("ShaderManager is null - creating grid with basic shaders");
		}

		// ジオメトリ作成
		auto geomResult = createGridGeometry();
		if (!geomResult)return geomResult;

		// ルートシグネチャ作成
		auto rootSigResult = createGridRootSignature();
		if (!rootSigResult) return rootSigResult;

		// パイプラインステート作成
		auto psoResult = createGridPipelineState(shaderManager);
		if (!psoResult) return psoResult;

		m_gridInitialized = true;
		Utils::log_info("Grid initialized successfully");
		return {};
	}

	Utils::VoidResult EditorView::createGridGeometry()
	{
		auto dev = m_device->getDevice();

		// グリッドの設定
		const float gridSize = 50.0f;
		const float gridStep = 1.0f;
		const int gridLines = static_cast<int>(gridSize / gridStep);

		std::vector<GridVertex> vertices;

		// X軸方向の線
		for (int i = -gridLines; i <= gridLines; ++i)
		{
			float pos = i * gridStep;
			Math::Vector4 color = (i == 0) ? 
				Math::Vector4(0.0f, 0.0f, 1.0f, 1.0f) // Z軸は蒼
				: Math::Vector4(0.3f, 0.3f, 0.3f, 1.0f);
			vertices.push_back({ Math::Vector3(-gridSize,0.0f,pos),color });
			vertices.push_back({ Math::Vector3(gridSize, 0.0f,pos),color });
		}


		// Z軸方向の線
		for (int i = -gridLines; i <= gridLines; ++i)
		{
			float pos = i * gridStep;
			Math::Vector4 color = (i == 0) ?
				Math::Vector4(1.0f, 0.0f, 0.0f, 1.0f) // X軸は赤
				: Math::Vector4(0.3f, 0.3f, 0.3f, 1.0f);
			vertices.push_back({ Math::Vector3(pos,0.0f,-gridSize),color });
			vertices.push_back({ Math::Vector3(pos, 0.0f,gridSize),color });
		}



		m_gridVertexCount = static_cast<UINT>(vertices.size());
		const UINT vertexBufferSize = static_cast<UINT>(sizeof(GridVertex) * vertices.size());

		// 頂点バッファ作成
		auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

		CHECK_HR(dev->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_gridVertexBuffer)
		), Utils::ErrorType::ResourceCreation, "Failed to create grid vertex buffer");

		// データをコピー
		void* pData = nullptr;
		D3D12_RANGE readRange{ 0,0 };
		m_gridVertexBuffer->Map(0, &readRange, &pData);
		memcpy(pData, vertices.data(), vertexBufferSize);
		m_gridVertexBuffer->Unmap(0, nullptr);

		// 頂点バッファビュー設定
		m_gridVertexBufferView.BufferLocation = m_gridVertexBuffer->GetGPUVirtualAddress();
		m_gridVertexBufferView.StrideInBytes = sizeof(GridVertex);
		m_gridVertexBufferView.SizeInBytes = vertexBufferSize;

		// カメラ定数バッファ作成
		const UINT cbSize = (sizeof(GridCameraConstants) + 255) & ~255;
		auto cbDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);

		CHECK_HR(dev->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&cbDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_gridCameraBuffer)
		), Utils::ErrorType::ResourceCreation, "Failed to create grid camera buffer");

		// カメラバッファをマップしたままにする
		CHECK_HR(m_gridCameraBuffer->Map(0, &readRange, &m_gridCameraMapped),
			Utils::ErrorType::ResourceCreation, "Failed to map grid camera buffer");

		return {};
	}

	Utils::VoidResult EditorView::createGridRootSignature()
	{
		auto dev = m_device->getDevice();

		// ルートパラメータ: CB
		CD3DX12_ROOT_PARAMETER1 rootParams[1] = {};
		rootParams[0].InitAsConstantBufferView(
			0,
			0,
			D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
			D3D12_SHADER_VISIBILITY_VERTEX
		);

		// ルートシグネチャ
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc{};
		rootSigDesc.Init_1_1(
			_countof(rootParams),
			rootParams,
			0,
			nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		);

		// シリアライズ
		ComPtr<ID3DBlob> sig;
		ComPtr<ID3DBlob> err;
		CHECK_HR(D3DX12SerializeVersionedRootSignature(
			&rootSigDesc,
			D3D_ROOT_SIGNATURE_VERSION_1_1,
			&sig,
			&err
		), Utils::ErrorType::ResourceCreation, "Failed to serialize grid root signature");

		CHECK_HR(dev->CreateRootSignature(
			0,
			sig->GetBufferPointer(),
			sig->GetBufferSize(),
			IID_PPV_ARGS(&m_gridRootSignature)
		), Utils::ErrorType::ResourceCreation, "Failed to create grid root signature");

		return {};
	}

	Utils::VoidResult EditorView::createGridPipelineState(ShaderManager* shaderManager)
	{
		auto dev = m_device->getDevice();

		if (!shaderManager)
		{
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
				"ShaderManager is required for grid initialization"));
		}

		// シェーダー読み込み
		ShaderCompileDesc vsDesc;
		vsDesc.filePath = "engine-assets/shaders/GridVS.hlsl";
		vsDesc.entryPoint = "main";
		vsDesc.type = ShaderType::Vertex;
		vsDesc.enableDebug = true;

		auto vertexShaderResult = shaderManager->loadShader(vsDesc);
		if (!vertexShaderResult)
		{
			return std::unexpected(Utils::make_error(Utils::ErrorType::ShaderCompilation,
				"Failed to load grid vertex shader"));
		}

		ShaderCompileDesc psDesc;
		psDesc.filePath = "engine-assets/shaders/GridPS.hlsl";
		psDesc.entryPoint = "main";
		psDesc.type = ShaderType::Pixel;
		psDesc.enableDebug = true;

		auto pixelShaderResult = shaderManager->loadShader(psDesc);
		if (!pixelShaderResult)
		{
			return std::unexpected(Utils::make_error(Utils::ErrorType::ShaderCompilation,
				"Failed to load grid pixel shader"));
		}

		// 入力レイアウト
		D3D12_INPUT_ELEMENT_DESC inputLayout[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,0,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
			{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,12,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		};

		// パイプラインステート設定
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = m_gridRootSignature.Get();
		psoDesc.VS = { vertexShaderResult->getBytecode(), vertexShaderResult->getBytecodeSize() };
		psoDesc.PS = { pixelShaderResult->getBytecode(), pixelShaderResult->getBytecodeSize() };
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.SampleMask = UINT_MAX;

		// ラスタライザ設定
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

		// デプスステンシル設定
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		psoDesc.DepthStencilState.DepthEnable = TRUE;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;

		psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count = 1;

		CHECK_HR(dev->CreateGraphicsPipelineState(
			&psoDesc,
			IID_PPV_ARGS(&m_gridPipelineState)
		), Utils::ErrorType::ResourceCreation, "Failed to create grid pipeline state");

		return {};
	}

}
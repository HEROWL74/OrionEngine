#include "Gizmo.hpp"
#include "engine/ThirdParty/d3dx12.h"
#include "../Utils/RayPicking.hpp"

namespace Editor::UI
{
	Engine::Utils::VoidResult Gizmo::initialize(Engine::Graphics::Device* device, Engine::Graphics::ShaderManager* shaderManager)
	{
		CHECK_CONDITION(device != nullptr, Engine::Utils::ErrorType::Unknown, "Device is null");
		CHECK_CONDITION(shaderManager != nullptr, Engine::Utils::ErrorType::Unknown, "ShaderManager is null");

		m_device = device;
		m_shaderManager = shaderManager;

		auto rootSigResult = createRootSignature();
		if (!rootSigResult) return rootSigResult;

		auto psoResult = createPipelineState();
		if (!psoResult) return psoResult;

		auto geomResult = createGeometry();
		if (!geomResult) return geomResult;

		auto cbResult = createConstantBuffer();
		if (!cbResult) return cbResult;

		Engine::Utils::log_info("Gizmo initialized successfully");
		return {};
	}

	void Gizmo::shutdown()
	{
		if (m_constantBuffer && m_cbMapped)
		{
			m_constantBuffer->Unmap(0, nullptr);
			m_cbMapped = nullptr;
		}

		m_constantBuffer.Reset();
		m_indexBuffer.Reset();
		m_vertexBuffer.Reset();
		m_pso.Reset();
		m_rootSig.Reset();

		m_device = nullptr;
		m_shaderManager = nullptr;
	}

	void Gizmo::render(ID3D12GraphicsCommandList* commandList, const Engine::Graphics::Camera& camera, Core::GameObject* targetObject)
	{
		if (!targetObject || !m_device || !m_rootSig || !m_pso || targetObject->isDestroyed())
			return;

		Math::Vector3 position = targetObject->getTransform()->getPosition();

		if (m_type == GizmoType::Translation)
		{
			renderTranslationGizmo(commandList, camera, position);
		}
	}

	GizmoAxis Gizmo::hitTest(const Math::Vector3& rayOrigin,
		const Math::Vector3& rayDirection,
		Engine::Core::GameObject* targetObject) const
	{
		if (!targetObject)
			return GizmoAxis::None;

		Math::Vector3 gizmoPos = targetObject->getTransform()->getPosition();
		float gizmoLength = 2.0f;
		float threshold = 0.2f;

		Math::Vector3 xAxisCenter = gizmoPos + Math::Vector3::right() * (gizmoLength * 0.5f);
		float distanceX;
		if (EditorUtils::RayPicking::rayIntersectsSphere(rayOrigin, rayDirection, xAxisCenter, threshold, distanceX))
		{
			return GizmoAxis::X;
		}

		Math::Vector3 yAxisCenter = gizmoPos + Math::Vector3::up() * (gizmoLength * 0.5f);
		float distanceY;
		if (EditorUtils::RayPicking::rayIntersectsSphere(rayOrigin, rayDirection, yAxisCenter, threshold, distanceY))
		{
			return GizmoAxis::Y;
		}

		Math::Vector3 zAxisCenter = gizmoPos + Math::Vector3::forward() * (gizmoLength * 0.5f);
		float distanceZ;
		if (EditorUtils::RayPicking::rayIntersectsSphere(rayOrigin, rayDirection, zAxisCenter, threshold, distanceZ))
		{
			return GizmoAxis::Z;
		}

		return GizmoAxis::None;
	}

	void Gizmo::beginDrag(GizmoAxis axis,
		const Math::Vector3& rayOrigin,
		const Math::Vector3& rayDirection,
		const Math::Vector3& objectPosition)
	{
		m_isDragging = true;
		m_selectedAxis = axis;
		m_dragStartObjectPosition = objectPosition;

		switch (axis)
		{
		case GizmoAxis::X:
			m_dragAxisDirection = Math::Vector3::right();
			m_dragPlaneNormal = Math::Vector3::up();
			break;
		case GizmoAxis::Y:
			m_dragAxisDirection = Math::Vector3::up();
			m_dragPlaneNormal = Math::Vector3::right();
			break;
		case GizmoAxis::Z:
			m_dragAxisDirection = Math::Vector3::forward();
			m_dragPlaneNormal = Math::Vector3::up();
			break;
		default:
			m_dragAxisDirection = Math::Vector3::up();
			m_dragPlaneNormal = Math::Vector3::up();
			break;
		}

		float distance;
		if (EditorUtils::RayPicking::rayIntersectsPlane(rayOrigin, rayDirection, objectPosition, m_dragPlaneNormal, distance))
		{
			m_dragStartPosition = rayOrigin + rayDirection * distance;
		}
		else
		{
			m_dragStartPosition = objectPosition;
		}
	}

	void Gizmo::processDrag(const Math::Vector3& rayOrigin,
		const Math::Vector3& rayDirection,
		Math::Vector3& outNewPosition)
	{
		if (!m_isDragging)
		{
			outNewPosition = m_dragStartObjectPosition;
			return;
		}

		float distance;
		if (EditorUtils::RayPicking::rayIntersectsPlane(rayOrigin, rayDirection, m_dragStartObjectPosition, m_dragPlaneNormal, distance))
		{
			Math::Vector3 currentPoint = rayOrigin + rayDirection * distance;
			Math::Vector3 projectedPoint = projectPointOnAxis(currentPoint, m_dragStartObjectPosition, m_dragAxisDirection);
			Math::Vector3 delta = projectedPoint - m_dragStartObjectPosition;
			outNewPosition = m_dragStartObjectPosition + delta;
		}
		else
		{
			outNewPosition = m_dragStartObjectPosition;
		}
	}

	void Gizmo::finishDrag()
	{
		m_isDragging = false;
		m_selectedAxis = GizmoAxis::None;
	}

	Math::Vector3 Gizmo::projectPointOnAxis(const Math::Vector3& point,
		const Math::Vector3& axisOrigin,
		const Math::Vector3& axisDirection) const
	{
		Math::Vector3 toPoint = point - axisOrigin;
		float projection = Math::Vector3::dot(toPoint, axisDirection);
		return axisOrigin + axisDirection * projection;
	}

	void Gizmo::renderTranslationGizmo(ID3D12GraphicsCommandList* commandList,
		const Engine::Graphics::Camera& camera,
		const Math::Vector3& position)
	{
		if (!commandList || !m_vertexBuffer || !m_indexBuffer || !m_constantBuffer)
			return;

		float gizmoScale = calculateGizmoScale(camera, position);

		Math::Matrix4 world = Math::Matrix4::translation(position);
		Math::Matrix4 worldViewProjection = camera.getViewProjectionMatrix() * world;

		if (m_cbMapped)
		{
			GizmoConstants constants;
			constants.worldViewProjection = worldViewProjection;
			constants.selectedColor = Math::Vector4(1.0f, 1.0f, 0.0f, 1.0f);
			constants.scale = gizmoScale;

			memcpy(m_cbMapped, &constants, sizeof(GizmoConstants));
		}

		commandList->SetPipelineState(m_pso.Get());
		commandList->SetGraphicsRootSignature(m_rootSig.Get());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		commandList->IASetVertexBuffers(0, 1, &m_vbv);
		commandList->IASetIndexBuffer(&m_ibv);
		commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());
		commandList->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);
	}

	float Gizmo::calculateGizmoScale(const Engine::Graphics::Camera& camera, const Math::Vector3& position)
	{
		float distance = Math::Vector3::distance(camera.getPosition(), position);
		float fov = camera.getFov();
		float scale = distance * std::tan(Math::radians(fov * 0.5f)) * 0.1f;
		return scale;
	}

	Engine::Utils::VoidResult Gizmo::createRootSignature()
	{
		auto dev = m_device->getDevice();

		CD3DX12_ROOT_PARAMETER1 rootParams[1] = {};
		rootParams[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc{};
		rootSigDesc.Init_1_1(_countof(rootParams), rootParams, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> sig, err;
		CHECK_HR(D3DX12SerializeVersionedRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &sig, &err),
			Engine::Utils::ErrorType::ResourceCreation, "Failed to serialize Gizmo root signature");

		CHECK_HR(dev->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&m_rootSig)),
			Engine::Utils::ErrorType::ResourceCreation, "Failed to create Gizmo root signature");

		return {};
	}

	Engine::Utils::VoidResult Gizmo::createPipelineState()
	{
		auto dev = m_device->getDevice();

		Engine::Graphics::ShaderCompileDesc vsDesc;
		vsDesc.filePath = "engine-assets/shaders/GizmoVS.hlsl";
		vsDesc.entryPoint = "main";
		vsDesc.type = Engine::Graphics::ShaderType::Vertex;
		vsDesc.enableDebug = true;

		auto vertexShaderResult = m_shaderManager->loadShader(vsDesc);
		if (!vertexShaderResult)
		{
			return std::unexpected(Engine::Utils::make_error(Engine::Utils::ErrorType::ShaderCompilation, "Failed to load Gizmo vertex shader"));
		}

		Engine::Graphics::ShaderCompileDesc psDesc;
		psDesc.filePath = "engine-assets/shaders/GizmoPS.hlsl";
		psDesc.entryPoint = "main";
		psDesc.type = Engine::Graphics::ShaderType::Pixel;
		psDesc.enableDebug = true;

		auto pixelShaderResult = m_shaderManager->loadShader(psDesc);
		if (!pixelShaderResult)
		{
			return std::unexpected(Engine::Utils::make_error(Engine::Utils::ErrorType::ShaderCompilation, "Failed to load Gizmo pixel shader"));
		}

		D3D12_INPUT_ELEMENT_DESC inputLayout[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = m_rootSig.Get();
		psoDesc.VS = { vertexShaderResult->getBytecode(), vertexShaderResult->getBytecodeSize() };
		psoDesc.PS = { pixelShaderResult->getBytecode(), pixelShaderResult->getBytecodeSize() };
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count = 1;

		CHECK_HR(dev->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pso)),
			Engine::Utils::ErrorType::ResourceCreation, "Failed to create Gizmo pipeline state");

		return {};
	}

	Engine::Utils::VoidResult Gizmo::createGeometry()
	{
		auto dev = m_device->getDevice();

		// Translation Gizmoの頂点データ（3本の軸 + 矢印）
		const float axisLength = 2.0f; // 長さを2倍に
		const float arrowSize = 0.8f;  // 矢印のサイズ
		std::vector<GizmoVertex> vertices;

		// === X軸（赤）===
		// 軸の線
		vertices.push_back({ Math::Vector3(0, 0, 0), Math::Vector4(1, 0, 0, 1) });
		vertices.push_back({ Math::Vector3(axisLength, 0, 0), Math::Vector4(1, 0, 0, 1) });

		// 矢印（三角錐の輪郭）
		Math::Vector3 arrowBase(axisLength - arrowSize, 0, 0);
		Math::Vector3 arrowTip(axisLength, 0, 0);
		vertices.push_back({ arrowBase + Math::Vector3(0, arrowSize, 0), Math::Vector4(1, 0, 0, 1) });
		vertices.push_back({ arrowTip, Math::Vector4(1, 0, 0, 1) });
		vertices.push_back({ arrowBase + Math::Vector3(0, -arrowSize, 0), Math::Vector4(1, 0, 0, 1) });
		vertices.push_back({ arrowTip, Math::Vector4(1, 0, 0, 1) });
		vertices.push_back({ arrowBase + Math::Vector3(0, 0, arrowSize), Math::Vector4(1, 0, 0, 1) });
		vertices.push_back({ arrowTip, Math::Vector4(1, 0, 0, 1) });
		vertices.push_back({ arrowBase + Math::Vector3(0, 0, -arrowSize), Math::Vector4(1, 0, 0, 1) });
		vertices.push_back({ arrowTip, Math::Vector4(1, 0, 0, 1) });

		// === Y軸（緑）===
		// 軸の線
		vertices.push_back({ Math::Vector3(0, 0, 0), Math::Vector4(0, 1, 0, 1) });
		vertices.push_back({ Math::Vector3(0, axisLength, 0), Math::Vector4(0, 1, 0, 1) });

		// 矢印
		Math::Vector3 arrowBaseY(0, axisLength - arrowSize, 0);
		Math::Vector3 arrowTipY(0, axisLength, 0);
		vertices.push_back({ arrowBaseY + Math::Vector3(arrowSize, 0, 0), Math::Vector4(0, 1, 0, 1) });
		vertices.push_back({ arrowTipY, Math::Vector4(0, 1, 0, 1) });
		vertices.push_back({ arrowBaseY + Math::Vector3(-arrowSize, 0, 0), Math::Vector4(0, 1, 0, 1) });
		vertices.push_back({ arrowTipY, Math::Vector4(0, 1, 0, 1) });
		vertices.push_back({ arrowBaseY + Math::Vector3(0, 0, arrowSize), Math::Vector4(0, 1, 0, 1) });
		vertices.push_back({ arrowTipY, Math::Vector4(0, 1, 0, 1) });
		vertices.push_back({ arrowBaseY + Math::Vector3(0, 0, -arrowSize), Math::Vector4(0, 1, 0, 1) });
		vertices.push_back({ arrowTipY, Math::Vector4(0, 1, 0, 1) });

		// === Z軸（青）===
		// 軸の線
		vertices.push_back({ Math::Vector3(0, 0, 0), Math::Vector4(0, 0, 1, 1) });
		vertices.push_back({ Math::Vector3(0, 0, axisLength), Math::Vector4(0, 0, 1, 1) });

		// 矢印
		Math::Vector3 arrowBaseZ(0, 0, axisLength - arrowSize);
		Math::Vector3 arrowTipZ(0, 0, axisLength);
		vertices.push_back({ arrowBaseZ + Math::Vector3(arrowSize, 0, 0), Math::Vector4(0, 0, 1, 1) });
		vertices.push_back({ arrowTipZ, Math::Vector4(0, 0, 1, 1) });
		vertices.push_back({ arrowBaseZ + Math::Vector3(-arrowSize, 0, 0), Math::Vector4(0, 0, 1, 1) });
		vertices.push_back({ arrowTipZ, Math::Vector4(0, 0, 1, 1) });
		vertices.push_back({ arrowBaseZ + Math::Vector3(0, arrowSize, 0), Math::Vector4(0, 0, 1, 1) });
		vertices.push_back({ arrowTipZ, Math::Vector4(0, 0, 1, 1) });
		vertices.push_back({ arrowBaseZ + Math::Vector3(0, -arrowSize, 0), Math::Vector4(0, 0, 1, 1) });
		vertices.push_back({ arrowTipZ, Math::Vector4(0, 0, 1, 1) });

		// インデックスデータ（全ての線分）
		std::vector<uint16_t> indices;
		for (uint16_t i = 0; i < static_cast<uint16_t>(vertices.size()); ++i)
		{
			indices.push_back(i);
		}
		m_indexCount = static_cast<UINT>(indices.size());

		// 頂点バッファの作成
		const UINT vertexBufferSize = static_cast<UINT>(sizeof(GizmoVertex) * vertices.size());
		auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

		CHECK_HR(dev->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vertexBuffer)
		), Engine::Utils::ErrorType::ResourceCreation, "Failed to create Gizmo vertex buffer");

		// 頂点データをコピー
		void* pData = nullptr;
		D3D12_RANGE readRange{ 0, 0 };
		m_vertexBuffer->Map(0, &readRange, &pData);
		memcpy(pData, vertices.data(), vertexBufferSize);
		m_vertexBuffer->Unmap(0, nullptr);

		// 頂点バッファビューの設定
		m_vbv.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vbv.StrideInBytes = sizeof(GizmoVertex);
		m_vbv.SizeInBytes = vertexBufferSize;

		// インデックスバッファの作成
		const UINT indexBufferSize = static_cast<UINT>(sizeof(uint16_t) * indices.size());
		bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

		CHECK_HR(dev->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_indexBuffer)
		), Engine::Utils::ErrorType::ResourceCreation, "Failed to create Gizmo index buffer");

		// インデックスデータをコピー
		m_indexBuffer->Map(0, &readRange, &pData);
		memcpy(pData, indices.data(), indexBufferSize);
		m_indexBuffer->Unmap(0, nullptr);

		// インデックスバッファビューの設定
		m_ibv.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
		m_ibv.Format = DXGI_FORMAT_R16_UINT;
		m_ibv.SizeInBytes = indexBufferSize;

		return {};
	}

	Engine::Utils::VoidResult Gizmo::createConstantBuffer()
	{
		auto dev = m_device->getDevice();

		const UINT cbSize = (sizeof(GizmoConstants) + 255) & ~255;
		auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);

		CHECK_HR(dev->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_constantBuffer)),
			Engine::Utils::ErrorType::ResourceCreation, "Failed to create Gizmo constant buffer");

		D3D12_RANGE readRange{ 0, 0 };
		CHECK_HR(m_constantBuffer->Map(0, &readRange, &m_cbMapped),
			Engine::Utils::ErrorType::ResourceCreation, "Failed to map Gizmo constant buffer");

		return {};
	}
}
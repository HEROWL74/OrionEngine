#include "Gizmo.hpp"
#include "engine/ThirdParty/d3dx12.h"

namespace Editor::UI
{
	Utils::VoidResult Gizmo::initialize(Engine::Graphics::Device* device, Engine::Graphics::ShaderManager* shaderManager)
	{
		CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
		CHECK_CONDITION(shaderManager != nullptr, Utils::ErrorType::Unknown, "ShaderManager is null");

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

		Utils::log_info("Gizmo initialized successfully");
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

		Utils::log_info("Gizmo shutdown completed");
	}

	// 描画
	void Gizmo::render(ID3D12GraphicsCommandList* commandList, const Engine::Graphics::Camera& camera, Core::GameObject* targetObject)
	{
		if (!targetObject || !m_device || !m_rootSig || !m_pso)
		{
			return;
		}

		Math::Vector3 position = targetObject->getTransform()->getPosition();

		switch (m_type)
		{
		case GizmoType::None:
			break;
		case GizmoType::Translation:
			renderTranslationGizmo(commandList, camera, position);
			break;
		case GizmoType::Rotation:
			break;
		case GizmoType::Scale:
			break;
		default:
			break;
		}
	}

	// Rayとの交差判定
	GizmoAxis Gizmo::hitTest(const Math::Vector3& rayOrigin,
		const Math::Vector3& rayDirection,
		Core::GameObject* targetObject) const
	{
		if (!targetObject)
			return GizmoAxis::None;

		Math::Vector3 gizmoPos = targetObject->getTransform()->getPosition();
		float gizmoLength = 1.0f; // 軸の長さ
		float threshold = 0.1f;   // ヒット判定の閾値

		float distance = 0.0f;

		// X軸のテスト（赤）
		if (rayIntersectsAxis(rayOrigin, rayDirection,
			gizmoPos, gizmoPos + Math::Vector3(gizmoLength, 0, 0),
			threshold, distance))
		{
			return GizmoAxis::X;
		}

		// Y軸のテスト（緑）
		if (rayIntersectsAxis(rayOrigin, rayDirection,
			gizmoPos, gizmoPos + Math::Vector3(0, gizmoLength, 0),
			threshold, distance))
		{
			return GizmoAxis::Y;
		}

		// Z軸のテスト（青）
		if (rayIntersectsAxis(rayOrigin, rayDirection,
			gizmoPos, gizmoPos + Math::Vector3(0, 0, gizmoLength),
			threshold, distance))
		{
			return GizmoAxis::Z;
		}

		return GizmoAxis::None;
	}

	// Gizmoの操作
	void Gizmo::startDrag(GizmoAxis axis, const Math::Vector3& rayOrigin,
		const Math::Vector3& rayDirection)
	{
		m_isDragging = true;
		m_selectedAxis = axis;
		m_dragStartPosition = rayOrigin;

		// ドラッグ平面の法線を設定
		switch (axis)
		{
		case GizmoAxis::X:
			m_dragPlaneNormal = Math::Vector3::right();
			break;
		case GizmoAxis::Y:
			m_dragPlaneNormal = Math::Vector3::up();
			break;
		case GizmoAxis::Z:
			m_dragPlaneNormal = Math::Vector3::forward();
			break;
		default:
			m_dragPlaneNormal = Math::Vector3::up();
			break;
		}
	}

	void Gizmo::updateDrag(const Math::Vector3& rayOrigin,
		const Math::Vector3& rayDirection)
	{
		if (!m_isDragging)
			return;

		// Rayと平面の交差点を計算
		Math::Vector3 intersection;
		if (rayIntersectsPlane(rayOrigin, rayDirection,
			m_dragStartPosition, m_dragPlaneNormal, intersection))
		{
			// TODO: 交差点に基づいて移動量を計算
		}
	}

	void Gizmo::endDrag()
	{
		m_isDragging = false;
		m_selectedAxis = GizmoAxis::None;
	}

	void Gizmo::renderTranslationGizmo(ID3D12GraphicsCommandList* commandList,
		const Engine::Graphics::Camera& camera,
		const Math::Vector3& position)
	{
		if (!commandList || !m_vertexBuffer || !m_indexBuffer || !m_constantBuffer)
		{
			return;
		}

		// Gizmoのスケールを計算
		float gizmoScale = calculateGizmoScale(camera, position);

		// ワールド行列を作成
		Math::Matrix4 world = Math::Matrix4::translation(position);
		Math::Matrix4 worldViewProjection = camera.getViewProjectionMatrix() * world;

		// コンスタントバッファを更新
		if (m_cbMapped)
		{
			GizmoConstants constants;
			constants.worldViewProjection = worldViewProjection;
			constants.selectedColor = Math::Vector4(1.0f, 1.0f, 0.0f, 1.0f); // 選択時は黄色
			constants.scale = gizmoScale;

			memcpy(m_cbMapped, &constants, sizeof(GizmoConstants));
		}

		// パイプラインステートとルートシグネチャを設定
		commandList->SetPipelineState(m_pso.Get());
		commandList->SetGraphicsRootSignature(m_rootSig.Get());

		// トポロジーを設定
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

		// 頂点バッファとインデックスバッファを設定
		commandList->IASetVertexBuffers(0, 1, &m_vbv);
		commandList->IASetIndexBuffer(&m_ibv);

		// コンスタントバッファを設定
		commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());

		// 描画
		commandList->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);
	}

	// Rayと軸の交差判定
	bool Gizmo::rayIntersectsAxis(const Math::Vector3& rayOrigin,
		const Math::Vector3& rayDirection,
		const Math::Vector3& axisStart,
		const Math::Vector3& axisEnd,
		float threshold,
		float& outDistance) const
	{
		Math::Vector3 axisDir = (axisEnd - axisStart).normalized();
		Math::Vector3 toRay = rayOrigin - axisStart;

		float a = Math::Vector3::dot(rayDirection, rayDirection);
		float b = -2.0f * Math::Vector3::dot(rayDirection, axisDir);
		float c = Math::Vector3::dot(axisDir, axisDir);

		float d = Math::Vector3::dot(toRay, rayDirection);
		float e = -Math::Vector3::dot(toRay, axisDir);
		float f = Math::Vector3::dot(toRay, toRay);

		float det = a * c - b * b * 0.25f;
		if (std::abs(det) < 1e-6f)
			return false;

		float s = (c * d + b * 0.5f * e) / det;
		float t = (b * 0.5f * d + a * e) / det;

		Math::Vector3 closestOnRay = rayOrigin + rayDirection * s;
		Math::Vector3 closestOnAxis = axisStart + axisDir * t;

		float distance = Math::Vector3::distance(closestOnRay, closestOnAxis);

		if (distance < threshold && t >= 0.0f && t <= (axisEnd - axisStart).length())
		{
			outDistance = s;
			return true;
		}

		return false;
	}

	// Rayと平面の交差判定
	bool Gizmo::rayIntersectsPlane(const Math::Vector3& rayOrigin,
		const Math::Vector3& rayDirection,
		const Math::Vector3& planePoint,
		const Math::Vector3& planeNormal,
		Math::Vector3& outIntersection)
	{
		float denom = Math::Vector3::dot(planeNormal, rayDirection);

		if (std::abs(denom) < 1e-6f)
			return false; // Rayが平面に平行

		float t = Math::Vector3::dot(planePoint - rayOrigin, planeNormal) / denom;

		if (t < 0.0f)
			return false; // 交点がRayの後ろ

		outIntersection = rayOrigin + rayDirection * t;
		return true;
	}

	// Gizmoのスケール計算
	float Gizmo::calculateGizmoScale(const Engine::Graphics::Camera& camera, const Math::Vector3& position)
	{
		// カメラからGizmoまでの距離を計算
		float distance = Math::Vector3::distance(camera.getPosition(), position);

		// 距離に応じてスケールを調整（常に一定の画面サイズに見えるように）
		float baseFov = 45.0f;
		float fov = camera.getFov();
		float scale = distance * std::tan(Math::radians(fov * 0.5f)) * 0.1f;

		return scale;
	}

	Utils::VoidResult Gizmo::createRootSignature()
	{
		auto dev = m_device->getDevice();

		// ルートパラメータ: 定数バッファビュー (CBV)
		CD3DX12_ROOT_PARAMETER1 rootParams[1] = {};
		rootParams[0].InitAsConstantBufferView(
			0,  // register(b0)
			0,  // space
			D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
			D3D12_SHADER_VISIBILITY_ALL
		);

		// ルートシグネチャの作成
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
		), Utils::ErrorType::ResourceCreation, "Failed to serialize Gizmo root signature");

		CHECK_HR(dev->CreateRootSignature(
			0,
			sig->GetBufferPointer(),
			sig->GetBufferSize(),
			IID_PPV_ARGS(&m_rootSig)
		), Utils::ErrorType::ResourceCreation, "Failed to create Gizmo root signature");

		return {};
	}

	Utils::VoidResult Gizmo::createPipelineState()
	{
		auto dev = m_device->getDevice();

		// シェーダーの読み込み
		Engine::Graphics::ShaderCompileDesc vsDesc;
		vsDesc.filePath = "engine-assets/shaders/GizmoVS.hlsl";
		vsDesc.entryPoint = "main";
		vsDesc.type = Engine::Graphics::ShaderType::Vertex;
		vsDesc.enableDebug = true;

		auto vertexShaderResult = m_shaderManager->loadShader(vsDesc);
		if (!vertexShaderResult)
		{
			return std::unexpected(Utils::make_error(Utils::ErrorType::ShaderCompilation,
				"Failed to load Gizmo vertex shader"));
		}

		Engine::Graphics::ShaderCompileDesc psDesc;
		psDesc.filePath = "engine-assets/shaders/GizmoPS.hlsl";
		psDesc.entryPoint = "main";
		psDesc.type = Engine::Graphics::ShaderType::Pixel;
		psDesc.enableDebug = true;

		auto pixelShaderResult = m_shaderManager->loadShader(psDesc);
		if (!pixelShaderResult)
		{
			return std::unexpected(Utils::make_error(Utils::ErrorType::ShaderCompilation,
				"Failed to load Gizmo pixel shader"));
		}

		// 入力レイアウト
		D3D12_INPUT_ELEMENT_DESC inputLayout[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		};

		// パイプラインステート設定
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = m_rootSig.Get();
		psoDesc.VS = { vertexShaderResult->getBytecode(), vertexShaderResult->getBytecodeSize() };
		psoDesc.PS = { pixelShaderResult->getBytecode(), pixelShaderResult->getBytecodeSize() };
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.SampleMask = UINT_MAX;

		// ラスタライザ設定
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

		// デプスステンシル設定（Gizmoは常に手前に表示）
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

		psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count = 1;

		CHECK_HR(dev->CreateGraphicsPipelineState(
			&psoDesc,
			IID_PPV_ARGS(&m_pso)
		), Utils::ErrorType::ResourceCreation, "Failed to create Gizmo pipeline state");

		return {};
	}
	Utils::VoidResult Gizmo::createGeometry()
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
		), Utils::ErrorType::ResourceCreation, "Failed to create Gizmo vertex buffer");

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
		), Utils::ErrorType::ResourceCreation, "Failed to create Gizmo index buffer");

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

	Utils::VoidResult Gizmo::createConstantBuffer()
	{
		auto dev = m_device->getDevice();

		// 256バイトアライメント
		const UINT cbSize = (sizeof(GizmoConstants) + 255) & ~255;
		auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);

		CHECK_HR(dev->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_constantBuffer)
		), Utils::ErrorType::ResourceCreation, "Failed to create Gizmo constant buffer");

		// マップしたままにする
		D3D12_RANGE readRange{ 0, 0 };
		CHECK_HR(m_constantBuffer->Map(0, &readRange, &m_cbMapped),
			Utils::ErrorType::ResourceCreation, "Failed to map Gizmo constant buffer");

		return {};
	}
}
#include "Skybox.hpp"
#include <d3dcompiler.h>
#include <DirectXTex.h>
#include <format>
#include "../ThirdParty/d3dx12.h"

namespace Engine::Graphics
{
	namespace 
	{
		struct SkyboxVertex { float x, y, z; }; // 位置だけ
		struct CameraCB {
			Math::Matrix4 viewNoTrans;  // 平行移動を消したView
			Math::Matrix4 proj;
		};
		static_assert(sizeof(CameraCB) % 256 == 0 || sizeof(CameraCB) < 256, "CB must be 256-aligned-ish");
	}

	Utils::VoidResult Skybox::initialize(Device* device, ShaderManager* shaderManager)
	{
		CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
		CHECK_CONDITION(shaderManager != nullptr, Utils::ErrorType::Unknown, "ShaderManager is null");

		m_device = device;
		m_shaderManager = shaderManager;

		// キューブマップテクスチャの読み込み
		auto loadResult = loadCubeTexture(L"engine-assets/skybox/cubemap.dds");
		if (!loadResult) return loadResult;  

		// ルートシグネチャの作成
		auto rootSigResult = createRootSignature();
		if (!rootSigResult) return rootSigResult; 

		// パイプラインステートの作成
		auto psoResult = createPipelineState();
		if (!psoResult) return psoResult;

		// ジオメトリの作成
		auto geomResult = createGeometry();
		if (!geomResult) return geomResult;

		// カメラ定数バッファの作成
		auto cbResult = createCameraCB();
		if (!cbResult) return cbResult;

		Utils::log_info("Skybox initialized successfully");

		return {};
	}
	void Skybox::shutdown()
	{
		m_cameraCB.Reset();
		m_cubeTexture.Reset();
		m_ib.Reset();
		m_vb.Reset();
		m_pso.Reset();
		m_rootSig.Reset();

		m_device = nullptr;
		m_shaderManager = nullptr;

		Utils::log_info("Skybox shutdown completed");
	}

	void Skybox::render(ID3D12GraphicsCommandList* cmd, const Camera& camera)
	{
		if (!m_device || !m_rootSig || !m_pso)
		{
			return;
		}

		// カメラ定数バッファを更新
		updateCameraCB(camera);

		// ルートシグネチャとパイプラインステートを設定
		cmd->SetGraphicsRootSignature(m_rootSig.Get());
		cmd->SetPipelineState(m_pso.Get());

		// プリミティブトポロジーを設定
		cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// 頂点バッファとインデックスバッファを設定
		cmd->IASetVertexBuffers(0, 1, &m_vbv);
		cmd->IASetIndexBuffer(&m_ibv);

		// ルートパラメータを設定
		// 0: カメラ定数バッファ
		cmd->SetGraphicsRootConstantBufferView(0, m_cameraCB->GetGPUVirtualAddress());

		// 1: キューブマップテクスチャ（ディスクリプタテーブル）
		cmd->SetGraphicsRootDescriptorTable(1, m_cubeSrv);

		// 描画コマンドを発行
		cmd->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);
	}

	Utils::VoidResult Skybox::loadCubeTexture(const std::wstring& filepath)
	{
		using namespace DirectX;

		auto dev = m_device->getDevice();
		auto cmdQueue = m_device->getGraphicsQueue();
		// DDSファイルからキューブマップを読み込み
		TexMetadata metadara{};
		ScratchImage image;
		CHECK_HR(LoadFromDDSFile(filepath.c_str(), DDS_FLAGS_NONE, &metadara, image),
			Utils::ErrorType::FileI0, "Failed to load cube texture from file");

		// デバッグ情報を出力
		Utils::log_info(std::format("DDS Texture Info:"));
		Utils::log_info(std::format("  Width: {}", metadara.width));
		Utils::log_info(std::format("  Height: {}", metadara.height));
		Utils::log_info(std::format("  Depth: {}", metadara.depth));
		Utils::log_info(std::format("  ArraySize: {}", metadara.arraySize));
		Utils::log_info(std::format("  MipLevels: {}", metadara.mipLevels));
		Utils::log_info(std::format("  IsCubemap: {}", metadara.IsCubemap() ? "Yes" : "No"));
		Utils::log_info(std::format("  Format: {}", static_cast<int>(metadara.format)));
		Utils::log_info(std::format("  ImageCount: {}", image.GetImageCount()));

		// キューブマップでない場合はエラー
		CHECK_CONDITION(metadara.IsCubemap(), Utils::ErrorType::Unknown,
			"Texture is not a cubemap format");

		// テクスチャリソースの作成
		D3D12_RESOURCE_DESC texDesc = {};
		texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		texDesc.Width = metadara.width;
		texDesc.Height = static_cast<UINT>(metadara.height);
		texDesc.DepthOrArraySize = 6;
		texDesc.MipLevels = static_cast<UINT16>(metadara.mipLevels);
		texDesc.Format = metadara.format;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

		CHECK_HR(dev->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_cubeTexture)
		), Utils::ErrorType::ResourceCreation, "Failed to create cube texture resource");


		// アップロード用の中間バッファのサイズを計算
		UINT64 uploadBufferSize = GetRequiredIntermediateSize(
			m_cubeTexture.Get(),0,
			static_cast<UINT>(metadara.arraySize * metadara.mipLevels)
		);

		// アップロードバッファの作成
		ComPtr<ID3D12Resource> uploadBuffer;
		auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

		CHECK_HR(dev->CreateCommittedResource(
			&uploadHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&uploadBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&uploadBuffer)
		), Utils::ErrorType::ResourceCreation, "Failed to create upload buffer");

		// サブリソースデータの準備
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;
		for (size_t arrayIndex = 0; arrayIndex < metadara.arraySize; ++arrayIndex)
		{
			for (size_t mipIndex = 0; mipIndex < metadara.mipLevels; ++mipIndex)
			{
				const Image* img = image.GetImage(mipIndex, arrayIndex, 0);

				if (!img)
				{
					return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
						std::format("Failed to get image at mip {}, array {}", mipIndex, arrayIndex)));
				}

				D3D12_SUBRESOURCE_DATA subresource = {};
				subresource.pData = img->pixels;
				subresource.RowPitch = img->rowPitch;
				subresource.SlicePitch = img->slicePitch;
				subresources.push_back(subresource);
			}
		}
		// コマンドアロケータとコマンドリストの作成
		ComPtr<ID3D12CommandAllocator> ca;
		ComPtr<ID3D12GraphicsCommandList> cmdList;

		CHECK_HR(dev->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&ca)
		), Utils::ErrorType::ResourceCreation, "Failed to create skybox command allocator");

		CHECK_HR(dev->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			ca.Get(),
			nullptr,
			IID_PPV_ARGS(&cmdList)), Utils::ErrorType::ResourceCreation, "Failed to create skybox command list");

		// サブリソースデータをアップロード
		UpdateSubresources(
			cmdList.Get(),
			m_cubeTexture.Get(),
			uploadBuffer.Get(),
			0,
			0,
			static_cast<UINT>(subresources.size()),
			subresources.data()
		);

		//　リソースバリアでシェーダーリソースとして使用可能にする
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_cubeTexture.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);
		cmdList->ResourceBarrier(1, &barrier);

		//コマンドリストをクローズして実行
		cmdList->Close();
		ID3D12CommandList* cmdLists[] = { cmdList.Get() };

		cmdQueue->ExecuteCommandLists(1, cmdLists);

		m_device->waitForGpu();
		auto srvHandles = m_device->allocateSrvDescriptor();;
		m_cubeSrv = srvHandles.GpuHandle;


		// SRV Settings
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = metadara.format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = static_cast<UINT>(metadara.mipLevels);
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

		// Create SRV
		dev->CreateShaderResourceView(
			m_cubeTexture.Get(),
			&srvDesc,
			srvHandles.CpuHandle
		);

		Utils::log_info("Cube texture loaded successfully");
		
		return {};
	}

	Utils::VoidResult Skybox::createRootSignature()
	{
		auto dev = m_device->getDevice();

		CD3DX12_ROOT_PARAMETER1 rootParams[2] = {};

		// カメラ用の定数バッファ
		rootParams[0].InitAsConstantBufferView(
			0,
			0,
			D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
			D3D12_SHADER_VISIBILITY_VERTEX
		);

		// CubeMapTexture
		CD3DX12_DESCRIPTOR_RANGE1 srvRange{};
		srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		rootParams[1].InitAsDescriptorTable(
			1,
			&srvRange,
			D3D12_SHADER_VISIBILITY_ALL
		);

		// Static Sampler Settings
		CD3DX12_STATIC_SAMPLER_DESC samplerDesc(
			0,
			D3D12_FILTER_MIN_MAG_MIP_LINEAR,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP
		);

		// RootSignature Settings
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc{};
		rootSigDesc.Init_1_1(
			_countof(rootParams),
			rootParams,
			1,
			&samplerDesc,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		);

		// Serialize
		ComPtr<ID3DBlob> sig;
		ComPtr<ID3DBlob> err;
		CHECK_HR(D3DX12SerializeVersionedRootSignature(
			&rootSigDesc,
			D3D_ROOT_SIGNATURE_VERSION_1_1,
			&sig,
			&err
		), Utils::ErrorType::ResourceCreation, "Failed to serialize skybox root signature");
 
		CHECK_HR(dev->CreateRootSignature(
			0,
			sig->GetBufferPointer(),
			sig->GetBufferSize(),
			IID_PPV_ARGS(&m_rootSig)
		), Utils::ErrorType::ResourceCreation, "Failed to create skybox root signature");

		return {};
	}

	Utils::VoidResult Skybox::createPipelineState()
	{
		auto dev = m_device->getDevice();

		ShaderCompileDesc vsDesc;
		vsDesc.filePath = "engine-assets/shaders/SkyboxVS.hlsl";
		vsDesc.entryPoint = "main";
		vsDesc.type = ShaderType::Vertex;
		vsDesc.enableDebug = true;

		auto vertexShaderResult = m_shaderManager->loadShader(vsDesc);
		if (!vertexShaderResult)
		{
			Utils::log_warning("Failed to load vertex shader for skybox");
			return std::unexpected(Utils::make_error(Utils::ErrorType::ShaderCompilation, "Failed to load vertex shader"));
		}

		ShaderCompileDesc psDesc;
		psDesc.filePath = "engine-assets/shaders/SkyboxPS.hlsl";
		psDesc.entryPoint = "main";
		psDesc.type = ShaderType::Pixel;
		psDesc.enableDebug = true;

		auto pixelShaderResult = m_shaderManager->loadShader(psDesc);
		if (!pixelShaderResult)
		{
			Utils::log_warning("Failed to load pixel shader for skybox");
			return std::unexpected(Utils::make_error(Utils::ErrorType::ShaderCompilation, "Failed to load pixel shader"));
		}

		// Input Layout
		D3D12_INPUT_ELEMENT_DESC inputLayout[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,0,0,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
		};

		// PSO Settings
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = m_rootSig.Get();
		psoDesc.VS = { vertexShaderResult->getBytecode(), vertexShaderResult->getBytecodeSize() };
		psoDesc.PS = { pixelShaderResult->getBytecode(), pixelShaderResult->getBytecodeSize() };
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.SampleMask = UINT_MAX;

		// RasterizerState
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

		// DepthStencilState
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

		psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count = 1;

		CHECK_HR(dev->CreateGraphicsPipelineState(
			&psoDesc,
			IID_PPV_ARGS(&m_pso)
		), Utils::ErrorType::ResourceCreation, "Failed to create skybox pipeline state");

		return { };
	}

	Utils::VoidResult Skybox::createGeometry()
	{
		// 立方体の8頂点を定義
		SkyboxVertex vertices[] =
		{
			//前面
			{-1.0f,1.0f,-1.0f},
			{1.0f,1.0f,-1.0f},
			{1.0f,-1.0f,-1.0f},
			{-1.0f,-1.0f,-1.0f},

			//背面
			{-1.0f,1.0f,1.0f},
			{1.0f,1.0f,1.0f},
			{1.0f,-1.0f,1.0f},
			{-1.0f,-1.0f,1.0f}
		};

		// インデックスデータ
		UINT16 indices[] = {
			// 前面
			0, 1, 2,  0, 2, 3,
			// 右面
			1, 5, 6,  1, 6, 2,
			// 背面
			5, 4, 7,  5, 7, 6,
			// 左面
			4, 0, 3,  4, 3, 7,
			// 上面
			4, 5, 1,  4, 1, 0,
			// 下面
			3, 2, 6,  3, 6, 7
		};

		auto dev = m_device->getDevice();

		m_indexCount = _countof(indices);
		
		const UINT vertexBufferSize = sizeof(vertices);
		const UINT indexBufferSize = sizeof(indices);

		// 頂点バッファの作成
		auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto vbDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

		CHECK_HR(dev->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&vbDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vb)
		), Utils::ErrorType::ResourceCreation, "Failed to create skybox vertex buffer");

		// 頂点データをコピー
		void* pVertexDataBegin = nullptr;
		D3D12_RANGE readRange(0, 0);
		m_vb->Map(0, &readRange, &pVertexDataBegin);

		memcpy(pVertexDataBegin, vertices, vertexBufferSize);
		m_vb->Unmap(0, nullptr);

		m_vbv.BufferLocation = m_vb->GetGPUVirtualAddress();
		m_vbv.StrideInBytes = sizeof(SkyboxVertex);
		m_vbv.SizeInBytes = vertexBufferSize;

		auto ibDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
		
		CHECK_HR(dev->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&ibDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_ib)
		), Utils::ErrorType::ResourceCreation, "Failed to create skybox index buffer");

		// インデックスデータコピー
		void* pIndexDataBegin = nullptr;
		m_ib->Map(0, &readRange, &pIndexDataBegin);

		memcpy(pIndexDataBegin, indices, indexBufferSize);
		m_ib->Unmap(0, nullptr);

		// インデックスバッファビューの設定
		m_ibv.BufferLocation = m_ib->GetGPUVirtualAddress();
		m_ibv.Format = DXGI_FORMAT_R16_UINT;
		m_ibv.SizeInBytes = indexBufferSize;

		return {};

	}

	Utils::VoidResult Skybox::createCameraCB()
	{
		auto dev = m_device->getDevice();
		const UINT cbSize = (sizeof(CameraCB) + 255) & ~255;

		auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);

		CHECK_HR(dev->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_cameraCB)
		), Utils::ErrorType::ResourceCreation, "Failed to create camera constant buffer");

		return {};
	}

	void Skybox::updateCameraCB(const Camera& camera)
	{
		Math::Matrix4 view = camera.getViewMatrix();
		Math::Matrix4 viewNoTrans = view;

		// 平行移動成分を除去（4列目の上3要素をゼロに設定）
		viewNoTrans.m[0][3] = 0.0f;
		viewNoTrans.m[1][3] = 0.0f;
		viewNoTrans.m[2][3] = 0.0f;

		Math::Matrix4 proj = camera.getProjectionMatrix();

		CameraCB cbData;
		cbData.viewNoTrans = viewNoTrans;
		cbData.proj = proj;

		void* pCbDataBegin = nullptr;
		D3D12_RANGE readRange(0, 0);
		m_cameraCB->Map(0, &readRange, &pCbDataBegin);

		memcpy(pCbDataBegin, &cbData, sizeof(CameraCB));
		m_cameraCB->Unmap(0, nullptr);
	}
	
}
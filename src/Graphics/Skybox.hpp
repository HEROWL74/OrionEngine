#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <memory>
#include "../Graphics/Device.hpp"
#include "../Graphics/ShaderManager.hpp"
#include "../Math/Math.hpp"
#include "../Graphics/Camera.hpp"
#include "../Utils/Common.hpp"

namespace Engine::Graphics
{
	class Skybox
	{
	public:
		Utils::VoidResult initialize(Device* device, ShaderManager* shaderManager);
		void shutdown();

		//毎フレーム呼び出し
		void render(ID3D12GraphicsCommandList* cmd, const Camera& camera);

	private:
		//GPUリソース
		ComPtr<ID3D12RootSignature> m_rootSig;
		ComPtr<ID3D12PipelineState> m_pso;

		ComPtr<ID3D12Resource> m_vb;
		ComPtr<ID3D12Resource> m_ib;
		D3D12_VERTEX_BUFFER_VIEW m_vbv{};
		D3D12_INDEX_BUFFER_VIEW m_ibv{};
		UINT m_indexCount = 0;

		ComPtr<ID3D12Resource> m_cubeTexture;
		D3D12_GPU_DESCRIPTOR_HANDLE m_cubeSrv{};

		ComPtr<ID3D12Resource> m_cameraCB;

		//参照
		Device* m_device = nullptr;
		ShaderManager* m_shaderManager = nullptr;

		//内部関数
		Utils::VoidResult loadCubeTexture(const std::wstring& filepath);
		Utils::VoidResult createRootSignature();
		Utils::VoidResult createPipelineState();
		Utils::VoidResult createGeometry();
		Utils::VoidResult createCameraCB();
		void updateCameraCB(const Camera& camera);
 	};
}
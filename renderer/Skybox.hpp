#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <memory>
#include "Device.hpp"
#include "ShaderManager.hpp"
#include "../engine/Math/Math.hpp"
#include "../engine/World/Camera.hpp"
#include "../engine/Utils/Common.hpp"

namespace Renderer
{
	class Skybox
	{
	public:
		Utils::VoidResult initialize(Device* device, ShaderManager* shaderManager);
		void shutdown();

		//豈弱ヵ繝ｬ繝ｼ繝蜻ｼ縺ｳ蜃ｺ縺・
		void render(ID3D12GraphicsCommandList* cmd, const World::Camera& camera);

	private:
		//GPU繝ｪ繧ｽ繝ｼ繧ｹ
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
		void* m_cameraCbMapped = nullptr;

		//蜿ら・
		Device* m_device = nullptr;
		ShaderManager* m_shaderManager = nullptr;

		//蜀・Κ髢｢謨ｰ
		Utils::VoidResult loadCubeTexture(const std::wstring& filepath);
		Utils::VoidResult createRootSignature();
		Utils::VoidResult createPipelineState();
		Utils::VoidResult createGeometry();
		Utils::VoidResult createCameraCB();
		void updateCameraCB(const World::Camera& camera);
 	};
}


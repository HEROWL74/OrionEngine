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

	Utils::VoidResult Skybox::loadCubeTexture(const std::wstring& filepath)
	{
		using namespace DirectX;

		// DDSファイルからキューブマップを読み込み
		TexMetadata metadara{};
		ScratchImage image;
		CHECK_HR(LoadFromDDSFile(filepath.c_str(), DDS_FLAGS_NONE, &metadara, image),
			Utils::ErrorType::FileI0, "Failed to load cube texture from file");
		return {};
	}

}
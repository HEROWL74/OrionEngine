#include "Skybox.hpp"
#include <d3dcompiler.h>
#include <format>
#include "../ThirdParty/d3dx12.h"

namespace Engine::Graphics
{
	namespace 
	{
		struct SkyboxVertex { float x, y, z; }; // ˆÊ’u‚¾‚¯
		struct CameraCB {
			Math::Matrix4 viewNoTrans;  // •½sˆÚ“®‚ğÁ‚µ‚½View
			Math::Matrix4 proj;
		};
		static_assert(sizeof(CameraCB) % 256 == 0 || sizeof(CameraCB) < 256, "CB must be 256-aligned-ish");
	}

}
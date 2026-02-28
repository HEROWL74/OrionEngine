// src/engine/Utils/RenderContext.hpp
#pragma once
#include <d3d12.h>
#include "engine/Graphics/Camera.hpp"

namespace Engine::Utils
{
	// 繝ｬ繝ｳ繝繝ｪ繝ｳ繧ｰ繧ｳ繝ｳ繝・く繧ｹ繝亥ｮ夂ｾｩ
	enum class RenderViewType : uint8_t
	{
		Editor = 0,   // 繧ｨ繝・ぅ繧ｿ繝薙Η繝ｼ逕ｨ
		Game = 1,     // 繧ｲ繝ｼ繝繝薙Η繝ｼ逕ｨ
		Shadow = 2,   // 蟆・擂霑ｽ蜉縺吶ｋ莠亥ｮ・
		Reflection = 3,   //縲縺・▽縺玖ｿｽ蜉縺吶ｋ莠亥ｮ・
		Count
	};

	// 繝ｬ繝ｳ繝繝ｪ繝ｳ繧ｰ繧ｳ繝ｳ繝・く繧ｹ繝・
	struct RenderContext
	{
		ID3D12GraphicsCommandList* commandList = nullptr;
		const Graphics::Camera* camera = nullptr;
		RenderViewType viewType = RenderViewType::Editor;
		uint32_t frameIndex = 0;

        // 螳壽焚繝舌ャ繝輔ぃ繧､繝ｳ繝・ャ繧ｯ繧ｹ繧定ｨ育ｮ暦ｼ医ン繝･繝ｼ縺斐→縺ｫ迢ｬ遶具ｼ・
        uint32_t getConstantBufferIndex() const
        {
            return (static_cast<uint32_t>(viewType) * 2) + (frameIndex % 2);
        }

        // 繝・ヰ繝・げ逕ｨ
        const char* getViewTypeName() const
        {
            switch (viewType)
            {
            case RenderViewType::Editor: return "Editor";
            case RenderViewType::Game: return "Game";
            case RenderViewType::Shadow: return "Shadow";
            case RenderViewType::Reflection: return "Reflection";
            default: return "Unknown";
            }
        }
	};

    // 螳壽焚繝舌ャ繝輔ぃ縺ｮ蠢・ｦ∵焚繧定ｨ育ｮ・
    constexpr uint32_t GetRequiredConstantBufferCount()
    {
        // 蜷・ン繝･繝ｼ繧ｿ繧､繝励＃縺ｨ縺ｫ2縺､・医ム繝悶Ν繝舌ャ繝輔ぃ繝ｪ繝ｳ繧ｰ・・
        return static_cast<uint32_t>(RenderViewType::Count) * 2;
    }
}


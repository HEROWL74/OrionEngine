// src/engine/Utils/RenderContext.hpp
#pragma once
#include <d3d12.h>
#include "engine/Graphics/Camera.hpp"

namespace Engine::Utils
{
	// レンダリングコンテキスト定義
	enum class RenderViewType : uint8_t
	{
		Editor = 0,   // エディタビュー用
		Game = 1,     // ゲームビュー用
		Shadow = 2,   // 将来追加する予定
		Reflection = 3,   //　いつか追加する予定
		Count
	};

	// レンダリングコンテキスト
	struct RenderContext
	{
		ID3D12GraphicsCommandList* commandList = nullptr;
		const Graphics::Camera* camera = nullptr;
		RenderViewType viewType = RenderViewType::Editor;
		uint32_t frameIndex = 0;

        // 定数バッファインデックスを計算（ビューごとに独立）
        uint32_t getConstantBufferIndex() const
        {
            return (static_cast<uint32_t>(viewType) * 2) + (frameIndex % 2);
        }

        // デバッグ用
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

    // 定数バッファの必要数を計算
    constexpr uint32_t GetRequiredConstantBufferCount()
    {
        // 各ビュータイプごとに2つ（ダブルバッファリング）
        return static_cast<uint32_t>(RenderViewType::Count) * 2;
    }
}
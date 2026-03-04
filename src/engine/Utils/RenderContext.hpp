// src/engine/Utils/RenderContext.hpp
#pragma once
#include <d3d12.h>
#include "engine/Graphics/Camera.hpp"
#include "engine/Graphics/PipelineStateCache.hpp"

namespace Engine::Utils
{
	enum class RenderViewType : uint8_t
	{
		Editor = 0,
		Game = 1,
		Shadow = 2,
		Reflection = 3,
		Count
	};

	struct RenderContext
	{
		ID3D12GraphicsCommandList* commandList = nullptr;
		const Graphics::Camera* camera = nullptr;
		RenderViewType                viewType = RenderViewType::Editor;
		uint32_t                      frameIndex = 0;

		// PSOキャッシュ — Rendererは render() 時にここから借りる
		Graphics::PipelineStateCache* psoCache = nullptr;

		uint32_t getConstantBufferIndex() const
		{
			return (static_cast<uint32_t>(viewType) * 2) + (frameIndex % 2);
		}

		const char* getViewTypeName() const
		{
			switch (viewType)
			{
			case RenderViewType::Editor:     return "Editor";
			case RenderViewType::Game:       return "Game";
			case RenderViewType::Shadow:     return "Shadow";
			case RenderViewType::Reflection: return "Reflection";
			default:                         return "Unknown";
			}
		}
	};

	constexpr uint32_t GetRequiredConstantBufferCount()
	{
		return static_cast<uint32_t>(RenderViewType::Count) * 2;
	}
}
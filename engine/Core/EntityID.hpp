// src/Core/EntityID.hpp
#pragma once
#include <cstdint>
#include <functional>

namespace Engine::Core
{
	// =========================================================
	// EntityID
	// GameObjectの実体を指す軽量なハンドル
	// index: CompoenntStorage配列のインデックス
	// generation: 破棄済みEntityへのアクセスを検出する世代番号
	// ==========================================================
	struct EntityID
	{
		uint32_t index = 0;
		uint32_t generation = 0;

		bool isValid() const { return index != 0; }

		bool operator==(const EntityID& other) const
		{
			return index == other.index && generation == other.generation;
		}
		bool operator!=(const EntityID& other) const { return !(*this == other); }
	};

	constexpr EntityID INVALID_ENTITY = { 0,0 };
}

// unordered_mapのキーに使えるようにhashを定義
namespace std
{
	template<>
	struct hash<Engine::Core::EntityID>
	{
		size_t operator()(const Engine::Core::EntityID& id) const noexcept
		{
			return hash<uint64_t>()(
				(static_cast<uint64_t>(id.generation) << 32) | id.index
				);
		}
	};
}
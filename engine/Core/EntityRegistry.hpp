// engine/Core/EntityRegistry.hpp
#pragma once
#include "EntityID.hpp"
#include <vector>
#include <cassert>

namespace Engine::Core
{
	// ===========================================
	// EntityRegistry
	// EntiryIDの発行・破棄・生存確認を一元管理する
	// ===========================================
	class EntityRegistry
	{
	public:
		EntityRegistry()
		{
			// index = 0はINVALID_ENTITYとして予約
			m_generations.push_back(0);
		}

		// 新しいEntityIDを発行する
		EntityID create()
		{
			if (!m_freeList.empty())
			{
				uint32_t index = m_freeList.back();
				m_freeList.pop_back();
				return EntityID{ index, m_generations[index] };
			}

			uint32_t index = static_cast<uint32_t>(m_generations.size());
			m_generations.push_back(0);
			return EntityID{ index,0 };
		}

		// EntityIDを破棄し、インデックスを再利用可能にする
		void destroy(EntityID id)
		{
			assert(isAlive(id) && "Destroying an already-dead entity");
			m_generations[id.index]++;
			m_freeList.push_back(id.index);
		}

		// EntityIDがまだ有効かどうか確認する
		bool isAlive(EntityID id)const
		{
			if (id.index == 0 || id.index >= m_generations.size()) return false;
			return m_generations[id.index] == id.generation;
		}

		size_t aliveCount() const
		{
			// 予約分（index=0）とfreeList分を除いた数
			return m_generations.size() - 1 - m_freeList.size();
		}
	private:
		std::vector<uint32_t> m_generations; //インデックスごとの世代番号
		std::vector<uint32_t> m_freeList;   // 再利用可能なインデックス一覧
	};
}
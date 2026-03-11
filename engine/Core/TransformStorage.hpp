// src/Core/TransformStorage.hpp
#pragma once

#include "EntityID.hpp"
#include "../Math/Math.hpp"
#include <vector>
#include <unordered_map>

namespace Engine::Core
{
	// ================================================
	// Transform Storage
	// SoA :
	// positions[0], positions[1], positions[2] ... 連続
	// rotations[0], rotations[1], rotations[2] ... 連続
	// scales[0], scales[1], scales[2] ... 連続
	// -> update時にキャッシュラインを無駄なく使える
	// ================================================
	class TransformStorage
	{
	public:
		// Transformデータの参照。外部からの読み書きに使う
		struct TransformData
		{
			Math::Vector3 positions;
			Math::Vector3 rotation;
			Math::Vector3 scale;
		};

		// EntityIDに対応するTransformスロットを確保する
		void create(EntityID id)
		{
			uint32_t slot = static_cast<uint32_t>(m_positions.size());

			m_positions.push_back(Math::Vector3::zero());
			m_rotations.push_back(Math::Vector3::zero());
			m_scales.push_back(Math::Vector3::one());
			m_worldMatrices.push_back(Math::Matrix4{});
			m_dirty.push_back(true);

			m_idToSlot[id] = slot;
			m_slotToId.push_back(id);
		}

		// EntityIDに対応するスロットを解放する
		// スワップ削除0(1)を維持する
		void destroy(EntityID id)
		{
			auto it = m_idToSlot.find(id);
			if (it == m_idToSlot.end()) return;

			uint32_t slot = it->second;
			uint32_t lastSlot = static_cast<uint32_t>(m_positions.size()) - 1;

			if (slot != lastSlot)
			{
				m_positions[slot] = m_positions[lastSlot];
				m_rotations[slot] = m_rotations[lastSlot];
				m_scales[slot] = m_scales[lastSlot];
				m_worldMatrices[slot] = m_worldMatrices[lastSlot];
				m_dirty[slot] = m_dirty[lastSlot];

				EntityID movedId = m_slotToId[lastSlot];
				m_slotToId[slot] = movedId;
				m_idToSlot.erase(it);        // 削除対象をerase（itが有効なうちに）
				m_idToSlot[movedId] = slot;  // 後でリマップ（リハッシュが起きてもitはもう使わない）
			}
			else
			{
				m_idToSlot.erase(it);        // 末尾削除はシンプルにerase
			}

			m_positions.pop_back();
			m_rotations.pop_back();
			m_scales.pop_back();
			m_worldMatrices.pop_back();
			m_dirty.pop_back();
			m_slotToId.pop_back();
		}

		// ------- Setter -------

		void setPosition(EntityID id, const Math::Vector3& v)
		{
			uint32_t s = slot(id); m_positions[s] = v; m_dirty[s] = true;
		}

		void setRotation(EntityID id, const Math::Vector3& v)
		{
			uint32_t s = slot(id); m_rotations[s] = v; m_dirty[s] = true;
		}

		void setScale(EntityID id, const Math::Vector3& v)
		{
			uint32_t s = slot(id); m_scales[s] = v; m_dirty[s] = true;
		}

		void translate(EntityID id, const Math::Vector3& delta)
		{
			uint32_t s = slot(id); m_positions[s] += delta; m_dirty[s] = true;
		}

		void rotate(EntityID id, const Math::Vector3& delta)
		{
			uint32_t s = slot(id); m_rotations[s] += delta; m_dirty[s] = true;
		}

		// ------- Getter -------
		const Math::Vector3& getPosition(EntityID id) const { return m_positions[slot(id)]; }
		const Math::Vector3& getRotation(EntityID id) const { return m_rotations[slot(id)]; }
		const Math::Vector3& getScale(EntityID id) const { return m_scales[slot(id)]; }

		const Math::Matrix4& getWorldMatrix(EntityID id)
		{
			uint32_t s = slot(id);
			if (m_dirty[s])
			{
				updateWorldMatrix(s);
				m_dirty[s] = false;
			}
			return m_worldMatrices[s];
		}


		bool has(EntityID id) const { return m_idToSlot.find(id) != m_idToSlot.end(); }

		// バッチ更新（毎フレームScene側から呼ぶ）
		void flushDirty()
		{
			for (uint32_t s = 0; s < static_cast<uint32_t>(m_dirty.size()); ++s)
			{
				if (m_dirty[s])
				{
					updateWorldMatrix(s);
					m_dirty[s] = false;
				}
			}
		}

		size_t size() const { return m_positions.size(); }

		// ------- SoA配列への直接アクセス（レンダラー用） -------
		const std::vector<Math::Vector3>& positions() const { return m_positions; }
		const std::vector<Math::Vector3>& rotations() const { return m_rotations; }
		const std::vector<Math::Vector3>& scales() const { return m_scales; }
		const std::vector<Math::Matrix4>& worldMatrices() const { return m_worldMatrices; }
		const std::vector<EntityID>& slotToId() const { return m_slotToId; }
	private:
		// Soa本体
		std::vector<Math::Vector3> m_positions;
		std::vector<Math::Vector3> m_rotations;
		std::vector<Math::Vector3> m_scales;
		std::vector<Math::Matrix4> m_worldMatrices;
		std::vector<bool> m_dirty;

		// EntityID ↔ スロット番号の双方向マップ
		std::unordered_map<EntityID, uint32_t> m_idToSlot;
		std::vector<EntityID> m_slotToId;

		uint32_t slot(EntityID id) const
		{
			return m_idToSlot.at(id);
		}

		void updateWorldMatrix(uint32_t s)
		{
			const float sx = m_scales[s].x, sy = m_scales[s].y, sz = m_scales[s].z;
			const float px = m_positions[s].x, py = m_positions[s].y, pz = m_positions[s].z;

			const float rx = Math::radians(m_rotations[s].x);
			const float ry = Math::radians(m_rotations[s].y);
			const float rz = Math::radians(m_rotations[s].z);

			const float cx = std::cos(rx), ox = std::sin(rx);
			const float cy = std::cos(ry), oy = std::sin(ry);
			const float cz = std::cos(rz), oz = std::sin(rz);

			// R = Rx * Ry * Rz を展開、さらにSを右から掛けてTRSを一発で書き込む
			auto& r = m_worldMatrices[s].m;

			r[0][0] = (cy * cz) * sx;
			r[0][1] = (-cy * oz) * sx;
			r[0][2] = (oy)*sx;
			r[0][3] = px;

			r[1][0] = (ox * oy * cz + cx * oz) * sy;
			r[1][1] = (-ox * oy * oz + cx * cz) * sy;
			r[1][2] = (-ox * cy) * sy;
			r[1][3] = py;

			r[2][0] = (-cx * oy * cz + ox * oz) * sz;
			r[2][1] = (cx * oy * oz + ox * cz) * sz;
			r[2][2] = (cx * cy) * sz;
			r[2][3] = pz;

			r[3][0] = 0.0f;
			r[3][1] = 0.0f;
			r[3][2] = 0.0f;
			r[3][3] = 1.0f;
		}
	};
}
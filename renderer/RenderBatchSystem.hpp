// src/Graphics/RenderBatchSustem.hpp
#pragma once

#include "../engine/Core/EntityID.hpp"
#include "../engine/Core/TransformStorage.hpp"
#include "Device.hpp"
#include "Material.hpp"
#include "ShaderManager.hpp"
#include "CubeRenderer.hpp"
#include "RenderContext.hpp"
#include <vector>
#include <unordered_map>

namespace Renderer
{
	//レンダリング可能なオブジェクトの種類
	enum class RenderableType
	{
		Cube,
		//SphereやPlaneも追加予定
	};

	// ============================
	// RenderEntry
	// ============================
	struct RenderEntry
	{
		Core::EntityID entityId;
		RenderableType type;
		std::shared_ptr<Material> material;
		bool visible;
	};

	// ============================
	// RenderBatchSystem
	// ============================
    class RenderBatchSystem
    {
    public:
        RenderBatchSystem() = default;
        ~RenderBatchSystem() = default;

        RenderBatchSystem(const RenderBatchSystem&) = delete;
        RenderBatchSystem& operator=(const RenderBatchSystem&) = delete;

        [[nodiscard]] Utils::VoidResult initialize(Device* device, ShaderManager* shaderManager)
        {
            CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
            m_device = device;
            m_shaderManager = shaderManager;
            m_initialized = true;
            return {};
        }

        // EntityをRenderBatchSystemに登録する
        void registerEntity(Core::EntityID id, RenderableType type, std::shared_ptr<Material> material)
        {
            for (auto& entry : m_entries)
            {
                if (entry.entityId == id)
                {
                    entry.type = type;
                    entry.material = material;
                    return;
                }
            }
            m_entries.push_back({ id, type, material, true });

            // エントリに対応するレンダラーを生成
            auto renderer = std::make_unique<CubeRenderer>();
            auto result = renderer->initialize(m_device);
            if (!result)
            {
                Utils::log_error(result.error());
                m_cubeRenderers.push_back(nullptr);
            }
            else
            {
                m_cubeRenderers.push_back(std::move(renderer));
            }
        }

        void unregisterEntity(Core::EntityID id)
        {
            for (size_t i = 0; i < m_entries.size(); ++i)
            {
                if (m_entries[i].entityId == id)
                {
                    m_entries.erase(m_entries.begin() + i);
                    m_cubeRenderers.erase(m_cubeRenderers.begin() + i); // ← 同期して削除
                    return;
                }
            }
        }

        void setVisible(Core::EntityID id, bool visible)
        {
            for (auto& entry : m_entries)
                if (entry.entityId == id) { entry.visible = visible; return; }
        }

        void setMaterial(Core::EntityID id, std::shared_ptr<Material> material)
        {
            for (auto& entry : m_entries)
                if (entry.entityId == id) { entry.material = material; return; }
        }

        // =====================================================
        // renderAll
        // TransformStorageから行列を直接参照してバッチ描画する
        // =====================================================
        void renderAll(const Utils::RenderContext& context,
            const Core::TransformStorage& transforms,
            MaterialManager* materialManager = nullptr)
        {
            if (!m_initialized || !context.psoCache) return;

            Utils::log_info(std::format("renderAll: entries={}, renderers={}",
                m_entries.size(), m_cubeRenderers.size()));

            for (size_t i = 0; i < m_entries.size(); ++i)
            {
                auto& entry = m_entries[i];

                Utils::log_info(std::format("renderAll: entry[{}] id=({},{}) has={}",
                    i, entry.entityId.index, entry.entityId.generation,
                    transforms.has(entry.entityId)));

                if (!entry.visible) continue;
                if (!transforms.has(entry.entityId)) continue;

                auto mat = entry.material;
                if (!mat && materialManager)
                    mat = materialManager->getDefaultMaterial();

                auto* renderer = m_cubeRenderers[i].get();
                if (!renderer || !renderer->isValid()) continue;

                const auto& pos = transforms.getPosition(entry.entityId);
                const auto& rot = transforms.getRotation(entry.entityId);
                const auto& scale = transforms.getScale(entry.entityId);

                renderer->setPosition(pos);
                renderer->setRotation(rot);
                renderer->setScale(scale);
                renderer->setMaterial(mat);
                renderer->render(context);
            }
        }

        void clear()
        {
            m_entries.clear();
            m_cubeRenderers.clear();
        }

        bool isRegistered(Core::EntityID id) const
        {
            for (const auto& e : m_entries)
                if (e.entityId == id) return true;
            return false;
        }

        // シリアライザ用：EntityIDに対応するエントリを取得する
        const RenderEntry* findEntry(Core::EntityID id) const
        {
            for (const auto& e : m_entries)
                if (e.entityId == id) return &e;
            return nullptr;
        }

    private:
        Device* m_device = nullptr;
        ShaderManager* m_shaderManager = nullptr;
        bool           m_initialized = false;

        // 描画対象エントリ（連続配列）
        std::vector<RenderEntry> m_entries;

        // 型ごとの共有レンダラー（GameObjectごとに持たない）
        std::vector<std::unique_ptr<CubeRenderer>> m_cubeRenderers;
    };
}
// engine/Core/ComponentBatchSystem.hpp
#pragma once

#include "EntityID.hpp"
#include <vector>
#include <memory>
#include <unordered_map>
#include <typeindex>
#include <functional>
#include <cassert>

namespace Engine::Core
{
    struct IComponentPool
    {
        virtual ~IComponentPool() = default;
        virtual void startAll() = 0;
        virtual void updateAll(float deltaTime) = 0;
        virtual void lateUpdateAll(float deltaTime) = 0;
        virtual void destroyEntity(EntityID id) = 0;
        virtual bool has(EntityID id) const = 0;
    };

    // =========================================================
    // ComponentPool<T>
    //
    // T はコピー不可・ムーブ不可でも動作するよう
    // unique_ptr<T> で保持する。
    // スワップ削除はポインタのswapなのでO(1)かつ安全。
    // =========================================================
    template<typename T>
    class ComponentPool : public IComponentPool
    {
    public:
        template<typename... Args>
        T* add(EntityID id, Args&&... args)
        {
            // assertの代わりにログを出してリターン（Releaseでも検出できる）
            if (has(id))
            {
                // どの型が2重登録されているか分かる
                Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
                    std::format("ComponentPool::add duplicate! type={} id=({},{})",
                        typeid(T).name(), id.index, id.generation)));
                return get(id);  // クラッシュを防いで既存を返す
            }

            uint32_t slot = static_cast<uint32_t>(m_components.size());
            m_components.push_back(std::make_unique<T>(std::forward<Args>(args)...));
            m_entityIds.push_back(id);
            m_idToSlot[id] = slot;

            return m_components.back().get();
        }

        // スワップ削除 O(1) — unique_ptrのswapなのでコピー不可型も問題なし
        void destroyEntity(EntityID id) override
        {
            auto it = m_idToSlot.find(id);
            if (it == m_idToSlot.end()) return;

            uint32_t slot = it->second;
            uint32_t lastSlot = static_cast<uint32_t>(m_components.size()) - 1;

            if (slot != lastSlot)
            {
                std::swap(m_components[slot], m_components[lastSlot]);
                m_entityIds[slot] = m_entityIds[lastSlot];
                // スワップ前に erase してからリマップ（イテレータ無効化を回避）
                m_idToSlot.erase(it);
                m_idToSlot[m_entityIds[slot]] = slot;
            }
            else
            {
                m_idToSlot.erase(it);
            }

            m_components.pop_back();
            m_entityIds.pop_back();
        }
        bool has(EntityID id) const override
        {
            return m_idToSlot.find(id) != m_idToSlot.end();
        }

        T* get(EntityID id)
        {
            auto it = m_idToSlot.find(id);
            return it != m_idToSlot.end() ? m_components[it->second].get() : nullptr;
        }

        const T* get(EntityID id) const
        {
            auto it = m_idToSlot.find(id);
            return it != m_idToSlot.end() ? m_components[it->second].get() : nullptr;
        }

        void startAll() override
        {
            for (auto& c : m_components) c->start();
        }

        void updateAll(float deltaTime) override
        {
            for (auto& c : m_components)
                if (c->isEnabled()) c->update(deltaTime);
        }

        void lateUpdateAll(float deltaTime) override
        {
            for (auto& c : m_components)
                if (c->isEnabled()) c->lateUpdate(deltaTime);
        }

        void forEach(std::function<void(EntityID, T&)> fn)
        {
            for (uint32_t i = 0; i < m_components.size(); ++i)
                fn(m_entityIds[i], *m_components[i]);
        }

        void forEach(std::function<void(EntityID, const T&)> fn) const
        {
            for (uint32_t i = 0; i < m_components.size(); ++i)
                fn(m_entityIds[i], *m_components[i]);
        }

        size_t size() const { return m_components.size(); }

    private:
        std::vector<std::unique_ptr<T>> m_components;  // コピー不可型も保持できる
        std::vector<EntityID>           m_entityIds;
        std::unordered_map<EntityID, uint32_t> m_idToSlot;
    };

    // =========================================================
    // ComponentBatchSystem
    // =========================================================
    class ComponentBatchSystem
    {
    public:
        template<typename T, typename... Args>
        T* add(EntityID id, Args&&... args)
        {
            return getOrCreatePool<T>()->add(id, std::forward<Args>(args)...);
        }

        template<typename T>
        T* get(EntityID id)
        {
            auto* pool = findPool<T>();
            return pool ? pool->get(id) : nullptr;
        }

        template<typename T>
        const T* get(EntityID id) const
        {
            auto* pool = findPool<T>();
            return pool ? pool->get(id) : nullptr;
        }

        template<typename T>
        bool has(EntityID id) const
        {
            auto* pool = findPool<T>();
            return pool && pool->has(id);
        }

        void destroyEntity(EntityID id)
        {
            for (auto& [type, pool] : m_pools)
                pool->destroyEntity(id);
        }

        void startAll()
        {
            for (auto& [type, pool] : m_pools)
                pool->startAll();
        }

        void updateAll(float deltaTime)
        {
            for (auto& [type, pool] : m_pools)
                pool->updateAll(deltaTime);
        }

        void lateUpdateAll(float deltaTime)
        {
            for (auto& [type, pool] : m_pools)
                pool->lateUpdateAll(deltaTime);
        }

        template<typename T>
        void forEach(std::function<void(EntityID, T&)> fn)
        {
            auto* pool = findPool<T>();
            if (pool) pool->forEach(fn);
        }

        template<typename T>
        void forEach(std::function<void(EntityID, const T&)> fn) const
        {
            auto* pool = findPool<T>();
            if (pool) pool->forEach(fn);
        }

        template<typename T>
        ComponentPool<T>* getPool()
        {
            return findPool<T>();
        }

        void clear() { m_pools.clear(); }

    private:
        std::unordered_map<std::type_index, std::unique_ptr<IComponentPool>> m_pools;

        template<typename T>
        ComponentPool<T>* getOrCreatePool()
        {
            auto key = std::type_index(typeid(T));
            auto [it, inserted] = m_pools.emplace(key, nullptr);
            if (inserted)
                it->second = std::make_unique<ComponentPool<T>>();
            return static_cast<ComponentPool<T>*>(it->second.get());
        }

        template<typename T>
        ComponentPool<T>* findPool()
        {
            auto it = m_pools.find(std::type_index(typeid(T)));
            return it != m_pools.end() ? static_cast<ComponentPool<T>*>(it->second.get()) : nullptr;
        }

        template<typename T>
        const ComponentPool<T>* findPool() const
        {
            auto it = m_pools.find(std::type_index(typeid(T)));
            return it != m_pools.end() ? static_cast<const ComponentPool<T>*>(it->second.get()) : nullptr;
        }
    };

} 
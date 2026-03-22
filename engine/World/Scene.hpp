// engine/World/Scene.hpp
#pragma once
#include "../Core/GameObject.hpp"
#include "../Core/EntityID.hpp"
#include "../Core/TransformStorage.hpp"
#include "../Core/ComponentBatchSystem.hpp"
#include "../renderer/Device.hpp"
#include "Camera.hpp"
#include "../renderer/Material.hpp"
#include "../renderer/RenderBatchSystem.hpp"
#include "../renderer/ShaderManager.hpp"
#include "../UI/UIComponent.hpp"
#include "../renderer/RenderContext.hpp"

namespace Engine::World
{
    using namespace Renderer;
    class Scene
    {
    public:
        Scene() = default;
        ~Scene() = default;

        Scene(const Scene&) = delete;
        Scene& operator=(const Scene&) = delete;
        Scene(Scene&&) = delete;
        Scene& operator=(Scene&&) = delete;

        [[nodiscard]] Utils::VoidResult initialize(Device* device, ShaderManager* shaderManager,
            MaterialManager* materialManager = nullptr);

        // ---- GameObject管理 ----
        Core::GameObject* createGameObject(const std::string& name = "GameObject");
        void destroyGameObject(Core::GameObject* gameObject);
        Core::GameObject* findGameObject(const std::string& name) const;
        Core::GameObject* findGameObjectById(Core::EntityID id) const;
        Core::GameObject* findObjectByName(const std::string& name);
        const std::vector<std::unique_ptr<Core::GameObject>>& getGameObjects() const { return m_gameObjects; }

        // ---- 選択 ----
        void setSelectedObject(Core::GameObject* object);
        Core::GameObject* getSelectedObject() const;
        void clearSelection();

        // ---- ライフサイクル ----
        void start();
        void update(float deltaTime);
        void lateUpdate(float deltaTime);
        void clear();
        void processPendingDestroy();

        // ---- コンポーネント登録（旧: gameObject->addComponent<T>()の代替） ----
        // Transformを持たないコンポーネントはこちらで登録する
        template<typename T, typename... Args>
        T* addComponent(Core::GameObject* obj, Args&&... args)
        {
            if (!obj) return nullptr;
            T* comp = m_componentBatch.add<T>(obj->getId(), std::forward<Args>(args)...);
            if (comp)
                comp->setGameObject(obj);
            return comp;
        }

        template<typename T>
        T* getComponent(Core::GameObject* obj)
        {
            if (!obj) return nullptr;
            return m_componentBatch.get<T>(obj->getId());
        }

        template<typename T>
        bool hasComponent(Core::GameObject* obj) const
        {
            if (!obj) return false;
            return m_componentBatch.has<T>(obj->getId());
        }

        template<typename T>
        void removeComponent(Core::GameObject* obj)
        {
            if (!obj) return;
            auto* pool = m_componentBatch.getPool<T>();
            if (pool) pool->destroyEntity(obj->getId());
        }

        // ComponentBatchSystemへの直接アクセス（型ごとのバッチ処理用）
        Core::ComponentBatchSystem& getComponentBatch() { return m_componentBatch; }
        const Core::ComponentBatchSystem& getComponentBatch() const { return m_componentBatch; }

        // ---- レンダリング ----
        void renderBatch(const Utils::RenderContext& context)
        {
            if (!m_initialized) return;
            m_renderBatch.renderAll(context, m_transformStorage, m_materialManager);
        }

        void registerRenderable(
            Core::GameObject* obj,
            RenderableType            type = RenderableType::Cube,
            std::shared_ptr<Material> material = nullptr)
        {
            if (obj) m_renderBatch.registerEntity(obj->getId(), type, material);
        }

        void unregisterRenderable(Core::GameObject* obj)
        {
            if (obj) m_renderBatch.unregisterEntity(obj->getId());
        }

        void setRenderableVisible(Core::GameObject* obj, bool visible)
        {
            if (obj) m_renderBatch.setVisible(obj->getId(), visible);
        }

        void setRenderableMaterial(Core::GameObject* obj, std::shared_ptr<Material> mat)
        {
            if (obj) m_renderBatch.setMaterial(obj->getId(), mat);
        }

        // RenderBatchSystemへの直接アクセス（シリアライザ用）
        const RenderBatchSystem& getRenderBatch() const { return m_renderBatch; }

        // ---- TransformStorage直接アクセス（物理・アニメーション用） ----
        Core::TransformStorage& getTransformStorage() { return m_transformStorage; }
        const Core::TransformStorage& getTransformStorage() const { return m_transformStorage; }

        // ---- UI ----
        EngineUI::UIText* createUIText(const std::string& name = "UIText")
        {
            auto* go = createGameObject(name);
            auto* uitext = addComponent<EngineUI::UIText>(go);
            uitext->setName(name);
            return uitext;
        }

        std::vector<EngineUI::UIText*> getUITexts()
        {
            std::vector<EngineUI::UIText*> texts;
            m_componentBatch.forEach<EngineUI::UIText>(
                [&](Core::EntityID, EngineUI::UIText& text) {
                    texts.push_back(&text);
                });
            return texts;
        }

        std::vector<const EngineUI::UIText*> getUITexts() const
        {
            std::vector<const EngineUI::UIText*> texts;
            m_componentBatch.forEach<EngineUI::UIText>(
                [&](Core::EntityID, const EngineUI::UIText& text) {
                    texts.push_back(&text);
                });
            return texts;
        }

        void clearUITexts() { m_uiTexts.clear(); }

    private:
        Device* m_device = nullptr;
        ShaderManager* m_shaderManager = nullptr;
        MaterialManager* m_materialManager = nullptr;
        bool             m_initialized = false;

        Core::TransformStorage    m_transformStorage;   // Transformデータ（SoA）
        Core::ComponentBatchSystem m_componentBatch;    // 型ごとに連続配列で管理
        RenderBatchSystem         m_renderBatch;        // バッチ描画

        std::vector<std::unique_ptr<Core::GameObject>> m_gameObjects;
        std::vector<Core::GameObject*>                 m_pendingDestroy;
        Core::GameObject* m_selectedObject = nullptr;
        std::vector<std::unique_ptr<Engine::EngineUI::UIText>> m_uiTexts;
    };
}
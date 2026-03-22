// engine/World/Scene.cpp
#include "Scene.hpp"
#include "../Physics/PhysicsSystem.hpp"
#include <chrono>

namespace Engine::World
{
    Utils::VoidResult Scene::initialize(Device* device, ShaderManager* shaderManager,
        MaterialManager* materialManager)
    {
        CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
        CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");

        m_device = device;
        m_shaderManager = shaderManager;
        m_materialManager = materialManager;

        auto result = m_renderBatch.initialize(device, shaderManager);
        if (!result) return result;

        m_initialized = true;
        return {};
    }

    Core::GameObject* Scene::createGameObject(const std::string& name)
    {
        auto gameObject = std::make_unique<Core::GameObject>(name);
        Core::GameObject* ptr = gameObject.get();

        // TransformStorageにスロット確保 → Transformプロキシにバインド
        m_transformStorage.create(ptr->getId());
        ptr->bindTransformStorage(&m_transformStorage);

        m_gameObjects.push_back(std::move(gameObject));
        return ptr;
    }

    void Scene::destroyGameObject(Core::GameObject* gameObject)
    {
        if (!gameObject || gameObject->isDestroyed()) return;

        if (m_selectedObject == gameObject)
            m_selectedObject = nullptr;

        gameObject->destroy();
        m_pendingDestroy.push_back(gameObject);
    }

    void Scene::start()
    {
        // ComponentBatchSystemが全コンポーネントをまとめてstart
        m_componentBatch.startAll();
    }

    void Scene::update(float deltaTime)
    {
        auto t0 = std::chrono::high_resolution_clock::now();
        m_componentBatch.updateAll(deltaTime);
        auto t1 = std::chrono::high_resolution_clock::now();

        m_transformStorage.flushDirty();
        auto t2 = std::chrono::high_resolution_clock::now();

        Physics::PhysicsSystem::get().update(*this);
        auto t3 = std::chrono::high_resolution_clock::now();

        auto ms = [](auto a, auto b) {
            return std::chrono::duration<double, std::milli>(b - a).count();
            };

        static int counter = 0;
        if (counter++ % 60 == 0)
            Utils::log_info(std::format(
                "  componentBatch={:.3f}ms  flushDirty={:.3f}ms  physics={:.3f}ms",
                ms(t0, t1), ms(t1, t2), ms(t2, t3)));

        processPendingDestroy();
    }

    void Scene::lateUpdate(float deltaTime)
    {
        m_componentBatch.lateUpdateAll(deltaTime);
    }

    void Scene::clear()
    {
        Utils::log_info("Scene::clear() called");

        if (m_device)
        {
            Utils::log_info("Waiting for GPU before clearing scene...");
            m_device->waitForGpu();
        }

        Physics::PhysicsSystem::get().clear();

        m_selectedObject = nullptr;

        m_componentBatch.clear();
        m_renderBatch.clear();

        m_gameObjects.clear();
        m_pendingDestroy.clear();

        Utils::log_info("Scene cleared completely");
    }

    Core::GameObject* Scene::findGameObject(const std::string& name) const
    {
        for (const auto& obj : m_gameObjects)
            if (obj && obj->getName() == name && !obj->isDestroyed())
                return obj.get();
        return nullptr;
    }

    Core::GameObject* Scene::findObjectByName(const std::string& name)
    {
        return findGameObject(name);
    }

    Core::GameObject* Scene::findGameObjectById(Core::EntityID id) const
    {
        for (const auto& obj : m_gameObjects)
            if (obj && !obj->isDestroyed() && obj->getId() == id)
                return obj.get();
        return nullptr;
    }
    void Scene::processPendingDestroy()
    {
        if (m_pendingDestroy.empty()) return;
        if (m_device) m_device->waitForGpu();

        for (auto* gameObject : m_pendingDestroy)
        {
            if (!gameObject) continue;

            if (m_selectedObject == gameObject)
                m_selectedObject = nullptr;

            // ここでm_nameを読む前にログ
            Utils::log_info("processPendingDestroy: before getId");
            Core::EntityID id = gameObject->getId();
            Utils::log_info(std::format("processPendingDestroy: id=({},{})", id.index, id.generation));

            auto it = std::find_if(m_gameObjects.begin(), m_gameObjects.end(),
                [gameObject](const auto& obj) { return obj.get() == gameObject; });

            if (it != m_gameObjects.end())
            {
                Utils::log_info("processPendingDestroy: before transformStorage.destroy");
                m_transformStorage.destroy(id);
                Utils::log_info("processPendingDestroy: before componentBatch.destroyEntity");
                m_componentBatch.destroyEntity(id);
                Utils::log_info("processPendingDestroy: before renderBatch.unregisterEntity");
                m_renderBatch.unregisterEntity(id);
                Utils::log_info("processPendingDestroy: before gameObjects.erase");
                m_gameObjects.erase(it);
                Utils::log_info("processPendingDestroy: done");
            }
            else
            {
                Utils::log_info("processPendingDestroy: gameObject not found in m_gameObjects!");
            }
        }
        m_pendingDestroy.clear();
    }
    void Scene::setSelectedObject(Core::GameObject* object)
    {
        if (object && object->isDestroyed()) return;
        m_selectedObject = object;
    }

    Core::GameObject* Scene::getSelectedObject() const
    {
        if (m_selectedObject && m_selectedObject->isDestroyed())
            return nullptr;
        return m_selectedObject;
    }

    void Scene::clearSelection()
    {
        m_selectedObject = nullptr;
    }
}
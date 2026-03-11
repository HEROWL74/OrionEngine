// Core/PlayModeController.cpp
#include "PlayModeController.hpp"
#include "../engine/Core/GameObject.hpp"
#include "../engine/Scripting/LuaScriptComponent.hpp"
#include "../engine/Scripting/ScriptManager.hpp"
#include "../engine/Audio/AudioComponent.hpp"
#include "../engine/World/ActiveScene.hpp"

namespace Editor::EditorCore
{
    void PlayModeController::initialize(Engine::World::Scene* scene)
    {
        m_scene = scene;
        Engine::World::setActiveScene(scene);
        Engine::Utils::log_info("PlayModeController initialized");
        m_initialized = true;
    }

    void PlayModeController::play()
    {
        if (!m_scene)
        {
            Engine::Utils::log_warning("PlayModeController::play() called with null scene");
            return;
        }
        if (m_currentState == EditorState::Playing)
        {
            Engine::Utils::log_warning("Already in Play mode");
            return;
        }

        EditorState oldState = m_currentState;

        if (m_currentState == EditorState::Edit)
        {
            Engine::Utils::log_info("Entering Play mode...");

            // ActiveSceneを確実に設定
            Engine::World::setActiveScene(m_scene);
            Engine::Utils::log_info(std::format("ActiveScene set to: {}", (void*)m_scene));

            // シーンの状態を保存（新しいGameObjectsが作られた後）
            captureSceneState();

            m_scene->start();
        }
        else if (m_currentState == EditorState::Paused)
        {
            Engine::Utils::log_info("Resuming from pause...");
        }

        m_currentState = EditorState::Playing;
        notifyStateChanged(oldState, m_currentState);
    }

    void PlayModeController::pause()
    {
        if (m_currentState != EditorState::Playing)
        {
            Engine::Utils::log_warning("Can only pause when playing");
            return;
        }

        EditorState oldState = m_currentState;
        m_currentState = EditorState::Paused;
        notifyStateChanged(oldState, m_currentState);
        Engine::Utils::log_info("Game paused");
    }

    void PlayModeController::stop()
    {
        if (m_currentState == EditorState::Edit)
        {
            Engine::Utils::log_warning("Already in Edit mode");
            return;
        }

        Engine::Utils::log_info("Stopping Play mode...");
        EditorState oldState = m_currentState;

        // すべてのAudioComponentを停止
        if (m_scene)
        {
            for (auto& obj : m_scene->getGameObjects())
            {
                if (obj)
                {
                    auto* audioComp = obj->getComponent<Engine::Audio::AudioComponent>();
                    if (audioComp)
                    {
                        audioComp->stop();
                    }
                }
            }
        }

        // Lua VMリセットのみ実行
        Engine::Utils::log_info("Reloading Lua VM...");
        Engine::Scripting::ScriptManager::get().reloadAll();

        // シーンの状態を復元
        restoreSceneState();

        m_currentState = EditorState::Edit;
        notifyStateChanged(oldState, m_currentState);
    }

    void PlayModeController::step() const
    {
        if (m_currentState != EditorState::Paused)
        {
            Engine::Utils::log_warning("Step is only available in Pause mode");
            return;
        }

        float fixedDeltaTime = 1.0f / 60.0f; // 60FPS想定

        Engine::Utils::log_info("Stepped one frame");
    }

    void PlayModeController::addStateChangedCallback(EditorStateChangedCallback callback)
    {
        m_stateChangedCallbacks.push_back(callback);
    }

    void PlayModeController::notifyStateChanged(EditorState oldState, EditorState newState)
    {
        for (const auto& callback : m_stateChangedCallbacks)
        {
            callback(oldState, newState);
        }
    }

    void PlayModeController::captureSceneState()
    {
        if (!m_scene) return;

        Engine::Utils::log_info("Capturing scene state...");
        m_sceneSnapshots.clear();

        for (auto& obj : m_scene->getGameObjects())
        {
            if (!obj) continue;

            auto snapshot = std::make_unique<GameObjectSnapshot>();
            snapshot->name = obj->getName();
            snapshot->active = obj->isActive();

            auto* t = obj->getTransform();
            if (t)
            {
                snapshot->position = t->getPosition();
                snapshot->rotation = t->getRotation();
                snapshot->scale = t->getScale();
            }

            m_sceneSnapshots.push_back(std::move(snapshot));
        }

        Engine::Utils::log_info(std::format("Captured {} object states", m_sceneSnapshots.size()));
    }

    void PlayModeController::restart()
    {
        Engine::Utils::log_info("Restart requested - scheduling for next frame");
        m_pendingRestart = true;
    }

    void PlayModeController::update()
    {
        // 遅延実行されたリスタート処理を実行
        if (m_pendingRestart)
        {
            m_pendingRestart = false;
            performRestart();
        }
    }

    void PlayModeController::performRestart()
    {
        Engine::Utils::log_info(
            std::format("performRestart() executing. isPlaying={}", isPlaying())
        );
        Engine::Utils::log_info(std::format(
            "Restart context: device={}, shader={}, material={}, texture={}, path={}",
            (void*)m_device,
            (void*)m_shaderManager,
            (void*)m_materialManager,
            (void*)m_textureManager,
            m_scenePath
        ));

        if (!isPlaying())
            return;

        if (!m_scene)
        {
            Engine::Utils::log_error(Utils::make_error(
                Utils::ErrorType::Unknown,
                "performRestart(): m_scene is null"
            ));
            return;
        }

        // リスタート処理中フラグをON（レンダリングをスキップさせる）
        m_isRestarting = true;

        Engine::Utils::log_info("Restarting game...");

        EditorState oldState = m_currentState;
        m_currentState = EditorState::Edit;
        notifyStateChanged(oldState, m_currentState);

        Engine::Utils::log_info("Stopping all audio components...");
        for (auto& obj : m_scene->getGameObjects())
        {
            if (obj)
            {
                auto* audioComp = obj->getComponent<Engine::Audio::AudioComponent>();
                if (audioComp)
                {
                    audioComp->stop();
                }
            }
        }

        // GPU同期を確実に行う
        Engine::Utils::log_info("Waiting for GPU...");
        if (m_device)
        {
            m_device->waitForGpu();
        }

        Engine::Utils::log_info("Clearing all snapshots...");
        m_sceneSnapshots.clear();

        Engine::Utils::log_info("Clearing scene...");
        m_scene->clear();

        // もう一度GPU同期
        if (m_device)
        {
            m_device->waitForGpu();
        }

        Engine::Utils::log_info("Reloading Lua VM...");
        Engine::Scripting::ScriptManager::get().reloadAll();

        Engine::Utils::log_info(std::format("Reloading scene from: {}", m_scenePath));
        Engine::World::SceneSerializer serializer;
        auto result = serializer.loadScene(
            *m_scene,
            m_device,
            m_shaderManager,
            m_materialManager,
            m_textureManager,
            m_scenePath
        );

        if (!result)
        {
            Engine::Utils::log_error(result.error());
            m_isRestarting = false;
            return;
        }

        Engine::Utils::log_info("Scene reloaded successfully");

        Engine::World::setActiveScene(m_scene);
        Engine::Utils::log_info(std::format("ActiveScene reset to: {}", (void*)m_scene));

        // リスタート処理完了
        m_isRestarting = false;

        play();
    }

    void PlayModeController::setSceneLoadContext(
        Renderer::Device* device,
        Renderer::ShaderManager* shaderManager,
        Renderer::MaterialManager* materialManager,
        Renderer::TextureManager* textureManager,
        const std::string& scenePath
    )
    {
        m_device = device;
        m_shaderManager = shaderManager;
        m_materialManager = materialManager;
        m_textureManager = textureManager;
        m_scenePath = scenePath;

        Engine::Utils::log_info(
            std::format("Scene load context set: {}", scenePath)
        );
    }

    void PlayModeController::restoreSceneState()
    {
        if (!m_scene) return;

        Engine::Utils::log_info("Restoring scene state...");

        auto& gameObjects = m_scene->getGameObjects();

        for (auto& snap : m_sceneSnapshots)
        {
            if (!snap) continue;

            Engine::Core::GameObject* foundObj = nullptr;
            for (auto& obj : gameObjects)
            {
                if (obj && obj->getName() == snap->name)
                {
                    foundObj = obj.get();
                    break;
                }
            }

            if (foundObj && !foundObj->isDestroyed())
            {
                foundObj->setActive(snap->active);

                auto* t = foundObj->getTransform();
                if (t)
                {
                    t->setPosition(snap->position);
                    t->setRotation(snap->rotation);
                    t->setScale(snap->scale);
                }
            }
        }

        Engine::Utils::log_info("Scene state restored");
    }
}


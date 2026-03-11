// src/Core/PlayModeController.hpp
#pragma once

#include "EditorState.hpp"
#include "../engine/Utils/Common.hpp"
#include "../engine/Math/Math.hpp"
#include "../engine/Graphics/Scene.hpp"
#include "../engine/Graphics/SceneSerializer.hpp"
#include "../engine/Graphics/ShaderManager.hpp"
#include "../engine/Graphics/Material.hpp"
#include "../engine/Graphics/Texture.hpp"
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>

namespace Engine::Graphics { class Scene; }

namespace Editor::EditorCore
{
    using namespace Engine;
    class GameObject;

    // GameObjectの状態を保存する構造体
    struct GameObjectSnapshot
    {
        std::string name;
        bool active{};

        // Transform情報
        Engine::Math::Vector3 position;
        Engine::Math::Vector3 rotation;
        Engine::Math::Vector3 scale;

        // TODO: 他のコンポーネント情報も必要に応じて追加
    };

    class PlayModeController
    {
    public:
        PlayModeController() = default;
        ~PlayModeController() = default;

        // 初期化
        void initialize(Engine::Graphics::Scene* scene);

        // Play Mode制御
        void play();
        void pause();
        void stop();
        void step() const; // 1フレーム進める（Pause中のみ）
        void restart();

        // 状態取得
        EditorState getState() const { return m_currentState; }
        bool isPlaying() const { return m_currentState == EditorState::Playing; }
        bool isPaused() const { return m_currentState == EditorState::Paused; }
        bool isInPlayMode() const { return m_currentState != EditorState::Edit; }

        // コールバック登録
        void addStateChangedCallback(EditorStateChangedCallback callback);

        void setSceneLoadContext(
            Engine::Graphics::Device* device,
            Engine::Graphics::ShaderManager* shaderManager,
            Engine::Graphics::MaterialManager* materialManager,
            Engine::Graphics::TextureManager* textureManager,
            const std::string& scenePath
        );

        // フレーム更新時に呼び出す（遅延実行を処理）
        void update();

        bool isReady() const
        {
            return m_initialized && m_scene != nullptr;
        }

        // リスタート検知
        bool isRestarting() const { return m_isRestarting; }

    private:
        EditorState m_currentState = EditorState::Edit;
        Engine::Graphics::Scene* m_scene = nullptr;

        std::vector<EditorStateChangedCallback> m_stateChangedCallbacks;

        // シーンのスナップショット
        std::vector<std::unique_ptr<GameObjectSnapshot>> m_sceneSnapshots;

        // 状態変更を通知
        void notifyStateChanged(EditorState oldState, EditorState newState);

        // スナップショット管理
        void captureSceneState();
        void restoreSceneState();

        // 実際のリスタート処理（遅延実行用）
        void performRestart();

    private:
        Engine::Graphics::Device* m_device = nullptr;
        Engine::Graphics::ShaderManager* m_shaderManager = nullptr;
        Engine::Graphics::MaterialManager* m_materialManager = nullptr;
        Engine::Graphics::TextureManager* m_textureManager = nullptr;
        Engine::Scripting::ScriptManager* m_scriptManager = nullptr;

        std::string m_scenePath;

        // 遅延実行フラグ
        bool m_pendingRestart = false;
        bool m_initialized = false;
        bool m_isRestarting = false;
    };
}


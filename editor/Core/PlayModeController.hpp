// editor/Core/PlayModeController.hpp
#pragma once

#include "../engine/Math/Math.hpp"
#include "../engine/Utils/Common.hpp"
#include "../engine/World/Scene.hpp"
#include "../engine/World/SceneSerializer.hpp"
#include "../renderer/Material.hpp"
#include "../renderer/ShaderManager.hpp"
#include "../renderer/Texture.hpp"
#include "EditorState.hpp"
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>


namespace Engine::World {
class Scene;
}

namespace Editor::EditorCore {
using namespace Engine;
class GameObject;

// GameObjectの状態を保存する構造体
struct GameObjectSnapshot {
  std::string name;
  bool active{};

  // Transform情報
  Engine::Math::Vector3 position;
  Engine::Math::Vector3 rotation;
  Engine::Math::Vector3 scale;

  // TODO: 他のコンポーネント情報も必要に応じて追加
};

class PlayModeController {
public:
  PlayModeController() = default;
  ~PlayModeController() = default;

  // 初期化
  void initialize(Engine::World::Scene *scene);

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

  // ---------------------------------------------------------------
  // プレイ開始直前に呼ばれるコールバック
  //   GameLogicProfiler のリセット・コンポーネント登録などに使う
  // ---------------------------------------------------------------
  void setOnPlayCallback(std::function<void()> callback) {
    m_onPlayCallback = std::move(callback);
  }

  void setSceneLoadContext(Renderer::Device *device,
                           Renderer::ShaderManager *shaderManager,
                           Renderer::MaterialManager *materialManager,
                           Renderer::TextureManager *textureManager,
                           const std::string &scenePath);

  // フレーム更新時に呼び出す（遅延実行を処理）
  void update();

  bool isReady() const { return m_initialized && m_scene != nullptr; }

  // リスタート検知
  bool isRestarting() const { return m_isRestarting; }

private:
  EditorState m_currentState = EditorState::Edit;
  Engine::World::Scene *m_scene = nullptr;

  // プレイ開始コールバック
  std::function<void()> m_onPlayCallback;

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
  Renderer::Device *m_device = nullptr;
  Renderer::ShaderManager *m_shaderManager = nullptr;
  Renderer::MaterialManager *m_materialManager = nullptr;
  Renderer::TextureManager *m_textureManager = nullptr;
  Engine::Scripting::ScriptManager *m_scriptManager = nullptr;

  std::string m_scenePath;

  // 遅延実行フラグ
  bool m_pendingRestart = false;
  bool m_initialized = false;
  bool m_isRestarting = false;
};
} // namespace Editor::EditorCore

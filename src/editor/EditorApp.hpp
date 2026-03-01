// src/editor/EditorApp.hpp
#pragma once

#include <Windows.h>
#include <memory>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <chrono>
#include <mutex>
#include <filesystem>
#include "../engine/Core/Window.hpp"
#include "../engine/Core/ProjectSettings.hpp"
#include "../engine/Graphics/Device.hpp"
#include "../engine/Graphics/Camera.hpp"
#include "../engine/Input/InputManager.hpp"
#include "../engine/Utils/Common.hpp"
#include "../engine/Graphics/RenderComponent.hpp"
#include "UI/imgui_internal.h"
#include "UI/ImGuiManager.hpp"
#include "../engine/Graphics/ShaderManager.hpp"
#include "UI/ProjectWindow.hpp"
#include "UI/ContextMenu.hpp"
#include "../engine/Scripting/ScriptManager.hpp"
#include "../engine/Scripting/LuaBindings.hpp"
#include "../engine/Graphics/Scene.hpp"
#include "../engine/Graphics/Skybox.hpp"
#include "../engine/Graphics/SceneSerializer.hpp"
#include "Views/GameView.hpp"
#include "Views/EditorView.hpp"
#include "editor/UI/EditorViewWindow.hpp"
#include "editor/UI/GameViewWindow.hpp"
#include "buildworker/BuildSystem.hpp"
#include "editor/UI/BuildWindow.hpp"
#include "editor/UI/ToolbarWindow.hpp"
#include "editor/Core/PlayModeController.hpp"
#include "../engine/Input/InputSystem.hpp"
#include "../engine/UI/UIComponent.hpp"
#include "../engine/UI/UITextRenderer.hpp"
#include "../engine/Graphics/ActiveScene.hpp"

using Microsoft::WRL::ComPtr;

namespace Editor
{
    class EditorApp
    {
    public:
        EditorApp() = default;
        ~EditorApp() = default;

        EditorApp(const EditorApp&) = delete;
        EditorApp& operator=(const EditorApp&) = delete;
        EditorApp(EditorApp&&) = delete;
        EditorApp& operator=(EditorApp&&) = delete;

        [[nodiscard]] Engine::Utils::VoidResult initialize(HINSTANCE hInstance, int nCmdShow,
            const std::filesystem::path& projectPath = {});
        [[nodiscard]] int run();

    private:
        // ウィンドウとデバイス管理
        Engine::Core::Window m_window;
        Engine::Graphics::Device m_device;
        Engine::Graphics::TriangleRenderer m_triangleRenderer;
        Engine::Graphics::CubeRenderer m_cubeRenderer;
        std::vector<Engine::Graphics::CubeRenderer> m_cubes;

        // カメラ
        Engine::Graphics::Camera m_editorCamera;
        Engine::Graphics::Camera m_gameCamera;
        std::unique_ptr<Engine::Graphics::FPSCameraController> m_cameraController;
        Engine::Input::InputManager m_inputManager;

        // Scene & Views
        Engine::Graphics::Scene m_scene;
        Engine::Graphics::Skybox m_skybox;
        UI::EditorView m_editorView;
        UI::GameView m_gameView;

        // Luabind
        std::unique_ptr<Engine::Scripting::LuaBindings> m_luaBindings;

        // UIText ptr
        std::unique_ptr<Engine::EngineUI::UITextRenderer> m_uiTextRenderer;
        std::vector<std::shared_ptr<Engine::EngineUI::UIText>> m_uiTexts;

        // Toolbar
        std::unique_ptr<UI::ToolbarWindow> m_toolbarWindow;
        bool m_layoutInitialized = false;
        bool m_dockNeedsRebuild = true;

        // Serializer
        Engine::Graphics::SceneSerializer m_sceneSerializer;
        std::string m_currentScenePath = "Assets/scenes/default.scene";

        // スワップチェーン関係
        ComPtr<ID3D12CommandQueue>   m_commandQueue;
        ComPtr<IDXGISwapChain3>      m_swapChain;
        ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
        ComPtr<ID3D12Resource>       m_renderTargets[2];

        // 深度バッファ関係
        ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
        ComPtr<ID3D12Resource>       m_depthStencilBuffer;

        // コマンド関係
        ComPtr<ID3D12CommandAllocator>     m_commandAllocator;
        ComPtr<ID3D12GraphicsCommandList>  m_commandList;

        // 同期用
        ComPtr<ID3D12Fence> m_fence;
        UINT64  m_fenceValue = 0;
        HANDLE  m_fenceEvent = nullptr;

        // 描画関係
        UINT       m_frameIndex = 0;
        bool       m_isResizing = false;
        std::mutex m_resizeMutex;

        // 時間管理
        std::chrono::high_resolution_clock::time_point m_lastFrameTime{};
        float m_deltaTime = 0.0f;
        float m_currentFPS = 0.0f;
        int   m_frameCount = 0;
        float m_frameTimeAccumulator = 0.0f;

        // ImGui関係
        UI::ImGuiManager m_imguiManager;
        std::unique_ptr<UI::DebugWindow>            m_debugWindow;
        std::unique_ptr<UI::SceneHierarchyWindow>   m_hierarchyWindow;
        std::unique_ptr<UI::InspectorWindow>        m_inspectorWindow;
        std::unique_ptr<UI::ProjectWindow>          m_projectWindow;
        std::unique_ptr<UI::EditorViewWindow>       m_editorViewWindow;
        std::unique_ptr<UI::GameViewWindow>         m_gameViewWindow;

        // Build関連
        std::unique_ptr<Build::BuildSystem> m_buildSystem;
        std::unique_ptr<UI::BuildWindow>    m_buildWindow;

        // マテリアル関係
        Engine::Graphics::MaterialManager             m_materialManager;
        Engine::Graphics::TextureManager              m_textureManager;
        std::unique_ptr<Engine::Graphics::ShaderManager> m_shaderManager;

        // コンテキストメニュー関係
        Engine::Core::GameObject* createPrimitiveObject(UI::PrimitiveType type, const std::string& name);
        void deleteGameObject(Engine::Core::GameObject* object);
        Engine::Core::GameObject* duplicateGameObject(Engine::Core::GameObject* original);
        void renameGameObject(Engine::Core::GameObject* object, const std::string& newName);
        Engine::Graphics::RenderableType primitiveToRenderableType(UI::PrimitiveType type);
        UI::PrimitiveType renderableToPrimitiveType(Engine::Graphics::RenderableType renderType);
        std::string generateUniqueName(const std::string& baseName);
        Engine::EngineUI::UIText* createUIElement(UI::UIElementType type, const std::string& name);
        void deleteUIText(Engine::EngineUI::UIText* text);
        void renameUIText(Engine::EngineUI::UIText* text, const std::string& newName);

        EditorCore::PlayModeController m_playModeController;

        // 初期化処理
        [[nodiscard]] Engine::Utils::VoidResult initD3D();
        [[nodiscard]] Engine::Utils::VoidResult initializeInput();
        [[nodiscard]] Engine::Utils::VoidResult createCommandQueue();
        [[nodiscard]] Engine::Utils::VoidResult createSwapChain();
        [[nodiscard]] Engine::Utils::VoidResult createRenderTargets();
        [[nodiscard]] Engine::Utils::VoidResult createDepthStencilBuffer();
        [[nodiscard]] Engine::Utils::VoidResult createCommandObjects();
        [[nodiscard]] Engine::Utils::VoidResult createSyncObjects();

        // 更新・描画処理
        void update();
        void render();

        // 時間管理
        void updateDeltaTime();
        void processInput();

        // フレーム完了待ち
        void waitForPreviousFrame();

        // リソースの破棄
        void cleanup();

        // イベントハンドラ
        void onWindowResize(int width, int height);
        void onWindowClose();

        // Scene関連
        void createInitialScene();
        void createNewScene();
        void saveScene();
        void saveSceneAs();
        void openScene();

        // Layout
        void setupFixedLayout();

        // 入力イベントハンドラ
        void onKeyPressed(Engine::Input::KeyCode key);
        void onKeyReleased(Engine::Input::KeyCode key);
        void onMouseMove(int x, int y, int deltaX, int deltaY);
        void onMouseButtonPressed(Engine::Input::MouseButton button, int x, int y);
        void onMouseButtonReleased(Engine::Input::MouseButton button, int x, int y);
    };
}


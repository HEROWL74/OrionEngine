// src/runtime/GameApp.hpp
#pragma once

#include <Windows.h>
#include <memory>
#include "../engine/Utils/Common.hpp"
#include "../engine/Core/Window.hpp"
#include "../engine/Core/ProjectSettings.hpp"
#include "../renderer/Device.hpp"
#include "../engine/World/Scene.hpp"
#include "../engine/World/Camera.hpp"
#include "../engine/World/CameraComponent.hpp"
#include "../renderer/Skybox.hpp"
#include "../renderer/ShaderManager.hpp"
#include "../engine/World/SceneSerializer.hpp"
#include "../renderer/Material.hpp"
#include "../renderer/Texture.hpp"
#include "../engine/World/SplashScreen.hpp"
#include "../engine/Scripting/ScriptManager.hpp"
#include "../engine/Scripting/LuaBindings.hpp"
#include "../engine/Input/InputSystem.hpp"
#include "../renderer/UITextRenderer.hpp"
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <chrono>

using Microsoft::WRL::ComPtr;

namespace Runtime
{
    class GameApp
    {
    public:
        Engine::Utils::VoidResult initialize(HINSTANCE hInstance, int nCmdShow,
            const std::filesystem::path& projectPath = {});
        int run();

    private:
        bool m_running = true;
        bool m_showingSplash = true;
        bool m_pendingReloadScene = false;
        bool m_isRestarting = false;

        // ウィンドウとデバイス
        Engine::Core::Window     m_window;
        Renderer::Device m_device;

        // シーンとカメラ
        // m_camera はゲームカメラ。シーンに MainCamera という名前の
        // GameObject がいればその CameraComponent から毎フレーム同期する。
        // いない場合はデフォルト値（後方互換）を使用する。
        Engine::World::Scene        m_scene;
        Engine::World::Camera       m_camera;
        Engine::Core::GameObject* m_mainCameraObject = nullptr; // MainCameraへの参照（nullable）
        Renderer::Skybox            m_skybox;
        Engine::World::SplashScreen m_splashScreen;

        // UIText
        std::unique_ptr<Engine::EngineUI::UITextRenderer> m_uiTextRenderer;

        // Luabind
        std::unique_ptr<Engine::Scripting::LuaBindings> m_luaBindings;

        // マネージャー類
        std::unique_ptr<Renderer::ShaderManager> m_shaderManager;
        Renderer::MaterialManager m_materialManager;
        Renderer::TextureManager  m_textureManager;
        Engine::World::SceneSerializer m_sceneSerializer;

        // DirectX リソース
        ComPtr<ID3D12CommandQueue>        m_commandQueue;
        ComPtr<IDXGISwapChain3>           m_swapChain;
        ComPtr<ID3D12DescriptorHeap>      m_rtvHeap;
        ComPtr<ID3D12Resource>            m_renderTargets[2];
        ComPtr<ID3D12DescriptorHeap>      m_dsvHeap;
        ComPtr<ID3D12Resource>            m_depthStencilBuffer;
        ComPtr<ID3D12CommandAllocator>    m_commandAllocator;
        ComPtr<ID3D12GraphicsCommandList> m_commandList;
        ComPtr<ID3D12Fence>               m_fence;
        UINT64 m_fenceValue = 0;
        HANDLE m_fenceEvent = nullptr;
        UINT   m_frameIndex = 0;

        // 時間管理
        std::chrono::high_resolution_clock::time_point m_lastFrameTime{};
        float m_deltaTime = 0.0f;

        // 初期化ヘルパー
        [[nodiscard]] Engine::Utils::VoidResult initD3D();
        [[nodiscard]] Engine::Utils::VoidResult createCommandQueue();
        [[nodiscard]] Engine::Utils::VoidResult createSwapChain();
        [[nodiscard]] Engine::Utils::VoidResult createRenderTargets();
        [[nodiscard]] Engine::Utils::VoidResult createDepthStencilBuffer();
        [[nodiscard]] Engine::Utils::VoidResult createCommandObjects();
        [[nodiscard]] Engine::Utils::VoidResult createSyncObjects();
        [[nodiscard]] Engine::Utils::VoidResult loadScene();

        // シーンのMainCameraをm_cameraに同期する
        void syncCameraFromScene();

        // 更新・描画
        void update();
        void render();
        void updateDeltaTime();
        void waitForPreviousFrame();
        void cleanup();
        void onWindowResize(int width, int height);
        void onWindowClose();
    };
}
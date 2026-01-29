// src/editor/EditorApp.cpp
#include "EditorApp.hpp"
#include <format>
#include <filesystem> 

namespace Editor
{
    using namespace Engine;

    Utils::VoidResult EditorApp::initialize(HINSTANCE hInstance, int nCmdShow)
    {
        Utils::log_info("Initializing Game Engine...");

        // ウィンドウの作成
        Core::WindowSettings windowSettings{
            .title = L"Orion Engine",
            .width = 1280,
            .height = 720,
            .resizable = true,
            .fullScreen = false
        };

        Utils::log_info("WindowSettings is successfully");

        auto windowResult = m_window.create(hInstance, windowSettings);
        if (!windowResult)
        {
            Utils::log_error(windowResult.error());
            return std::unexpected(windowResult.error());
        }

        Utils::log_info("Window create is successfully");

        // ウィンドウイベントのコールバックを設定
        m_window.setResizeCallback([this](int width, int height) {
            onWindowResize(width, height);
            });

        m_window.setCloseCallback([this]() {
            onWindowClose();
            });

        m_window.show(nCmdShow);

        // DirectX 12の初期化
        auto d3dResult = initD3D();
        if (!d3dResult)
        {
            Utils::log_error(d3dResult.error());
            return std::unexpected(d3dResult.error());
        }

        // 入力システムの初期化
        auto inputResult = initializeInput();
        if (!inputResult)
        {
            Utils::log_error(inputResult.error());
            return std::unexpected(inputResult.error());
        }

        Utils::log_info("Game Engine initialization completed successfully!");
        return {};
    }

    int EditorApp::run()
    {
        Utils::log_info("Starting main loop...");

        // メインループ
        while (m_window.processMessages())
        {
            update();
            render();
        }

        cleanup();

        Utils::log_info("Application terminated successfully.");
        return 0;
    }

    Engine::Utils::VoidResult EditorApp::initD3D()
    {
        Utils::log_info("Initializing DirectX 12...");

        // デバイス初期化
        Graphics::DeviceSettings deviceSettings{
            .enableDebugLayer = true,
            .enableGpuValidation = false,
            .minFeatureLevel = D3D_FEATURE_LEVEL_11_0,
            .preferHighPerformanceAdapter = true
        };

        auto deviceResult = m_device.initialize(deviceSettings);
        if (!deviceResult) return deviceResult;

        // DirectXリソース基本初期化
        auto queueResult = createCommandQueue();
        if (!queueResult) return queueResult;

        auto swapChainResult = createSwapChain();
        if (!swapChainResult) return swapChainResult;

        auto renderTargetResult = createRenderTargets();
        if (!renderTargetResult) return renderTargetResult;

        auto depthStencilResult = createDepthStencilBuffer();
        if (!depthStencilResult) return depthStencilResult;

        auto commandResult = createCommandObjects();
        if (!commandResult) return commandResult;

        auto syncResult = createSyncObjects();
        if (!syncResult) return syncResult;

        // ImGui初期化
        auto imguiResult = m_imguiManager.initialize(&m_device, m_window.getHandle(), m_commandQueue.Get());
        if (!imguiResult) return imguiResult;

        m_window.setMessageCallback(
            [&](HWND hwnd, UINT msg, WPARAM w, LPARAM l) -> bool
            {
                return m_imguiManager.handleWindowMessage(hwnd, msg, w, l);
            }
        );

        // ShaderManager初期化
        Utils::log_info("Initializing ShaderManager...");
        m_shaderManager = std::make_unique<Graphics::ShaderManager>();

        auto shaderManagerResult = m_shaderManager->initialize(&m_device);
        if (!shaderManagerResult) {
            Utils::log_error(shaderManagerResult.error());
            return shaderManagerResult;
        }

        if (!m_shaderManager || !m_shaderManager.get()) {
            Utils::log_warning("ShaderManager pointer is invalid after initialization");
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "ShaderManager is null after initialization"));
        }

        Utils::log_info("ShaderManager initialization completed successfully");

        // Skybox初期化
        Utils::log_info("Initializing Skybox...");
        auto skyboxResult = m_skybox.initialize(&m_device, m_shaderManager.get());
        if (!skyboxResult)
        {
            Utils::log_error(skyboxResult.error());
            return skyboxResult;
        }
        Utils::log_info("Skybox initialized successfully");

        // TextureManager初期化
        auto textureManagerResult = m_textureManager.initialize(&m_device);
        if (!textureManagerResult) return textureManagerResult;

        // MaterialManager初期化
        auto materialManagerResult = m_materialManager.initialize(&m_device);
        if (!materialManagerResult) return materialManagerResult;

        // Scene初期化
        auto sceneResult = m_scene.initialize(&m_device);
        if (!sceneResult)
        {
           return sceneResult;
        }

        // UITextRenderer初期化
        Utils::log_info("Initializing UITextRenderer...");
        m_uiTextRenderer = std::make_unique<Engine::EngineUI::UITextRenderer>();
        auto uiTextResult = m_uiTextRenderer->initialize(&m_device, m_shaderManager.get());
        if(!uiTextResult) 
        {
            Utils::log_error(uiTextResult.error());
            return uiTextResult;
        }
        // EditorView と GameView の初期化
        const auto [clientWidth, clientHeight] = m_window.getClientSize();

        Utils::log_info("Initializing EditorView...");
        auto editorViewResult = m_editorView.initialize(&m_device, clientWidth, clientHeight, m_shaderManager.get());
        if (!editorViewResult)
        {
            Utils::log_error(editorViewResult.error());
            return editorViewResult;
        }

        Utils::log_info("Initializing GameView...");
        auto gameViewResult = m_gameView.initialize(&m_device, clientWidth, clientHeight);
        if (!gameViewResult)
        {
            Utils::log_error(gameViewResult.error());
            return gameViewResult;
        }

        m_editorView.setSkybox(&m_skybox);
        m_editorView.setScene(&m_scene);
        m_editorView.setUITextRenderer(m_uiTextRenderer.get());
        m_gameView.setSkybox(&m_skybox);
        m_gameView.setScene(&m_scene);
        m_gameView.setUITextRenderer(m_uiTextRenderer.get());

        // GPU同期後に再度登録
        m_device.waitForGpu();

        // Lua 初期化
        auto& scriptMgr = Scripting::ScriptManager::get();
        scriptMgr.initialize();

        // バインディング登録(C++クラスをLuaに公開)
        m_luaBindings->registerBindings(scriptMgr.getLuaState());

        // 起動時にdefault.sceneがあれば読み込む、無ければ初期シーンを作成
        Utils::log_info("Checking for default scene...");
        if (std::filesystem::exists("assets/scenes/default.scene"))
        {
            Utils::log_info("Default scene found, loading...");

            auto loadResult = m_sceneSerializer.loadScene(
                m_scene,
                &m_device,
                m_shaderManager.get(),
                &m_materialManager,
                &m_textureManager,
                "assets/scenes/default.scene"
            );

            if (loadResult)
            {
                m_currentScenePath = "assets/scenes/default.scene";
                Utils::log_info(std::format("Default scene loaded successfully. Object count: {}",
                    m_scene.getGameObjects().size()));
            }
            else
            {
                Utils::log_warning("Failed to load default scene, creating initial scene");
                createInitialScene();
            }
        }
        else
        {
            Utils::log_info("No default scene found, creating initial scene");
            createInitialScene();
        }

        // ProjectWindow作成
        m_projectWindow = std::make_unique<UI::ProjectWindow>();
        m_projectWindow->setImGuiManager(&m_imguiManager);
        m_projectWindow->setTextureManager(&m_textureManager);
        m_projectWindow->setMaterialManager(&m_materialManager);
        m_projectWindow->setProjectPath("assets");

        // カメラ初期化
        m_editorCamera.setPerspective(45.0f, static_cast<float>(clientWidth) / clientHeight, 0.1f, 100.0f);
        m_editorCamera.setPosition({ 8.0f, 8.0f, 8.0f });
        m_editorCamera.lookAt({ 0.0f, 0.0f, 0.0f });

        m_gameCamera.setPerspective(45.0f, static_cast<float>(clientWidth) / clientHeight, 0.1f, 100.0f);
        m_gameCamera.setPosition({ 0.0f, 5.0f, 8.0f });
        m_gameCamera.lookAt({ 0.0f, 0.0f, 0.0f });

        m_cameraController = std::make_unique<Graphics::FPSCameraController>(&m_editorCamera);
        m_cameraController->setMovementSpeed(5.0f);
        m_cameraController->setMouseSensitivity(0.1f);

        // === UIウィンドウ作成 ===
        m_debugWindow = std::make_unique<UI::DebugWindow>();
        m_hierarchyWindow = std::make_unique<UI::SceneHierarchyWindow>();
        m_inspectorWindow = std::make_unique<UI::InspectorWindow>();
        m_toolbarWindow = std::make_unique<UI::ToolbarWindow>();

        m_toolbarWindow->setPlayModeController(&m_playModeController);

        m_editorViewWindow = std::make_unique<UI::EditorViewWindow>();

        // 重要: Window経由でInputManagerを取得
        auto* inputManager = m_window.getInputManager();
        if (!inputManager)
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
                "Failed to get InputManager from Window"));
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
                "InputManager is null"));
        }

        if (!inputManager->isInitialized())
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
                "InputManager is not initialized when creating EditorViewWindow"));
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
                "InputManager not initialized"));
        }

        Utils::log_info(std::format("Initializing EditorViewWindow with InputManager (initialized: {})",
            inputManager->isInitialized()));

        m_editorViewWindow->initialize(&m_imguiManager, &m_editorView, inputManager);
        m_editorViewWindow->setCamera(&m_editorCamera);

        m_gameViewWindow = std::make_unique<UI::GameViewWindow>();
        m_gameViewWindow->initialize(&m_imguiManager, &m_gameView);
        m_gameViewWindow->setCamera(&m_gameCamera);

        // BuildSystemとBuildWindowを初期化
        Utils::log_info("Initializing BuildSystem...");
        m_buildSystem = std::make_unique<Build::BuildSystem>();
        m_buildWindow = std::make_unique<UI::BuildWindow>();
        m_buildWindow->initialize(m_buildSystem.get());

        Utils::log_info("BuildSystem initialized successfully");
        Utils::log_info("Build output will be created in: dist/OrionGame (Release only)");

        // UIウィンドウ設定
        m_hierarchyWindow->setScene(&m_scene);
        m_inspectorWindow->setScene(&m_scene);

        // PlayModeController初期化
        m_playModeController.initialize(&m_scene);
        Utils::log_info("PlayModeController initialized with scene");
        m_debugWindow->setPlayModeController(&m_playModeController);

        m_hierarchyWindow->setSelectionChangedCallback([this](Core::GameObject* object) {
            Utils::log_info(std::format("GameObject selection changed to: {}",
                object ? object->getName() : "null"));

            // EditorViewに選択を伝える
            m_editorView.setSelectedObject(object);

            // InspectorWindowに選択を伝える
            m_inspectorWindow->setSelectedObject(object);
            });

        // コンテキストメニューのコールバック設定
        m_hierarchyWindow->setCreateObjectCallback([this](UI::PrimitiveType type, const std::string& name) -> Core::GameObject* {
            return createPrimitiveObject(type, name);
            });

        m_hierarchyWindow->setDeleteObjectCallback([this](Core::GameObject* object) {
            deleteGameObject(object);
            });

        m_hierarchyWindow->setDuplicateObjectCallback([this](Core::GameObject* object) -> Core::GameObject* {
            return duplicateGameObject(object);
            });

        m_hierarchyWindow->setRenameObjectCallback([this](Core::GameObject* object, const std::string& newName) {
            renameGameObject(object, newName);
            });

        m_hierarchyWindow->setCreateUIElementCallback([this](UI::UIElementType type, const std::string& name) -> Engine::EngineUI::UIText* {
            return createUIElement(type, name);
            });

        m_hierarchyWindow->setDeleteUITextCallback([this](Engine::EngineUI::UIText* text) {
            deleteUIText(text);
            });

        m_hierarchyWindow->setRenameUITextCallback([this](Engine::EngineUI::UIText* text, const std::string& newName) {
            renameUIText(text, newName);
            });

        // UI選択コールバック
        m_hierarchyWindow->setUISelectionChangedCallback([this](Engine::EngineUI::UIText* text) {
            Utils::log_info(std::format("UIText selection changed to: {}",
                text ? text->getName() : "null"));

            // InspectorWindowにUIText選択を伝える
            m_inspectorWindow->setSelectedUIText(text);

            // GameObjectの選択はクリア
            if (text)
            {
                m_editorView.setSelectedObject(nullptr);
            }
            });

        m_inspectorWindow->setMaterialManager(&m_materialManager);
        m_inspectorWindow->setTextureManager(&m_textureManager);

        m_scene.start();

        Utils::log_info("DirectX 12 initialization completed successfully!");
        return {};
    }

    Engine::Utils::VoidResult EditorApp::initializeInput()
    {
        Utils::log_info("Initializing input system...");

        // Window経由でInputManagerを取得
        auto* inputManager = m_window.getInputManager();
        if (!inputManager)
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
                "InputManager not available from Window"));
        }

        // InputManagerが初期化されているか確認
        if (!inputManager->isInitialized())
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
                "InputManager from Window is not initialized"));
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
                "InputManager not initialized"));
        }

        Utils::log_info(std::format("InputManager initialized: {}", inputManager->isInitialized()));

        // 初期状態では相対モードOFF
        inputManager->setRelativeMouseMode(false);
        inputManager->setMouseSensitivity(0.1f);

        // コールバック設定
        inputManager->setKeyPressedCallback([this](Input::KeyCode key) {
            onKeyPressed(key);
            });

        inputManager->setKeyReleasedCallback([this](Input::KeyCode key) {
            onKeyReleased(key);
            });

        inputManager->setMouseMoveCallback([this](int x, int y, int deltaX, int deltaY) {
            onMouseMove(x, y, deltaX, deltaY);
            });

        inputManager->setMouseButtonPressedCallback([this](Input::MouseButton button, int x, int y) {
            onMouseButtonPressed(button, x, y);
            });

        inputManager->setMouseButtonReleasedCallback([this](Input::MouseButton button, int x, int y) {
            onMouseButtonReleased(button, x, y);
            });

        Input::InputSystem::get().setInputManager(inputManager);

        Utils::log_info("Input system initialized successfully!");
        return {};
    }

    Utils::VoidResult EditorApp::createCommandQueue()
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        CHECK_HR(m_device.getDevice()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)),
            Utils::ErrorType::DeviceCreation, "Failed to create command queue");

        return {};
    }

    Utils::VoidResult EditorApp::createSwapChain()
    {
        const auto [clientWidth, clientHeight] = m_window.getClientSize();

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
        swapChainDesc.BufferCount = 2;
        swapChainDesc.Width = clientWidth;
        swapChainDesc.Height = clientHeight;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;

        ComPtr<IDXGISwapChain1> swapChain1;
        CHECK_HR(m_device.getDXGIFactory()->CreateSwapChainForHwnd(
            m_commandQueue.Get(),
            m_window.getHandle(),
            &swapChainDesc,
            nullptr, nullptr,
            &swapChain1),
            Utils::ErrorType::SwapChainCreation, "Failed to create swap chain");

        CHECK_HR(swapChain1.As(&m_swapChain),
            Utils::ErrorType::SwapChainCreation, "Failed to cast swap chain to IDXGISwapChain3");

        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

        return {};
    }

    Utils::VoidResult EditorApp::createRenderTargets()
    {
        // レンダーターゲットビュー用デスクリプタヒープの作成
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
        rtvHeapDesc.NumDescriptors = 2;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        CHECK_HR(m_device.getDevice()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)),
            Utils::ErrorType::ResourceCreation, "Failed to create RTV descriptor heap");

        // レンダーターゲットビューの作成
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        const UINT rtvDescriptorSize = m_device.getDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        for (UINT i = 0; i < 2; i++)
        {
            CHECK_HR(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])),
                Utils::ErrorType::ResourceCreation,
                std::format("Failed to get swap chain buffer {}", i));

            m_device.getDevice()->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
            rtvHandle.ptr += rtvDescriptorSize;
        }

        return {};
    }

    Utils::VoidResult EditorApp::createCommandObjects()
    {
        // コマンドアロケーターの作成
        CHECK_HR(m_device.getDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)),
            Utils::ErrorType::ResourceCreation, "Failed to create command allocator");

        // コマンドリストの作成
        CHECK_HR(m_device.getDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
            m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)),
            Utils::ErrorType::ResourceCreation, "Failed to create command list");

        // コマンドリストは作成時に記録状態になっているので、一度クローズする
        m_commandList->Close();

        return {};
    }

    //深度ステンシルバッファの作成
    Utils::VoidResult EditorApp::createDepthStencilBuffer()
    {
        const auto [clientWidth, clientHeight] = m_window.getClientSize();

        //深度ステンシルバッファ用のヒープ作成
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
        dsvHeapDesc.NumDescriptors = 1;                          //このヒープに何個のDSVを入れるか
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;       //このヒープは何用？
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;     //このヒープに特別な使い方をするか？

        CHECK_HR(m_device.getDevice()->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)),
            Utils::ErrorType::ResourceCreation, "Failed to create DSV descriptor heap");

        //深度ステンシルバッファを作成
        D3D12_CLEAR_VALUE depthOptimizedClearValue{};
        depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT; //バッファのフォーマット指定・32bitの浮動小数点形式の深度バッファ
        depthOptimizedClearValue.DepthStencil.Depth = 1.0f;      //Zバッファをどんな値でクリア（初期化）するか？」
        depthOptimizedClearValue.DepthStencil.Stencil = 0;       //ステンシル値の初期値（マスク描画などで使う番号）

        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;                   //GPUのためのメモリを使いたいときに指定
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;//特別な目的がない限りUNKNOWNに設定
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN; //特別な目的がない限りUNKNOWNに設定
        heapProps.CreationNodeMask = 1;                             //どのGPUノードでこのリソースを作るか 
        heapProps.VisibleNodeMask = 1;                              //どのノードからアクセス可能にするか

        D3D12_RESOURCE_DESC depthStencilDesc{};
        depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; //リソースの種類は 2D テクスチャ
        depthStencilDesc.Alignment = 0;                                  //アライメントは0でOK（自動で最適な値が設定される）
        depthStencilDesc.Width = clientWidth;                            //深度バッファの解像度（幅←・高さ）
        depthStencilDesc.Height = clientHeight;                          //深度バッファの解像度（幅・高さ←）
        depthStencilDesc.DepthOrArraySize = 1;                           //1枚のテクスチャ（Depth=1）
        depthStencilDesc.MipLevels = 1;                                  //ミップマップは使わない（Zバッファには不要）
        depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;                 //Zバッファの形式（32bit 浮動小数点の深度）
        depthStencilDesc.SampleDesc.Count = 1;                           //マルチサンプリング（MSAA）の設定
        depthStencilDesc.SampleDesc.Quality = 0;                         //マルチサンプリング（MSAA）の設定
        depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;          //自動レイアウトでOK
        depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;//このリソースを 「深度バッファとして使います」 と明示

        CHECK_HR(m_device.getDevice()->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &depthStencilDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthOptimizedClearValue,
            IID_PPV_ARGS(&m_depthStencilBuffer)),
            Utils::ErrorType::ResourceCreation, "Failed to create depth stencil buffer");

        //深度ステンシルビュー設定
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;

        m_device.getDevice()->CreateDepthStencilView(
            m_depthStencilBuffer.Get(),
            &dsvDesc,
            m_dsvHeap->GetCPUDescriptorHandleForHeapStart()
        );

        return {};
    }

    Utils::VoidResult EditorApp::createSyncObjects()
    {
        // 同期用フェンスの作成
        CHECK_HR(m_device.getDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)),
            Utils::ErrorType::ResourceCreation, "Failed to create fence");

        m_fenceValue = 1;

        // フェンスイベントの作成
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        CHECK_CONDITION(m_fenceEvent != nullptr, Utils::ErrorType::ResourceCreation,
            "Failed to create fence event");

        return {};
    }

    void EditorApp::update()
    {
        updateDeltaTime();

        m_editorViewWindow->processResize();
        m_gameViewWindow->processResize();

        //processInput();

        if (m_playModeController.isPlaying())
        {
            static int updateCount = 0;
            if (updateCount < 5) {
                Utils::log_info(std::format("Scene update called, frame: {}", updateCount));
                updateCount++;
            }

            Scripting::ScriptManager::get().checkForUpdates();
            m_scene.update(m_deltaTime);
            m_scene.lateUpdate(m_deltaTime);
        }

        m_debugWindow->setFPS(m_currentFPS);
        m_debugWindow->setFrameTime(m_deltaTime);
        m_debugWindow->setObjectCount(m_scene.getGameObjects().size());

        auto* triangleObject = m_scene.findGameObject("Triangle");
        if (triangleObject)
        {
            static float triangleRotation = 0.0f;
            triangleRotation += 30.0f * m_deltaTime;
            triangleObject->getTransform()->setRotation(Math::Vector3(0.0f, triangleRotation, 0.0f));
        }

        auto* inputManager = m_window.getInputManager();
        if (inputManager)
        {
            // 相対モードでない場合のみリセット
            // 相対モードの場合は processInput() でリセット済み
            if (!inputManager->getMouseState().isRelativeMode)
            {
                inputManager->resetMouseDelta();
            }
        }
    }

    void EditorApp::render()
    {
        bool isResizing = false;
        {
            std::lock_guard<std::mutex> lock(m_resizeMutex);
            isResizing = m_isResizing;
        }

        if (!m_imguiManager.isInitialized())
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "ImGuiManager not initialized"));
            return;
        }

        if (!m_renderTargets[m_frameIndex] || !m_commandList || !m_swapChain)
        {
            Utils::log_warning("Render resources not ready, skipping frame");
            return;
        }

        if (m_uiTextRenderer)
        {
            m_uiTextRenderer->beginFrame();
        }

        m_commandAllocator->Reset();
        m_commandList->Reset(m_commandAllocator.Get(), nullptr);

        m_editorView.render(m_scene, m_commandList.Get(), m_editorCamera, m_frameIndex);
        m_gameView.render(m_scene, m_commandList.Get(), m_gameCamera, m_frameIndex);

        // コマンドリストをクローズして3D描画を完了
        HRESULT hrClose = m_commandList->Close();
        if (FAILED(hrClose))
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
                std::format("Failed to close command list for 3D rendering: 0x{:08X}", static_cast<unsigned>(hrClose))));
            return;
        }

        // 3D描画コマンドを実行
        ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
        m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        // GPU同期を確実に行う
        const UINT64 fenceValue = m_fenceValue;
        m_commandQueue->Signal(m_fence.Get(), fenceValue);
        m_fenceValue++;

        if (m_fence->GetCompletedValue() < fenceValue)
        {
            m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent);
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }

        // 再度コマンドリストをリセットしてメインウィンドウの描画を開始
        m_commandAllocator->Reset();
        m_commandList->Reset(m_commandAllocator.Get(), nullptr);

        static bool firstFrame = true;
        if (firstFrame) {
            Utils::log_info(std::format("First render - EditorCamera: ({:.2f}, {:.2f}, {:.2f})",
                m_editorCamera.getPosition().x,
                m_editorCamera.getPosition().y,
                m_editorCamera.getPosition().z));
            firstFrame = false;
        }

        // メインウィンドウへの描画
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        m_commandList->ResourceBarrier(1, &barrier);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        const UINT rtvDescriptorSize = m_device.getDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        rtvHandle.ptr += m_frameIndex * rtvDescriptorSize;

        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

        m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

        const float clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
        m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        const auto [clientWidth, clientHeight] = m_window.getClientSize();
        D3D12_VIEWPORT viewport{};
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = static_cast<float>(clientWidth);
        viewport.Height = static_cast<float>(clientHeight);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;

        D3D12_RECT scissorRect{};
        scissorRect.left = 0;
        scissorRect.top = 0;
        scissorRect.right = clientWidth;
        scissorRect.bottom = clientHeight;

        m_commandList->RSSetViewports(1, &viewport);
        m_commandList->RSSetScissorRects(1, &scissorRect);

        m_imguiManager.newFrame();

        setupFixedLayout();
        m_toolbarWindow->draw();

        // メニューバー描画
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New Scene", "Ctrl+N"))
                {
                    createNewScene();
                }

                if (ImGui::MenuItem("Open Scene...", "Ctrl+O"))
                {
                    openScene();
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
                {
                    saveScene();
                }

                if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
                {
                    saveSceneAs();
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Exit", "Alt+F4"))
                {
                    PostQuitMessage(0);
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Build"))
            {
                if (ImGui::MenuItem("Build Project", "Ctrl+B"))
                {
                    m_buildWindow->show();
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Open Build Folder"))
                {
                    std::filesystem::path root = std::filesystem::current_path();
                    while (root.has_parent_path())
                    {
                        if (std::filesystem::exists(root / "CMakeLists.txt"))
                            break;
                        root = root.parent_path();
                    }

                    std::string buildPath = (root / "dist" / "OrionGame").string();
                    std::string command = "explorer \"" + buildPath + "\"";
                    system(command.c_str());
                }

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        m_editorViewWindow->draw();
        m_gameViewWindow->draw();
        m_hierarchyWindow->draw();
        m_inspectorWindow->draw();
        m_debugWindow->draw();
        m_projectWindow->draw();
        m_buildWindow->draw();
        processInput();

        if (isResizing)
        {
            ImGui::Render();
            m_commandList->Close();
            return;
        }

        m_imguiManager.render(m_commandList.Get());

        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        m_commandList->ResourceBarrier(1, &barrier);

        hrClose = m_commandList->Close();
        if (FAILED(hrClose))
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
                std::format("Failed to close command list: 0x{:08X}", static_cast<unsigned>(hrClose))));
            return;
        }

        ppCommandLists[0] = m_commandList.Get();
        m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        HRESULT hr = m_swapChain->Present(1, 0);
        if (FAILED(hr))
        {
            Utils::log_warning(std::format("Present failed: 0x{:08X}", static_cast<unsigned>(hr)));
            return;
        }

        waitForPreviousFrame();
    }

    void EditorApp::updateDeltaTime()
    {
        auto currentTime = std::chrono::high_resolution_clock::now();
        if (m_lastFrameTime.time_since_epoch().count() == 0)
        {
            m_lastFrameTime = currentTime;
        }

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - m_lastFrameTime);
        m_deltaTime = duration.count() / 1000000.0f;
        m_lastFrameTime = currentTime;

        // フレームレートの計算
        m_frameCount++;
        m_frameTimeAccumulator += m_deltaTime;
        if (m_frameTimeAccumulator >= 1.0f)
        {
            m_currentFPS = m_frameCount / m_frameTimeAccumulator;
            m_frameCount = 0;
            m_frameTimeAccumulator = 0.0f;

            // FPSをウィンドウタイトルに表示
            std::wstring title = std::format(L"Orion Engine - FPS: {:.1f}", m_currentFPS);
            m_window.setTitle(title);
        }
    }

    void EditorApp::processInput()
    {
        auto* inputManager = m_window.getInputManager();
        if (!inputManager || !m_cameraController)
        {
            return;
        }

        if (!inputManager->isInitialized())
        {
            Utils::log_warning("processInput: InputManager not initialized");
            return;
        }

        ImGuiIO& io = ImGui::GetIO();

        bool isSceneWindowFocused = m_editorViewWindow && m_editorViewWindow->isFocused();
        bool isSceneWindowHovered = m_editorViewWindow && m_editorViewWindow->isHovered();

        bool allowKeyboardInput = isSceneWindowHovered && !ImGui::IsAnyItemActive();

        if (allowKeyboardInput)
        {
            bool forward = inputManager->isKeyDown(Input::KeyCode::W);
            bool backward = inputManager->isKeyDown(Input::KeyCode::S);
            bool left = inputManager->isKeyDown(Input::KeyCode::A);
            bool right = inputManager->isKeyDown(Input::KeyCode::D);
            bool up = inputManager->isKeyDown(Input::KeyCode::E);
            bool down = inputManager->isKeyDown(Input::KeyCode::Q);

            m_cameraController->processKeyboard(forward, backward, left, right, up, down, m_deltaTime);
        }

        static bool wasCameraControlActive = false;
        bool cameraControlRequested = m_editorViewWindow && m_editorViewWindow->isCameraControlRequested();

        if (cameraControlRequested)
        {
            if (!wasCameraControlActive)
            {
                Utils::log_info("!!! ACTIVATING CAMERA CONTROL MODE !!!");
                inputManager->setRelativeMouseMode(true);
                io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
                io.WantCaptureMouse = false;
                wasCameraControlActive = true;
            }

            int deltaX = inputManager->getMouseDeltaX();
            int deltaY = inputManager->getMouseDeltaY();

            Utils::log_info(std::format(">>> Camera delta check: X={}, Y={}, RelativeMode={}",
                deltaX, deltaY, inputManager->getMouseState().isRelativeMode));

            if (deltaX != 0 || deltaY != 0)
            {
                m_cameraController->processMouseMovement(
                    static_cast<float>(deltaX),
                    static_cast<float>(deltaY)
                );
            }

            inputManager->resetMouseDelta();
        }
        else if (wasCameraControlActive)
        {
            Utils::log_info("!!! DEACTIVATING CAMERA CONTROL MODE !!!");
            inputManager->setRelativeMouseMode(false);
            io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
            inputManager->resetMouseDelta();
            wasCameraControlActive = false;
        }

        if (isSceneWindowHovered)
        {
            float wheelDelta = inputManager->getMouseWheelDelta();
            if (wheelDelta != 0.0f)
            {
                m_editorCamera.moveForward(wheelDelta * 2.0f);
            }
        }
    }

    void EditorApp::waitForPreviousFrame()
    {
        // 必要なオブジェクトが初期化されているかチェック
        if (!m_commandQueue || !m_fence || !m_fenceEvent)
        {
            Utils::log_warning("DirectX objects not initialized in waitForPreviousFrame");
            return;
        }

        const UINT64 fence = m_fenceValue;
        HRESULT hr = m_commandQueue->Signal(m_fence.Get(), fence);
        if (FAILED(hr))
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
                "Failed to signal fence", hr));
            return;
        }

        m_fenceValue++;

        if (m_fence->GetCompletedValue() < fence)
        {
            hr = m_fence->SetEventOnCompletion(fence, m_fenceEvent);
            if (FAILED(hr))
            {
                Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
                    "Failed to set fence event", hr));
                return;
            }
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }

        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    }

    void EditorApp::cleanup()
    {
        // GPUの処理完了を待つ
        waitForPreviousFrame();

        // フェンスイベントのクリーンアップ
        if (m_fenceEvent)
        {
            CloseHandle(m_fenceEvent);
            m_fenceEvent = nullptr;
        }

        Utils::log_info("DirectX 12 resources cleaned up.");
    }

    void EditorApp::onWindowResize(int width, int height)
    {
        Utils::log_info(std::format("App::onWindowResize called: {}x{}", width, height));

        if (width <= 0 || height <= 0)
        {
            Utils::log_warning(std::format("Invalid resize dimensions: {}x{}", width, height));
            return;
        }

        if (!m_commandQueue || !m_swapChain || !m_fence)
        {
            Utils::log_info("DirectX 12 not initialized yet");
            if (height > 0)
            {
                m_editorCamera.updateAspect(static_cast<float>(width) / height);
                m_gameCamera.updateAspect(static_cast<float>(width) / height);
            }
            return;
        }

        Utils::log_info("Starting safe DirectX resize process...");

        try
        {
            Utils::log_info("Complete GPU synchronization BEFORE shutdown");
            waitForPreviousFrame();

            const UINT64 flushFence = m_fenceValue;
            m_commandQueue->Signal(m_fence.Get(), flushFence);
            m_fenceValue++;

            if (m_fence->GetCompletedValue() < flushFence)
            {
                m_fence->SetEventOnCompletion(flushFence, m_fenceEvent);
                WaitForSingleObject(m_fenceEvent, INFINITE);
            }

            Utils::log_info("GPU fully synchronized");

            if (m_imguiManager.isInitialized())
            {
                Utils::log_info("Shutting down ImGui");
                m_imguiManager.shutdown();
            }

            Utils::log_info("Clearing DirectX resources");
            for (UINT i = 0; i < 2; i++)
            {
                if (m_renderTargets[i])
                {
                    m_renderTargets[i].Reset();
                }
            }

            if (m_depthStencilBuffer)
            {
                m_depthStencilBuffer.Reset();
            }

            Utils::log_info("Resizing swap chain");
            HRESULT hr = m_swapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);

            if (FAILED(hr))
            {
                Utils::log_error(Utils::make_error(Utils::ErrorType::SwapChainCreation,
                    std::format("Failed to resize swap chain: 0x{:08x}", static_cast<unsigned>(hr)), hr));
                return;
            }

            auto renderTargetResult = createRenderTargets();
            if (!renderTargetResult)
            {
                Utils::log_error(renderTargetResult.error());
                return;
            }

            auto depthStencilResult = createDepthStencilBuffer();
            if (!depthStencilResult)
            {
                Utils::log_error(depthStencilResult.error());
                return;
            }

            m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

            m_editorCamera.updateAspect(static_cast<float>(width) / height);
            m_gameCamera.updateAspect(static_cast<float>(width) / height);

            Utils::log_info("Resizing EditorView and GameView");
            m_editorView.resize(width, height);
            m_gameView.resize(width, height);

            Utils::log_info("Reinitializing ImGui");
            auto imguiResult = m_imguiManager.initialize(&m_device, m_window.getHandle(), m_commandQueue.Get());
            if (!imguiResult)
            {
                Utils::log_error(imguiResult.error());
                return;
            }

            m_window.setMessageCallback(
                [&](HWND hwnd, UINT msg, WPARAM w, LPARAM l)
                {
                    return m_imguiManager.handleWindowMessage(hwnd, msg, w, l);
                }
            );

            Utils::log_info("Re-registering ProjectWindow textures");
            if (m_projectWindow)
            {
                m_projectWindow->setImGuiManager(&m_imguiManager);
                m_projectWindow->setTextureManager(&m_textureManager);
            }

            Utils::log_info("DirectX resize completed successfully");
        }
        catch (const std::exception& e)
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
                std::format("Exception during resize: {}", e.what())));
        }
        catch (...)
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "Unknown exception during resize"));
        }
        m_dockNeedsRebuild = true;
    }

    void EditorApp::setupFixedLayout()
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGuiID dockspaceID = ImGui::GetID("MainDockSpace");

        ImGuiDockNodeFlags dockspaceFlags =
            ImGuiDockNodeFlags_NoDockingInCentralNode;;

        ImVec2 workPos = viewport->WorkPos;
        ImVec2 workSize = viewport->WorkSize;

        const float toolbarHeight = 40.0f;
        const float menuBarHeight = ImGui::GetFrameHeight();
        workPos.y += toolbarHeight + menuBarHeight;
        workSize.y -= toolbarHeight + menuBarHeight;

        ImGui::SetNextWindowPos(workPos);
        ImGui::SetNextWindowSize(workSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags hostWindowFlags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        ImGui::Begin("DockSpaceWindow", nullptr, hostWindowFlags);
        ImGui::PopStyleVar(3);

        ImGui::DockSpace(dockspaceID, ImVec2(0, 0), dockspaceFlags);

        if (!m_layoutInitialized || m_dockNeedsRebuild)
        {
            ImGui::DockBuilderRemoveNode(dockspaceID);
            ImGui::DockBuilderAddNode(
                dockspaceID,
                dockspaceFlags | ImGuiDockNodeFlags_DockSpace
            );

            ImGui::DockBuilderSetNodeSize(dockspaceID, workSize);

            ImGuiID dockMain = dockspaceID;

            ImGuiID dockBottom =
                ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down, 0.28f, nullptr, &dockMain);

            ImGuiID dockLeft =
                ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.22f, nullptr, &dockMain);

            ImGuiID dockRight =
                ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.33f, nullptr, &dockMain);

            ImGuiID dockCenter = dockMain;

            ImGui::DockBuilderDockWindow("Scene Hierarchy", dockLeft);
            ImGui::DockBuilderDockWindow("Inspector", dockRight);
            ImGui::DockBuilderDockWindow("Scene", dockCenter);
            ImGui::DockBuilderDockWindow("Game", dockCenter);
            ImGui::DockBuilderDockWindow("Project", dockBottom);
            ImGui::DockBuilderDockWindow("Debug info", dockBottom);

            ImGui::DockBuilderFinish(dockspaceID);

            m_layoutInitialized = true;
            m_dockNeedsRebuild = false;
        }

        ImGui::End();
    }

    void EditorApp::onWindowClose()
    {
        Utils::log_info("Window close requested.");
        PostQuitMessage(0);
    }


    void EditorApp::onKeyPressed(Input::KeyCode key)
    {
        // ImGuiが入力をキャプチャしている場合はスキップ
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureKeyboard)
        {
            return;
        }

        // デバッグ用：キー入力のログ出力
        if (key == Input::KeyCode::Escape)
        {
            Utils::log_info("Escape key pressed - requesting exit");
            PostQuitMessage(0);
        }
        else if (key == Input::KeyCode::F1)
        {
            // F1キーでマウス相対モードの切り替え
            auto* inputManager = m_window.getInputManager();
            if (inputManager)
            {
                bool currentMode = inputManager->getMouseState().isRelativeMode;
                inputManager->setRelativeMouseMode(!currentMode);
                Utils::log_info(std::format("Mouse relative mode: {}", !currentMode ? "ON" : "OFF"));
            }
        }
    }

    void EditorApp::onKeyReleased(Input::KeyCode key)
    {
        // 現在は何もしない
    }

    [[maybe_unused]]
    void EditorApp::onMouseMove(int x, int y, int deltaX, int deltaY)
    {
        // マウス移動の処理はprocessInput()で行う
    }

    void EditorApp::onMouseButtonPressed(Input::MouseButton button, int x, int y)
    {
        // この関数は何もしない（processInput()で処理する）
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse)
        {
            return;
        }

        // デバッグログのみ
        Utils::log_info(std::format("Mouse button {} pressed at ({}, {})",
            static_cast<int>(button), x, y));

        // 何もしない！processInput() に任せる
    }
    void EditorApp::onMouseButtonReleased(Input::MouseButton button, int x, int y)
    {
        // この関数も何もしない
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse)
        {
            return;
        }

        // デバッグログのみ
        Utils::log_info(std::format("Mouse button {} released at ({}, {})",
            static_cast<int>(button), x, y));

        // 何もしない！processInput() に任せる
    }

    Core::GameObject* EditorApp::createPrimitiveObject(UI::PrimitiveType type, const std::string& name)
    {
        // 新しいGameObjectを作成
        auto* newObject = m_scene.createGameObject(name);
        if (!newObject) return nullptr;

        // デフォルト位置設定
        Math::Vector3 cameraPos = m_gameCamera.getPosition();
        Math::Vector3 cameraForward = m_gameCamera.getForward();
        newObject->getTransform()->setPosition(cameraPos + cameraForward * 3.0f);

        // RenderComponentを追加（コンポーネントは生ポインタで作成）
        Graphics::RenderableType renderType = primitiveToRenderableType(type);
        auto* renderComponent = newObject->addComponent<Graphics::RenderComponent>(renderType);

        if (!renderComponent)
        {
            m_scene.destroyGameObject(newObject);
            return nullptr;
        }

        // マテリアルを作成
        auto material = m_materialManager.createMaterial(name + "_Material");
        if (material)
        {
            Graphics::MaterialProperties props;

            // タイプ別に色を設定
            switch (type)
            {
            case UI::PrimitiveType::Cube:
                props.albedo = Math::Vector3(0.8f, 0.8f, 0.8f);
                break;
            case UI::PrimitiveType::Sphere:
                props.albedo = Math::Vector3(1.0f, 0.5f, 0.5f);
                break;
            case UI::PrimitiveType::Plane:
                props.albedo = Math::Vector3(0.5f, 1.0f, 0.5f);
                break;
            case UI::PrimitiveType::Cylinder:
                props.albedo = Math::Vector3(0.5f, 0.5f, 1.0f);
                break;
            }

            props.metallic = 0.0f;
            props.roughness = 0.5f;
            material->setProperties(props);
            renderComponent->setMaterial(material);
        }

        // MaterialManagerを設定
        renderComponent->setMaterialManager(&m_materialManager);

        // ShaderManagerで初期化
        if (m_shaderManager)
        {
            auto initResult = renderComponent->initialize(&m_device, m_shaderManager.get());
            if (!initResult)
            {
                Utils::log_error(initResult.error());
                m_scene.destroyGameObject(newObject);
                return nullptr;
            }
        }

        Utils::log_info(std::format("Created new object: {}", name));
        return newObject;
    }

    void EditorApp::deleteGameObject(Core::GameObject* object)
    {
        if (!object)
        {
            Utils::log_warning("Attempted to delete null object");
            return;
        }

        if (object->isDestroyed())
        {
            Utils::log_warning("Object already destroyed");
            return;
        }

        std::string objectName = object->getName();
        Utils::log_info(std::format("Starting deletion of object: {}", objectName));

        try
        {
            // GPU同期
            Utils::log_info("Waiting for GPU...");
            m_device.waitForGpu();

            // EditorViewのGizmo選択をクリア
            Utils::log_info("Clearing Gizmo selection...");
            m_editorView.clearGizmoSelection();

            // Sceneの選択をクリア（これにより全UIが選択解除を認識する）
            if (m_scene.getSelectedObject() == object)
            {
                Utils::log_info("Clearing scene selection...");
                m_scene.clearSelection();
            }

            // オブジェクト削除
            Utils::log_info("Destroying GameObject...");
            m_scene.destroyGameObject(object);

            // ポインタを明示的にnullに（呼び出し元のローカル変数）
            object = nullptr;

            Utils::log_info(std::format("Successfully deleted object: {}", objectName));
        }
        catch (const std::exception& e)
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
                std::format("Exception during deleteGameObject: {}", e.what())));
        }
        catch (...)
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
                "Unknown exception during deleteGameObject"));
        }
    }

    Core::GameObject* EditorApp::duplicateGameObject(Core::GameObject* original)
    {
        if (!original) return nullptr;

        // オリジナルの情報を取得
        auto* originalRender = original->getComponent<Graphics::RenderComponent>();
        if (!originalRender) return nullptr;

        // 新しい名前を生成
        std::string newName = generateUniqueName(original->getName() + "_Copy");

        // プリミティブタイプを使用して新規作成
        UI::PrimitiveType primitiveType = renderableToPrimitiveType(originalRender->getRenderableType());
        auto* newObject = createPrimitiveObject(primitiveType, newName);

        if (!newObject) return nullptr;

        // Transformをコピー
        auto* originalTransform = original->getTransform();
        auto* newTransform = newObject->getTransform();
        if (originalTransform && newTransform)
        {
            newTransform->setPosition(originalTransform->getPosition() + Math::Vector3(1.0f, 0.0f, 0.0f));
            newTransform->setRotation(originalTransform->getRotation());
            newTransform->setScale(originalTransform->getScale());
        }

        return newObject;
    }
    Engine::EngineUI::UIText* EditorApp::createUIElement(UI::UIElementType type, const std::string& name)
    {
        if (type != UI::UIElementType::Text) return nullptr;

        Utils::log_info(std::format("EditorApp::createUIElement called with name: {}", name));

        // GameObjectを作成してUITextコンポーネントを追加
        auto* gameObject = m_scene.createGameObject(name);
        if (!gameObject)
        {
            Utils::log_error(Engine::Utils::make_error(Engine::Utils::ErrorType::Unknown,
                "Failed to create GameObject for UIText"));
            return nullptr;
        }

        auto* uiText = gameObject->addComponent<Engine::EngineUI::UIText>();
        if (!uiText)
        {
            Utils::log_error(Engine::Utils::make_error(Engine::Utils::ErrorType::Unknown,
                "Failed to add UIText component"));
            m_scene.destroyGameObject(gameObject);
            return nullptr;
        }

        uiText->setName(name);

        Math::Vector3 cameraPos = m_editorCamera.getPosition();
        Math::Vector3 cameraForward = m_editorCamera.getForward();
        Math::Vector3 targetPos = cameraPos + cameraForward * 5.0f;

        // GameObjectのTransformを設定
        gameObject->getTransform()->setPosition(targetPos);
        gameObject->getTransform()->setRotation(Math::Vector3(0.0f, 0.0f, 0.0f));
        gameObject->getTransform()->setScale(Math::Vector3(0.01f, 0.01f, 0.01f));

        // UITextの値を同期
        uiText->syncFromGameObjectTransform();

        // または、UITextに直接設定(内部でsyncToが呼ばれる)
        uiText->setPosition(targetPos);
        uiText->setRotation(Math::Vector3(0.0f, 0.0f, 0.0f));
        uiText->setScale(Math::Vector3(0.01f, 0.01f, 0.01f));

        // テキストプロパティのデフォルト値
        uiText->setText("New Text");
        uiText->setFontSize(32.0f);
        uiText->setColor(Math::Vector3(1.0f, 1.0f, 1.0f));
        uiText->setAlpha(1.0f);

        return uiText;
    }
    void EditorApp::deleteUIText(Engine::EngineUI::UIText* text)
    {
        if (!text) return;


        m_scene.removeUIText(text);

        // Inspector選択クリア
        if (m_inspectorWindow && m_inspectorWindow->getSelectedUIText() == text)
        {
            m_inspectorWindow->setSelectedUIText(nullptr);
        }

        // Hierarchy選択クリア
        if (m_hierarchyWindow && m_hierarchyWindow->getSelectedUIText() == text)
        {
            m_hierarchyWindow->clearUISelection();
        }

        Utils::log_info("Deleted UIText from Scene");
    }
    void EditorApp::renameUIText(Engine::EngineUI::UIText* text, const std::string& newName)
    {
        if (!text) return;

        std::string oldName = text->getName();
        text->setName(newName);

        Utils::log_info(std::format("Renamed UIText: {} -> {}", oldName, newName));
    }

    std::string EditorApp::generateUniqueName(const std::string& baseName)
    {
        std::string candidateName = baseName;
        int counter = 1;

        // 同じ名前が存在する限りカウンターを増やす
        while (m_scene.findGameObject(candidateName) != nullptr)
        {
            candidateName = baseName + "_" + std::to_string(counter);
            counter++;

            // 無限ループ防止
            if (counter > 1000)
            {
                candidateName = baseName + "_" + std::to_string(std::time(nullptr));
                break;
            }
        }

        return candidateName;
    }

    // リネーム機能（簡易版）
    void EditorApp::renameGameObject(Core::GameObject* object, const std::string& newName)
    {
        if (!object) return;

        std::string oldName = object->getName();
        object->setName(newName);

        Utils::log_info(std::format("Renamed object: {} -> {}", oldName, newName));
    }

    Graphics::RenderableType EditorApp::primitiveToRenderableType(UI::PrimitiveType type)
    {
        switch (type)
        {
        case UI::PrimitiveType::Cube:
            return Graphics::RenderableType::Cube;
        case UI::PrimitiveType::Sphere:
            // 現在はCubeのみ実装されているため、将来的に追加
            return Graphics::RenderableType::Cube;
        case UI::PrimitiveType::Plane:
            return Graphics::RenderableType::Triangle; // 仮実装
        case UI::PrimitiveType::Cylinder:
            return Graphics::RenderableType::Cube; // 仮実装
        default:
            return Graphics::RenderableType::Cube;
        }
    }

    UI::PrimitiveType EditorApp::renderableToPrimitiveType(Graphics::RenderableType renderType)
    {
        switch (renderType)
        {
        case Graphics::RenderableType::Cube:
            return UI::PrimitiveType::Cube;
        case Graphics::RenderableType::Triangle:
            return UI::PrimitiveType::Plane; // 仮実装
        default:
            return UI::PrimitiveType::Cube;
        }
    }

    void EditorApp::createInitialScene()
    {
        Utils::log_info("Creating initial scene with test objects...");

        // ShaderManagerポインタ取得
        if (!m_shaderManager || !m_shaderManager.get()) {
            Utils::log_warning("ShaderManager is null");
            return;
        }

        Graphics::ShaderManager* shaderMgrPtr = m_shaderManager.get();

        // テストキューブ作成
        auto* cube = m_scene.createGameObject("Cube1");
        cube->getTransform()->setPosition(Math::Vector3(0.0f, 0.0f, 0.0f));
        auto* cube1 = cube->addComponent<Graphics::RenderComponent>(Graphics::RenderableType::Cube);

        // マテリアルを作成
        auto cubeTexMat = m_materialManager.createMaterial("CubeWithTexture_Material");
        if (cubeTexMat) {
            // テクスチャをロード
            auto baseColorTex = m_textureManager.loadTexture("assets/textures/brick_BaseColor.jpg", true, true);

            // プロパティ設定
            Graphics::MaterialProperties cubeProps;
            cubeProps.metallic = 0.0f;
            cubeProps.roughness = 0.5f;
            cubeTexMat->setProperties(cubeProps);

            // テクスチャが読めたら Albedo にセット
            if (baseColorTex) {
                cubeTexMat->setTexture(Graphics::TextureType::Albedo, baseColorTex);
            }

            // マテリアルをCubeにアタッチ
            cube1->setMaterial(cubeTexMat);
        }

        // MaterialManagerを設定
        cube1->setMaterialManager(&m_materialManager);

        // RenderComponentを初期化
        auto cubeInitResult1 = cube1->initialize(&m_device, shaderMgrPtr);
        if (!cubeInitResult1) {
            Utils::log_error(cubeInitResult1.error());
            return;
        }

        Utils::log_info("Initial scene created successfully");
    }

    void EditorApp::createNewScene()
    {
        Utils::log_info("Creating new scene...");

        // 既存のGameObjectを全て削除
        auto& gameObjects = m_scene.getGameObjects();
        std::vector<Core::GameObject*> objectsToDelete;

        for (const auto& obj : gameObjects)
        {
            if (obj)
            {
                objectsToDelete.push_back(obj.get());
            }
        }

        for (auto* obj : objectsToDelete)
        {
            deleteGameObject(obj);
        }

        m_scene.clearUITexts();
        m_currentScenePath = "assets/scenes/untitled.scene";
        Utils::log_info("New scene created");
    }

    void EditorApp::saveScene()
    {
        Utils::log_info(std::format("Saving scene to: {}", m_currentScenePath));

        // ディレクトリが存在しない場合は作成
        std::filesystem::path scenePath(m_currentScenePath);
        auto parentPath = scenePath.parent_path();

        if (!parentPath.empty() && !std::filesystem::exists(parentPath))
        {
            std::filesystem::create_directories(parentPath);
        }

        // シーンを保存
        auto result = m_sceneSerializer.saveScene(m_scene, m_currentScenePath);

        if (result)
        {
            Utils::log_info("Scene saved successfully");
        }
        else
        {
            Utils::log_error(result.error());
        }
    }

    void EditorApp::saveSceneAs()
    {
        // TODO: ファイル選択ダイアログを実装
        // 現在は固定パスで保存
        Utils::log_info("Save Scene As...");

        static int sceneCounter = 1;
        m_currentScenePath = std::format("assets/scenes/scene_{}.scene", sceneCounter++);

        saveScene();
    }

    void EditorApp::openScene()
    {
        Utils::log_info("Opening scene...");

        std::string filepath = "assets/scenes/default.scene";

        // ファイルが存在するかチェック
        if (!std::filesystem::exists(filepath))
        {
            Utils::log_warning(std::format("Scene file not found: {}", filepath));
            Utils::log_info("Creating new scene instead");
            createNewScene();
            return;
        }

        // 既存のGameObjectを削除
        Utils::log_info("Clearing current scene...");
        auto& gameObjects = m_scene.getGameObjects();
        std::vector<Core::GameObject*> objectsToDelete;

        for (const auto& obj : gameObjects)
        {
            if (obj)
            {
                objectsToDelete.push_back(obj.get());
            }
        }

        for (auto* obj : objectsToDelete)
        {
            m_scene.destroyGameObject(obj);
        }

        m_scene.clearUITexts();

        // GPU同期を確実に行う
        m_device.waitForGpu();

        Utils::log_info(std::format("Loading scene from: {}", filepath));

        // シーンを読み込み
        auto result = m_sceneSerializer.loadScene(
            m_scene,
            &m_device,
            m_shaderManager.get(),
            &m_materialManager,
            &m_textureManager,
            filepath
        );

        if (result)
        {
            m_currentScenePath = filepath;
            Utils::log_info(std::format("Scene loaded successfully. Object count: {}",
                m_scene.getGameObjects().size()));

            // 読み込んだシーンを開始
            m_scene.start();
        }
        else
        {
            Utils::log_error(result.error());
        }
    }

}


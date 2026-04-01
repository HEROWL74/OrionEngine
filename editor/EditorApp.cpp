// editor/EditorApp.cpp
#include "PixWrapper.hpp" 
#include "EditorApp.hpp"
#include <format>
#include <filesystem>

namespace Editor
{
    using namespace Engine;

    // 実行ファイルのディレクトリを返す（絶対パス）
    static std::filesystem::path GetExeDir()
    {
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        return std::filesystem::path(path).parent_path();
    }

    static std::filesystem::path resolveEngineRoot()
    {
        // OrionEditor.exe は build/editor/Debug/ にある
        // engine-assets は build/editor/ にある → 1階層上
        // バージョン管理する場合は versions/vX.X.X/ を探す
        auto exeDir = GetExeDir();  // ProjectSettings.cpp の無名namespaceから移動推奨

        // exe と同階層に engine-assets/ があればそこ
        if (std::filesystem::exists(exeDir / "engine-assets"))
            return exeDir;

        // 上の階層を探す（開発ビルド）
        auto current = exeDir;
        while (current.has_parent_path() && current != current.parent_path())
        {
            if (std::filesystem::exists(current / "engine-assets"))
                return current;
            current = current.parent_path();
        }

        return exeDir;  // フォールバック
    }

    Utils::VoidResult EditorApp::initialize(HINSTANCE hInstance, int nCmdShow,
        const std::filesystem::path& projectPath)
    {
        Utils::log_info("Initializing Game Engine...");

        // ============================================================
        // ProjectSettings をエディタ用として読み込む
        // project-templates/3d/ProjectSettings.json を参照
        // ============================================================
        auto& settings = Engine::Core::ProjectSettings::get();
        auto engineRoot = resolveEngineRoot();

        if (!projectPath.empty() && std::filesystem::exists(projectPath / "ProjectSettings.json"))
        {
            // --project 引数あり: そのまま使う
            settings.load(projectPath / "ProjectSettings.json");
        }
        else
        {
            // 引数なし: 従来の探索ロジック（loadForEditor）
            settings.loadForEditor();
        }

        // engineRoot を明示的に注入
        Engine::Core::EnginePaths paths;
        paths.engineRoot = engineRoot;
        paths.projectRoot = settings.getProjectRootDir();
        settings.setPaths(paths);

        // ウィンドウの作成
        // タイトル・サイズ・フルスクリーンは ProjectSettings.json の値を使用
        const auto& wincfg = settings.getWindowConfig();
        Core::WindowSettings windowSettings{
            .title = settings.getProjectNameW(),
            .width = wincfg.width,
            .height = wincfg.height,
            .resizable = true,
            .fullScreen = wincfg.fullscreen
        };

        Utils::log_info("WindowSettings is successfully");

        auto windowResult = m_window.create(hInstance, windowSettings);
        if (!windowResult)
        {
            Utils::log_error(windowResult.error());
            return std::unexpected(windowResult.error());
        }

        Utils::log_info("Window create is successfully");

        m_window.setResizeCallback([this](int width, int height) {
            onWindowResize(width, height);
            });

        m_window.setCloseCallback([this]() {
            onWindowClose();
            });

        m_window.show(nCmdShow);

        auto d3dResult = initD3D();
        if (!d3dResult)
        {
            Utils::log_error(d3dResult.error());
            return std::unexpected(d3dResult.error());
        }

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
        /*
            {
            auto start = std::chrono::high_resolution_clock::now();

            for (int i = 0; i < 1000; ++i)
                m_scene.registerRenderable(
                    m_scene.createGameObject("Bench_" + std::to_string(i)),
                    Graphics::RenderableType::Cube, nullptr);

            auto end = std::chrono::high_resolution_clock::now();
            Utils::log_info(std::format("registerRenderable x5000: {:.3f}ms",
                std::chrono::duration<double, std::milli>(end - start).count()));
        }
        */

        while (m_window.processMessages())
        {
            auto T0 = std::chrono::high_resolution_clock::now();
            update();
            auto T1 = std::chrono::high_resolution_clock::now();
            render();
            auto T2 = std::chrono::high_resolution_clock::now();

            static int c = 0;
            if (c++ % 60 == 0)
            {
                auto ms = [](auto a, auto b) {
                    return std::chrono::duration<double, std::milli>(b - a).count();
                    };
                Utils::log_info(std::format(
                    "update={:.3f}ms  render={:.3f}ms  total={:.3f}ms",
                    ms(T0, T1), ms(T1, T2), ms(T0, T2)));
            }
        }
        cleanup();

        Utils::log_info("Application terminated successfully.");
        return 0;
    }
    Engine::Utils::VoidResult EditorApp::initD3D()
    {
        Utils::log_info("Initializing DirectX 12...");

        Renderer::DeviceSettings deviceSettings{
            .enableDebugLayer = true,
            .enableGpuValidation = false,
            .minFeatureLevel = D3D_FEATURE_LEVEL_11_0,
            .preferHighPerformanceAdapter = true
        };

        auto deviceResult = m_device.initialize(deviceSettings);
        if (!deviceResult) return deviceResult;

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

        auto imguiResult = m_imguiManager.initialize(&m_device, m_window.getHandle(), m_commandQueue.Get());
        if (!imguiResult) return imguiResult;

        m_window.setMessageCallback(
            [&](HWND hwnd, UINT msg, WPARAM w, LPARAM l) -> bool {
                return m_imguiManager.handleWindowMessage(hwnd, msg, w, l);
            }
        );

        Utils::log_info("Initializing ShaderManager...");
        m_shaderManager = std::make_unique<Renderer::ShaderManager>();
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

        auto psoCacheResult = m_psoCache.initialize(&m_device, m_shaderManager.get());
        if (!psoCacheResult) return psoCacheResult;

        Utils::log_info("PSOCache initialization completed successfully");

        Utils::log_info("Initializing Skybox...");
        auto editorSkyboxResult = m_editorSkybox.initialize(&m_device, m_shaderManager.get());
        auto gameSkyboxResult = m_gameSkybox.initialize(&m_device, m_shaderManager.get());
        Utils::log_info("Skybox initialized successfully");

        auto textureManagerResult = m_textureManager.initialize(&m_device);
        if (!textureManagerResult) return textureManagerResult;

        auto materialManagerResult = m_materialManager.initialize(&m_device);
        if (!materialManagerResult) return materialManagerResult;

        auto sceneResult = m_scene.initialize(&m_device, m_shaderManager.get());
        if (!sceneResult) return sceneResult;

        Engine::World::setActiveScene(&m_scene);
        Utils::log_info("Active scene set in EditorApp");

        Utils::log_info("Initializing UITextRenderer...");
        m_uiTextRenderer = std::make_unique<Engine::EngineUI::UITextRenderer>();
        auto uiTextResult = m_uiTextRenderer->initialize(&m_device, m_shaderManager.get());
        if (!uiTextResult) { Utils::log_error(uiTextResult.error()); return uiTextResult; }

        const auto [clientWidth, clientHeight] = m_window.getClientSize();

        Utils::log_info("Initializing EditorView...");
        auto editorViewResult = m_editorView.initialize(&m_device, clientWidth, clientHeight, m_shaderManager.get(), &m_psoCache);
        if (!editorViewResult) { Utils::log_error(editorViewResult.error()); return editorViewResult; }
        m_editorView.setPSOCache(&m_psoCache);

        Utils::log_info("Initializing GameView...");
        auto gameViewResult = m_gameView.initialize(&m_device, clientWidth, clientHeight, &m_psoCache);
        if (!gameViewResult) { Utils::log_error(gameViewResult.error()); return gameViewResult; }
        m_gameView.setPSOCache(&m_psoCache);

        m_editorView.setSkybox(&m_editorSkybox);
        m_editorView.setScene(&m_scene);
        m_editorView.setUITextRenderer(m_uiTextRenderer.get());

        if (auto* fxaa = m_editorView.getFXAARenderer())
        {
            fxaa->setQuality(0.75f, 0.125f, 0.0312f);
        }
        else
        {
            Utils::log_warning("FXAA renderer not available");
        }

        m_gameView.setSkybox(&m_gameSkybox);
        m_gameView.setScene(&m_scene);
        m_gameView.setUITextRenderer(m_uiTextRenderer.get());

        m_device.waitForGpu();

        // ============================================================
        // ConsoleWindow（Lua 初期化より前に作成）
        // コンストラクタ内で LogDispatcher にリスナーが登録される
        // ============================================================
        m_consoleWindow = std::make_unique<UI::ConsoleWindow>();

        // ============================================================
        // GameLogicConsoleWindow + Profiler
        // LuaBindings より前に作成して installLeafCallback を呼ぶ
        // ============================================================
        m_gameLogicConsole = std::make_unique<UI::GameLogicConsoleWindow>();
        m_gameLogicProfiler = std::make_unique<UI::GameLogicProfiler>();
        m_gameLogicProfiler->setWindow(m_gameLogicConsole.get());

        // ============================================================
        // Lua
        // ============================================================
        m_luaBindings = std::make_unique<Engine::Scripting::LuaBindings>();

        // LeafCallback を LuaBindings に注入する。
        // registerBindings() より前に呼ぶことが必須。
        // bindTransform() がコールバックをキャプチャして usertype を登録するため。
        m_gameLogicProfiler->installLeafCallback(m_luaBindings.get());

        auto& scriptMgr = Scripting::ScriptManager::get();

        auto registerAllBindings = [this](sol::state& lua)
            {
                Utils::log_info("Registering all Lua bindings...");
                m_luaBindings->registerBindings(lua);

                lua["Editor"] = lua.create_table();
                lua["Editor"]["play"] = [this]() {
                    if (!m_playModeController.isReady()) {
                        Utils::log_warning("Play called before PlayModeController ready");
                        return;
                    }
                    m_playModeController.play();
                    };
                lua["Editor"]["stop"] = [this]() { m_playModeController.stop(); };
                lua["Editor"]["restartGame"] = [this]() { m_playModeController.restart(); };

                Utils::log_info("All Lua bindings registered successfully");
            };

        scriptMgr.setBindingCallback(registerAllBindings);
        scriptMgr.initialize();
        Utils::log_info("Lua scripting system initialized");

        // ============================================================
        // Scene Load
        // ============================================================
        auto& settings = Engine::Core::ProjectSettings::get();
        std::filesystem::path defaultScenePath = settings.getDefaultScenePath();
        defaultScenePath = std::filesystem::weakly_canonical(defaultScenePath);

        if (std::filesystem::exists(defaultScenePath))
        {
            auto loadResult = m_sceneSerializer.loadScene(
                m_scene,
                &m_device,
                m_shaderManager.get(),
                &m_materialManager,
                &m_textureManager,
                defaultScenePath
            );
            if (loadResult)
            {
                m_currentScenePath = defaultScenePath.string();
                Utils::log_info("Default scene loaded: " + m_currentScenePath);
            }
            else
            {
                m_currentScenePath = defaultScenePath.string();
                Utils::log_warning("Failed to load default scene, creating initial scene");
                createInitialScene();
            }
        }
        else
        {
            m_currentScenePath = defaultScenePath.string();
            createInitialScene();
        }

        Utils::log_info(std::format("Checking for default scene: {}", defaultScenePath.string()));

        // ============================================================
        // PlayModeController
        // ============================================================
        m_playModeController.initialize(&m_scene);
        m_playModeController.setSceneLoadContext(
            &m_device, m_shaderManager.get(),
            &m_materialManager, &m_textureManager,
            m_currentScenePath
        );

        // ============================================================
        // ProjectWindow
        // ============================================================
        std::string assetRootPath = settings.getAssetRootPath().string();
        std::replace(assetRootPath.begin(), assetRootPath.end(), '\\', '/');

        Utils::log_info("ProjectWindow asset root: " + assetRootPath);

        m_projectWindow = std::make_unique<UI::ProjectWindow>();
        m_projectWindow->setImGuiManager(&m_imguiManager);
        m_projectWindow->setTextureManager(&m_textureManager);
        m_projectWindow->setMaterialManager(&m_materialManager);
        m_projectWindow->setProjectPath(assetRootPath);

        // カメラ初期化
        m_editorCamera.setPerspective(45.0f, static_cast<float>(clientWidth) / clientHeight, 0.1f, 100.0f);
        m_editorCamera.setPosition({ 8.0f, 8.0f, 8.0f });
        m_editorCamera.lookAt({ 0.0f, 0.0f, 0.0f });

        m_gameCamera.setPerspective(45.0f, static_cast<float>(clientWidth) / clientHeight, 0.1f, 100.0f);
        m_gameCamera.setPosition({ 0.0f, 5.0f, 8.0f });
        m_gameCamera.lookAt({ 0.0f, 0.0f, 0.0f });

        m_cameraController = std::make_unique<World::FPSCameraController>(&m_editorCamera);
        m_cameraController->setMovementSpeed(5.0f);
        m_cameraController->setMouseSensitivity(0.1f);

        // ============================================================
        // UIウィンドウ作成
        // ============================================================
        m_debugWindow = std::make_unique<UI::DebugWindow>();
        m_hierarchyWindow = std::make_unique<UI::SceneHierarchyWindow>();
        m_inspectorWindow = std::make_unique<UI::InspectorWindow>();
        m_toolbarWindow = std::make_unique<UI::ToolbarWindow>();
        m_toolbarWindow->setPlayModeController(&m_playModeController);
        m_editorViewWindow = std::make_unique<UI::EditorViewWindow>();

        // ============================================================
        // プレイ開始時: カウンタリセット + 全 LuaScriptComponent を登録
        // registerComponent() が ProfileCallback と ScopeCallback を両方セットする
        // ============================================================
        m_playModeController.setOnPlayCallback([this]()
            {
                m_gameLogicProfiler->reset();

                int registered = 0;
                for (auto& obj : m_scene.getGameObjects())
                {
                    if (!obj) continue;
                    auto* luaComp = m_scene.getComponent<Engine::Scripting::LuaScriptComponent>(obj.get());
                    Utils::log_info(std::format("  [Profiler] {} -> LuaComp: {}",
                        obj->getName(), luaComp ? "FOUND" : "NULL"));
                    if (luaComp)
                    {
                        m_gameLogicProfiler->registerComponent(luaComp);
                        registered++;
                    }
                }
                Utils::log_info(std::format("[Profiler] registered {} components", registered));
            });

        // ============================================================
        // InputManager
        // ============================================================
        auto* inputManager = m_window.getInputManager();
        if (!inputManager)
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "Failed to get InputManager from Window"));
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "InputManager is null"));
        }

        Engine::Input::InputSystem::get().setInputManager(inputManager);
        Utils::log_info("InputSystem bound to InputManager");

        if (!inputManager->isInitialized())
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "InputManager is not initialized when creating EditorViewWindow"));
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "InputManager not initialized"));
        }

        Utils::log_info(std::format("Initializing EditorViewWindow with InputManager (initialized: {})",
            inputManager->isInitialized()));

        m_editorViewWindow->initialize(&m_imguiManager, &m_editorView, inputManager);
        m_editorViewWindow->setCamera(&m_editorCamera);

        m_gameViewWindow = std::make_unique<UI::GameViewWindow>();
        m_gameViewWindow->initialize(&m_imguiManager, &m_gameView);

        if (auto* mainCamObj = m_scene.findGameObject("MainCamera"))
        {
            Utils::log_info("[DEBUG] MainCamera found after loadScene");
            auto* camComp = m_scene.getComponentBatch()
                .get<Engine::World::CameraComponent>(mainCamObj->getId());
            Utils::log_info(std::format("[DEBUG] CameraComponent via batch: {}",
                camComp ? "OK" : "NULL"));
            auto* camCompOld = mainCamObj->getComponent<Engine::World::CameraComponent>();
            Utils::log_info(std::format("[DEBUG] CameraComponent via getComponent: {}",
                camCompOld ? "OK" : "NULL"));
        }

        Utils::log_info("Initializing BuildSystem...");
        m_buildWindow = std::make_unique<UI::BuildWindow>();
        m_buildWindow->initialize();
        Utils::log_info("BuildSystem initialized successfully");

        m_hierarchyWindow->setScene(&m_scene);
        m_inspectorWindow->setScene(&m_scene);
        m_debugWindow->setPlayModeController(&m_playModeController);

        m_hierarchyWindow->setSelectionChangedCallback([this](Core::GameObject* object) {
            Utils::log_info(std::format("GameObject selection changed to: {}",
                object ? object->getName() : "null"));
            m_editorView.setSelectedObject(object);
            m_inspectorWindow->setSelectedObject(object);
            });

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

        m_hierarchyWindow->setUISelectionChangedCallback([this](Engine::EngineUI::UIText* text) {
            Utils::log_info(std::format("UIText selection changed to: {}",
                text ? text->getName() : "null"));
            m_inspectorWindow->setSelectedUIText(text);
            if (text) m_editorView.setSelectedObject(nullptr);
            });

        m_hierarchyWindow->setCreateGameCameraCallback([this](const std::string& name) -> Core::GameObject* {
            return createGameCamera(name);
            });

        m_inspectorWindow->setMaterialManager(&m_materialManager);
        m_inspectorWindow->setTextureManager(&m_textureManager);

        Utils::log_info("DirectX 12 initialization completed successfully!");
        return {};
    }

    Engine::Utils::VoidResult EditorApp::initializeInput()
    {
        Utils::log_info("Initializing input system...");

        auto* inputManager = m_window.getInputManager();
        if (!inputManager)
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
                "InputManager not available from Window"));
        }

        if (!inputManager->isInitialized())
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
                "InputManager from Window is not initialized"));
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
                "InputManager not initialized"));
        }

        Utils::log_info(std::format("InputManager initialized: {}", inputManager->isInitialized()));

        inputManager->setRelativeMouseMode(false);
        inputManager->setMouseSensitivity(0.1f);

        inputManager->setKeyPressedCallback([this](Input::KeyCode key) { onKeyPressed(key); });
        inputManager->setKeyReleasedCallback([this](Input::KeyCode key) { onKeyReleased(key); });
        inputManager->setMouseMoveCallback([this](int x, int y, int deltaX, int deltaY) { onMouseMove(x, y, deltaX, deltaY); });
        inputManager->setMouseButtonPressedCallback([this](Input::MouseButton button, int x, int y) { onMouseButtonPressed(button, x, y); });
        inputManager->setMouseButtonReleasedCallback([this](Input::MouseButton button, int x, int y) { onMouseButtonReleased(button, x, y); });

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
            m_commandQueue.Get(), m_window.getHandle(),
            &swapChainDesc, nullptr, nullptr, &swapChain1),
            Utils::ErrorType::SwapChainCreation, "Failed to create swap chain");

        CHECK_HR(swapChain1.As(&m_swapChain),
            Utils::ErrorType::SwapChainCreation, "Failed to cast swap chain to IDXGISwapChain3");

        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
        return {};
    }

    Utils::VoidResult EditorApp::createRenderTargets()
    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
        rtvHeapDesc.NumDescriptors = 2;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        CHECK_HR(m_device.getDevice()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)),
            Utils::ErrorType::ResourceCreation, "Failed to create RTV descriptor heap");

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
        CHECK_HR(m_device.getDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)),
            Utils::ErrorType::ResourceCreation, "Failed to create command allocator");

        CHECK_HR(m_device.getDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
            m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)),
            Utils::ErrorType::ResourceCreation, "Failed to create command list");

        m_commandList->Close();
        return {};
    }

    Utils::VoidResult EditorApp::createDepthStencilBuffer()
    {
        const auto [clientWidth, clientHeight] = m_window.getClientSize();

        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        CHECK_HR(m_device.getDevice()->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)),
            Utils::ErrorType::ResourceCreation, "Failed to create DSV descriptor heap");

        D3D12_CLEAR_VALUE depthOptimizedClearValue{};
        depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
        depthOptimizedClearValue.DepthStencil.Stencil = 0;

        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC depthStencilDesc{};
        depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthStencilDesc.Alignment = 0;
        depthStencilDesc.Width = clientWidth;
        depthStencilDesc.Height = clientHeight;
        depthStencilDesc.DepthOrArraySize = 1;
        depthStencilDesc.MipLevels = 1;
        depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
        depthStencilDesc.SampleDesc.Count = 1;
        depthStencilDesc.SampleDesc.Quality = 0;
        depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        CHECK_HR(m_device.getDevice()->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &depthStencilDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthOptimizedClearValue,
            IID_PPV_ARGS(&m_depthStencilBuffer)),
            Utils::ErrorType::ResourceCreation, "Failed to create depth stencil buffer");

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;

        m_device.getDevice()->CreateDepthStencilView(
            m_depthStencilBuffer.Get(), &dsvDesc,
            m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
        return {};
    }

    Utils::VoidResult EditorApp::createSyncObjects()
    {
        CHECK_HR(m_device.getDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)),
            Utils::ErrorType::ResourceCreation, "Failed to create fence");

        m_fenceValue = 1;
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        CHECK_CONDITION(m_fenceEvent != nullptr, Utils::ErrorType::ResourceCreation, "Failed to create fence event");
        return {};
    }

    void EditorApp::update()
    {
        updateDeltaTime();

        m_editorViewWindow->processResize();
        m_gameViewWindow->processResize();

        m_playModeController.update();

        if (m_playModeController.isRestarting())
            return;

        if (m_playModeController.isPlaying())
        {
            Scripting::ScriptManager::get().checkForUpdates();
            m_scene.update(m_deltaTime);
            m_scene.lateUpdate(m_deltaTime);
            m_scene.processPendingDestroy();

            // 全 LuaScriptComponent の計測結果を GameLogicConsoleWindow へ送信
            m_gameLogicProfiler->flushToWindow();
        }
        else
        {
            m_scene.getTransformStorage().flushDirty();
            m_scene.processPendingDestroy();
        }

        // CameraComponentのTransformを毎フレーム同期（Play/Editor両対応）
        if (auto* camObj = m_scene.findGameObject("MainCamera"))
        {
            auto* camComp = m_scene.getComponentBatch()
                .get<Engine::World::CameraComponent>(camObj->getId());
            if (camComp)
            {
                camComp->syncFromTransform();
                m_gameCamera = camComp->getCamera();
            }
        }

        m_debugWindow->setFPS(m_currentFPS);
        m_debugWindow->setFrameTime(m_deltaTime);
        m_debugWindow->setObjectCount(m_scene.getGameObjects().size());

        processInput();
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

        if (m_uiTextRenderer) m_uiTextRenderer->beginFrame();

        m_commandAllocator->Reset();
        m_commandList->Reset(m_commandAllocator.Get(), nullptr);

        PIXBeginEvent(m_commandList.Get(), PIX_COLOR_INDEX(1), "3D Scene Pass");

        PIXBeginEvent(m_commandList.Get(), PIX_COLOR_INDEX(2), "Editor View Render");
        m_editorView.render(m_scene, m_commandList.Get(), m_editorCamera, m_frameIndex);
        PIXEndEvent(m_commandList.Get());

        const World::Camera* gameCamera = &m_gameCamera;
        if (auto* camObj = m_scene.findGameObject("MainCamera"))
        {
            Utils::log_info(std::format("[DEBUG] render: MainCamera id=({},{})",
                camObj->getId().index, camObj->getId().generation));
            auto* camComp = m_scene.getComponentBatch()
                .get<Engine::World::CameraComponent>(camObj->getId());
            Utils::log_info(std::format("[DEBUG] render: CameraComponent via batch: {}",
                camComp ? "OK" : "NULL"));
        }
        else
        {
            Utils::log_warning("MainCamera found but NO CameraComponent!");
        }

        PIXBeginEvent(m_commandList.Get(), PIX_COLOR_INDEX(3), "Game View Render");
        m_gameView.render(m_scene, m_commandList.Get(), *gameCamera, m_frameIndex);
        PIXEndEvent(m_commandList.Get());

        PIXEndEvent(m_commandList.Get());

        HRESULT hrClose = m_commandList->Close();
        if (FAILED(hrClose))
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
                std::format("Failed to close command list for 3D rendering: 0x{:08X}", static_cast<unsigned>(hrClose))));
            return;
        }

        ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
        m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        const UINT64 fenceValue = m_fenceValue;
        m_commandQueue->Signal(m_fence.Get(), fenceValue);
        m_fenceValue++;

        if (m_fence->GetCompletedValue() < fenceValue)
        {
            m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent);
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }

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

        PIXBeginEvent(m_commandList.Get(), PIX_COLOR_INDEX(4), "UI & ImGui Pass");

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

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New Scene", "Ctrl+N"))           createNewScene();
                if (ImGui::MenuItem("Open Scene...", "Ctrl+O"))       openScene();
                ImGui::Separator();
                if (ImGui::MenuItem("Save Scene", "Ctrl+S"))          saveScene();
                if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S")) saveSceneAs();
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Alt+F4"))           PostQuitMessage(0);
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
                        if (std::filesystem::exists(root / "CMakeLists.txt")) break;
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
        m_gameLogicConsole->draw();
        m_consoleWindow->draw();
        m_debugWindow->draw();
        m_projectWindow->draw();
        m_buildWindow->draw();

        if (isResizing)
        {
            ImGui::Render();
            PIXEndEvent(m_commandList.Get());
            m_commandList->Close();
            return;
        }

        m_imguiManager.render(m_commandList.Get());

        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        m_commandList->ResourceBarrier(1, &barrier);

        PIXEndEvent(m_commandList.Get());

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

        m_frameCount++;
        m_frameTimeAccumulator += m_deltaTime;
        if (m_frameTimeAccumulator >= 1.0f)
        {
            m_currentFPS = m_frameCount / m_frameTimeAccumulator;
            m_frameCount = 0;
            m_frameTimeAccumulator = 0.0f;
        }
    }

    void EditorApp::processInput()
    {
        auto* inputManager = m_window.getInputManager();
        if (!inputManager || !m_cameraController) return;

        if (!inputManager->isInitialized())
        {
            Utils::log_warning("processInput: InputManager not initialized");
            return;
        }

        ImGuiIO& io = ImGui::GetIO();

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
                    static_cast<float>(deltaY));
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
        if (!m_commandQueue || !m_fence || !m_fenceEvent)
        {
            Utils::log_warning("DirectX objects not initialized in waitForPreviousFrame");
            return;
        }

        const UINT64 fence = m_fenceValue;
        HRESULT hr = m_commandQueue->Signal(m_fence.Get(), fence);
        if (FAILED(hr))
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "Failed to signal fence", hr));
            return;
        }
        m_fenceValue++;

        if (m_fence->GetCompletedValue() < fence)
        {
            hr = m_fence->SetEventOnCompletion(fence, m_fenceEvent);
            if (FAILED(hr))
            {
                Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "Failed to set fence event", hr));
                return;
            }
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }

        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    }

    void EditorApp::cleanup()
    {
        waitForPreviousFrame();

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
            }
            return;
        }

        try
        {
            waitForPreviousFrame();

            const UINT64 flushFence = m_fenceValue;
            m_commandQueue->Signal(m_fence.Get(), flushFence);
            m_fenceValue++;

            if (m_fence->GetCompletedValue() < flushFence)
            {
                m_fence->SetEventOnCompletion(flushFence, m_fenceEvent);
                WaitForSingleObject(m_fenceEvent, INFINITE);
            }

            if (m_imguiManager.isInitialized()) m_imguiManager.shutdown();

            for (UINT i = 0; i < 2; i++) { if (m_renderTargets[i]) m_renderTargets[i].Reset(); }
            if (m_depthStencilBuffer) m_depthStencilBuffer.Reset();

            HRESULT hr = m_swapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
            if (FAILED(hr))
            {
                Utils::log_error(Utils::make_error(Utils::ErrorType::SwapChainCreation,
                    std::format("Failed to resize swap chain: 0x{:08x}", static_cast<unsigned>(hr)), hr));
                return;
            }

            auto renderTargetResult = createRenderTargets();
            if (!renderTargetResult) { Utils::log_error(renderTargetResult.error()); return; }

            auto depthStencilResult = createDepthStencilBuffer();
            if (!depthStencilResult) { Utils::log_error(depthStencilResult.error()); return; }

            m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
            m_editorCamera.updateAspect(static_cast<float>(width) / height);
            m_gameCamera.updateAspect(static_cast<float>(width) / height);

            m_editorView.resize(width, height);
            m_gameView.resize(width, height);

            auto imguiResult = m_imguiManager.initialize(&m_device, m_window.getHandle(), m_commandQueue.Get());
            if (!imguiResult) { Utils::log_error(imguiResult.error()); return; }

            m_window.setMessageCallback(
                [&](HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
                    return m_imguiManager.handleWindowMessage(hwnd, msg, w, l);
                }
            );

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

        ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_NoDockingInCentralNode;

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
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
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
            ImGui::DockBuilderAddNode(dockspaceID, dockspaceFlags | ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspaceID, workSize);

            ImGuiID dockMain = dockspaceID;
            ImGuiID dockBottom = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down, 0.28f, nullptr, &dockMain);
            ImGuiID dockLeft = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.22f, nullptr, &dockMain);
            ImGuiID dockRight = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.33f, nullptr, &dockMain);
            ImGuiID dockCenter = dockMain;

            ImGui::DockBuilderDockWindow("Scene Hierarchy", dockLeft);
            ImGui::DockBuilderDockWindow("Inspector", dockRight);
            ImGui::DockBuilderDockWindow("Game Logic Structure", dockRight);
            ImGui::DockBuilderDockWindow("Scene", dockCenter);
            ImGui::DockBuilderDockWindow("Game", dockCenter);
            ImGui::DockBuilderDockWindow("Project", dockBottom);
            ImGui::DockBuilderDockWindow("Debug info", dockBottom);
            ImGui::DockBuilderDockWindow("Console", dockBottom);
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
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureKeyboard) return;

        if (key == Input::KeyCode::Escape)
        {
            Utils::log_info("Escape key pressed - requesting exit");
            PostQuitMessage(0);
        }
        else if (key == Input::KeyCode::F1)
        {
            auto* inputManager = m_window.getInputManager();
            if (inputManager)
            {
                bool currentMode = inputManager->getMouseState().isRelativeMode;
                inputManager->setRelativeMouseMode(!currentMode);
                Utils::log_info(std::format("Mouse relative mode: {}", !currentMode ? "ON" : "OFF"));
            }
        }
    }

    void EditorApp::onKeyReleased(Input::KeyCode key) {}

    [[maybe_unused]]
    void EditorApp::onMouseMove(int x, int y, int deltaX, int deltaY) {}

    void EditorApp::onMouseButtonPressed(Input::MouseButton button, int x, int y)
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse) return;
        Utils::log_info(std::format("Mouse button {} pressed at ({}, {})", static_cast<int>(button), x, y));
    }

    void EditorApp::onMouseButtonReleased(Input::MouseButton button, int x, int y)
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse) return;
        Utils::log_info(std::format("Mouse button {} released at ({}, {})", static_cast<int>(button), x, y));
    }

    Core::GameObject* EditorApp::createPrimitiveObject(UI::PrimitiveType type, const std::string& name)
    {
        auto* newObject = m_scene.createGameObject(name);
        if (!newObject) return nullptr;

        // プリミティブはエディタカメラの前方に配置する（GameCameraではなくEditorCamera基準）
        Math::Vector3 cameraPos = m_editorCamera.getPosition();
        Math::Vector3 cameraForward = m_editorCamera.getForward();
        newObject->getTransform()->setPosition(cameraPos + cameraForward * 3.0f);

        Renderer::RenderableType renderType = primitiveToRenderableType(type);

        auto material = m_materialManager.createMaterial(name + "_Material");
        if (material)
        {
            Renderer::MaterialProperties props;
            switch (type)
            {
            case UI::PrimitiveType::Cube:     props.albedo = Math::Vector3(0.8f, 0.8f, 0.8f); break;
            case UI::PrimitiveType::Sphere:   props.albedo = Math::Vector3(1.0f, 0.5f, 0.5f); break;
            case UI::PrimitiveType::Plane:    props.albedo = Math::Vector3(0.5f, 1.0f, 0.5f); break;
            case UI::PrimitiveType::Cylinder: props.albedo = Math::Vector3(0.5f, 0.5f, 1.0f); break;
            }
            props.metallic = 0.0f;
            props.roughness = 0.5f;
            material->setProperties(props);
        }

        // RenderComponentの代わりにRenderBatchSystemへ登録
        m_scene.registerRenderable(newObject, renderType, material);

        Utils::log_info(std::format("Created new object: {}", name));
        return newObject;
    }

    void EditorApp::deleteGameObject(Core::GameObject* object)
    {
        if (!object) { Utils::log_warning("Attempted to delete null object"); return; }
        if (object->isDestroyed()) { Utils::log_warning("Object already destroyed"); return; }

        std::string objectName = object->getName();
        Utils::log_info(std::format("Starting deletion of object: {}", objectName));

        try
        {
            m_device.waitForGpu();
            m_editorView.clearGizmoSelection();

            // 削除前に全UIの選択をクリア
            if (m_inspectorWindow) m_inspectorWindow->setSelectedObject(nullptr);
            if (m_hierarchyWindow) m_hierarchyWindow->clearUISelection();
            if (m_scene.getSelectedObject() == object) m_scene.clearSelection();

            m_scene.destroyGameObject(object);
            object = nullptr;

            Utils::log_info(std::format("Successfully deleted object: {}", objectName));
        }
        catch (const std::exception& e)
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
                std::format("Exception during deleteGameObject: {}", e.what())));
        }
    }

    Core::GameObject* EditorApp::duplicateGameObject(Core::GameObject* original)
    {
        if (!original) return nullptr;

        std::string newName = generateUniqueName(original->getName() + "_Copy");
        auto* newObject = m_scene.createGameObject(newName);
        if (!newObject) return nullptr;

        auto* originalTransform = original->getTransform();
        auto* newTransform = newObject->getTransform();
        if (originalTransform && newTransform)
        {
            newTransform->setPosition(originalTransform->getPosition() + Math::Vector3(1, 0, 0));
            newTransform->setRotation(originalTransform->getRotation());
            newTransform->setScale(originalTransform->getScale());
        }

        // RenderEntryもコピー
        const auto* entry = m_scene.getRenderBatch().findEntry(original->getId());
        if (entry)
            m_scene.registerRenderable(newObject, entry->type, entry->material);

        return newObject;
    }

    Engine::EngineUI::UIText* EditorApp::createUIElement(UI::UIElementType type, const std::string& name)
    {
        if (type != UI::UIElementType::Text) return nullptr;

        Utils::log_info(std::format("EditorApp::createUIElement called with name: {}", name));

        auto* gameObject = m_scene.createGameObject(name);
        if (!gameObject)
        {
            Utils::log_error(Engine::Utils::make_error(Engine::Utils::ErrorType::Unknown, "Failed to create GameObject for UIText"));
            return nullptr;
        }

        auto* uiText = gameObject->addComponent<Engine::EngineUI::UIText>();
        if (!uiText)
        {
            Utils::log_error(Engine::Utils::make_error(Engine::Utils::ErrorType::Unknown, "Failed to add UIText component"));
            m_scene.destroyGameObject(gameObject);
            return nullptr;
        }

        uiText->setName(name);

        Math::Vector3 cameraPos = m_editorCamera.getPosition();
        Math::Vector3 cameraForward = m_editorCamera.getForward();
        Math::Vector3 targetPos = cameraPos + cameraForward * 5.0f;

        gameObject->getTransform()->setPosition(targetPos);
        gameObject->getTransform()->setRotation(Math::Vector3(0.0f, 0.0f, 0.0f));
        gameObject->getTransform()->setScale(Math::Vector3(0.05f, 0.05f, 0.05f));

        uiText->setText("New Text");
        uiText->setFontSize(32.0f);
        uiText->setColor(Math::Vector3(1.0f, 1.0f, 1.0f));
        uiText->setAlpha(1.0f);
        uiText->syncFromGameObjectTransform();

        return uiText;
    }

    void EditorApp::deleteUIText(Engine::EngineUI::UIText* text)
    {
        if (!text) return;

        if (m_inspectorWindow && m_inspectorWindow->getSelectedUIText() == text)
            m_inspectorWindow->setSelectedUIText(nullptr);

        if (m_hierarchyWindow && m_hierarchyWindow->getSelectedUIText() == text)
            m_hierarchyWindow->clearUISelection();

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

        while (m_scene.findGameObject(candidateName) != nullptr)
        {
            candidateName = baseName + "_" + std::to_string(counter);
            counter++;
            if (counter > 1000)
            {
                candidateName = baseName + "_" + std::to_string(std::time(nullptr));
                break;
            }
        }
        return candidateName;
    }

    Core::GameObject* EditorApp::createGameCamera(const std::string& name)
    {
        if (m_scene.findGameObject(name) != nullptr)
        {
            Utils::log_warning(std::format("GameCamera '{}' already exists in scene", name));
            return nullptr;
        }

        auto* go = m_scene.createGameObject(name);
        if (!go) return nullptr;

        // gameObject->addComponent ではなく scene.addComponent に変更
        auto* camComp = m_scene.addComponent<Engine::World::CameraComponent>(go);
        if (!camComp)
        {
            m_scene.destroyGameObject(go);
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
                "Failed to addComponent<CameraComponent>"));
            return nullptr;
        }

        const auto [w, h] = m_window.getClientSize();
        float aspect = h > 0 ? static_cast<float>(w) / h : 1.0f;
        camComp->setPerspective(45.0f, aspect, 0.1f, 100.0f);

        go->getTransform()->setPosition({ 0.0f, 1.0f, -10.0f });
        go->getTransform()->setRotation({ 0.0f, 0.0f, 0.0f });

        camComp->syncFromTransform();
        m_gameCamera = camComp->getCamera();

        if (m_gameViewWindow)
            m_gameViewWindow->setGameCameraObject(go);

        Utils::log_info(std::format("Created GameCamera: {}", name));
        return go;
    }

    void EditorApp::renameGameObject(Core::GameObject* object, const std::string& newName)
    {
        if (!object) return;
        std::string oldName = object->getName();
        object->setName(newName);
        Utils::log_info(std::format("Renamed object: {} -> {}", oldName, newName));
    }

    Renderer::RenderableType EditorApp::primitiveToRenderableType(UI::PrimitiveType type)
    {
        switch (type)
        {
        case UI::PrimitiveType::Cube:     return Renderer::RenderableType::Cube;
        case UI::PrimitiveType::Sphere:   return Renderer::RenderableType::Cube;
        case UI::PrimitiveType::Plane:
        case UI::PrimitiveType::Cylinder: return Renderer::RenderableType::Cube;
        default:                          return Renderer::RenderableType::Cube;
        }
    }

    UI::PrimitiveType EditorApp::renderableToPrimitiveType(Renderer::RenderableType renderType)
    {
        switch (renderType)
        {
        case Renderer::RenderableType::Cube:     return UI::PrimitiveType::Cube;
            return UI::PrimitiveType::Plane;
        default:                                 return UI::PrimitiveType::Cube;
        }
    }

    void EditorApp::createInitialScene()
    {
        Utils::log_info("Creating initial scene with test objects...");

        auto& settings = Engine::Core::ProjectSettings::get();
        std::string texturePath = (settings.getAssetRootPath() / "textures/brick_BaseColor.jpg").string();
        std::replace(texturePath.begin(), texturePath.end(), '\\', '/');

        auto cubeTexMat = m_materialManager.createMaterial("CubeWithTexture_Material");
        if (cubeTexMat)
        {
            Renderer::MaterialProperties props;
            props.metallic = 0.0f;
            props.roughness = 0.5f;
            cubeTexMat->setProperties(props);

            auto baseColorTex = m_textureManager.loadTexture(texturePath, true, true);
            if (baseColorTex)
                cubeTexMat->setTexture(Renderer::TextureType::Albedo, baseColorTex);
        }

        Utils::log_info("Initial scene created successfully");
    }

    void EditorApp::createNewScene()
    {
        Utils::log_info("Creating new scene...");

        auto& gameObjects = m_scene.getGameObjects();
        std::vector<Core::GameObject*> objectsToDelete;
        for (const auto& obj : gameObjects)
        {
            if (obj) objectsToDelete.push_back(obj.get());
        }
        for (auto* obj : objectsToDelete) deleteGameObject(obj);

        m_scene.clearUITexts();

        // デフォルトシーンパスも ProjectSettings から
        auto& settings = Engine::Core::ProjectSettings::get();
        m_currentScenePath = (settings.getAssetRootPath() / "scenes/untitled.scene").string();
        std::replace(m_currentScenePath.begin(), m_currentScenePath.end(), '\\', '/');

        Utils::log_info("New scene created: " + m_currentScenePath);
    }

    void EditorApp::saveScene()
    {
        // m_currentScenePathが空ならデフォルトパスを設定
        if (m_currentScenePath.empty())
        {
            auto& settings = Engine::Core::ProjectSettings::get();
            m_currentScenePath = (settings.getAssetRootPath() / "scenes/default.scene").string();
            std::replace(m_currentScenePath.begin(), m_currentScenePath.end(), '\\', '/');
        }

        std::filesystem::path scenePath(m_currentScenePath);

        // 念のため絶対パス確認
        Utils::log_info(std::format("Saving scene to: {}", scenePath.string()));
        Utils::log_info(std::format("  is_absolute: {}", scenePath.is_absolute()));
        Utils::log_info(std::format("  has_extension: {}", scenePath.has_extension()));

        m_scene.processPendingDestroy();

        auto parentPath = scenePath.parent_path();
        if (!parentPath.empty() && !std::filesystem::exists(parentPath))
        {
            std::filesystem::create_directories(parentPath);
        }

        auto result = m_sceneSerializer.saveScene(m_scene, scenePath);
        if (result)
            Utils::log_info("Scene saved successfully");
        else
            Utils::log_error(result.error());
    }

    void EditorApp::saveSceneAs()
    {
        Utils::log_info("Save Scene As...");

        auto& settings = Engine::Core::ProjectSettings::get();
        static int sceneCounter = 1;
        m_currentScenePath = (settings.getAssetRootPath()
            / std::format("scenes/scene_{}.scene", sceneCounter++)).string();
        std::replace(m_currentScenePath.begin(), m_currentScenePath.end(), '\\', '/');

        saveScene();
    }

    void EditorApp::openScene()
    {
        Utils::log_info("Opening scene...");

        auto& settings = Engine::Core::ProjectSettings::get();
        std::string filepath = settings.getDefaultScenePath().string();
        std::replace(filepath.begin(), filepath.end(), '\\', '/');

        if (!std::filesystem::exists(filepath))
        {
            Utils::log_warning(std::format("Scene file not found: {}", filepath));
            Utils::log_info("Creating new scene instead");
            createNewScene();
            return;
        }

        Utils::log_info("Clearing current scene...");
        auto& gameObjects = m_scene.getGameObjects();
        std::vector<Core::GameObject*> objectsToDelete;
        for (const auto& obj : gameObjects)
        {
            if (obj) objectsToDelete.push_back(obj.get());
        }
        for (auto* obj : objectsToDelete) m_scene.destroyGameObject(obj);

        m_scene.clearUITexts();
        m_device.waitForGpu();

        Utils::log_info(std::format("Loading scene from: {}", filepath));

        auto result = m_sceneSerializer.loadScene(
            m_scene, &m_device, m_shaderManager.get(),
            &m_materialManager, &m_textureManager, filepath);

        if (result)
        {
            m_currentScenePath = filepath;
            Utils::log_info(std::format("Scene loaded successfully. Object count: {}",
                m_scene.getGameObjects().size()));

            // ロードしたシーンにMainCameraがあればGameViewWindowに通知する
            if (auto* mainCamObj = m_scene.findGameObject("MainCamera"))
            {
                if (auto* camComp = m_scene.getComponent<Engine::World::CameraComponent>(mainCamObj))
                {
                    camComp->syncFromTransform();
                    m_gameCamera = camComp->getCamera();
                    if (m_gameViewWindow)
                        m_gameViewWindow->setGameCameraObject(mainCamObj);
                    Utils::log_info("MainCamera synced after scene load");
                }
            }
            else
            {
                // MainCameraなし：GameViewWindowのGameObject参照をリセット
                if (m_gameViewWindow)
                    m_gameViewWindow->setGameCameraObject(nullptr);
            }
        }
        else
        {
            Utils::log_error(result.error());
        }
    }
    

}
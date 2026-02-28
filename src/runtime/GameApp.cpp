// src/runtime/GameApp.cpp
#include "GameApp.hpp"
#include "engine/Utils/RenderContext.hpp"
#include "engine/Core/Window.hpp"
#include "engine/Core/ProjectSettings.hpp"
#include "engine/Graphics/ActiveScene.hpp"
#include <format>

namespace Runtime
{
    Engine::Utils::VoidResult GameApp::initialize(HINSTANCE hInstance, int nCmdShow)
    {
        Engine::Utils::log_info("Initializing Runtime Game...");

        // ============================================================
        // ProjectSettings を読み込む（assets/ 直下を優先して自動探索）
        // ============================================================
        auto& settings = Engine::Core::ProjectSettings::get();
        settings.loadForRuntime();

        const auto& wincfg = settings.getWindowConfig();

        Engine::Utils::log_info("ProjectName : " + settings.getProjectName());
        Engine::Utils::log_info("DefaultScene: " + settings.getDefaultScene());

        // ウィンドウの作成
        Engine::Core::WindowSettings windowSettings{};
        windowSettings.title = settings.getProjectNameW();   // ProjectName をタイトルに
        windowSettings.width = wincfg.width;
        windowSettings.height = wincfg.height;
        windowSettings.resizable = true;

        auto result = m_window.create(hInstance, windowSettings);
        if (!result) return result;

        m_window.setResizeCallback([this](int width, int height) {
            onWindowResize(width, height);
            });

        m_window.setCloseCallback([this]() {
            onWindowClose();
            m_running = false;
            });

        m_window.show(nCmdShow);

        // DirectX 初期化
        auto d3dResult = initD3D();
        if (!d3dResult)
        {
            Engine::Utils::log_error(d3dResult.error());
            return d3dResult;
        }

        Engine::Utils::log_info("Runtime initialization completed!");
        return {};
    }

    int GameApp::run()
    {
        Engine::Utils::log_info("Starting main loop...");

        while (m_window.processMessages())
        {
            update();
            render();
        }

        cleanup();
        return 0;
    }

    void GameApp::onWindowResize(int width, int height)
    {
        Engine::Utils::log_info(std::format("App::onWindowResize called: {}x{}", width, height));

        if (width <= 0 || height <= 0)
        {
            Engine::Utils::log_warning(std::format("Invalid resize dimensions: {}x{}", width, height));
            return;
        }

        if (!m_commandQueue || !m_swapChain || !m_fence)
        {
            Engine::Utils::log_info("DirectX 12 not initialized yet");
            if (height > 0)
            {
                m_camera.updateAspect(static_cast<float>(width) / height);
            }
            return;
        }

        Engine::Utils::log_info("Starting safe DirectX resize process...");

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

            for (UINT i = 0; i < 2; i++)
            {
                if (m_renderTargets[i]) m_renderTargets[i].Reset();
            }
            if (m_depthStencilBuffer) m_depthStencilBuffer.Reset();

            HRESULT hr = m_swapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
            if (FAILED(hr))
            {
                Engine::Utils::log_error(Engine::Utils::make_error(
                    Engine::Utils::ErrorType::SwapChainCreation,
                    std::format("Failed to resize swap chain: 0x{:08x}", static_cast<unsigned>(hr)), hr));
                return;
            }

            auto renderTargetResult = createRenderTargets();
            if (!renderTargetResult) { Engine::Utils::log_error(renderTargetResult.error()); return; }

            auto depthStencilResult = createDepthStencilBuffer();
            if (!depthStencilResult) { Engine::Utils::log_error(depthStencilResult.error()); return; }

            m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
            m_camera.updateAspect(static_cast<float>(width) / height);

            Engine::Utils::log_info("DirectX resize completed successfully");
        }
        catch (const std::exception& e)
        {
            Engine::Utils::log_error(Engine::Utils::make_error(
                Engine::Utils::ErrorType::Unknown,
                std::format("Exception during resize: {}", e.what())));
        }
        catch (...)
        {
            Engine::Utils::log_error(Engine::Utils::make_error(
                Engine::Utils::ErrorType::Unknown, "Unknown exception during resize"));
        }
    }

    void GameApp::onWindowClose()
    {
        Engine::Utils::log_info("Window close requested.");
    }

    Engine::Utils::VoidResult GameApp::initD3D()
    {
        Engine::Utils::log_info("Initializing DirectX 12...");

        // デバイス初期化
        Engine::Graphics::DeviceSettings deviceSettings{
            .enableDebugLayer = false,
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

        // ShaderManager 初期化
        m_shaderManager = std::make_unique<Engine::Graphics::ShaderManager>();
        auto shaderResult = m_shaderManager->initialize(&m_device);
        if (!shaderResult) return shaderResult;

        // Skybox 初期化
        auto skyboxResult = m_skybox.initialize(&m_device, m_shaderManager.get());
        if (!skyboxResult) return skyboxResult;

        // TextureManager 初期化
        auto textureResult = m_textureManager.initialize(&m_device);
        if (!textureResult) return textureResult;

        // Splash Screen 初期化
        Engine::Utils::log_info("Initializing Splash Screen...");
        auto splashResult = m_splashScreen.initialize(&m_device, m_shaderManager.get(), &m_textureManager);
        if (!splashResult)
        {
            Engine::Utils::log_warning("Failed to initialize splash screen, continuing anyway");
            m_showingSplash = false;
        }

        // MaterialManager 初期化
        auto materialResult = m_materialManager.initialize(&m_device);
        if (!materialResult) return materialResult;

        // Scene 初期化
        auto sceneResult = m_scene.initialize(&m_device);
        if (!sceneResult) return sceneResult;

        Engine::Graphics::setActiveScene(&m_scene);
        Engine::Utils::log_info("Active scene set in GameApp");

        // UITextRenderer 初期化
        Engine::Utils::log_info("Initializing UITextRenderer...");
        m_uiTextRenderer = std::make_unique<Engine::EngineUI::UITextRenderer>();
        auto uiTextResult = m_uiTextRenderer->initialize(&m_device, m_shaderManager.get());
        if (!uiTextResult)
        {
            Engine::Utils::log_error(uiTextResult.error());
            return uiTextResult;
        }

        // InputSystem 初期化
        Engine::Utils::log_info("Initializing InputSystem...");
        auto* inputManager = m_window.getInputManager();
        if (!inputManager || !inputManager->isInitialized())
        {
            return std::unexpected(Engine::Utils::make_error(
                Engine::Utils::ErrorType::Unknown,
                "InputManager not initialized"));
        }
        Engine::Input::InputSystem::get().setInputManager(inputManager);
        inputManager->resetMouseDelta();

        m_device.waitForGpu();

        // Lua 初期化
        m_luaBindings = std::make_unique<Engine::Scripting::LuaBindings>();
        auto& scriptMgr = Engine::Scripting::ScriptManager::get();
        scriptMgr.initialize();

        auto& lua = scriptMgr.getLuaState();

        auto registerAllBindings = [this](sol::state& lua) {
            m_luaBindings->registerBindings(lua);

            lua["Editor"] = lua.create_table();
            lua["Editor"]["restartGame"] = [this]() {
                Engine::Utils::log_info("Editor.restartGame() requested");
                m_pendingReloadScene = true;
                };
            };

        registerAllBindings(lua);
        scriptMgr.setBindingCallback(registerAllBindings);

        // シーン読み込み
        auto loadResult = loadScene();
        if (!loadResult) return loadResult;

        // カメラ初期化
        const auto [width, height] = m_window.getClientSize();
        m_camera.setPerspective(45.0f, static_cast<float>(width) / height, 0.1f, 100.0f);
        m_camera.setPosition({ 0.0f, 5.0f, 8.0f });
        m_camera.lookAt({ 0.0f, 0.0f, 0.0f });

        m_scene.start();

        Engine::Utils::log_info("DirectX 12 initialized successfully!");
        return {};
    }

    Engine::Utils::VoidResult GameApp::loadScene()
    {
        Engine::Utils::log_info("Loading scene...");

        auto& settings = Engine::Core::ProjectSettings::get();
        std::string scenePath = settings.getDefaultScene();

        if (!std::filesystem::exists(scenePath))
        {
            std::string fallback = "Assets/scenes/default.scene";
            if (std::filesystem::exists(fallback))
            {
                Engine::Utils::log_warning("Scene not found at '" + scenePath
                    + "', falling back to '" + fallback + "'");
                scenePath = fallback;
            }
        }

        auto result = m_sceneSerializer.loadScene(
            m_scene,
            &m_device,
            m_shaderManager.get(),
            &m_materialManager,
            &m_textureManager,
            scenePath
        );

        if (result)
        {
            Engine::Utils::log_info(std::format("Scene loaded: {} ({} objects)",
                scenePath, m_scene.getGameObjects().size()));
        }
        else
        {
            Engine::Utils::log_error(result.error());
            return result;
        }

        return {};
    }

    Engine::Utils::VoidResult GameApp::createCommandQueue()
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        CHECK_HR(m_device.getDevice()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)),
            Engine::Utils::ErrorType::DeviceCreation, "Failed to create command queue");
        return {};
    }

    Engine::Utils::VoidResult GameApp::createSwapChain()
    {
        const auto [width, height] = m_window.getClientSize();

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
        swapChainDesc.BufferCount = 2;
        swapChainDesc.Width = width;
        swapChainDesc.Height = height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;

        ComPtr<IDXGISwapChain1> swapChain1;
        CHECK_HR(m_device.getDXGIFactory()->CreateSwapChainForHwnd(
            m_commandQueue.Get(), m_window.getHandle(),
            &swapChainDesc, nullptr, nullptr, &swapChain1),
            Engine::Utils::ErrorType::SwapChainCreation, "Failed to create swap chain");

        CHECK_HR(swapChain1.As(&m_swapChain),
            Engine::Utils::ErrorType::SwapChainCreation, "Failed to cast swap chain");

        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
        return {};
    }

    Engine::Utils::VoidResult GameApp::createRenderTargets()
    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
        rtvHeapDesc.NumDescriptors = 2;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        CHECK_HR(m_device.getDevice()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)),
            Engine::Utils::ErrorType::ResourceCreation, "Failed to create RTV heap");

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
            m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        const UINT rtvDescriptorSize =
            m_device.getDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        for (UINT i = 0; i < 2; i++)
        {
            CHECK_HR(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])),
                Engine::Utils::ErrorType::ResourceCreation, "Failed to get swap chain buffer");
            m_device.getDevice()->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
            rtvHandle.ptr += rtvDescriptorSize;
        }
        return {};
    }

    Engine::Utils::VoidResult GameApp::createDepthStencilBuffer()
    {
        const auto [width, height] = m_window.getClientSize();

        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        CHECK_HR(m_device.getDevice()->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)),
            Engine::Utils::ErrorType::ResourceCreation, "Failed to create DSV heap");

        D3D12_CLEAR_VALUE depthClearValue{};
        depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        depthClearValue.DepthStencil.Depth = 1.0f;
        depthClearValue.DepthStencil.Stencil = 0;

        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC depthDesc{};
        depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthDesc.Width = width;
        depthDesc.Height = height;
        depthDesc.DepthOrArraySize = 1;
        depthDesc.MipLevels = 1;
        depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
        depthDesc.SampleDesc.Count = 1;
        depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        CHECK_HR(m_device.getDevice()->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &depthDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClearValue,
            IID_PPV_ARGS(&m_depthStencilBuffer)),
            Engine::Utils::ErrorType::ResourceCreation, "Failed to create depth stencil buffer");

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

        m_device.getDevice()->CreateDepthStencilView(
            m_depthStencilBuffer.Get(), &dsvDesc,
            m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
        return {};
    }

    Engine::Utils::VoidResult GameApp::createCommandObjects()
    {
        CHECK_HR(m_device.getDevice()->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)),
            Engine::Utils::ErrorType::ResourceCreation, "Failed to create command allocator");

        CHECK_HR(m_device.getDevice()->CreateCommandList(
            0, D3D12_COMMAND_LIST_TYPE_DIRECT,
            m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)),
            Engine::Utils::ErrorType::ResourceCreation, "Failed to create command list");

        m_commandList->Close();
        return {};
    }

    Engine::Utils::VoidResult GameApp::createSyncObjects()
    {
        CHECK_HR(m_device.getDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)),
            Engine::Utils::ErrorType::ResourceCreation, "Failed to create fence");

        m_fenceValue = 1;
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        CHECK_CONDITION(m_fenceEvent != nullptr,
            Engine::Utils::ErrorType::ResourceCreation, "Failed to create fence event");
        return {};
    }

    void GameApp::update()
    {
        updateDeltaTime();

        if (m_showingSplash)
        {
            m_showingSplash = m_splashScreen.update(m_deltaTime);
            return;
        }

        if (m_pendingReloadScene)
        {
            m_pendingReloadScene = false;
            Engine::Utils::log_info("Executing pending reloadScene...");
            m_isRestarting = true;

            m_device.waitForGpu();
            m_scene.clear();
            m_device.waitForGpu();

            Engine::Scripting::ScriptManager::get().reloadAll();

            auto result = loadScene();
            if (result)
            {
                Engine::Utils::log_info("Scene reloaded successfully");
                m_scene.start();
            }
            else
            {
                Engine::Utils::log_error(result.error());
            }

            m_isRestarting = false;
            return;
        }

        Engine::Scripting::ScriptManager::get().checkForUpdates();
        m_scene.update(m_deltaTime);
        m_scene.lateUpdate(m_deltaTime);
    }

    void GameApp::render()
    {
        if (m_isRestarting) return;

        if (m_uiTextRenderer) m_uiTextRenderer->beginFrame();

        m_commandAllocator->Reset();
        m_commandList->Reset(m_commandAllocator.Get(), nullptr);

        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        m_commandList->ResourceBarrier(1, &barrier);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
            m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        const UINT rtvDescriptorSize =
            m_device.getDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        rtvHandle.ptr += m_frameIndex * rtvDescriptorSize;

        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
            m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

        m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

        const float clearColor[] = { 0.1f, 0.1f, 0.15f, 1.0f };
        m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        const auto [width, height] = m_window.getClientSize();
        D3D12_VIEWPORT viewport{};
        viewport.Width = static_cast<float>(width);
        viewport.Height = static_cast<float>(height);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;

        D3D12_RECT scissorRect{};
        scissorRect.right = width;
        scissorRect.bottom = height;

        m_commandList->RSSetViewports(1, &viewport);
        m_commandList->RSSetScissorRects(1, &scissorRect);

        if (m_showingSplash)
        {
            m_splashScreen.render(m_commandList.Get());
        }
        else
        {
            m_skybox.render(m_commandList.Get(), m_camera);
            m_scene.render(m_commandList.Get(), m_camera, m_frameIndex);

            if (m_uiTextRenderer)
            {
                Engine::Utils::RenderContext context;
                context.commandList = m_commandList.Get();
                context.frameIndex = m_frameIndex;

                for (auto* text : m_scene.getUITexts())
                {
                    if (text && text->isVisible())
                    {
                        m_uiTextRenderer->draw(context, *text, width, height, &m_camera);
                    }
                }
            }
        }

        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        m_commandList->ResourceBarrier(1, &barrier);
        m_commandList->Close();

        ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
        m_commandQueue->ExecuteCommandLists(1, ppCommandLists);

        // VSync は ProjectSettings に従う
        const bool vsync = Engine::Core::ProjectSettings::get().getWindowConfig().vsync;
        m_swapChain->Present(vsync ? 1 : 0, 0);

        waitForPreviousFrame();
    }

    void GameApp::updateDeltaTime()
    {
        auto currentTime = std::chrono::high_resolution_clock::now();
        if (m_lastFrameTime.time_since_epoch().count() == 0)
        {
            m_lastFrameTime = currentTime;
        }
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - m_lastFrameTime);
        m_deltaTime = duration.count() / 1000000.0f;
        m_lastFrameTime = currentTime;
    }

    void GameApp::waitForPreviousFrame()
    {
        const UINT64 fence = m_fenceValue;
        m_commandQueue->Signal(m_fence.Get(), fence);
        m_fenceValue++;

        if (m_fence->GetCompletedValue() < fence)
        {
            m_fence->SetEventOnCompletion(fence, m_fenceEvent);
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }
        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    }

    void GameApp::cleanup()
    {
        waitForPreviousFrame();
        if (m_fenceEvent)
        {
            CloseHandle(m_fenceEvent);
            m_fenceEvent = nullptr;
        }
        Engine::Utils::log_info("Runtime cleaned up.");
    }

} // namespace Runtime


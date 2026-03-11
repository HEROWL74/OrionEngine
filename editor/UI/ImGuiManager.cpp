//src/UI/ImGuiManager.cpp
#include "ImGuiManager.hpp"
#include "ProjectWindow.hpp"  // AssetInfoを使う
#include "../engine/Scripting/LuaScriptComponent.hpp"
#include <format>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Editor::UI
{
	//================================================================
	//ImGuiManager実装
	//================================================================
	ImGuiManager::~ImGuiManager()
	{
		shutdown();
	}

	Utils::VoidResult ImGuiManager::initialize(Graphics::Device* device, HWND hwnd, ID3D12CommandQueue* commandQueue, DXGI_FORMAT rtvFormat, UINT frameCount)
	{
		if (m_initialized)
		{
			Utils::log_warning("ImGuiManager already initialized");
			return {};
		}

		CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
		CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");
		CHECK_CONDITION(commandQueue != nullptr, Utils::ErrorType::Unknown, "CommandQueue is null");
		CHECK_CONDITION(hwnd != nullptr, Utils::ErrorType::Unknown, "HWND is null");

		m_device = device;
		m_hwnd = hwnd;
		m_rtvFormat = rtvFormat;
		m_frameCount = frameCount;
		m_commandQueue = commandQueue;

		Utils::log_info("Initializing ImGui...");

		try
		{
			IMGUI_CHECKVERSION();
			m_context = ImGui::CreateContext();
			if (!m_context)
			{
				return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Failed to create ImGui context"));
			}

			ImGui::SetCurrentContext(m_context);

			// ImGui設定
			ImGuiIO& io = ImGui::GetIO();
			io.IniFilename = nullptr;
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

			// フォントセット
			io.Fonts->AddFontDefault();

			ImGui::StyleColorsDark();
			createGUIStyle();

			auto heapResult = createDescriptorHeap();
			if (!heapResult)
			{
				ImGui::DestroyContext(m_context);
				m_context = nullptr;
				return heapResult;
			}

			m_nextFreeIndex = 1;

			if (!ImGui_ImplWin32_Init(hwnd))
			{
				ImGui::DestroyContext(m_context);
				m_context = nullptr;
				return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Failed to initialize ImGui Win32"));
			}

			if (!m_commandQueue)
			{
				ImGui_ImplWin32_Shutdown();
				ImGui::DestroyContext(m_context);
				m_context = nullptr;
				return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "CommandQueue became null before DX12 init"));
			}

			if (!ImGui_ImplDX12_Init(
				m_device->getDevice(),
				static_cast<int>(frameCount),
				rtvFormat,
				m_srvDescHeap.Get(),
				m_srvDescHeap->GetCPUDescriptorHandleForHeapStart(),
				m_srvDescHeap->GetGPUDescriptorHandleForHeapStart()))
			{
				ImGui_ImplWin32_Shutdown();
				ImGui::DestroyContext(m_context);
				m_context = nullptr;
				return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Failed to initialize ImGui DX12"));
			}

			auto fontResult = createFontTextureManually();
			if (!fontResult)
			{
				ImGui_ImplDX12_Shutdown();
				ImGui_ImplWin32_Shutdown();
				ImGui::DestroyContext(m_context);
				m_context = nullptr;
				return fontResult;
			}

			RECT rect;
			if (GetClientRect(hwnd, &rect))
			{
				io.DisplaySize = ImVec2(static_cast<float>(rect.right - rect.left),
					static_cast<float>(rect.bottom - rect.top));
			}

			m_initialized = true;
			Utils::log_info("ImGui initialized successfully!");
			return {};
		}
		catch (const std::exception& e)
		{
			Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
				std::format("Exception during ImGui initialization: {}", e.what())));
			shutdown();
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "ImGui initialization failed"));
		}
		catch (...)
		{
			Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "Unknown exception during ImGui initialization"));
			shutdown();
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "ImGui initialization failed"));
		}
	}

	Utils::VoidResult ImGuiManager::createFontTextureManually()
	{
		if (!m_commandQueue)
		{
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "CommandQueue is null in createFontTextureManually"));
		}

		if (!m_device || !m_device->getDevice())
		{
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Device is null in createFontTextureManually"));
		}

		Utils::log_info("Creating font texture manually...");

		try
		{
			ImGuiIO& io = ImGui::GetIO();
			unsigned char* pixels;
			int width, height;
			io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

			ComPtr<ID3D12CommandAllocator> commandAllocator;
			ComPtr<ID3D12GraphicsCommandList> commandList;

			CHECK_HR(m_device->getDevice()->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)),
				Utils::ErrorType::ResourceCreation, "Failed to create font command allocator");

			CHECK_HR(m_device->getDevice()->CreateCommandList(
				0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr,
				IID_PPV_ARGS(&commandList)),
				Utils::ErrorType::ResourceCreation, "Failed to create font command list");

			
			if (!m_commandQueue)
			{
				return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "CommandQueue became null before CreateDeviceObjects"));
			}

			if (!ImGui_ImplDX12_CreateDeviceObjects())
			{
				Utils::log_warning("ImGui_ImplDX12_CreateDeviceObjects failed, but continuing");
			}

			commandList->Close();

			if (!m_commandQueue)
			{
				return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "CommandQueue became null before execution"));
			}

			ID3D12CommandList* cmdLists[] = { commandList.Get() };
			m_commandQueue->ExecuteCommandLists(1, cmdLists);

			ComPtr<ID3D12Fence> fence;
			CHECK_HR(m_device->getDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)),
				Utils::ErrorType::ResourceCreation, "Failed to create font fence");

			HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			CHECK_CONDITION(fenceEvent != nullptr, Utils::ErrorType::ResourceCreation, "Failed to create fence event");

			const UINT64 fenceValue = 1;

			if (!m_commandQueue)
			{
				CloseHandle(fenceEvent);
				return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "CommandQueue became null before signal"));
			}

			m_commandQueue->Signal(fence.Get(), fenceValue);
			fence->SetEventOnCompletion(fenceValue, fenceEvent);
			WaitForSingleObject(fenceEvent, INFINITE);
			CloseHandle(fenceEvent);

			Utils::log_info("Font texture created manually successfully");
			return {};
		}
		catch (const std::exception& e)
		{
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
				std::format("Exception in createFontTextureManually: {}", e.what())));
		}
		catch (...)
		{
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Unknown exception in createFontTextureManually"));
		}
	}

	void ImGuiManager::newFrame()
	{
		if (!m_initialized || !m_context)
		{
			Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "ImGuiManager not initialized in newFrame"));
			return;
		}

		if (m_hwnd)
		{
			RECT rect;
			if (GetClientRect(m_hwnd, &rect))
			{
				ImGuiIO& io = ImGui::GetIO();
				float width = static_cast<float>(rect.right - rect.left);
				float height = static_cast<float>(rect.bottom - rect.top);

				if (width > 0 && height > 0)
				{
					io.DisplaySize = ImVec2(width, height);
					io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
				}
			}
		}

		try
		{
			if (!m_initialized || !m_context)
				return;

			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
		}
		catch (const std::exception& e)
		{
			Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
				std::format("Exception in ImGui newFrame: {}", e.what())));
			throw; 
		}
		catch (...)
		{
			Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
				"Unknown exception in ImGui newFrame"));
			throw; 
		}
	}



	void ImGuiManager::shutdown()
	{
		if (!m_initialized)
		{
			return;
		}

		Utils::log_info("Shutting down ImGui...");

		if (m_context)
		{
			ImGui::SetCurrentContext(m_context);
		}

		try
		{
			ImGui_ImplDX12_Shutdown();
		}
		catch (...)
		{
			Utils::log_warning("Exception during ImGui_ImplDX12_Shutdown");
		}

		try
		{
			ImGui_ImplWin32_Shutdown();
		}
		catch (...)
		{
			Utils::log_warning("Exception during ImGui_ImplWin32_Shutdown");
		}

		if (m_context)
		{
			ImGui::DestroyContext(m_context);
			m_context = nullptr;
		}

		m_srvDescHeap.Reset();
		m_commandQueue = nullptr; 
		m_device = nullptr;
		m_hwnd = nullptr;
		m_initialized = false;

		Utils::log_info("ImGui shutdown completed");
	}



	void ImGuiManager::render(ID3D12GraphicsCommandList* commandList) const
	{
		if (!m_initialized || !m_context)
		{
			Utils::log_warning("ImGuiManager not initialized, skipping render");
			return;
		}

		ImGuiContext* currentContext = ImGui::GetCurrentContext();
		if (currentContext != m_context)
		{
			ImGui::SetCurrentContext(m_context);
		}

		ImGui::Render();

		ID3D12DescriptorHeap* descriptorHeaps[] = { m_srvDescHeap.Get() };
		commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
	}

	bool ImGuiManager::handleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) const
	{
		if (!m_initialized || !m_context)
		{
			return false;
		}

		ImGuiContext* currentContext = ImGui::GetCurrentContext();
		if (currentContext != m_context)
		{
			ImGui::SetCurrentContext(m_context);
		}

		ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
	}

	Utils::VoidResult ImGuiManager::createDescriptorHeap()
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc{};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = 128;//ImGui
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		CHECK_HR(m_device->getDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_srvDescHeap)),
			Utils::ErrorType::ResourceCreation, "Failed to create ImGui descriptor heap");

		m_srvIncSize = m_device->getDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_descriptorSize = m_srvIncSize;
		m_srvCpuStart = m_srvDescHeap->GetCPUDescriptorHandleForHeapStart();
		m_srvGpuStart = m_srvDescHeap->GetGPUDescriptorHandleForHeapStart();
		m_maxSrv = desc.NumDescriptors;

		return {};
	}
	void ImGuiManager::onWindowResize(int width, int height)
	{
		Utils::log_info(std::format("ImGuiManager::onWindowResize: {}x{}", width, height));

		if (!m_initialized || !m_context)
		{
			Utils::log_warning("ImGuiManager not properly initialized");
			return;
		}

		if (width <= 0 || height <= 0)
		{
			Utils::log_warning(std::format("Invalid ImGui resize dimensions: {}x{}", width, height));
			return;
		}

		ImGuiContext* savedContext = ImGui::GetCurrentContext();
		ImGui::SetCurrentContext(m_context);

		try
		{
			ImGuiIO& io = ImGui::GetIO();
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // ドッキングを有効化
			io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // 複数ウィンドウ分離も可能

			if (std::abs(io.DisplaySize.x - width) > 1.0f || std::abs(io.DisplaySize.y - height) > 1.0f)
			{
				io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
				io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

				Utils::log_info(std::format("ImGui display size updated to: {}x{}", width, height));
			}
		}
		catch (...)
		{
			Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "Exception in ImGui resize"));
		}


		if (savedContext)
		{
			ImGui::SetCurrentContext(savedContext);
		}
	}


	void ImGuiManager::invalidateDeviceObjects()
	{
		Utils::log_info("invalidateDeviceObjects called - use reinitializeForResize instead");
	}

	void ImGuiManager::createDeviceObjects()
	{
		Utils::log_info("createDeviceObjects called - use reinitializeForResize instead");
	}
	Utils::VoidResult ImGuiManager::reinitializeForResize()
	{
		if (!m_initialized || !m_device || !m_commandQueue)
		{
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
				"ImGuiManager not properly initialized for reinitialize"));
		}

		Utils::log_info("Reinitializing ImGui for resize...");

	    ImGui::SetCurrentContext(m_context);


		ImGui_ImplDX12_Shutdown();


		if (!ImGui_ImplDX12_Init(
			m_device->getDevice(),
			static_cast<int>(m_frameCount),
			m_rtvFormat,
			m_srvDescHeap.Get(),
			m_srvDescHeap->GetCPUDescriptorHandleForHeapStart(),
			m_srvDescHeap->GetGPUDescriptorHandleForHeapStart()))
		{
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
				"Failed to reinitialize ImGui DX12"));
		}


		auto fontResult = createFontTextureManually();
		if (!fontResult)
		{
			return fontResult;
		}

		Utils::log_info("ImGui reininitialized successfully for resize");
		return {};
	}

	ImTextureID ImGuiManager::registerTexture(Graphics::Texture* tex)
	{
		if (!tex || !m_device || !m_srvDescHeap || !m_initialized)
		{
			Utils::log_warning("Cannot register texture - ImGuiManager not properly initialized or texture is null");
			return 0;
		}

		// 空きスロット確保
		if (m_nextFreeIndex >= m_maxSrv)
		{
			Utils::log_warning("ImGui descriptor heap is full, cannot register more textures");
			return 0;
		}

		const UINT idx = m_nextFreeIndex++;

		// この"ImGui用ヒープ"の CPU/GPU ハンドルを計算
		D3D12_CPU_DESCRIPTOR_HANDLE cpu = m_srvCpuStart;
		cpu.ptr += static_cast<SIZE_T>(idx) * m_srvIncSize;
		D3D12_GPU_DESCRIPTOR_HANDLE gpu = m_srvGpuStart;
		gpu.ptr += static_cast<UINT64>(idx) * m_srvIncSize;

		// SRV を"ImGui用ヒープ"に作る
		D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
		auto resDesc = tex->getResource()->GetDesc();
		srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv.Format = resDesc.Format;
		srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv.Texture2D.MipLevels = tex->getMipLevels();

		m_device->getDevice()->CreateShaderResourceView(tex->getResource(), &srv, cpu);

		// GPUハンドル値をImTextureIDとして返す
		ImTextureID result = (ImTextureID)(intptr_t)gpu.ptr;

		Utils::log_info(std::format(
			"RegisterTexture: name={} format={} mipLevels={} cpu.ptr={} gpu.ptr={}",
			tex->getDesc().debugName,
			(int)resDesc.Format,
			tex->getMipLevels(),
			(uint64_t)cpu.ptr,
			(uint64_t)gpu.ptr));

		return result;
	}

	ImTextureID ImGuiManager::registerRenderTarget(
		ID3D12Resource* texture,
		DXGI_FORMAT format)
	{
		if (!texture || !m_device || !m_srvDescHeap || !m_initialized)
		{
			Utils::log_warning("Cannot register render target - invalid parameters");
			return -1;
		}

		if (m_nextFreeIndex >= m_maxSrv)
		{
			Utils::log_warning("ImGui descriptor heap is full");
			return -1;
		}

		const UINT idx = m_nextFreeIndex++;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MipLevels = 1;

		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(
			m_srvDescHeap->GetCPUDescriptorHandleForHeapStart(),
			idx,
			m_srvIncSize  // m_descriptorSize ではなく m_srvIncSize を使用
		);

		m_device->getDevice()->CreateShaderResourceView(
			texture,
			&srvDesc,
			cpuHandle
		);

		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(
			m_srvDescHeap->GetGPUDescriptorHandleForHeapStart(),
			idx,
			m_srvIncSize  // m_descriptorSize ではなく m_srvIncSize を使用
		);

		ImTextureID result = static_cast<ImTextureID>(gpuHandle.ptr);

		Utils::log_info(std::format(
			"RegisterRenderTarget: idx={} format={} gpu.ptr=0x{:016X}",
			idx, static_cast<int>(format), gpuHandle.ptr));

		return result;
	}


	void ImGuiManager::createGUIStyle()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4* colors = style.Colors;

		// === ベースカラー ===
		colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.11f, 0.11f, 1.0f); // #1C1C1C
		colors[ImGuiCol_ChildBg] = ImVec4(0.11f, 0.11f, 0.11f, 1.0f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.0f);

		// === フレーム ===
		colors[ImGuiCol_Border] = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);

		// === ボタン ===
		colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.40f, 0.65f, 1.0f); // ブルー寄り
		colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.45f, 0.70f, 1.0f);

		// === ヘッダー（TreeNode, CollapsingHeader, Selectable）===
		colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.40f, 0.65f, 1.0f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.45f, 0.70f, 1.0f);

		// === タブ ===
		colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.25f, 0.40f, 0.65f, 1.0f);
		colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.45f, 0.70f, 1.0f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.45f, 0.70f, 1.0f);

		// === スクロールバー ===
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.11f, 0.11f, 0.11f, 1.0f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);

		// === スライダー・チェックボックスなど ===
		colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.40f, 0.65f, 1.0f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.45f, 0.70f, 1.0f);

		// === テキスト ===
		colors[ImGuiCol_Text] = ImVec4(0.85f, 0.85f, 0.85f, 1.0f); // 明るいグレー
		colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);

		// === スタイルパラメータ ===
		style.WindowRounding = 0.0f;
		style.FrameRounding = 2.0f;
		style.ScrollbarRounding = 3.0f;
		style.GrabRounding = 2.0f;
	}

	void ImGuiManager::clearRenderTargetDescriptors()
	{
		if (m_device && m_device->getSrvHeap())
		{
			Utils::log_info("Clearing ImGui render target descriptors");
			// ここでは何もしない（次のregisterRenderTargetで上書きされる）
		}
	}


	//=====================================================================
	//DebugWindow
	//=====================================================================
	void DebugWindow::draw()
	{
		if (!m_visible) return;

		if (ImGui::Begin(m_title.c_str(), &m_visible))
		{
			// ====== Play Controls ======
			ImGui::Text("Play Controls");
			ImGui::Separator();

			if (ImGui::Button("Play")) {
				if (m_playModeController) {
					m_playModeController->play();
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Pause")) {
				if (m_playModeController) {
					m_playModeController->pause();
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Stop")) {
				if (m_playModeController) {
					m_playModeController->stop();
				}
			}

			ImGui::Spacing();

			// ====== Performance ======
			ImGui::Text("Performance");
			ImGui::Separator();
			ImGui::Text("FPS: %.1f", m_fps);
			ImGui::Text("Frame Time: %.3f ms", m_frameTime * 1000.0f);

			ImGui::Spacing();
			ImGui::Text("Scene Info");
			ImGui::Separator();
			ImGui::Text("Object: %zu", m_objectCount);

			ImGui::Spacing();
			ImGui::Text("Controls");
			ImGui::Separator();
			ImGui::Text("WASD: Move camera");
			ImGui::Text("Mouse: Look around");
			ImGui::Text("F1: Toggle mouse mode");
			ImGui::Text("ESC: Exit");
		}
		ImGui::End();
	}

	//======================================================================
	//Scene HierarchyWindow実装
	//======================================================================
	SceneHierarchyWindow::SceneHierarchyWindow() : ImGuiWindow("Scene Hierarchy")
	{
		m_contextMenu = std::make_unique<ContextMenu>();
	}

	void SceneHierarchyWindow::draw()
	{
		if (!m_visible || !m_scene) return;

		if (ImGui::Begin(m_title.c_str(), &m_visible))
		{
			Core::GameObject* currentSelection = m_scene->getSelectedObject();

			if (currentSelection && currentSelection->isDestroyed())
			{
				m_scene->clearSelection();
				currentSelection = nullptr;
			}

			// GameObjectsセクション
			ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "GameObjects");
			ImGui::Separator();

			const auto& gameObjects = m_scene->getGameObjects();
			for (const auto& gameObject : gameObjects)
			{
				if (gameObject && gameObject->isActive() && !gameObject->isDestroyed())
				{
					drawGameObject(gameObject.get(), currentSelection);
				}
			}

			ImGui::Spacing();
			ImGui::Spacing();

			// 空白エリアのコンテキストメニュー
			if (ImGui::BeginPopupContextWindow("HierarchyContextMenu",
				ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
			{
				if (m_contextMenu)
				{
					m_contextMenu->drawCreateMenu();
				}
				ImGui::EndPopup();
			}
		}
		ImGui::End();

		if (m_contextMenu)
		{
			m_contextMenu->drawModals();
		}
	}



	void SceneHierarchyWindow::drawUIText(Engine::EngineUI::UIText* text, int index)
	{
		if (!text) return;

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

		// 選択状態の判定
		if (m_selectedUIText == text)
		{
			flags |= ImGuiTreeNodeFlags_Selected;
		}

		std::string label = text->getName() + "##uitext_" + std::to_string(index);
		ImGui::TreeNodeEx(label.c_str(), flags);

		// クリックされた場合
		if (ImGui::IsItemClicked())
		{
			// GameObjectの選択をクリア
			if (m_scene)
			{
				m_scene->clearSelection();
			}

			// UITextを選択
			m_selectedUIText = text;

			// Inspectorに通知
			if (m_onUISelectionChanged)
			{
				m_onUISelectionChanged(text);
			}

			Utils::log_info(std::format("Selected UIText: {}", text->getName()));
		}

		// コンテキストメニュー
		if (ImGui::BeginPopupContextItem())
		{
			if (m_contextMenu)
			{
				m_contextMenu->drawUITextContextMenu(text);
			}
			ImGui::EndPopup();
		}
	}

	void SceneHierarchyWindow::setCreateUIElementCallback(
		std::function<Engine::EngineUI::UIText* (UI::UIElementType, const std::string&)> callback)
	{
		if (m_contextMenu)
		{
			m_contextMenu->setCreateUIElementCallback(callback);
		}
	}

	void SceneHierarchyWindow::setDeleteUITextCallback(
		std::function<void(Engine::EngineUI::UIText*)> callback)
	{
		if (m_contextMenu)
		{
			m_contextMenu->setDeleteUITextCallback(callback);
		}
	}

	void SceneHierarchyWindow::setRenameUITextCallback(
		std::function<void(Engine::EngineUI::UIText*, const std::string&)> callback)
	{
		if (m_contextMenu)
		{
			m_contextMenu->setRenameUITextCallback(callback);
		}
	}

	void SceneHierarchyWindow::drawGameObject(Core::GameObject* gameObject, Core::GameObject* currentSelection)
	{
		if (!gameObject || gameObject->isDestroyed())
		{
			return;
		}

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;

		// Sceneの選択状態を反映
		if (currentSelection == gameObject)
		{
			flags |= ImGuiTreeNodeFlags_Selected;
		}

		if (gameObject->getChildren().empty())
		{
			flags |= ImGuiTreeNodeFlags_Leaf;
		}

		std::string nodeName = gameObject->getName();

		// UITextコンポーネントがあればアイコンを表示
		auto* uiTextComponent = gameObject->getComponent<Engine::EngineUI::UIText>();
		if (uiTextComponent)
		{
			nodeName = "[T] " + nodeName; // Text icon
		}

		ImGui::PushID(gameObject);
		bool nodeOpen = ImGui::TreeNodeEx(nodeName.c_str(), flags);

		if (ImGui::IsItemClicked() && !gameObject->isDestroyed())
		{
			// Sceneの選択状態を更新
			m_scene->setSelectedObject(gameObject);

			if (uiTextComponent)
			{
				Utils::log_info(std::format("Selected GameObject with UIText: {}", gameObject->getName()));

				// UITextコンポーネントをInspectorに表示するため、UIText選択として扱う
				m_selectedUIText = uiTextComponent;

				// InspectorにUITextを通知
				if (m_onUISelectionChanged)
				{
					m_onUISelectionChanged(uiTextComponent);
				}
			}
			else
			{
				Utils::log_info(std::format("Selected GameObject: {}", gameObject->getName()));

				// 通常のGameObject選択
				m_selectedUIText = nullptr;

				// InspectorにGameObjectを通知
				if (m_onSelectionChanged)
				{
					m_onSelectionChanged(gameObject);
				}

				// InspectorのUIText選択をクリア
				if (m_onUISelectionChanged)
				{
					m_onUISelectionChanged(nullptr);
				}
			}
		}

		// コンテキストメニュー
		if (m_contextMenu && !gameObject->isDestroyed())
		{
			m_contextMenu->drawGameObjectContextMenu(gameObject);
		}

		// 子オブジェクトの描画
		if (nodeOpen)
		{
			for (const auto& child : gameObject->getChildren())
			{
				if (child && child->isActive() && !child->isDestroyed())
				{
					drawGameObject(child.get(), currentSelection);
				}
			}
			ImGui::TreePop();
		}

		ImGui::PopID();
	}

	void SceneHierarchyWindow::setSelectionChangedCallback(std::function<void(Core::GameObject*)> callback)
	{
		m_onSelectionChanged = callback;
	}

	void SceneHierarchyWindow::setCreateObjectCallback(std::function<Core::GameObject* (UI::PrimitiveType, const std::string&)> callback)
	{
		if (m_contextMenu)
		{
			m_contextMenu->setCreateObjectCallback(callback);
		}
	}

	void SceneHierarchyWindow::setDeleteObjectCallback(std::function<void(Core::GameObject*)> callback)
	{
		if (m_contextMenu)
		{
			m_contextMenu->setDeleteObjectCallback(callback);
		}
	}

	void SceneHierarchyWindow::setDuplicateObjectCallback(std::function<Core::GameObject* (Core::GameObject*)> callback)
	{
		if (m_contextMenu)
		{
			m_contextMenu->setDuplicateObjectCallback(callback);
		}
	}

	void SceneHierarchyWindow::setRenameObjectCallback(std::function<void(Core::GameObject*, const std::string&)> callback)
	{
		if (m_contextMenu)
		{
			m_contextMenu->setRenameObjectCallback(callback);
		}
	}

	//=======================================================================
	//InspectorWindow
	//=======================================================================
	void InspectorWindow::draw()
	{
		if (!ImGui::Begin("Inspector", nullptr))
		{
			ImGui::End();
			return;
		}

		if (m_selectedUIText)
		{
			Utils::log_info("Drawing UIText Inspector");
			drawUITextInspector();
			ImGui::End();
			return;
		}

		if (m_selectedObject)
		{
			Utils::log_info(std::format("Drawing GameObject Inspector: {}", m_selectedObject->getName()));
			drawGameObjectInspector();
			ImGui::End();
			return;
		}

		ImGui::TextDisabled("No object selected");
		ImGui::End();
	}

	void InspectorWindow::drawGameObjectInspector()
	{
		if (!m_selectedObject) return;

		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.6f, 0.8f, 1.0f));

		// GameObject名前表示
		ImGui::Text("GameObject: %s", m_selectedObject->getName().c_str());
		ImGui::Separator();

		// Active状態
		bool active = m_selectedObject->isActive();
		if (ImGui::Checkbox("Active", &active))
		{
			m_selectedObject->setActive(active);
		}

		ImGui::Spacing();

		// Transformコンポーネント
		auto* transform = m_selectedObject->getTransform();
		if (transform)
		{
			drawTransformComponent(transform);
		}

		// RenderComponentがあれば表示
		if (m_scene)
		{
			const auto* entry = m_scene->getRenderBatch().findEntry(m_selectedObject->getId());
			if (entry)
				drawRenderEntry(m_selectedObject);
		}

		// BoxColliderがあれば表示
		auto* boxCollider = m_selectedObject->getComponent<Physics::BoxCollider>();
		if (boxCollider)
		{
			drawBoxColliderComponent(boxCollider);
		}

		// LuaScriptComponentがあれば表示
		auto* scriptComponent = m_selectedObject->getComponent<Scripting::LuaScriptComponent>();

		// AudioComponentがあれば表示
		auto* audioComponent = m_selectedObject->getComponent<Audio::AudioComponent>();
		if (audioComponent)
		{
			drawAudioComponent(audioComponent);
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (ImGui::CollapsingHeader("Add Component", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// BoxCollider追加ボタン
			if (!boxCollider)
			{
				if (ImGui::Button("Box Collider", ImVec2(-1, 30)))
				{
					auto* newCollider = m_selectedObject->addComponent<Physics::BoxCollider>();
					if (newCollider)
					{
						newCollider->setSize(Math::Vector3(1.0f, 1.0f, 1.0f));
						Utils::log_info(std::format("BoxCollider added to {}", m_selectedObject->getName()));
					}
				}
			}

			if (!audioComponent)
			{
				if (ImGui::Button("Audio", ImVec2(-1, 30)))
				{
					auto* newAudio = m_selectedObject->addComponent<Audio::AudioComponent>();
					if (newAudio)
					{
						auto initResult = newAudio->initialize();
						if (initResult)
						{
							Utils::log_info(std::format("AudioComponent added to {}", m_selectedObject->getName()));
						}
						else
						{
							Utils::log_error(initResult.error());
						}
					}
				}
			}
		}

		ImGui::Spacing();

		if (ImGui::CollapsingHeader("Attach Script", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (!scriptComponent)
			{
				ImGui::BeginChild("##ScriptDropZone", ImVec2(-1, 80), true);
				ImGui::TextWrapped("Drop Lua Script Here");
				ImGui::TextDisabled("(or drag anywhere in Inspector)");

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
					{
						const AssetPayload* dropped = static_cast<const AssetPayload*>(payload->Data);
						if (dropped && dropped->type == static_cast<int>(UI::AssetInfo::Type::Script))
						{
							auto* newScript = m_selectedObject->addComponent<Engine::Scripting::LuaScriptComponent>(dropped->path);
							if (newScript)
							{
								Utils::log_info(std::format("Lua script attached: {}", dropped->path));
							}
						}
					}
					ImGui::EndDragDropTarget();
				}
				ImGui::EndChild();
			}
			else
			{
				std::string scriptPath = scriptComponent->getScriptPath();
				std::filesystem::path path(scriptPath);
				std::string fileName = path.filename().string();

				ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "Attached Script:");
				ImGui::SameLine();
				ImGui::Text("%s", fileName.c_str());

				ImGui::TextDisabled("Full Path:");
				ImGui::TextWrapped("%s", scriptPath.c_str());

				ImGui::Spacing();

				ImGui::BeginChild("##ScriptReplaceZone", ImVec2(-1, 60), true);
				ImGui::TextWrapped("Drop new script to replace");

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
					{
						const AssetPayload* dropped = static_cast<const AssetPayload*>(payload->Data);
						if (dropped && dropped->type == static_cast<int>(UI::AssetInfo::Type::Script))
						{
							// 既存のスクリプトを削除して新しいものを追加
							m_selectedObject->removeComponent<Engine::Scripting::LuaScriptComponent>();
							auto* newScript = m_selectedObject->addComponent<Engine::Scripting::LuaScriptComponent>(dropped->path);
							if (newScript)
							{
								Utils::log_info(std::format("Lua script replaced: {}", dropped->path));
							}
						}
					}
					ImGui::EndDragDropTarget();
				}
				ImGui::EndChild();
			}
		}

		if (!scriptComponent && ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
			{
				const AssetPayload* dropped = static_cast<const AssetPayload*>(payload->Data);
				if (dropped && dropped->type == static_cast<int>(UI::AssetInfo::Type::Script))
				{
					auto* newScript = m_selectedObject->addComponent<Engine::Scripting::LuaScriptComponent>(dropped->path);
					if (newScript)
					{
						Utils::log_info(std::format("Lua script attached (anywhere): {}", dropped->path));
					}
				}
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::PopStyleColor();
	}

	void InspectorWindow::drawUITextInspector()
	{
		if (!m_selectedUIText) return;

		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.6f, 0.8f, 1.0f));

		// UIText名前表示
		ImGui::Text("UIText: %s", m_selectedUIText->getName().c_str());
		ImGui::Separator();

		// GameObject情報も表示
		auto* gameObject = m_selectedUIText->getGameObject();
		if (gameObject)
		{
			ImGui::TextDisabled("GameObject: %s", gameObject->getName().c_str());
			ImGui::Spacing();
		}

		// Transformセクション
		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
		{
			auto pos = m_selectedUIText->getPosition();
			auto rot = m_selectedUIText->getRotation();
			auto scale = m_selectedUIText->getScale();

			float position[3] = { pos.x, pos.y, pos.z };
			if (ImGui::DragFloat3("Position", position, 0.1f))
			{
				m_selectedUIText->setPosition(Math::Vector3(position[0], position[1], position[2]));
			}

			float rotation[3] = { rot.x, rot.y, rot.z };
			if (ImGui::DragFloat3("Rotation", rotation, 1.0f))
			{
				m_selectedUIText->setRotation(Math::Vector3(rotation[0], rotation[1], rotation[2]));
			}

			float scaleArr[3] = { scale.x, scale.y, scale.z };
			if (ImGui::DragFloat3("Scale", scaleArr, 0.001f, 0.001f, 10.0f))
			{
				m_selectedUIText->setScale(Math::Vector3(scaleArr[0], scaleArr[1], scaleArr[2]));
			}
		}

		// Textセクション
		if (ImGui::CollapsingHeader("Text Properties", ImGuiTreeNodeFlags_DefaultOpen))
		{
			static char textBuffer[256];
			std::string currentText = m_selectedUIText->getText();
			strncpy_s(textBuffer, currentText.c_str(), sizeof(textBuffer) - 1);

			if (ImGui::InputTextMultiline("Text", textBuffer, sizeof(textBuffer), ImVec2(-1, 80)))
			{
				m_selectedUIText->setText(textBuffer);
			}

			float fontSize = m_selectedUIText->getFontSize();
			if (ImGui::DragFloat("Font Size", &fontSize, 1.0f, 8.0f, 128.0f))
			{
				m_selectedUIText->setFontSize(fontSize);
			}

			auto color = m_selectedUIText->getColor();
			float colorArr[3] = { color.x, color.y, color.z };
			if (ImGui::ColorEdit3("Color", colorArr))
			{
				m_selectedUIText->setColor(Math::Vector3(colorArr[0], colorArr[1], colorArr[2]));
			}

			float alpha = m_selectedUIText->getAlpha();
			if (ImGui::SliderFloat("Alpha", &alpha, 0.0f, 1.0f))
			{
				m_selectedUIText->setAlpha(alpha);
			}
		}

		// Visibility
		if (ImGui::CollapsingHeader("Visibility", ImGuiTreeNodeFlags_DefaultOpen))
		{
			bool visible = m_selectedUIText->isVisible();
			if (ImGui::Checkbox("Visible", &visible))
			{
				m_selectedUIText->setVisible(visible);
			}

			bool enabled = m_selectedUIText->isEnabled();
			if (ImGui::Checkbox("Enabled", &enabled))
			{
				m_selectedUIText->setEnabled(enabled);
			}
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (gameObject)
		{
			if (ImGui::CollapsingHeader("Add Component", ImGuiTreeNodeFlags_DefaultOpen))
			{
				auto* boxCollider = gameObject->getComponent<Physics::BoxCollider>();
				if (!boxCollider)
				{
					if (ImGui::Button("Box Collider", ImVec2(-1, 30)))
					{
						auto* newCollider = gameObject->addComponent<Physics::BoxCollider>();
						if (newCollider)
						{
							newCollider->setSize(Math::Vector3(1.0f, 1.0f, 1.0f));
							Utils::log_info(std::format("BoxCollider added to {}", gameObject->getName()));
						}
					}
				}
			}

			ImGui::Spacing();

			auto* scriptComponent = gameObject->getComponent<Scripting::LuaScriptComponent>();

			if (ImGui::CollapsingHeader("Attach Script", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (!scriptComponent)
				{
					ImGui::BeginChild("##ScriptDropZone", ImVec2(-1, 80), true);
					ImGui::TextWrapped("Drop Lua Script Here");
					ImGui::TextDisabled("(or drag anywhere in Inspector)");

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
						{
							const AssetPayload* dropped = static_cast<const AssetPayload*>(payload->Data);
							if (dropped && dropped->type == static_cast<int>(UI::AssetInfo::Type::Script))
							{
								auto* newScript = gameObject->addComponent<Engine::Scripting::LuaScriptComponent>(dropped->path);
								if (newScript)
								{
									Utils::log_info(std::format("Lua script attached: {}", dropped->path));
								}
							}
						}
						ImGui::EndDragDropTarget();
					}
					ImGui::EndChild();
				}
				else
				{
					std::string scriptPath = scriptComponent->getScriptPath();
					std::filesystem::path path(scriptPath);
					std::string fileName = path.filename().string();

					ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "Attached Script:");
					ImGui::SameLine();
					ImGui::Text("%s", fileName.c_str());

					ImGui::TextDisabled("Full Path:");
					ImGui::TextWrapped("%s", scriptPath.c_str());

					ImGui::Spacing();

					ImGui::BeginChild("##ScriptReplaceZone", ImVec2(-1, 60), true);
					ImGui::TextWrapped("Drop new script to replace");

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
						{
							const AssetPayload* dropped = static_cast<const AssetPayload*>(payload->Data);
							if (dropped && dropped->type == static_cast<int>(UI::AssetInfo::Type::Script))
							{
								gameObject->removeComponent<Engine::Scripting::LuaScriptComponent>();
								auto* newScript = gameObject->addComponent<Engine::Scripting::LuaScriptComponent>(dropped->path);
								if (newScript)
								{
									Utils::log_info(std::format("Lua script replaced: {}", dropped->path));
								}
							}
						}
						ImGui::EndDragDropTarget();
					}
					ImGui::EndChild();
				}
			}

			if (!scriptComponent && ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
				{
					const AssetPayload* dropped = static_cast<const AssetPayload*>(payload->Data);
					if (dropped && dropped->type == static_cast<int>(UI::AssetInfo::Type::Script))
					{
						auto* newScript = gameObject->addComponent<Engine::Scripting::LuaScriptComponent>(dropped->path);
						if (newScript)
						{
							Utils::log_info(std::format("Lua script attached (anywhere): {}", dropped->path));
						}
					}
				}
				ImGui::EndDragDropTarget();
			}
		}

		ImGui::PopStyleColor();
	}

	void InspectorWindow::drawUITextProperties(Engine::EngineUI::UIText* text)
	{
		if (!text) return;

		ImGui::SeparatorText("UIText Properties");

		
		ImGui::PushID(text); 

		// Name
		char nameBuffer[256];
		strncpy_s(nameBuffer, text->getName().c_str(), sizeof(nameBuffer) - 1);
		if (ImGui::InputText("##Name", nameBuffer, sizeof(nameBuffer)))
		{
			text->setName(nameBuffer);
			text->markDirty();
		}
		ImGui::SameLine();
		ImGui::TextDisabled("Name");

		// Text content
		char textBuffer[512];
		strncpy_s(textBuffer, text->getText().c_str(), sizeof(textBuffer) - 1);
		if (ImGui::InputTextMultiline("##Text", textBuffer, sizeof(textBuffer), ImVec2(-1, 60)))
		{
			text->setText(textBuffer);
			text->markDirty();
		}
		ImGui::SameLine();
		ImGui::TextDisabled("Text");

		ImGui::Spacing();
		ImGui::SeparatorText("Transform (3D World Space)");

		// Position (x, y, z)
		Math::Vector3 position = text->getPosition();
		float pos[3] = { position.x, position.y, position.z };
		if (ImGui::DragFloat3("##Position", pos, 0.1f))
		{
			text->setPosition(Math::Vector3(pos[0], pos[1], pos[2]));
			text->markDirty();
		}
		ImGui::SameLine();
		ImGui::TextDisabled("Position");

		// Rotation (x, y, z) - Euler angles in degrees
		Math::Vector3 rotation = text->getRotation();
		float rot[3] = { rotation.x, rotation.y, rotation.z };
		if (ImGui::DragFloat3("##Rotation", rot, 1.0f, -360.0f, 360.0f))
		{
			text->setRotation(Math::Vector3(rot[0], rot[1], rot[2]));
			text->markDirty();
		}
		ImGui::SameLine();
		ImGui::TextDisabled("Rotation");

		// Scale (x, y, z)
		Math::Vector3 scale = text->getScale();
		float scl[3] = { scale.x, scale.y, scale.z };
		if (ImGui::DragFloat3("##Scale", scl, 0.001f, 0.001f, 10.0f))
		{
			text->setScale(Math::Vector3(scl[0], scl[1], scl[2]));
			text->markDirty();
		}
		ImGui::SameLine();
		ImGui::TextDisabled("Scale");

		ImGui::Spacing();
		ImGui::SeparatorText("Text Style");

		// Font Size
		float fontSize = text->getFontSize();
		if (ImGui::DragFloat("##FontSize", &fontSize, 1.0f, 8.0f, 128.0f))
		{
			text->setFontSize(fontSize);
			text->markDirty();
		}
		ImGui::SameLine();
		ImGui::TextDisabled("Font Size");

		// Color
		Math::Vector3 color = text->getColor();
		float colorArray[3] = { color.x, color.y, color.z };
		if (ImGui::ColorEdit3("##Color", colorArray))
		{
			text->setColor(Math::Vector3(colorArray[0], colorArray[1], colorArray[2]));
		}
		ImGui::SameLine();
		ImGui::TextDisabled("Color");

		// Alpha
		float alpha = text->getAlpha();
		if (ImGui::SliderFloat("##Alpha", &alpha, 0.0f, 1.0f))
		{
			text->setAlpha(alpha);
		}
		ImGui::SameLine();
		ImGui::TextDisabled("Alpha");

		// Visibility
		bool visible = text->isVisible();
		if (ImGui::Checkbox("##Visible", &visible))
		{
			text->setVisible(visible);
		}

		ImGui::Spacing();

		// Remove Component ボタン
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.1f, 0.1f, 1.0f));

		if (ImGui::Button("Remove Component", ImVec2(150, 25)))
		{
			if (m_scene)
			{
				auto* selectedObject = m_scene->getSelectedObject();
				if (selectedObject)
				{
					selectedObject->removeComponent<EngineUI::UIText>();
					Utils::log_info(std::format("UIText removed from {}", selectedObject->getName()));
				}
			}
		}

		ImGui::PopID(); 

		ImGui::Spacing();
		ImGui::TextWrapped("Note: UIText is a 3D object in world space. "
			"Position, rotation, and scale work the same as GameObjects. "
			"Use different camera angles to view from different perspectives.");
	}

	void InspectorWindow::drawTransformComponent(Core::Transform* transform)
	{
		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
		{
			auto& pos = transform->getPosition();
			float position[3] = { pos.x, pos.y, pos.z };
			if (ImGui::DragFloat3("Position", position, 0.1f))
			{
				transform->setPosition(Math::Vector3(position[0], position[1], position[2]));
			}

			auto& rot = transform->getRotation();
			float rotation[3] = { rot.x, rot.y, rot.z };
			if (ImGui::DragFloat3("Rotation", rotation, 1.0f))
			{
				transform->setRotation(Math::Vector3(rotation[0], rotation[1], rotation[2]));
			}

			auto& scale = transform->getScale();
			float scaleArray[3] = { scale.x, scale.y, scale.z };
			if (ImGui::DragFloat3("Scale", scaleArray, 0.1f, 0.1f, 10.0f))
			{
				transform->setScale(Math::Vector3(scaleArray[0], scaleArray[1], scaleArray[2]));
			}
		}
	}
	void InspectorWindow::drawRenderEntry(Core::GameObject* gameObject)
	{
		if (!gameObject || !m_scene) return;

		const auto* entry = m_scene->getRenderBatch().findEntry(gameObject->getId());
		if (!entry) return;

		if (ImGui::CollapsingHeader("Render Component", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// Visible
			bool visible = entry->visible;
			if (ImGui::Checkbox("Visible", &visible))
				m_scene->setRenderableVisible(gameObject, visible);

			// Type
			const char* types[] = { "Cube" };
			int currentType = static_cast<int>(entry->type);
			if (ImGui::Combo("Type", &currentType, types, IM_ARRAYSIZE(types)))
			{
				auto mat = entry->material; // entry は unregister 後に無効になるので先に取得
				m_scene->unregisterRenderable(gameObject);
				m_scene->registerRenderable(gameObject,
					static_cast<Graphics::RenderableType>(currentType), mat);
			}

			if (m_materialManager)
				drawMaterialEditor(gameObject);
		}
	}


	// ============================================================
	// drawMaterialEditor — RenderComponent* → GameObject* に変更
	// ============================================================

	void InspectorWindow::drawMaterialEditor(Core::GameObject* gameObject)
	{
		if (!gameObject || !m_scene) return;

		const auto* entry = m_scene->getRenderBatch().findEntry(gameObject->getId());
		if (!entry) return;

		auto currentMaterial = entry->material;

		if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
		{
			static char materialNameBuffer[256] = "";
			if (currentMaterial)
				strncpy_s(materialNameBuffer, currentMaterial->getName().c_str(),
					sizeof(materialNameBuffer));

			if (ImGui::InputText("Material Name", materialNameBuffer,
				sizeof(materialNameBuffer))) {
			}

			ImGui::SameLine();
			if (ImGui::Button("Create New"))
			{
				std::string newName = materialNameBuffer[0] ? materialNameBuffer : "New Material";
				auto newMat = m_materialManager->createMaterial(newName);
				if (newMat)
				{
					m_scene->setRenderableMaterial(gameObject, newMat);
					currentMaterial = newMat;
				}
			}

			ImGui::SameLine();
			if (ImGui::Button("Load Default"))
			{
				auto def = m_materialManager->getDefaultMaterial();
				if (def)
				{
					m_scene->setRenderableMaterial(gameObject, def);
					currentMaterial = def;
				}
			}

			if (currentMaterial)
			{
				ImGui::Separator();

				auto properties = currentMaterial->getProperties();
				bool changed = false;

				if (ImGui::CollapsingHeader("PBR Properties", ImGuiTreeNodeFlags_DefaultOpen))
				{
					// Albedoセクション（既存コードそのまま。renderComponentの参照をcurrentMaterialに変更）
					if (ImGui::CollapsingHeader("Albedo", ImGuiTreeNodeFlags_DefaultOpen))
					{
						auto& props = currentMaterial->getProperties();
						bool useTex = (props.useAlbedoTex != 0);

						if (ImGui::Checkbox("Use Albedo Texture", &useTex)) {
							props.useAlbedoTex = useTex ? 1 : 0;
							if (!useTex)
								currentMaterial->removeTexture(Engine::Graphics::TextureType::Albedo);
							currentMaterial->setDirty();
							(void)currentMaterial->updateConstantBuffer();
						}

						ImGui::BeginGroup();
						{
							ImVec2 slotSize(72, 72);
							ImGui::TextUnformatted("Albedo Map");
							ImGui::BeginChild("##AlbedoDropZone", slotSize, true,
								ImGuiWindowFlags_NoScrollbar);

							ImGui::TextWrapped(
								currentMaterial->hasTexture(Engine::Graphics::TextureType::Albedo)
								? "Assigned" : "Drop Texture Here");

							if (ImGui::BeginDragDropTarget()) {
								if (const ImGuiPayload* payload =
									ImGui::AcceptDragDropPayload("ASSET")) {
									const AssetPayload* dropped =
										static_cast<const AssetPayload*>(payload->Data);
									if (dropped &&
										dropped->type == static_cast<int>(UI::AssetInfo::Type::Texture)
										&& m_textureManager) {
										auto tex = m_textureManager->loadTexture(
											dropped->path, true, true);
										if (tex) {
											currentMaterial->setTexture(
												Engine::Graphics::TextureType::Albedo, tex);
											props.useAlbedoTex = 1;
											currentMaterial->setDirty();
											currentMaterial->updateConstantBuffer();
										}
									}
								}
								ImGui::EndDragDropTarget();
							}
							ImGui::EndChild();

							if (ImGui::SmallButton("Clear##Albedo")) {
								currentMaterial->removeTexture(
									Engine::Graphics::TextureType::Albedo);
								props.useAlbedoTex = 0;
								currentMaterial->setDirty();
								(void)currentMaterial->updateConstantBuffer();
							}
						}
						ImGui::EndGroup();

						ImGui::SameLine();
						ImGui::BeginGroup();
						{
							ImGui::TextUnformatted("Base Color");
							ImGui::BeginDisabled(useTex);
							float col[3] = { props.albedo.x, props.albedo.y, props.albedo.z };
							if (ImGui::ColorEdit3("##AlbedoColor", col,
								ImGuiColorEditFlags_DisplayRGB)) {
								props.albedo = Math::Vector3(col[0], col[1], col[2]);
								currentMaterial->setDirty();
								(void)currentMaterial->updateConstantBuffer();
							}
							ImGui::EndDisabled();
						}
						ImGui::EndGroup();
					}

					if (ImGui::SliderFloat("Metallic", &properties.metallic, 0, 1)) changed = true;
					if (ImGui::SliderFloat("Roughness", &properties.roughness, 0, 1)) changed = true;
					if (ImGui::SliderFloat("AO", &properties.ao, 0, 1)) changed = true;
					if (ImGui::SliderFloat("Alpha", &properties.alpha, 0, 1)) changed = true;
				}

				if (ImGui::CollapsingHeader("Emission"))
				{
					float e[3] = { properties.emissive.x, properties.emissive.y,
								   properties.emissive.z };
					if (ImGui::ColorEdit3("Emissive", e)) {
						properties.emissive = Math::Vector3(e[0], e[1], e[2]);
						changed = true;
					}
					if (ImGui::SliderFloat("Emissive Strength",
						&properties.emissiveStrength, 0, 5)) changed = true;
				}

				if (ImGui::CollapsingHeader("Textures"))
				{
					drawTextureSlot("Albedo", Graphics::TextureType::Albedo, currentMaterial);
					drawTextureSlot("Normal", Graphics::TextureType::Normal, currentMaterial);
					drawTextureSlot("Metallic", Graphics::TextureType::Metallic, currentMaterial);
					drawTextureSlot("Roughness", Graphics::TextureType::Roughness, currentMaterial);
					drawTextureSlot("AO", Graphics::TextureType::AO, currentMaterial);
					drawTextureSlot("Emissive", Graphics::TextureType::Emissive, currentMaterial);
					drawTextureSlot("Height", Graphics::TextureType::Height, currentMaterial);
				}

				if (ImGui::CollapsingHeader("UV Settings"))
				{
					float uvScale[2] = { properties.uvScale.x,  properties.uvScale.y };
					float uvOffset[2] = { properties.uvOffset.x, properties.uvOffset.y };
					if (ImGui::DragFloat2("UV Scale", uvScale, 0.01f)) {
						properties.uvScale = Math::Vector2(uvScale[0], uvScale[1]);  changed = true;
					}
					if (ImGui::DragFloat2("UV Offset", uvOffset, 0.01f)) {
						properties.uvOffset = Math::Vector2(uvOffset[0], uvOffset[1]); changed = true;
					}
				}

				if (changed)
					currentMaterial->setProperties(properties);
			}
		}
	}

	void InspectorWindow::drawBoxColliderComponent(Physics::BoxCollider* collider)
	{
		if (ImGui::CollapsingHeader("Box Collider", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// Is Trigger チェックボックス
			bool isTrigger = collider->isTrigger();
			if (ImGui::Checkbox("Is Trigger", &isTrigger))
			{
				collider->setTrigger(isTrigger);
			}

			// Center
			auto center = collider->getCenter();
			float centerArray[3] = { center.x, center.y, center.z };
			if (ImGui::DragFloat3("Center", centerArray, 0.1f))
			{
				collider->setCenter(Math::Vector3(centerArray[0], centerArray[1], centerArray[2]));
			}

			// Size
			auto size = collider->getSize();
			float sizeArray[3] = { size.x, size.y, size.z };
			if (ImGui::DragFloat3("Size", sizeArray, 0.1f, 0.01f, 100.0f))
			{
				collider->setSize(Math::Vector3(sizeArray[0], sizeArray[1], sizeArray[2]));
			}

			ImGui::Spacing();

			// Remove Component ボタン
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.1f, 0.1f, 1.0f));

			if (ImGui::Button("Remove Component", ImVec2(150, 25)))
			{
				if (m_scene)
				{
					auto* selectedObject = m_scene->getSelectedObject();
					if (selectedObject)
					{
						selectedObject->removeComponent<Physics::BoxCollider>();
						Utils::log_info(std::format("BoxCollider removed from {}", selectedObject->getName()));
					}
				}
			}

			ImGui::PopStyleColor(3);
		}
	}

	void InspectorWindow::drawAudioComponent(Audio::AudioComponent* audioComponent)
	{
		if (ImGui::CollapsingHeader("Audio Component", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// ファイルパス表示
			std::string filepath = audioComponent->getFilePath();
			if (!filepath.empty())
			{
				ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "Audio File:");
				ImGui::SameLine();

				// ファイル名だけを表示
				std::filesystem::path path(filepath);
				std::string fileName = path.filename().string();
				ImGui::Text("%s", fileName.c_str());

				ImGui::TextDisabled("Full Path:");
				ImGui::TextWrapped("%s", filepath.c_str());
				ImGui::Spacing();
			}
			else
			{
				ImGui::TextDisabled("No audio file loaded");
				ImGui::Spacing();
			}

			// ファイルドロップゾーン
			ImGui::BeginChild("##AudioDropZone", ImVec2(-1, 80), true);
			ImGui::TextWrapped("Drop WAV Audio File Here");
			ImGui::TextDisabled("(Supported: .wav)");

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
				{
					const AssetPayload* dropped = static_cast<const AssetPayload*>(payload->Data);
					if (dropped)
					{
						std::string ext = std::filesystem::path(dropped->path).extension().string();
						std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

						if (ext == ".wav")
						{
							auto loadResult = audioComponent->loadAudio(dropped->path);
							if (loadResult)
							{
								Utils::log_info(std::format("Audio loaded: {}", dropped->path));
							}
							else
							{
								Utils::log_error(loadResult.error());
							}
						}
						else
						{
							Utils::log_warning("Only .wav files are supported");
						}
					}
				}
				ImGui::EndDragDropTarget();
			}
			ImGui::EndChild();

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// オーディオが読み込まれている場合のみコントロールを表示
			if (!filepath.empty())
			{
				// 再生コントロール
				ImGui::Text("Playback Controls");
				ImGui::Separator();

				bool isPlaying = audioComponent->isPlaying();
				bool isPaused = audioComponent->isPaused();

				ImGui::BeginDisabled(isPlaying && !isPaused);
				if (ImGui::Button("Play", ImVec2(100, 30)))
				{
					audioComponent->play();
				}
				ImGui::EndDisabled();

				ImGui::SameLine();

				ImGui::BeginDisabled(!isPlaying || isPaused);
				if (ImGui::Button("Pause", ImVec2(100, 30)))
				{
					audioComponent->pause();
				}
				ImGui::EndDisabled();

				ImGui::SameLine();

				ImGui::BeginDisabled(!isPaused);
				if (ImGui::Button("Resume", ImVec2(100, 30)))
				{
					audioComponent->resume();
				}
				ImGui::EndDisabled();

				if (ImGui::Button("Stop", ImVec2(-1, 30)))
				{
					audioComponent->stop();
				}

				ImGui::Spacing();

				// ループ設定
				bool loop = audioComponent->isLoop();
				if (ImGui::Checkbox("Loop", &loop))
				{
					audioComponent->setLoop(loop);
				}

				// 音量設定
				float volume = audioComponent->getVolume();
				if (ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f))
				{
					audioComponent->setVolume(volume);
				}

				ImGui::Spacing();

				// ステータス表示
				ImGui::Text("Status:");
				ImGui::SameLine();
				if (isPlaying && !isPaused)
				{
					ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "Playing");
				}
				else if (isPaused)
				{
					ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.2f, 1.0f), "Paused");
				}
				else
				{
					ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Stopped");
				}
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// Remove Component ボタン
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.1f, 0.1f, 1.0f));

			if (ImGui::Button("Remove Component", ImVec2(150, 25)))
			{
				if (m_scene)
				{
					auto* selectedObject = m_scene->getSelectedObject();
					if (selectedObject)
					{
						selectedObject->removeComponent<Audio::AudioComponent>();
						Utils::log_info(std::format("AudioComponent removed from {}", selectedObject->getName()));
					}
				}
			}

			ImGui::PopStyleColor(3);
		}
	}

	void InspectorWindow::drawTextureSlot(const char* name,
		Graphics::TextureType textureType,
		std::shared_ptr<Graphics::Material> material)
	{
		ImGui::PushID(static_cast<int>(textureType));

		auto currentTexture = material->getTexture(textureType);
		auto properties = material->getProperties();
		bool changed = false;

		ImGui::Text("%s:", name);
		ImGui::SameLine(100);

		if (currentTexture)
		{
			ImGui::Button(currentTexture->getDesc().debugName.c_str(), ImVec2(150, 30));

			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::Text("Texture: %s", currentTexture->getDesc().debugName.c_str());
				ImGui::Text("Size: %dx%d", currentTexture->getWidth(), currentTexture->getHeight());
				ImGui::EndTooltip();
			}

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
				{
					const AssetInfo* droppedAsset = static_cast<const AssetInfo*>(payload->Data);
					if (droppedAsset->type == AssetInfo::Type::Texture && m_textureManager)
					{
						auto texture = m_textureManager->loadTexture(droppedAsset->path.string());
						if (texture)
						{
							material->setTexture(textureType, texture);
							Utils::log_info(std::format("Texture assigned: {} -> {}", droppedAsset->name, name));
						}
					}
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::SameLine();
			if (ImGui::Button("Clear"))
			{
				material->removeTexture(textureType);
				Utils::log_info(std::format("Texture cleared from slot: {}", name));
			}
		}
		else
		{
			if (textureType == Graphics::TextureType::Albedo)
			{
				float albedo[3] = { properties.albedo.x, properties.albedo.y, properties.albedo.z };
				if (ImGui::ColorEdit3("Albedo Color", albedo))
				{
					properties.albedo = Math::Vector3(albedo[0], albedo[1], albedo[2]);
					changed = true;
				}
			}
			else
			{
				ImGui::Button("Drag texture here", ImVec2(150, 30));
			}

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
				{
					const AssetInfo* droppedAsset = static_cast<const AssetInfo*>(payload->Data);
					if (droppedAsset->type == AssetInfo::Type::Texture && m_textureManager)
					{
						auto texture = m_textureManager->loadTexture(droppedAsset->path.string());
						if (texture)
						{
							material->setTexture(textureType, texture);
							Utils::log_info(std::format("Texture assigned: {} -> {}", droppedAsset->name, name));
						}
					}
				}
				ImGui::EndDragDropTarget();
			}
		}

		if (changed)
		{
			material->setProperties(properties);
		}

		ImGui::PopID();
	}


}


//src/UI/ImGuiManager.hpp
#pragma once

#include "imgui.h"
#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <memory>
#include <functional>
#include "engine/Graphics/Device.hpp"
#include "engine/Graphics/Device.hpp"
#include "engine/Utils/Common.hpp"
#include "engine/Core/GameObject.hpp"
#include "engine/Graphics/RenderComponent.hpp"
#include "engine/Graphics/Material.hpp"
#include "engine/Graphics/Texture.hpp"
#include "ContextMenu.hpp"
#include "engine/Core/PlayModeController.hpp"
#include "engine/Scripting/IScript.hpp"
#include "engine/Scripting/LuaScriptComponent.hpp"
#include "engine/Scripting/LuaScriptUtility.hpp"

//ImGui includes
#include "../UI/backends/imgui_impl_dx12.h"
#include "../UI/backends/imgui_impl_win32.h"

using Microsoft::WRL::ComPtr;

namespace Engine::UI
{
	//======================================================================
	//ImGuiクラス
	//======================================================================
	class ImGuiManager
	{
	public:
		ImGuiManager() = default;
		~ImGuiManager();

		//コピー・ムーブ禁止
		ImGuiManager(const ImGuiManager&) = delete;
		ImGuiManager& operator=(const ImGuiManager&) = delete;
		ImGuiManager(ImGuiManager&&) = delete;
		ImGuiManager& operator=(ImGuiManager&&) = delete;

		//初期化
		[[nodiscard]] Utils::VoidResult initialize(
			Graphics::Device* device,
			HWND hwnd,
			ID3D12CommandQueue* commandQueue,  // コマンドキューを3番目に移動
			DXGI_FORMAT rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM,
			UINT frameCount = 2
		);

		Utils::VoidResult createFontTextureManually();

		//終了処理
		void shutdown();

		//フレーム開始
		void newFrame();

		//描画
		void render(ID3D12GraphicsCommandList* commandList) const;

		//Win32メッセージ処理
		bool handleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) const;

		// リサイズ処理（改善版）
		void onWindowResize(int width, int height);

		//有効性のチェック
		[[nodiscard]] bool isInitialized() const { return m_initialized; }

		//ImGuiコンテキスト取得
		ImGuiContext* getContext() const { return m_context; }

		// リサイズ完了通知（新しく追加）
		void invalidateDeviceObjects();
		void createDeviceObjects();
		void clearRenderTargetDescriptors();

        //色合い設定
		void createGUIStyle();
		ImTextureID registerTexture(Graphics::Texture* tex);
		ImTextureID registerRenderTarget(ID3D12Resource* resource, DXGI_FORMAT format);
	private:
		bool m_initialized = false;
		ImGuiContext* m_context = nullptr;
		Graphics::Device* m_device = nullptr;
		HWND m_hwnd = nullptr;
		DXGI_FORMAT m_rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

		//DX12用のディスクリプタヒープ
		ComPtr<ID3D12DescriptorHeap> m_srvDescHeap;
		ID3D12CommandQueue* m_commandQueue = nullptr;  // コマンドキューへの参照
		UINT m_frameCount = 2;

		UINT m_srvIncSize = 0;
		D3D12_CPU_DESCRIPTOR_HANDLE m_srvCpuStart{};
		D3D12_GPU_DESCRIPTOR_HANDLE m_srvGpuStart{};
		UINT m_nextSrvIndex = 1;   
		UINT m_maxSrv = 0;

		//初期化ヘルパー
		[[nodiscard]] Utils::VoidResult createDescriptorHeap();
		[[nodiscard]] Utils::VoidResult reinitializeForResize();
	};

	//======================================================================
	//ImGuiウィンドウの基底クラス
	//======================================================================
	class ImGuiWindow
	{
	public:
		ImGuiWindow(const std::string& title, bool visible = true)
			:m_title(title), m_visible(visible) {
		}
		virtual ~ImGuiWindow() = default;

		//描画
		virtual void draw() = 0;

		//表示/非表示
		void setVisible(bool visible) { m_visible = visible; }
		bool isVisible() const { return m_visible; }

		//タイトル
		const std::string& getTitle() const { return m_title; }
		void setTitle(const std::string& title) { m_title = title; }

	protected:
		std::string m_title;
		bool m_visible;
	};

	//=======================================================================
	//デバッグ情報ウィンドウ
	//=======================================================================
	class DebugWindow : public ImGuiWindow
	{
	public:
		DebugWindow() : ImGuiWindow("Debug info") {}

		void draw() override;

		//デバッグ情報の設定
		void setPlayModeController(Core::PlayModeController* controller) { m_playModeController = controller; }
		void setFPS(float fps) { m_fps = fps; }
		void setFrameTime(float frameTime) { m_frameTime = frameTime; }
		void setObjectCount(size_t count) { m_objectCount = count; }

	private:
		float m_fps = 0.0f;
		float m_frameTime = 0.0f;
		size_t m_objectCount = 0;
		Core::PlayModeController* m_playModeController = nullptr;
	};

	//======================================================================
	//オブジェクト階層ウィンドウ
	//======================================================================
	class SceneHierarchyWindow : public ImGuiWindow
	{
	public:
		SceneHierarchyWindow();

		void draw() override;

		// シーンの設定
		void setScene(Graphics::Scene* scene) { m_scene = scene; }

		// 選択機能
		Core::GameObject* getSelectedObject() const { return m_selectedObject; }
		void setSelectedObject(Core::GameObject* object) { m_selectedObject = object; }

		// 選択変更コールバック
		void setSelectionChangedCallback(std::function<void(Core::GameObject*)> callback);

		// コンテキストメニューコールバック
		void setCreateObjectCallback(std::function<Core::GameObject* (UI::PrimitiveType, const std::string&)> callback);
		void setDeleteObjectCallback(std::function<void(Core::GameObject*)> callback);
		void setDuplicateObjectCallback(std::function<Core::GameObject* (Core::GameObject*)> callback);
		void setRenameObjectCallback(std::function<void(Core::GameObject*, const std::string&)> callback);

	private:
		Graphics::Scene* m_scene = nullptr;
		Core::GameObject* m_selectedObject = nullptr;
		std::function<void(Core::GameObject*)> m_onSelectionChanged;

		std::unique_ptr<ContextMenu> m_contextMenu;
		void drawGameObject(Core::GameObject* gameObject);
	};

	//=======================================================================
	//オブジェクトインスペクター
	//=======================================================================
	class InspectorWindow : public ImGuiWindow
	{
	public:
		InspectorWindow() : ImGuiWindow("Inspector") {}

		void draw() override;

		//選択オブジェクトの設定
		void setSelectedObject(Core::GameObject* object) { m_selectedObject = object; }

		// マテリアル・テクスチャ管理設定
		void setMaterialManager(Graphics::MaterialManager* manager) { m_materialManager = manager; }
		void setTextureManager(Graphics::TextureManager* manager) { m_textureManager = manager; }
		Core::GameObject* getSelectedObject() const { return m_selectedObject; }

	private:
		Core::GameObject* m_selectedObject = nullptr;
		Graphics::MaterialManager* m_materialManager = nullptr;
		Graphics::TextureManager* m_textureManager = nullptr;

		void drawTransformComponent(Core::Transform* transform);
		void drawRenderComponent(Graphics::RenderComponent* renderComponent);
		void drawScriptComponent(Scripting::LuaScriptComponent* luaScriptComponent);

		// マテリアル編集機能
		void drawMaterialEditor(Graphics::RenderComponent* renderComponent);
		void drawTextureSlot(const char* name, Graphics::TextureType textureType,
			std::shared_ptr<Graphics::Material> material);
	};
}

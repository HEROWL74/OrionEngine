//src/UI/ImGuiManager.hpp
#pragma once

#include "imgui.h"
#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <memory>
#include <functional>
#include "engine/ThirdParty/d3dx12.h"
#include "engine/Graphics/Device.hpp"
#include "engine/Utils/Common.hpp"
#include "engine/Core/GameObject.hpp"
#include "engine/Graphics/RenderComponent.hpp"
#include "engine/Graphics/Material.hpp"
#include "engine/Graphics/Texture.hpp"
#include "engine/Graphics/Scene.hpp"
#include "ContextMenu.hpp"
#include "../Core/PlayModeController.hpp"
#include "engine/Scripting/IScript.hpp"
#include "engine/Scripting/LuaScriptUtility.hpp"
#include "engine/Physics/BoxCollider.hpp"

#include "../UI/backends/imgui_impl_dx12.h"
#include "../UI/backends/imgui_impl_win32.h"

using Microsoft::WRL::ComPtr;

namespace Engine::Scripting
{
	class LuaScriptComponent;
}

namespace Editor::UI
{
	using namespace Engine;

	//======================================================================
	// ImGuiマネージャークラス
	//======================================================================
	class ImGuiManager
	{
	public:
		ImGuiManager() = default;
		~ImGuiManager();

		ImGuiManager(const ImGuiManager&) = delete;
		ImGuiManager& operator=(const ImGuiManager&) = delete;
		ImGuiManager(ImGuiManager&&) = delete;
		ImGuiManager& operator=(ImGuiManager&&) = delete;

		[[nodiscard]] Utils::VoidResult initialize(
			Graphics::Device* device,
			HWND hwnd,
			ID3D12CommandQueue* commandQueue,
			DXGI_FORMAT rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM,
			UINT frameCount = 2
		);

		Utils::VoidResult createFontTextureManually();
		void shutdown();
		void newFrame();
		void render(ID3D12GraphicsCommandList* commandList) const;
		bool handleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
		void onWindowResize(int width, int height);
		[[nodiscard]] bool isInitialized() const { return m_initialized; }
		ImGuiContext* getContext() const { return m_context; }
		void invalidateDeviceObjects();
		void createDeviceObjects();
		void clearRenderTargetDescriptors();
		void createGUIStyle();
		ImTextureID registerTexture(Graphics::Texture* tex);
		ImTextureID registerRenderTarget(ID3D12Resource* resource, DXGI_FORMAT format);

	private:
		bool m_initialized = false;
		ImGuiContext* m_context = nullptr;
		Graphics::Device* m_device = nullptr;
		HWND m_hwnd = nullptr;
		DXGI_FORMAT m_rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

		ComPtr<ID3D12DescriptorHeap> m_srvDescHeap;
		ID3D12CommandQueue* m_commandQueue = nullptr;
		UINT m_frameCount = 2;

		UINT m_srvIncSize = 0;
		D3D12_CPU_DESCRIPTOR_HANDLE m_srvCpuStart{};
		D3D12_GPU_DESCRIPTOR_HANDLE m_srvGpuStart{};
		UINT m_nextFreeIndex = 0;
		UINT m_descriptorSize = 0;
		UINT m_maxSrv = 0;

		[[nodiscard]] Utils::VoidResult createDescriptorHeap();
		[[nodiscard]] Utils::VoidResult reinitializeForResize();
	};

	//======================================================================
	// ImGuiウィンドウの基底クラス
	//======================================================================
	class ImGuiWindow
	{
	public:
		ImGuiWindow(const std::string& title, bool visible = true)
			:m_title(title), m_visible(visible) {
		}
		virtual ~ImGuiWindow() = default;

		virtual void draw() = 0;

		void setVisible(bool visible) { m_visible = visible; }
		bool isVisible() const { return m_visible; }

		const std::string& getTitle() const { return m_title; }
		void setTitle(const std::string& title) { m_title = title; }

	protected:
		std::string m_title;
		bool m_visible;
	};

	//=======================================================================
	// デバッグ情報ウィンドウ
	//=======================================================================
	class DebugWindow : public ImGuiWindow
	{
	public:
		DebugWindow() : ImGuiWindow("Debug info") {}

		void draw() override;

		void setPlayModeController(EditorCore::PlayModeController* controller) { m_playModeController = controller; }
		void setFPS(float fps) { m_fps = fps; }
		void setFrameTime(float frameTime) { m_frameTime = frameTime; }
		void setObjectCount(size_t count) { m_objectCount = count; }

	private:
		float m_fps = 0.0f;
		float m_frameTime = 0.0f;
		size_t m_objectCount = 0;
		EditorCore::PlayModeController* m_playModeController = nullptr;
	};

	//======================================================================
	// オブジェクト階層ウィンドウ - 選択管理をSceneに委譲
	//======================================================================
	class SceneHierarchyWindow : public ImGuiWindow
	{
	public:
		SceneHierarchyWindow();

		void draw() override;

		// Sceneの設定（選択状態もSceneから取得）
		void setScene(Graphics::Scene* scene) { m_scene = scene; }

		// 選択変更コールバック（Inspectorへの通知用）
		void setSelectionChangedCallback(std::function<void(Core::GameObject*)> callback);

		// コンテキストメニューコールバック
		void setCreateObjectCallback(std::function<Core::GameObject* (UI::PrimitiveType, const std::string&)> callback);
		void setDeleteObjectCallback(std::function<void(Core::GameObject*)> callback);
		void setDuplicateObjectCallback(std::function<Core::GameObject* (Core::GameObject*)> callback);
		void setRenameObjectCallback(std::function<void(Core::GameObject*, const std::string&)> callback);

	private:
		Graphics::Scene* m_scene = nullptr;
		std::function<void(Core::GameObject*)> m_onSelectionChanged;

		std::unique_ptr<ContextMenu> m_contextMenu;

		// currentSelectionを引数で受け取るように変更
		void drawGameObject(Core::GameObject* gameObject, Core::GameObject* currentSelection);
	};

	//=======================================================================
	// オブジェクトインスペクター - 選択をSceneから取得
	//=======================================================================
	class InspectorWindow : public ImGuiWindow
	{
	public:
		InspectorWindow() : ImGuiWindow("Inspector") {}

		void draw() override;

		// Sceneの設定（選択状態もSceneから取得）
		void setScene(Graphics::Scene* scene) { m_scene = scene; }

		// マテリアル・テクスチャ管理設定
		void setMaterialManager(Graphics::MaterialManager* manager) { m_materialManager = manager; }
		void setTextureManager(Graphics::TextureManager* manager) { m_textureManager = manager; }

	private:
		Graphics::Scene* m_scene = nullptr;
		Graphics::MaterialManager* m_materialManager = nullptr;
		Graphics::TextureManager* m_textureManager = nullptr;

		void drawTransformComponent(Core::Transform* transform);
		void drawRenderComponent(Graphics::RenderComponent* renderComponent);
		void drawScriptComponent(Scripting::LuaScriptComponent* luaScriptComponent);
		void drawMaterialEditor(Graphics::RenderComponent* renderComponent);
		void drawTextureSlot(const char* name, Graphics::TextureType textureType,
			std::shared_ptr<Graphics::Material> material);
		void drawBoxColliderComponent(Physics::BoxCollider* collider);
	};
}
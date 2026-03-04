// src/editor/Views/EditorView.hpp
#pragma once
#include <d3d12.h>
#include <memory>
#include <vector>
#include "engine/Graphics/Camera.hpp"
#include "engine/Graphics/Scene.hpp"
#include "engine/Graphics/RenderComponent.hpp"
#include "engine/Graphics/RenderTarget.hpp"
#include "engine/Graphics/FXAARenderer.hpp"
#include "engine/Graphics/Device.hpp"
#include "../UI/ImGuiManager.hpp"
#include "engine/Graphics/Skybox.hpp"
#include "engine/Graphics/PipelineStateCache.hpp"
#include "Gizmo.hpp"
#include "engine/UI/UIComponent.hpp"
#include "engine/UI/UITextRenderer.hpp"

namespace Editor::UI
{
	class EditorView
	{
	public:
		EditorView() = default;
		~EditorView() = default;

		[[nodiscard]] Engine::Utils::VoidResult initialize(Graphics::Device* device, uint32_t width, uint32_t height, Graphics::ShaderManager* shaderManager
			, Graphics::PipelineStateCache* psoCache);

		void render(Graphics::Scene& scene,
			ID3D12GraphicsCommandList* commandList,
			const Graphics::Camera& camera,
			UINT frameIndex);

		void renderEditorElements(ID3D12GraphicsCommandList* commandList,
			const Graphics::Camera& camera,
			UINT frameIndex);

		Graphics::RenderTarget* getRenderTarget() const
		{
			if (m_enableFXAA && m_fxaaOutputTarget)
			{
				return m_fxaaOutputTarget.get();
			}
			return m_renderTarget.get();
		}

		Graphics::FXAARenderer* getFXAARenderer() const
		{
			return m_fxaaRenderer.get();
		}

		void resize(uint32_t width, uint32_t height);

		bool isInitialized() const { return m_initialized; }

		void setShowGrid(bool show) { m_showGrid = show; }
		bool isShowingGrid() const { return m_showGrid; }

		void setShowGizmos(bool show) { m_showGizmos = show; }
		bool isShowingGizmos() const { return m_showGizmos; }

		void setSelectedObject(Core::GameObject* object) { m_selectedObject = object; }
		Core::GameObject* getSelectedObject() const { return m_selectedObject; }
		void setSkybox(Graphics::Skybox* skybox) { m_skybox = skybox; }
		void setPSOCache(Graphics::PipelineStateCache* psoCache) { m_psoCache = psoCache; }

		// Gizmo関連
		Gizmo* getGizmo() { return m_gizmo.get(); }
		void setGizmoType(GizmoType type) { if (m_gizmo) m_gizmo->setType(type); }

		// 解放
		void clearGizmoSelection()
		{
			if (m_gizmo)
			{
				m_gizmo->finishDrag();
			}
		}

		void setScene(Graphics::Scene* scene) { m_scene = scene; }
		void setUITextRenderer(EngineUI::UITextRenderer* renderer) { m_uiTextRenderer = renderer; }
	private:
		Graphics::Device* m_device = nullptr;
		UI::ImGuiManager* m_imguiManager = nullptr;
		std::unique_ptr<Graphics::RenderTarget> m_renderTarget;
		Graphics::Skybox* m_skybox = nullptr;
		Graphics::PipelineStateCache* m_psoCache = nullptr;
		bool m_initialized = false;
		uint32_t m_width = 0;
		uint32_t m_height = 0;

		bool m_showGrid = true;
		bool m_showGizmos = true;
		Core::GameObject* m_selectedObject = nullptr;

		Graphics::Scene* m_scene;
		EngineUI::UITextRenderer* m_uiTextRenderer = nullptr;

		// FXAA
		std::unique_ptr<Graphics::FXAARenderer> m_fxaaRenderer;
		std::unique_ptr<Graphics::RenderTarget> m_fxaaOutputTarget;
		bool m_enableFXAA = true; //比較用

		// Gizmo
		std::unique_ptr<Gizmo> m_gizmo;

		void renderGrid(ID3D12GraphicsCommandList* commandList, const Graphics::Camera& camera);
		void renderSelectionOutline(ID3D12GraphicsCommandList* commandList,
			const Graphics::Camera& camera,
			Core::GameObject* object);

	private:
		// Grid 関連の変数
		struct GridVertex
		{
			Math::Vector3 position;
			Math::Vector4 color;
		};

		struct GridCameraConstants
		{
			Math::Matrix4 viewProjection;
		};

		// グリッド用リソース
		ComPtr<ID3D12Resource> m_gridVertexBuffer;
		ComPtr<ID3D12Resource> m_gridCameraBuffer;
		D3D12_VERTEX_BUFFER_VIEW m_gridVertexBufferView{};
		ComPtr<ID3D12RootSignature> m_gridRootSignature;
		ComPtr<ID3D12PipelineState> m_gridPipelineState;
		void* m_gridCameraMapped = nullptr;
		UINT m_gridVertexCount = 0;
		bool m_gridInitialized = false;

	private:
		// Grid 関連の関数
		[[nodiscard]] Engine::Utils::VoidResult initializeGrid(Graphics::ShaderManager* shaderManager);
		[[nodiscard]] Engine::Utils::VoidResult createGridGeometry();
		[[nodiscard]] Engine::Utils::VoidResult createGridRootSignature();
		[[nodiscard]] Engine::Utils::VoidResult createGridPipelineState(Graphics::ShaderManager* shaderManager);
	};
}
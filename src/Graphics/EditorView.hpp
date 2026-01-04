//EditorView.hpp
#pragma once
#include <d3d12.h>
#include <memory>
#include "Camera.hpp"
#include "Scene.hpp"
#include "RenderComponent.hpp"
#include "RenderTarget.hpp"
#include "Device.hpp"
#include "../UI/ImGuiManager.hpp"
#include "Skybox.hpp"

namespace Engine::Graphics
{
	class EditorView
	{
	public:
		EditorView() = default;
		~EditorView() = default;

		[[nodiscard]] Utils::VoidResult initialize(Device* device,
			uint32_t width,
			uint32_t height,
			ShaderManager* shaderManager);

		void render(Scene& scene,
			ID3D12GraphicsCommandList* commandList,
			const Camera& camera,
			UINT frameIndex);
		void registerToImGui(UI::ImGuiManager* imgui);

		void renderEditorElements(ID3D12GraphicsCommandList* commandList,
			const Camera& camera,
			UINT frameIndex);

		ImTextureID getOutputTexture() const;

		void resize(uint32_t width, uint32_t height);

		bool isInitialized() const { return m_initialized; }

		void setShowGrid(bool show) { m_showGrid = show; }
		bool isShowingGrid() const { return m_showGrid; }

		void setShowGizmos(bool show) { m_showGizmos = show; }
		bool isShowingGizmos() const { return m_showGizmos; }

		void setSelectedObject(Core::GameObject* object) { m_selectedObject = object; }
		Core::GameObject* getSelectedObject() const { return m_selectedObject; }
		void setSkybox(Skybox* skybox) { m_skybox = skybox; }
	private:
		Device* m_device = nullptr;
		UI::ImGuiManager* m_imguiManager = nullptr;
		std::unique_ptr<RenderTarget> m_renderTarget;
		Skybox* m_skybox = nullptr;
		bool m_initialized = false;
		uint32_t m_width = 0;
		uint32_t m_height = 0;

		bool m_showGrid = true;
		bool m_showGizmos = true;
		Core::GameObject* m_selectedObject = nullptr;

		void renderGrid(ID3D12GraphicsCommandList* commandList, const Camera& camera);
		void renderSelectionOutline(ID3D12GraphicsCommandList* commandList,
			const Camera& camera,
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
		[[nodiscard]] Utils::VoidResult initializeGrid(ShaderManager* shaderManager);
		[[nodiscard]] Utils::VoidResult createGridGeometry();
		[[nodiscard]] Utils::VoidResult createGridRootSignature();
		[[nodiscard]] Utils::VoidResult createGridPipelineState(ShaderManager* shaderManager);
	};
}
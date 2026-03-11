#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <memory>
#include "../engine/Graphics/Device.hpp"
#include "../engine/Graphics/ShaderManager.hpp"
#include "../engine/Graphics/Camera.hpp"
#include "../engine/Core/GameObject.hpp"
#include "../engine/Math/Math.hpp"
#include "../engine/Utils/Common.hpp"

using Microsoft::WRL::ComPtr;

namespace Editor::UI
{
	using namespace Engine;
	enum class GizmoType
	{
		None,
		Translation,
		Rotation,
		Scale
	};

	enum class GizmoAxis
	{
		None = 0,
		X,
		Y,
		Z,
		XY,
		YZ,
		XZ
	};

	class Gizmo
	{
	public:
		Gizmo() = default;
		~Gizmo() = default;

		[[nodiscard]]
		Engine::Utils::VoidResult initialize(Engine::Graphics::Device* device, Engine::Graphics::ShaderManager* shaderManager);
		void shutdown();

		void render(ID3D12GraphicsCommandList* commandList, const Engine::Graphics::Camera& camera, Engine::
			Core::GameObject* targetObject);

		GizmoAxis hitTest(const Math::Vector3& rayOrigin,
			const Math::Vector3& rayDirection,
			Core::GameObject* targetObject) const;

		void beginDrag(GizmoAxis axis, const Math::Vector3& rayOrigin,
			const Math::Vector3& rayDirection,
			const Math::Vector3& objectPosition);
		void processDrag(const Math::Vector3& rayOrigin,
			const Math::Vector3& rayDirection,
			Math::Vector3& outNewPosition);
		void finishDrag();

		void setType(GizmoType type) { m_type = type; }
		GizmoType getType() const noexcept { return m_type; }

		bool isDragging() const { return m_isDragging; }
		GizmoAxis getSelectedAxis() const { return m_selectedAxis; }

	private:
		Engine::Graphics::Device* m_device = nullptr;
		Engine::Graphics::ShaderManager* m_shaderManager = nullptr;

		GizmoType m_type = GizmoType::Translation;
		GizmoAxis m_selectedAxis = GizmoAxis::None;
		bool m_isDragging = false;

		Math::Vector3 m_dragStartPosition;
		Math::Vector3 m_dragStartObjectPosition;
		Math::Vector3 m_dragPlaneNormal;
		Math::Vector3 m_dragAxisDirection;

		ComPtr<ID3D12RootSignature> m_rootSig;
		ComPtr<ID3D12PipelineState> m_pso;
		ComPtr<ID3D12Resource> m_vertexBuffer;
		ComPtr<ID3D12Resource> m_indexBuffer;
		D3D12_VERTEX_BUFFER_VIEW m_vbv{};
		D3D12_INDEX_BUFFER_VIEW m_ibv{};
		UINT m_indexCount = 0;

		ComPtr<ID3D12Resource> m_constantBuffer;
		void* m_cbMapped = nullptr;

		struct GizmoVertex
		{
			Math::Vector3 position;
			Math::Vector4 color;
		};

		struct GizmoConstants
		{
			Math::Matrix4 worldViewProjection;
			Math::Vector4 selectedColor;
			float scale{};
			float padding[3]{};
		};

		[[nodiscard]] Engine::Utils::VoidResult createRootSignature();
		[[nodiscard]] Engine::Utils::VoidResult createPipelineState();
		[[nodiscard]] Engine::Utils::VoidResult createGeometry();
		[[nodiscard]] Engine::Utils::VoidResult createConstantBuffer();

		void renderTranslationGizmo(ID3D12GraphicsCommandList* commandList,
			const Engine::Graphics::Camera& camera,
			const Math::Vector3& position);

		Math::Vector3 projectPointOnAxis(const Math::Vector3& point,
			const Math::Vector3& axisOrigin,
			const Math::Vector3& axisDirection) const;

		float calculateGizmoScale(const Engine::Graphics::Camera& camera, const Math::Vector3& position);
	};
}


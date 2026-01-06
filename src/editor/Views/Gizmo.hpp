#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <memory>
#include "engine/Graphics/Device.hpp"
#include "engine/Graphics/ShaderManager.hpp"
#include "engine/Graphics/Camera.hpp"
#include "engine/Core/GameObject.hpp"
#include "engine/Math/Math.hpp"
#include "engine/Utils/Common.hpp"

using Microsoft::WRL::ComPtr;

namespace Editor::UI
{
	using namespace Engine;
	// Gizmoの種類
	enum class GizmoType
	{
		None,
		Translation,   // 移動
		Rotation,      // 回転
		Scale          // スケール
	};

	// Gizmoの軸
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
		Utils::VoidResult initialize(Engine::Graphics::Device* device, Engine::Graphics::ShaderManager* shaderManager);
		void shutdown();

		// 描画
		void render(ID3D12GraphicsCommandList* commandList, const Engine::Graphics::Camera& camera, Core::GameObject* targetObject);

		// Ray との交差判定
		GizmoAxis hitTest(const Math::Vector3& rayOrigin,
			const Math::Vector3& rayDirection,
			Core::GameObject* targetObject) const;

		// Gizmoの操作
		void startDrag(GizmoAxis axes, const Math::Vector3& rayOrigin,
			const Math::Vector3& rayDirection);
		void updateDrag(const Math::Vector3& rayOrigin,
			const Math::Vector3& rayDirection);
		void endDrag();

		// 設定
		void setType(GizmoType type) { m_type = type; }
		GizmoType getType() const noexcept { return m_type; }

		bool isDragging() const { return m_isDragging; }
		GizmoAxis getSelectedAxis() const { return m_selectedAxis; }
	private:
		Engine::Graphics::Device* m_device = nullptr;
		Engine::Graphics::ShaderManager* m_shaderManager;

		GizmoType m_type = GizmoType::Translation;
		GizmoAxis m_selectedAxis = GizmoAxis::None;
		bool m_isDragging = false;

		// ドラッグ開始時の状態
		Math::Vector3 m_dragStartPosition;
		Math::Vector3 m_dragStartObjectPosition;
		Math::Vector3 m_dragPlaneNormal;

		// GPU リソース
		ComPtr<ID3D12RootSignature> m_rootSig;
		ComPtr<ID3D12PipelineState> m_pso;

		// Gizmo Geometry
		ComPtr<ID3D12Resource> m_vertexBuffer;
		ComPtr<ID3D12Resource> m_indexBuffer;
		D3D12_VERTEX_BUFFER_VIEW m_vbv;
		D3D12_INDEX_BUFFER_VIEW m_ibv;
		UINT m_indexCount = 0;

		// 定数バッファ
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
			float scale;
			float padding[3];
		};

		// 内部関数
		[[nodiscard]] Utils::VoidResult createRootSignature();
		[[nodiscard]] Utils::VoidResult createPipelineState();
		[[nodiscard]] Utils::VoidResult createGeometry();
		[[nodiscard]] Utils::VoidResult createConstantBuffer();

		void renderTranslationGizmo(ID3D12GraphicsCommandList* commandList,
			const Engine::Graphics::Camera& camera,
			const Math::Vector3& position);

		// Rayと軸の交差判定
		bool rayIntersectsAxis(const Math::Vector3& rayOrigin,
			const Math::Vector3& rayDirection,
			const Math::Vector3& axisStart,
			const Math::Vector3& axisEnd,
			float threshold,
			float& outDistance) const;

		// Rayと平面の交差判定
		bool rayIntersectsPlane(const Math::Vector3& rayOrigin,
			const Math::Vector3& rayDirection,
			const Math::Vector3& planePoint,
			const Math::Vector3& planeNormal,
			Math::Vector3& outIntersection);

		// Gizmoのスケール計算
		float calculateGizmoScale(const Engine::Graphics::Camera& camera, const Math::Vector3& position);
	};
}
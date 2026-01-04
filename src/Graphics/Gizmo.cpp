#include "Gizmo.hpp"

namespace Engine::Graphics
{
	Utils::VoidResult Gizmo::initialize(Device* device, ShaderManager* shaderManager)
	{
		CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
		CHECK_CONDITION(shaderManager != nullptr, Utils::ErrorType::Unknown, "ShaderManager is null");

		m_device = device;
		m_shaderManager = shaderManager;

		auto rootSigResult = createRootSignature();
		if (!rootSigResult) return rootSigResult;

		auto psoResult = createPipelineState();
		if (!psoResult) return psoResult;

		auto geomResult = createGeometry();
		if(!geomResult) return geomResult;

		auto cbResult = createConstantBuffer();
		if (!cbResult) return cbResult;

		Utils::log_info("Gizmo intialized successfully");
		return {};
	}
	void Gizmo::shutdown()
	{
		if (m_constantBuffer && m_cbMapped)
		{
			m_constantBuffer->Unmap(0, nullptr);
			m_cbMapped = nullptr;
		}

		m_constantBuffer.Reset();
		m_indexBuffer.Reset();
		m_vertexBuffer.Reset();
		m_pso.Reset();
		m_rootSig.Reset();

		m_device = nullptr;
		m_shaderManager = nullptr;

		Utils::log_info("Gizmo shutdown completed");
	}

	// 描画
	void Gizmo::render(ID3D12GraphicsCommandList* commandList, const Camera& camera, Core::GameObject* targetObject)
	{
		if (!targetObject || !m_device || !m_rootSig || !m_pso)
		{
			return;
		}

		Math::Vector3 position = targetObject->getTransform()->getPosition();

		switch (m_type)
		{
		case GizmoType::None:
			break;
		case GizmoType::Translation:
			renderTranslationGizmo(commandList, camera,position);
			break;
		case GizmoType::Rotation:
			break;
		case GizmoType::Scale:
			break;
		default:
			break;
		}
	}

	// Ray との交差判定
	GizmoAxis Gizmo::hitTest(const Math::Vector3& rayOrigin,
		const Math::Vector3& rayDirection,
		Core::GameObject* targetObject) const
	{

	}

	// Gizmoの操作
	void Gizmo::startDrag(GizmoAxis axes, const Math::Vector3& rayOrigin,
		const Math::Vector3& rayDirection)
	{

	}
	void Gizmo::updateDrag(const Math::Vector3& rayOrigin,
		const Math::Vector3& rayDirection)
	{

	}
	void Gizmo::endDrag()
	{

	}

	void Gizmo::renderTranslationGizmo(ID3D12GraphicsCommandList* commandList,
		const Camera& camera,
		const Math::Vector3& position)
	{

	}

	// Rayと軸の交差判定
	bool Gizmo::rayIntersectsAxis(const Math::Vector3& rayOrigin,
		const Math::Vector3& rayDirection,
		const Math::Vector3& axisStart,
		const Math::Vector3& axisEnd,
		float threshold,
		float& outDistance) const
	{

	}

	// Rayと平面の交差判定
	bool Gizmo::rayIntersectsPlane(const Math::Vector3& rayOrigin,
		const Math::Vector3& rayDirection,
		const Math::Vector3& planePoint,
		const Math::Vector3& planeNormal,
		Math::Vector3& outIntersection)
	{

	}

	// Gizmoのスケール計算
	float Gizmo::calculateGizmoScale(const Camera& camera, const Math::Vector3& position)
	{

	}

	[[nodiscard]] Utils::VoidResult Gizmo::createRootSignature()
	{

	}
	[[nodiscard]] Utils::VoidResult Gizmo::createPipelineState()
	{

	}
	[[nodiscard]] Utils::VoidResult Gizmo::createGeometry()
	{

	}
	[[nodiscard]] Utils::VoidResult Gizmo::createConstantBuffer()
	{

	}
}
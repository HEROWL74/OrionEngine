#include "EditorViewWindow.hpp"
#include "../Utils/RayPicking.hpp"
#include "imgui.h"
#include "engine/Utils/Common.hpp"

namespace Editor::UI
{
	void EditorViewWindow::initialize(ImGuiManager* imgui, EditorView* view, Engine::Input::InputManager* inputManager)
	{
		m_imgui = imgui;
		m_view = view;
		m_inputManager = inputManager;

		if (!m_inputManager || !m_inputManager->isInitialized())
		{
			Engine::Utils::log_error(Engine::Utils::make_error(Engine::Utils::ErrorType::Unknown,
				"EditorViewWindow: InputManager is null or not initialized"));
			return;
		}

		auto* rt = view->getRenderTarget();
		if (!rt)
			return;

		m_texture = imgui->registerRenderTarget(rt->getColorResource(), rt->getFormat());
		Engine::Utils::log_info("EditorViewWindow initialized");
	}

	void EditorViewWindow::draw()
	{
		if (!m_texture)
			return;

		// 位置とサイズ指定を削除し、ドッキングに任せる
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse;

		if (ImGui::Begin("Scene", nullptr, flags))
		{
			m_isFocused = ImGui::IsWindowFocused();
			m_isHovered = ImGui::IsWindowHovered();

			m_viewportPos = ImGui::GetCursorScreenPos();
			ImVec2 viewportSize = ImGui::GetContentRegionAvail();

			if (std::abs(viewportSize.x - m_lastSize.x) > 1.0f ||
				std::abs(viewportSize.y - m_lastSize.y) > 1.0f)
			{
				if (viewportSize.x > 0 && viewportSize.y > 0)
				{
					m_pendingWidth = static_cast<uint32_t>(viewportSize.x);
					m_pendingHeight = static_cast<uint32_t>(viewportSize.y);
					m_needsResize = true;
					m_lastSize = viewportSize;
				}
			}

			if (viewportSize.x > 0 && viewportSize.y > 0)
			{
				if (m_texture)
				{
					ImGui::Image(m_texture, viewportSize);

					if (ImGui::IsItemHovered())
					{
						handleMouseInput();
					}

					if (m_isDraggingGizmo)
					{
						updateGizmoDrag();
					}

					if (m_cameraControlRequested && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
					{
						endCameraControl();
					}

					if (m_isDraggingGizmo && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
					{
						endGizmoDrag();
					}
				}
				else
				{
					ImGui::Text("EditorView not ready");
				}
			}

			drawOverlay();
		}
		ImGui::End();
	}


	void EditorViewWindow::handleMouseInput()
	{
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !m_cameraControlRequested && !m_isDraggingGizmo)
		{
			if (m_view && m_view->isShowingGizmos() && m_view->getSelectedObject())
			{
				auto* gizmo = m_view->getGizmo();
				if (gizmo)
				{
					Math::Vector3 rayOrigin, rayDirection;
					getRayFromMouse(rayOrigin, rayDirection);

					GizmoAxis hitAxis = gizmo->hitTest(rayOrigin, rayDirection, m_view->getSelectedObject());

					if (hitAxis != GizmoAxis::None)
					{
						startGizmoDrag(hitAxis, rayOrigin, rayDirection);
						return;
					}
				}
			}

			handleObjectSelection();
		}

		if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && m_isDraggingGizmo)
		{
			updateGizmoDrag();
		}

		if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
		{
			startCameraControl();
		}
	}

	void EditorViewWindow::getRayFromMouse(Math::Vector3& outOrigin, Math::Vector3& outDirection)
	{
		if (!m_camera)
			return;

		ImVec2 mousePos = ImGui::GetMousePos();
		float localX = mousePos.x - m_viewportPos.x;
		float localY = mousePos.y - m_viewportPos.y;

		float viewportWidth = static_cast<float>(m_pendingWidth > 0 ? m_pendingWidth : m_lastSize.x);
		float viewportHeight = static_cast<float>(m_pendingHeight > 0 ? m_pendingHeight : m_lastSize.y);

		EditorUtils::RayPicking::screenToWorldRay(localX, localY, viewportWidth, viewportHeight, *m_camera, outOrigin, outDirection);
	}

	void EditorViewWindow::startGizmoDrag(GizmoAxis axis, const Math::Vector3& rayOrigin, const Math::Vector3& rayDirection)
	{
		if (!m_view)
			return;

		auto* gizmo = m_view->getGizmo();
		auto* selectedObject = m_view->getSelectedObject();
		if (!gizmo || !selectedObject)
			return;

		m_isDraggingGizmo = true;
		m_draggedAxis = axis;
		m_dragStartObjectPosition = selectedObject->getTransform()->getPosition();

		gizmo->beginDrag(axis, rayOrigin, rayDirection, m_dragStartObjectPosition);
	}

	void EditorViewWindow::updateGizmoDrag()
	{
		if (!m_view || !m_isDraggingGizmo)
			return;

		auto* gizmo = m_view->getGizmo();
		auto* selectedObject = m_view->getSelectedObject();
		if (!gizmo || !selectedObject)
			return;

		Math::Vector3 rayOrigin, rayDirection;
		getRayFromMouse(rayOrigin, rayDirection);

		Math::Vector3 newPosition;
		gizmo->processDrag(rayOrigin, rayDirection, newPosition);

		selectedObject->getTransform()->setPosition(newPosition);
	}

	void EditorViewWindow::endGizmoDrag()
	{
		if (!m_isDraggingGizmo)
			return;

		if (m_view)
		{
			auto* gizmo = m_view->getGizmo();
			if (gizmo)
			{
				gizmo->finishDrag();
			}
		}

		m_isDraggingGizmo = false;
		m_draggedAxis = GizmoAxis::None;
	}

	void EditorViewWindow::processResize()
	{
		if (m_needsResize && m_view)
		{
			m_view->resize(m_pendingWidth, m_pendingHeight);
			if (m_camera)
			{
				m_camera->updateAspect(static_cast<float>(m_pendingWidth) / m_pendingHeight);
			}

			auto* rt = m_view->getRenderTarget();
			if (rt && m_imgui)
			{
				m_texture = m_imgui->registerRenderTarget(rt->getColorResource(), rt->getFormat());
			}

			m_needsResize = false;
		}
	}

	void EditorViewWindow::handleObjectSelection()
	{
		if (!m_camera || !m_scene || !m_view)
			return;

		Math::Vector3 rayOrigin, rayDirection;
		getRayFromMouse(rayOrigin, rayDirection);

		auto hit = EditorUtils::RayPicking::raycast(rayOrigin, rayDirection, m_scene->getGameObjects());

		if (hit.hit)
		{
			m_view->setSelectedObject(hit.object);
		}
		else
		{
			m_view->setSelectedObject(nullptr);
		}
	}

	void EditorViewWindow::startCameraControl()
	{
		if (!m_camera || !m_inputManager || !m_inputManager->isInitialized())
			return;

		if (m_cameraControlRequested)
			return;

		m_cameraControlRequested = true;
		m_inputManager->setRelativeMouseMode(true);
	}

	void EditorViewWindow::endCameraControl()
	{
		if (!m_cameraControlRequested)
			return;

		m_cameraControlRequested = false;

		if (m_inputManager)
		{
			m_inputManager->setRelativeMouseMode(false);
		}
	}

	void EditorViewWindow::drawOverlay()
	{
		ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - 120, 10));

		if (m_view)
		{
			bool showGrid = m_view->isShowingGrid();
			if (ImGui::Checkbox("Grid", &showGrid))
			{
				m_view->setShowGrid(showGrid);
			}

			ImGui::SameLine();
			bool showGizmos = m_view->isShowingGizmos();
			if (ImGui::Checkbox("Gizmos", &showGizmos))
			{
				m_view->setShowGizmos(showGizmos);
			}
		}
	}
}
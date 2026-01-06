#include "EditorViewWindow.hpp"
#include "imgui.h"

namespace Editor::UI
{
	void EditorViewWindow::initialize(ImGuiManager* imgui, EditorView* view)
	{
		m_imgui = imgui;
		m_view = view;

		auto* rt = view->getRenderTarget();
		if (!rt)
			return;

		m_texture = imgui->registerRenderTarget(
			rt->getColorResource(),
			rt->getFormat()
		);
	}

	void EditorViewWindow::draw()
	{
		if (!m_texture)
			return;

		ImGuiViewport* vp = ImGui::GetMainViewport();
		ImVec2 wp = vp->WorkPos;
		ImVec2 ws = vp->WorkSize;

		const float LEFT = 0.22f;
		const float RIGHT = 0.26f;
		const float BOTTOM = 0.28f;

		ImVec2 scenePos = ImVec2(wp.x + ws.x * LEFT, wp.y);
		ImVec2 sceneSize = ImVec2(ws.x * (1.0f - LEFT - RIGHT), ws.y * (1.0f - BOTTOM));

		static ImVec2 prevDisplay(0, 0);
		ImGuiIO& io = ImGui::GetIO();
		bool resized = fabsf(prevDisplay.x - io.DisplaySize.x) > 1.0f ||
			fabsf(prevDisplay.y - io.DisplaySize.y) > 1.0f;
		prevDisplay = io.DisplaySize;
		ImGuiCond cond = resized ? ImGuiCond_Always : ImGuiCond_FirstUseEver;

		ImGui::SetNextWindowPos(scenePos, cond);
		ImGui::SetNextWindowSize(sceneSize, cond);

		ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse |
			ImGuiWindowFlags_NoMove;

		if (ImGui::Begin("Scene", nullptr, flags))
		{
			m_isFocused = ImGui::IsWindowFocused();
			m_isHovered = ImGui::IsWindowHovered();

			ImVec2 viewportSize = ImGui::GetContentRegionAvail();

			// リサイズ検出
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

			// テクスチャ表示
			if (viewportSize.x > 0 && viewportSize.y > 0)
			{
				if (m_texture)
				{
					// デバッグ: 表示しているテクスチャIDをログ出力
					static int sceneCounter = 0;
					if (sceneCounter++ % 120 == 0) {
						Utils::log_info(std::format("EditorViewWindow displaying texture: 0x{:016X}",
							m_texture));
					}

					ImGui::Image(m_texture, viewportSize);

					// マウス操作を検出
					if (ImGui::IsItemHovered())
					{
						// 左クリックされた
						if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
						{
							Utils::log_info(">>> EditorViewWindow: Left click detected on image!");
							m_cameraControlRequested = true;
						}
					}

					// 左ボタンが離された
					if (m_cameraControlRequested && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
					{
						Utils::log_info(">>> EditorViewWindow: Left button released!");
						m_cameraControlRequested = false;
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

	void EditorViewWindow::processResize()
	{
		if (m_needsResize && m_view)
		{
			m_view->resize(m_pendingWidth, m_pendingHeight);
			if (m_camera)
			{
				m_camera->updateAspect(static_cast<float>(m_pendingWidth) / m_pendingHeight);
			}

			// テクスチャを再登録
			auto* rt = m_view->getRenderTarget();
			if (rt && m_imgui)
			{
				m_texture = m_imgui->registerRenderTarget(
					rt->getColorResource(),
					rt->getFormat()
				);
			}

			m_needsResize = false;
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
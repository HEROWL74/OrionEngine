#include "GameViewWindow.hpp"
#include <imgui.h>

namespace Editor::UI
{
	void GameViewWindow::initialize(ImGuiManager* imgui, GameView* view)
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

	void GameViewWindow::draw()
	{
		if (!m_texture)
			return;

		ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse;

		if (ImGui::Begin("Game", nullptr, flags))
		{
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
				}
				else
				{
					ImGui::Text("GameView not ready");
				}
			}
		}
		ImGui::End();
	}

	void GameViewWindow::processResize()
	{
		if (m_needsResize && m_view)
		{
			m_view->resize(m_pendingWidth, m_pendingHeight);
			float aspect = static_cast<float>(m_pendingWidth) / m_pendingHeight;

			// CameraComponentにだけアスペクトを伝える
			if (m_gameCameraObject && !m_gameCameraObject->isDestroyed())
			{
				auto* camComp = m_gameCameraObject->getComponent<Engine::World::CameraComponent>();
				if (camComp)
					camComp->updateAspect(aspect);
			}

			// ★ resize後は必ずgetRenderTarget()で新しいリソースを取得して再登録
			auto* rt = m_view->getRenderTarget();
			if (rt && m_imgui)
			{
				// ★ 古いテクスチャIDを無効化してから再登録
				m_texture = NULL;
				m_texture = m_imgui->registerRenderTarget(
					rt->getColorResource(),
					rt->getFormat()
				);
			}
			m_needsResize = false;
		}
	}
}
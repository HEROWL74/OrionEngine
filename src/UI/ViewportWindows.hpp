#pragma once
#include "ImGuiManager.hpp"
#include "../Graphics/EditorView.hpp"
#include "../Graphics/GameView.hpp"

namespace Engine::UI
{
	class SceneViewportWindow : public ImGuiWindow
	{
	public:
		SceneViewportWindow() : ImGuiWindow("Scene", true) {}

		void draw() override
		{
			if (!m_visible || !m_editorView)
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

			ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

			if (ImGui::Begin(m_title.c_str(), &m_visible, flags))
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
					ImTextureID texId = m_editorView->getOutputTexture();
					if (texId)
					{
						ImGui::Image(texId, viewportSize);
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

		void processResize()
		{
			if (m_needsResize && m_editorView)
			{
				m_editorView->resize(m_pendingWidth, m_pendingHeight);
				if (m_camera)
				{
					m_camera->updateAspect(static_cast<float>(m_pendingWidth) / m_pendingHeight);
				}
				m_needsResize = false;
			}
		}

		void setEditorView(Graphics::EditorView* view) { m_editorView = view; }
		void setCamera(Graphics::Camera* camera) { m_camera = camera; }

	private:
		Graphics::EditorView* m_editorView = nullptr;
		Graphics::Camera* m_camera = nullptr;
		ImVec2 m_lastSize = { 0, 0 };
		bool m_needsResize = false;
		uint32_t m_pendingWidth = 0;
		uint32_t m_pendingHeight = 0;

		void drawOverlay()
		{
			ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - 120, 10));

			if (m_editorView)
			{
				bool showGrid = m_editorView->isShowingGrid();
				if (ImGui::Checkbox("Grid", &showGrid))
				{
					m_editorView->setShowGrid(showGrid);
				}

				ImGui::SameLine();
				bool showGizmos = m_editorView->isShowingGizmos();
				if (ImGui::Checkbox("Gizmos", &showGizmos))
				{
					m_editorView->setShowGizmos(showGizmos);
				}
			}
		}
	};

	class GameViewportWindow : public ImGuiWindow
	{
	public:
		GameViewportWindow() : ImGuiWindow("Game", true) {}

		void draw() override
		{
			if (!m_visible || !m_gameView)
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

			ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

			if (ImGui::Begin(m_title.c_str(), &m_visible, flags))
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
					ImTextureID texId = m_gameView->getOutputTexture();
					if (texId)
					{
						ImGui::Image(texId, viewportSize);
					}
					else
					{
						ImGui::Text("GameView not ready");
					}
				}
			}
			ImGui::End();
		}

		void processResize()
		{
			if (m_needsResize && m_gameView)
			{
				m_gameView->resize(m_pendingWidth, m_pendingHeight);
				if (m_camera)
				{
					m_camera->updateAspect(static_cast<float>(m_pendingWidth) / m_pendingHeight);
				}
				m_needsResize = false;
			}
		}

		void setGameView(Graphics::GameView* view) { m_gameView = view; }
		void setCamera(Graphics::Camera* camera) { m_camera = camera; }

	private:
		Graphics::GameView* m_gameView = nullptr;
		Graphics::Camera* m_camera = nullptr;
		ImVec2 m_lastSize = { 0, 0 };
		bool m_needsResize = false;
		uint32_t m_pendingWidth = 0;
		uint32_t m_pendingHeight = 0;
	};
}
// editor/UI/ToolbarWindow.hpp
#pragma once
#include "ImGuiManager.hpp"
#include "../Core/PlayModeController.hpp"

namespace Editor::UI
{
	class ToolbarWindow : public ImGuiWindow
	{
	public:
		ToolbarWindow() : ImGuiWindow("Toolbar", true) {}

		void draw() override
		{
			if (!m_visible || !m_playModeController) return;

			// メインビューポートの上部に固定
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImVec2 workPos = viewport->WorkPos;
			ImVec2 workSize = viewport->WorkSize;

			// ツールバーのサイズと位置
			const float toolbarHeight = 40.0f;
			ImGui::SetNextWindowPos(ImVec2(workPos.x, workPos.y), ImGuiCond_Always);
			ImGui::SetNextWindowSize(ImVec2(workSize.x, toolbarHeight), ImGuiCond_Always);

			ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
				ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoScrollbar |
				ImGuiWindowFlags_NoScrollWithMouse |
				ImGuiWindowFlags_NoBringToFrontOnFocus;

			if (ImGui::Begin("##Toolbar", &m_visible, flags))
			{
				// 中央揃え用のスペース
				float buttonWidth = 60.0f;
				float spacing = 10.0f;
				float totalWidth = (buttonWidth * 3) + (spacing * 2);
				float offsetX = (workSize.x - totalWidth) * 0.5f;

				ImGui::SetCursorPosX(offsetX);

				// 現在の状態を取得
				EditorCore::EditorState currentState = m_playModeController->getState();

				// Play ボタン
				bool isPlaying = (currentState == EditorCore::EditorState::Playing);
				if (isPlaying)
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
				}

				if (ImGui::Button("Play", ImVec2(buttonWidth, 30.0f)))
				{
					m_playModeController->play();
				}

				if (isPlaying)
				{
					ImGui::PopStyleColor();
				}

				ImGui::SameLine(0.0f, spacing);

				// Pause ボタン
				bool isPaused = (currentState == EditorCore::EditorState::Paused);
				if (isPaused)
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0.2f, 1.0f));
				}

				bool canPause = (currentState == EditorCore::EditorState::Playing);
				if (!canPause)
				{
					ImGui::BeginDisabled();
				}

				if (ImGui::Button("Pause", ImVec2(buttonWidth, 30.0f)))
				{
					if (isPaused)
					{
						m_playModeController->play(); // Resume
					}
					else
					{
						m_playModeController->pause();
					}
				}

				if (!canPause)
				{
					ImGui::EndDisabled();
				}

				if (isPaused)
				{
					ImGui::PopStyleColor();
				}

				ImGui::SameLine(0.0f, spacing);

				// Stop ボタン
				bool canStop = (currentState != EditorCore::EditorState::Edit);
				if (!canStop)
				{
					ImGui::BeginDisabled();
				}

				if (ImGui::Button("Stop", ImVec2(buttonWidth, 30.0f)))
				{
					m_playModeController->stop();
				}

				if (!canStop)
				{
					ImGui::EndDisabled();
				}

				// 右側に状態表示
				ImGui::SameLine();
				ImGui::SetCursorPosX(workSize.x - 150.0f);

				const char* stateText = "";
				ImVec4 stateColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

				switch (currentState)
				{
				case EditorCore::EditorState::Edit:
					stateText = "Edit Mode";
					stateColor = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
					break;
				case EditorCore::EditorState::Playing:
					stateText = "Playing";
					stateColor = ImVec4(0.2f, 1.0f, 0.2f, 1.0f);
					break;
				case EditorCore::EditorState::Paused:
					stateText = "Paused";
					stateColor = ImVec4(1.0f, 1.0f, 0.2f, 1.0f);
					break;
				}

				ImGui::TextColored(stateColor, "%s", stateText);
			}
			ImGui::End();
		}

		void setPlayModeController(EditorCore::PlayModeController* controller)
		{
			m_playModeController = controller;
		}

	private:
		EditorCore::PlayModeController* m_playModeController = nullptr;
	};
}


// editor/UI/GameLogicConsoleWindow.cpp

#include "GameLogicConsoleWindow.hpp"
#include <imgui.h>
namespace Editor::UI
{
	GameLogicConsoleWindow::GameLogicConsoleWindow()
		:ImGuiWindow("Game Logic Console", true)
	{
	}

	void GameLogicConsoleWindow::draw()
	{
		if (!m_visible) return;

		if (ImGui::Begin(m_title.c_str(), &m_visible))
		{
			// データが来ていなければプレースホルダー表示
			if (m_entries.empty())
			{
				ImGui::TextDisabled("No data Play the scene to see Lua execution");
			}
			else 
			{
				// ツリー表示に切り替える
				ImGui::Text("%zu entries recorded.", m_entries.size());
			}
		}

		ImGui::End();
	}

	void GameLogicConsoleWindow::setEntries(std::vector<GameLogicEntry> entries)
	{
		m_entries = std::move(entries);
	}
}

// src/editor/UI/GameLogicConsoleWindow.hpp
#pragma once
#include "../engine/Utils/Common.hpp"
#include "ImGuiManager.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace Editor::UI
{
	// =================================
	// 1フレーム内の1計測単位
	// =================================
	struct GameLogicEntry
	{
		uint64_t frameIndex = 0; // 何フレーム目か
		std::string objectName; //  GameObject::getName()の結果
		std::string scriptName; //  "Player.lua"など
		std::string functionName; // "onStart"と"onUpdate"をターゲットに設定
		float ms = 0.0f;
		bool succeeded = true;    // Luaでエラーが出なかったか
	}; 

	class GameLogicConsoleWindow : public ImGuiWindow
	{
	public:
		GameLogicConsoleWindow();
		~GameLogicConsoleWindow() = default;

		// コピー&ムーブ禁止
		GameLogicConsoleWindow(const GameLogicConsoleWindow&) = delete;
		GameLogicConsoleWindow& operator=(const GameLogicConsoleWindow&) = delete;
		GameLogicConsoleWindow(GameLogicConsoleWindow&&) = delete;
		GameLogicConsoleWindow& operator=(GameLogicConsoleWindow&&) = delete;

		void draw() override;

		// 毎フレーム終わりに外部から呼ぶ
		// LuaScriptComponent が計測した結果を一括で渡す
		void setEntries(std::vector<GameLogicEntry> entries);

		// 現在保持しているフレームの計測データを取得
		const std::vector<GameLogicEntry>& getEntires() const { return m_entries; }

	private:
		std::vector<GameLogicEntry> m_entries; //今フレームの計測結果
		uint64_t m_frameIndex = 0; // 外部から setEntries と同期させる想定
	};
}
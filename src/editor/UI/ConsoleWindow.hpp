#pragma once
#include "ImGuiManager.hpp"

namespace Editor::UI
{
	enum class LogType
	{
		None = 0,
		DebugLog,
		WarningLog,
		ErrorLog
	};

	class ConsoleWindow : public ImGuiWindow
	{
	public:
		ConsoleWindow() : ImGuiWindow("Console") {}
		~ConsoleWindow() = default;
		void initialize();

		void draw() override;

		void logStack(LogType type, std::string log);

		void setLogType(LogType type) { m_logType = type; }
	private:
		std::string m_consoleLog;
		LogType m_logType;
	};
}
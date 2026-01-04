#pragma once
#include <string>

namespace Engine::Scripting
{
	class CppScriptUtility
	{
	public:
		//ファイルを作成する（デフォルト内容含む）
		static bool createNewScript(const std::string& path);

		//VSCode開く
		static void openInVSCode(const std::string& path);

		//ファイル名cpp補正
		static std::string normalizePath(const std::string& name);
	};
}
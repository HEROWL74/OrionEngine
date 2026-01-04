//src/Scripting/LuaScriptUtility
#pragma once
#include <string>

namespace Engine::Scripting
{
	class LuaScriptUtility
	{
	public:
		//新しいLuaファイルを作成（デフォルト内容も含む）
		static bool createNewScript(const std::string& path);

		//VSCodeで開く
		static void openInVSCode(const std::string& path);

		//ファイル名lua補正
		static std::string normalizePath(const std::string& name);
	};
}

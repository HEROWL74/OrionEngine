//src/Scripting/LuaScriptUtility.cpp
#include "LuaScriptUtility.hpp"
#include <fstream>
#include <Windows.h>
#include <filesystem>

namespace Engine::Scripting
{
	bool LuaScriptUtility::createNewScript(const std::string& path)
	{
		std::ofstream file(path);
		if (!file.is_open()) return false;

		file << "-- " << path << "\n";
		file << "function onStart(obj)\n";
		file << "    print(\"Hello from " << path << "\")\n";
		file << "end\n\n";
		file << "function onUpdate(obj, dt)\n";
		file << "    -- update logic here\n";
		file << "end\n";
		file.close();
		return true;
	}

	void LuaScriptUtility::openInVSCode(const std::string& path)
	{
		std::filesystem::path scriptPath(path);
		std::string folder = scriptPath.parent_path().string();

		//"code"コマンドを使用
		ShellExecuteA(
			nullptr,
			"open",
			"code",
			folder.c_str(),
			nullptr,
			SW_HIDE
		);
	}

	std::string LuaScriptUtility::normalizePath(const std::string& name) {
		std::filesystem::path p(name);
		p.replace_extension(".lua");
		return p.string();
	}
}
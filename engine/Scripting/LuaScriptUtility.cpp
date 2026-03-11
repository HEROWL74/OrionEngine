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
		file << "\n";
		file << "local Script = {}\n";
		file << "\n";
		file << "function Script.onStart(obj)\n";
		file << "    print(\"Hello lua\")\n";
		file << "end\n";
		file << "\n";
		file << "function Script.onUpdate(obj, dt)\n";
		file << "    -- update logic here\n";
		file << "end\n";
		file << "\n";
		file << "return Script\n";
		file.close();
		return true;
	}

	void LuaScriptUtility::openInVSCode(const std::string& path)
	{
		std::filesystem::path scriptPath(path);
		std::string folder = scriptPath.parent_path().parent_path().string();

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


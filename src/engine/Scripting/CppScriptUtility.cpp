#include "CppScriptUtility.hpp"
#include <fstream>
#include <Windows.h>
#include <filesystem>

namespace Engine::Scripting
{
    bool CppScriptUtility::createNewScript(const std::string& path)
    {
        std::ofstream file(path);
        if (!file.is_open()) return false;

        // ファイル名（拡張子なし）をクラス名にする
        std::filesystem::path p(path);
        std::string className = p.stem().string();

        file << "#include \"Scripts.hpp\"\n\n";
        file << "class " << className << " : public Script::Cpp {\n";
        file << "public:\n";
        file << "    void OnStart() override {\n";
        file << "        // Start Code\n";
        file << "    }\n\n";
        file << "    void OnUpdate(float dt) override {\n";
        file << "        // Update Code\n";
        file << "    }\n";
        file << "};\n";

        return true;
    }


	void CppScriptUtility::openInVSCode(const std::string& path)
	{
        std::filesystem::path scriptPath(path);
        std::string folder = scriptPath.parent_path().string();

		ShellExecuteA(
			nullptr,
			"open",
			"code",
            folder.c_str(),
			nullptr,
			SW_HIDE
		);
	}

	std::string CppScriptUtility::normalizePath(const std::string& name) {
		std::filesystem::path p(name);
		p.replace_extension(".cpp"); // 強制的に置き換える
		return p.string();
	}

}
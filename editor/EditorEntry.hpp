#pragma once

#include <Windows.h>

namespace Editor
{
	int RunEditor(HINSTANCE hInstance, int nCmdShow,
		const std::filesystem::path& projectPath = {});
}


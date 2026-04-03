#pragma once

#include <Windows.h>
#include <filesystem>
namespace Editor {
int RunEditor(HINSTANCE hInstance, int nCmdShow,
              const std::filesystem::path &projectPath = {});
}

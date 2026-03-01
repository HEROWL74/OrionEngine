// src/editor/EditorMain.cpp
#include <Windows.h>
#include <filesystem>
#include "Editor/EditorEntry.hpp"

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE,
    _In_ LPSTR,
    _In_ int nCmdShow)
{
    // コマンドライン解析
    std::filesystem::path projectPath;
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    for (int i = 1; i < argc; ++i)
    {
        if (std::wstring(argv[i]) == L"--project" && i + 1 < argc)
        {
            projectPath = argv[++i];
        }
    }
    LocalFree(argv);

    return Editor::RunEditor(hInstance, nCmdShow, projectPath);
}



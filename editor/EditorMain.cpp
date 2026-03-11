// src/editor/EditorMain.cpp

// Windows Editor
#ifdef _WIN32
#include <Windows.h>
#include <filesystem>
#include "EditorEntry.hpp"


extern "C"
{
    __declspec(dllexport) extern const UINT D3D12SDKVersion = 615;
}

extern "C"
{
    __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";
}


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

#else 
#include <filesystem>
#include <cstdio>
#include "LinuxApp.hpp"

int main(int argc, char* argv[])
{
    std::filesystem::path projectPath;
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--project")
        {
            projectPath = argv[++i];
        }
    }

    Editor::LinuxApp app;
    if (!app.initialize(projectPath))
    {
        std::fprintf(stderr, "[EditorMain] LinuxApp::Initialize failed\n");
        return 1;
    }

    return app.run();
}
#endif


// runtime/GameMain.cpp
// OrionGame.exe のエントリポイント（ランチャー）

// 実行時の流れ:
//   1. exe と同じフォルダの OrionRuntime.dll を LoadLibrary
//   2. GameAppCreate / Initialize / Run / Destroy を呼ぶ
//   3. FreeLibrary して終了

#include <Windows.h>
#include <filesystem>
#include <string>

// =========================================================
// DLL から取得する関数ポインタ型
// IGameApp.hpp と同じシグネチャ（ヘッダを include しないため手書き）
// =========================================================
using FnGameAppCreate = void* (*)();
using FnGameAppInitialize = int (*)(void*, HINSTANCE, int, const wchar_t*);
using FnGameAppRun = int   (*)(void*);
using FnGameAppDestroy = void  (*)(void*);

// exe と同じディレクトリの OrionRuntime.dll のフルパスを返す
static std::wstring GetRuntimeDllPath()
{
    wchar_t exePath[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    return (std::filesystem::path(exePath).parent_path() / L"OrionRuntime.dll").wstring();
}

int WINAPI WinMain(
    _In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE,
    _In_     LPSTR,
    _In_     int nCmdShow)
{

    // --project 解析
    std::filesystem::path projectPath;
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    for (int i = 1; i < argc; ++i)
    {
        if (std::wstring(argv[i]) == L"--project" && i + 1 < argc)
            projectPath = argv[++i];
    }
    LocalFree(argv);

    // ---- OrionRuntime.dll をロード ----
    const std::wstring dllPath = GetRuntimeDllPath();
    HMODULE hRuntime = LoadLibraryW(dllPath.c_str());

    if (!hRuntime)
    {
        const DWORD err = GetLastError();
        const std::wstring msg =
            L"Failed to load OrionRuntime.dll\n"
            L"Path: " + dllPath + L"\n"
            L"Error code: " + std::to_wstring(err) + L"\n\n"
            L"Make sure OrionRuntime.dll is placed next to the .exe.";
        MessageBoxW(nullptr, msg.c_str(), L"Launcher Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    // ---- 2. 関数ポインタを取得 ----
    const auto fnCreate = reinterpret_cast<FnGameAppCreate>    (GetProcAddress(hRuntime, "GameAppCreate"));
    const auto fnInitialize = reinterpret_cast<FnGameAppInitialize>(GetProcAddress(hRuntime, "GameAppInitialize"));
    const auto fnRun = reinterpret_cast<FnGameAppRun>       (GetProcAddress(hRuntime, "GameAppRun"));
    const auto fnDestroy = reinterpret_cast<FnGameAppDestroy>   (GetProcAddress(hRuntime, "GameAppDestroy"));

    if (!fnCreate || !fnInitialize || !fnRun || !fnDestroy)
    {
        MessageBoxW(nullptr,
            L"OrionRuntime.dll is missing required exports.\n"
            L"The DLL may be corrupted or built from an incompatible version.",
            L"Launcher Error", MB_OK | MB_ICONERROR);
        FreeLibrary(hRuntime);
        return -1;
    }

    // ---- GameApp を生成・初期化・実行 ----
    void* app = fnCreate();
    if (!app)
    {
        MessageBoxW(nullptr,
            L"GameAppCreate() returned null.",
            L"Launcher Error", MB_OK | MB_ICONERROR);
        FreeLibrary(hRuntime);
        return -1;
    }

    int exitCode = 0;

    if (fnInitialize(app, hInstance, nCmdShow, projectPath.wstring().c_str()) == 0)
    {
        // 初期化成功 -> メインループ（ブロッキング）
        exitCode = fnRun(app);
    }
    else
    {
        // 失敗時のエラーダイアログは DLL 側 (GameAppExports.cpp) で表示済み
        exitCode = -1;
    }

    // ---- クリーンアップ ----
    fnDestroy(app);
    FreeLibrary(hRuntime);

    return exitCode;
}


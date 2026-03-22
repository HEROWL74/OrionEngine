#pragma once
#include <Windows.h>

#ifdef ORION_RUNTIME_EXPORTS
#   define ORION_API __declspec(dllexport)
#else
#   define ORION_API __declspec(dllimport)
#endif

extern "C"
{
    // GameApp インスタンスを生成して返す（失敗時は nullptr）
    ORION_API void* GameAppCreate();

    // 初期化。戻り値 0=成功、非0=失敗
    ORION_API int GameAppInitialize(void* app, HINSTANCE hInstance, int nCmdShow,
        const wchar_t* projectPath);

    // メインループ実行（ブロッキング）。戻り値=終了コード
    ORION_API int   GameAppRun(void* app);

    // インスタンスを破棄
    ORION_API void  GameAppDestroy(void* app);
}


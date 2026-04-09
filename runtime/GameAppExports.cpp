#define ORION_RUNTIME_EXPORTS
#include "GameApp.hpp"
#include "IGameApp.hpp"
#include <filesystem>
#include <new>


extern "C" {
ORION_API void *GameAppCreate() {
  return new (std::nothrow) Runtime::GameApp();
}

ORION_API int GameAppInitialize(void *app, HINSTANCE hInstance, int nCmdShow,
                                const wchar_t *projectPath) {
  if (!app)
    return -1;

  auto result = static_cast<Runtime::GameApp *>(app)->initialize(
      hInstance, nCmdShow,
      std::filesystem::path(projectPath ? projectPath : L""));
  if (!result) {
    MessageBoxA(nullptr, result.error().message.c_str(), "Initialization Error",
                MB_OK | MB_ICONERROR);
    return -1;
  }
  return 0;
}

ORION_API int GameAppRun(void *app) {
  if (!app)
    return -1;
  return static_cast<Runtime::GameApp *>(app)->run();
}

ORION_API void GameAppDestroy(void *app) {
  delete static_cast<Runtime::GameApp *>(app);
}
}

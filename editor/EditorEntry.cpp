
#include "../engine/Utils/Common.hpp"
#include "EditorApp.hpp"
#include <iostream>


namespace Editor {
int RunEditor(HINSTANCE hInstance, int nCmdShow,
              const std::filesystem::path &projectPath) {
#ifdef _DEBUG
  AllocConsole();

  FILE *fp;
  freopen_s(&fp, "CONOUT$", "w", stdout);
  freopen_s(&fp, "CONOUT$", "w", stderr);
  freopen_s(&fp, "CONIN$", "r", stdin);

  std::ios::sync_with_stdio(true);

  std::cout << "=== Orion Editor Debug Console ===\n";
#endif

  Editor::EditorApp app;

  auto initResult = app.initialize(hInstance, nCmdShow, projectPath);
  if (!initResult) {
    std::string errorDetail = initResult.error().what();

    MessageBoxA(NULL, errorDetail.c_str(), "Orion Engine - Fatal Error",
                MB_ICONERROR | MB_OK);

    Engine::Utils::log_error(initResult.error());
    return -1;
  }

  const int exitCode = app.run();

#ifdef _DEBUG
  std::cout << "Editor exited with code: " << exitCode << std::endl;
  FreeConsole();
#endif
  return exitCode;
}
} // namespace Editor

// editor/EditorMain.cpp

// Windows Editor
#ifdef _WIN32
#include "EditorEntry.hpp"
#include <Windows.h>
#include <filesystem>
#include <shellapi.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/xform.h>

#include "Utils/Common.hpp"

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPSTR,
                   _In_ int nCmdShow) {
  // コマンドライン解析
  std::filesystem::path projectPath;
  int argc;
  LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  for (int i = 1; i < argc; ++i) {
    if (std::wstring(argv[i]) == L"--project" && i + 1 < argc) {
      projectPath = argv[++i];
    }
  }
  LocalFree(argv);
  
  const std::string stageFile = "cube.usda";
  pxr::UsdStageRefPtr stage = pxr::UsdStage::CreateNew(stageFile);
  if (!stage)
  {
    Engine::Utils::log_info("Failed to create USD Stage.");
    return 1;
  }
  
  pxr::UsdGeomXform xform = pxr::UsdGeomXform::Define(stage, pxr::SdfPath("/OrionEngineCube"));
  if (!xform)
  {
    Engine::Utils::log_info("Failed to create xform prim.");
    return 1;
  }
  
  pxr::UsdGeomCube cube = pxr::UsdGeomCube::Define(stage, pxr::SdfPath("/OrionEngineCube/Cube"));
  if (!cube)
  {
    Engine::Utils::log_info("Failed to create Cube prim.");
    return 1;
  }
  
  cube.CreateSizeAttr().Set(2.0);
  stage->GetRootLayer()->Save();
  
  Engine::Utils::log_info("Successfully created USD Stage.");

  return Editor::RunEditor(hInstance, nCmdShow, projectPath);
}

#else
#include "LinuxApp.hpp"
#include <cstdio>
#include <filesystem>

int main(int argc, char *argv[]) {
  std::filesystem::path projectPath;
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "--project") {
      projectPath = argv[++i];
    }
  }

  Editor::LinuxApp app;
  if (!app.initialize(projectPath)) {
    std::fprintf(stderr, "[EditorMain] LinuxApp::Initialize failed\n");
    return 1;
  }

  return app.run();
}
#endif

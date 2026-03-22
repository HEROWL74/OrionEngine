
#include <iostream>
#include "../engine/Utils/Common.hpp"
#include "EditorApp.hpp"

namespace Editor
{
    int RunEditor(HINSTANCE hInstance, int nCmdShow,
        const std::filesystem::path& projectPath)
    {
#ifdef _DEBUG
        AllocConsole();
        freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
        freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
        std::cout << "=== Orion Editor Debug Console ===\n";
#endif

        Editor::EditorApp app;

        auto initResult = app.initialize(hInstance, nCmdShow, projectPath);
        if (!initResult)
        {
            std::string errorDetail = initResult.error().what();

            MessageBoxA(NULL, errorDetail.c_str(), "Orion Engine - Fatal Error", MB_ICONERROR | MB_OK);

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
}


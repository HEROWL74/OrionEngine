
#include <iostream>
#include "engine/Utils/Common.hpp"
#include "EditorApp.hpp"

namespace Editor
{
    int RunEditor(HINSTANCE hInstance, int nCmdShow)
    {

        Editor::EditorApp app;

        auto initResult = app.initialize(hInstance, nCmdShow);
        if (!initResult)
        {
            Engine::Utils::log_error(initResult.error());
            return -1;
        }

        const int exitCode = app.run();


        return exitCode;
    }
}

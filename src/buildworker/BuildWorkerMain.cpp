// src/editor/build/BuildWorkerMain.cpp
#include <Windows.h>
#include <iostream>

#include "BuildSystem.hpp"

using namespace Editor::Build;

int main(int argc, char** argv)
{
    MessageBoxA(nullptr, "BuildWorker STARTED", "DEBUG", MB_OK);
    try
    {
        // ===== コンソール準備 =====
        AllocConsole();
        FILE* out;
        freopen_s(&out, "CONOUT$", "w", stdout);
        freopen_s(&out, "CONOUT$", "w", stderr);

        SetConsoleTitleW(L"Orion Build Console");

        std::cout << "Starting build...\n\n";

        // ===== BuildSystem 起動 =====
        BuildSystem buildSystem;

        buildSystem.setProgressCallback(
            [](const BuildResult& result)
            {
                // シンプルにログ出すだけ
                std::cout
                    << "[" << static_cast<int>(result.status) << "] "
                    << result.message
                    << " (" << int(result.progress * 100) << "%)\n";
            }
        );

        bool success = buildSystem.build();

        // ===== 結果 =====
        if (success)
        {
            std::cout << "\n=================================\n";
            std::cout << "Build completed successfully!\n";
            std::cout << "Output: " << buildSystem.getResult().outputPath << "\n";
            std::cout << "=================================\n";
        }
        else
        {
            std::cout << "\n=================================\n";
            std::cout << "Build FAILED.\n";
            std::cout << "=================================\n";
        }

        system("pause");
        return success ? 0 : 1;

    }
    catch (const std::filesystem::filesystem_error& e)
    {
        std::cerr << "Filesystem error:\n";
        std::cerr << e.what() << "\n";
        system("pause");
        return 2;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception:\n";
        std::cerr << e.what() << "\n";
        system("pause");
        return 3;
    }
}

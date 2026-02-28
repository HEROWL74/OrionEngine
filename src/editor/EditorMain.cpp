// src/editor/EditorMain.cpp
#include <Windows.h>
#include "Editor/EditorEntry.hpp"

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE,
    _In_ LPSTR,
    _In_ int nCmdShow)
{
    return Editor::RunEditor(hInstance, nCmdShow);
}



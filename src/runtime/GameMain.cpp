// src/runtime/GameMain.cpp
#include <Windows.h>
#include "GameApp.hpp"

int WINAPI WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE,
	_In_ LPSTR,
	_In_ int nCmdShow)
{
	Runtime::GameApp app;

	auto init = app.initialize(hInstance, nCmdShow);
	if (!init)
		return -1;

#ifdef _DEBUG
	MessageBoxA(nullptr, "WinMain entered", "DEBUG", MB_OK);
#endif

	return app.run();
}
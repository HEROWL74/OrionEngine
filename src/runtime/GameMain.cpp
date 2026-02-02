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
	{
		// エラーメッセージを表示
		MessageBoxA(nullptr,
			init.error().message.c_str(),
			"Initialization Error",
			MB_OK | MB_ICONERROR);
		return -1;
	}

	int result = app.run();

	return result;
}
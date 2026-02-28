// src/Core/Window.cpp
#include "Window.hpp"
#include "../resources/orion_resource.h"
#include <format>

namespace Engine::Core {
	Window::~Window()
	{
		destroy();
	}

	Utils::VoidResult Window::create(HINSTANCE hInstance, const WindowSettings& settings)
	{
		m_hInstance = hInstance;

		auto result = registerWindowClass(hInstance);
		if (!result)
		{
			return result;
		}

		DWORD windowStyle = WS_OVERLAPPEDWINDOW;
		if (!settings.resizable)
		{
			windowStyle &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
		}

		RECT windowRect = { 0,0,settings.width, settings.height };
		AdjustWindowRect(&windowRect, windowStyle, FALSE);

		const int windowWidth = windowRect.right - windowRect.left;
		const int windowHeight = windowRect.bottom - windowRect.top;

		m_handle = CreateWindowExW(
			0,
			m_className.c_str(),
			settings.title.c_str(),
			windowStyle,
			settings.x,
			settings.y,
			windowWidth,
			windowHeight,
			nullptr,
			nullptr,
			hInstance,
			this
		);

		DragAcceptFiles(m_handle, TRUE);

		CHECK_CONDITION(m_handle != nullptr, Utils::ErrorType::WindowCreation,
			"Failed to create window");

		HICON hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
		CHECK_CONDITION(hIcon != nullptr, Utils::ErrorType::WindowCreation,
			"Failed to load window icon");

		SendMessageW(m_handle, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
		SendMessageW(m_handle, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

		m_inputManager = std::make_unique<Input::InputManager>();
		m_inputManager->initialize(m_handle);

		Utils::log_info(std::format("Window created: {}x{}", settings.width, settings.height));

		return {};
	}

	void Window::show(int nCmdShow) const noexcept
	{
		if (m_handle)
		{
			ShowWindow(m_handle, nCmdShow);
			UpdateWindow(m_handle);
		}
	}

	bool Window::processMessages() const noexcept
	{
		MSG msg{};
		while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				return false;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (m_inputManager)
		{
			m_inputManager->update();
		}

		return true;
	}

	std::pair<int, int> Window::getClientSize() const noexcept
	{
		if (!m_handle)
		{
			return { 0,0 };
		}

		RECT clientRect;
		GetClientRect(m_handle, &clientRect);
		return { clientRect.right - clientRect.left, clientRect.bottom - clientRect.top };
	}

	void Window::setTitle(std::wstring_view title) const noexcept
	{
		if (m_handle)
		{
			SetWindowTextW(m_handle, title.data());
		}
	}

	Input::InputManager* Window::getInputManager() const noexcept
	{
		return m_inputManager.get();
	}

	// ウィンドウクラス登録
	Utils::VoidResult Window::registerWindowClass(HINSTANCE hInstance)
	{
		// クラスネーム登録
		m_className = L"OrionEngineWindow";

		WNDCLASSEXW wcex{};
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = staticWindowProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInstance;
		wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
		wcex.lpszMenuName = nullptr;
		wcex.lpszClassName = m_className.c_str();
		wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));

		const ATOM atom = RegisterClassExW(&wcex);
		CHECK_CONDITION(atom != 0, Utils::ErrorType::WindowCreation,
			"Failed to register window class");

		return {};
	}

	LRESULT CALLBACK Window::staticWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		Window* window = nullptr;

		if (uMsg == WM_NCCREATE)
		{
			auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
			window = static_cast<Window*>(cs->lpCreateParams);
			SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
		}
		else
		{
			window = reinterpret_cast<Window*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
		}

		if (window)
			return window->windowProc(hWnd, uMsg, wParam, lParam);

		return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}



	LRESULT Window::windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		// Editor / Runtime に先に投げる
		if (m_messageCallback)
		{
			if (m_messageCallback(hWnd, uMsg, wParam, lParam))
			{
				return 0; // 外部が処理した
			}
		}

		if (uMsg == WM_INPUT)
		{
			if (m_inputManager)
			{
				m_inputManager->handleRawInput(lParam);
			}
			return 0; // WM_INPUTを処理した場合は0を返す
		}

		// engine 内部の処理（Input / Resize 等）
		switch (uMsg)
		{
		case WM_SIZE:
			if (m_resizeCallback)
			{
				m_resizeCallback(LOWORD(lParam), HIWORD(lParam));
			}
			return 0;

		case WM_CLOSE:
			if (m_closeCallback)
			{
				m_closeCallback();
			}
			DestroyWindow(hWnd);
			return 0;

		case WM_DROPFILES:
		{
			std::vector<std::filesystem::path> paths;

			HDROP hDrop = (HDROP)wParam;
			UINT count = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);

			for (UINT i = 0; i < count; ++i)
			{
				wchar_t path[MAX_PATH];
				DragQueryFileW(hDrop, i, path, MAX_PATH);
				paths.emplace_back(path);
			}

			DragFinish(hDrop);

			if (m_externalDropCallback)
				m_externalDropCallback(paths);

			return 0;
		}

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		}

		// 未処理は OS に返す
		return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}


	void Window::destroy() noexcept
	{
		if (m_handle)
		{
			SetWindowLongPtrW(m_handle, GWLP_USERDATA, 0);
			DestroyWindow(m_handle);
			m_handle = nullptr;
		}

		if (m_hInstance && !m_className.empty())
		{
			UnregisterClassW(m_className.c_str(), m_hInstance);
			m_className.clear();
		}

		m_hInstance = nullptr;
	}

}


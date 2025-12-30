// src/Core/Window.cpp
#include "Window.hpp"
#include "../../resources/resource.h"
#include <format>
#include "../UI/ImGuiManager.hpp"
namespace Engine::Core {
	Window::~Window()
	{
		destroy();
	}

	Window::Window(Window&& other) noexcept
		:m_handle(other.m_handle)
		, m_className(std::move(other.m_className))
		, m_hInstance(other.m_hInstance)
		, m_inputManager(std::move(other.m_inputManager))
		, m_resizeCallback(std::move(other.m_resizeCallback))
		, m_closeCallback(std::move(other.m_closeCallback))
	{
		other.m_handle = nullptr;
		other.m_hInstance = nullptr;
	}

	Window& Window::operator=(Window&& other) noexcept
	{
		if (this != &other)
		{
			destroy();

			m_handle = other.m_handle;
			m_className = std::move(other.m_className);
			m_hInstance = other.m_hInstance;
			m_inputManager = std::move(other.m_inputManager);
			m_resizeCallback = std::move(other.m_resizeCallback);
			m_closeCallback = std::move(other.m_closeCallback);

			other.m_handle = nullptr;
			other.m_hInstance = nullptr;

		}
		return *this;
	}

	// 繝代ヶ繝ｪ繝・け繝｡繧ｽ繝・ラ

	Utils::VoidResult Window::create(HINSTANCE hInstance, const WindowSettings& settings)
	{
		m_hInstance = hInstance;

		// 繧ｦ繧｣繝ｳ繝峨え繧ｯ繝ｩ繧ｹ繧堤匳骭ｲ縺吶ｋ
		auto result = registerWindowClass(hInstance);
		if (!result)
		{
			return result;
		}

		// 繧ｦ繧｣繝ｳ繝峨え繧ｹ繧ｿ繧､繝ｫ繧呈ｱｺ螳・
		DWORD windowStyle = WS_OVERLAPPEDWINDOW;
		if (!settings.resizable)
		{
			windowStyle &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
		}

		// 繧ｯ繝ｩ繧､繧｢繝ｳ繝磯伜沺縺ｮ繧ｵ繧､繧ｺ縺九ｉ繧ｦ繧｣繝ｳ繝峨え繧ｵ繧､繧ｺ繧定ｨ育ｮ・
		RECT windowRect = { 0,0,settings.width, settings.height };
		AdjustWindowRect(&windowRect, windowStyle, FALSE);

		const int windowWidth = windowRect.right - windowRect.left;
		const int windowHeight = windowRect.bottom - windowRect.top;

		// 繧ｦ繧｣繝ｳ繝峨え菴懈・
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

		HICON hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
		CHECK_CONDITION(hIcon != nullptr, Utils::ErrorType::WindowCreation,
			"Failed to load window icon");

		SendMessageW(m_handle, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
		SendMessageW(m_handle, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

		CHECK_CONDITION(m_handle != nullptr, Utils::ErrorType::WindowCreation,
			"Failed to create window");

		// 蜈･蜉帙す繧ｹ繝・Β繧貞・譛溷喧
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
		m_className = std::format(L"GameEngineWindow_{}",
			reinterpret_cast<uintptr_t>(this));

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
			CREATESTRUCTW* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
			window = static_cast<Window*>(createStruct->lpCreateParams);
			SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
		}
		else
		{
			window = reinterpret_cast<Window*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
		}

		if (window)
		{
			return window->windowProc(hWnd, uMsg, wParam, lParam);
		}

		return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}

	LRESULT Window::windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if (uMsg == WM_SIZE || uMsg == WM_SIZING || uMsg == WM_ENTERSIZEMOVE || uMsg == WM_EXITSIZEMOVE)
		{
			Utils::log_info(std::format("Window message received: 0x{:04x}", uMsg));
			if (uMsg == WM_SIZE)
			{
				int width = LOWORD(lParam);
				int height = HIWORD(lParam);
				Utils::log_info(std::format("WM_SIZE: {}x{}, wParam: {}", width, height, wParam));
			}
		}

		if (m_imguiManager) {
			Utils::log_info(std::format("Forwarding message 0x{:04x} to ImGui", uMsg));
			m_imguiManager->handleWindowMessage(hWnd, uMsg, wParam, lParam);


			ImGui::SetCurrentContext(m_imguiManager->getContext());
			ImGuiIO& io = ImGui::GetIO();

			if ((uMsg == WM_KEYDOWN || uMsg == WM_KEYUP || uMsg == WM_CHAR) && io.WantCaptureKeyboard)
			{
				Utils::log_info("ImGui captured keyboard message");
				return 0;
			}


			if ((uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP || uMsg == WM_RBUTTONDOWN ||
				uMsg == WM_RBUTTONUP || uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONUP ||
				uMsg == WM_MOUSEWHEEL || uMsg == WM_MOUSEMOVE) && io.WantCaptureMouse)
			{
				Utils::log_info("ImGui captured mouse message");
				return 0;
			}
		}


		if (m_inputManager)
		{
			ImGuiIO* io = nullptr;
			if (m_imguiManager && m_imguiManager->isInitialized() && m_imguiManager->getContext())
			{
				ImGui::SetCurrentContext(m_imguiManager->getContext());
				io = &ImGui::GetIO();
			}

			bool shouldProcessInput = true;
			if (io)
			{
				if ((uMsg == WM_KEYDOWN || uMsg == WM_KEYUP) && io->WantCaptureKeyboard)
					shouldProcessInput = false;
				if ((uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) && io->WantCaptureMouse)
					shouldProcessInput = false;
			}

			if (shouldProcessInput && m_inputManager->handleWindowMessage(hWnd, uMsg, wParam, lParam))
			{
				Utils::log_info(std::format("Input manager handled message 0x{:04x}", uMsg));
				return 0;
			}
		}

		switch (uMsg)
		{
		case WM_SIZE:
		{
			const int width = LOWORD(lParam);
			const int height = HIWORD(lParam);

			Utils::log_info(std::format("WM_SIZE received: {}x{}, wParam: {}", width, height, wParam));

			if (wParam == SIZE_MINIMIZED || width <= 0 || height <= 0)
			{
				Utils::log_info("Skipping resize (minimized or invalid size)");
				return 0;
			}

			static bool isResizing = false;
			if (isResizing)
			{
				Utils::log_info("Resize already in progress, skipping");
				return 0;
			}

			isResizing = true;


			if (m_resizeCallback)
			{
				Utils::log_info("Calling application resize callback");
				m_resizeCallback(width, height);
				Utils::log_info("Application resize callback completed");
			}

			isResizing = false;
			return 0;
		}
		break;

		case WM_ENTERSIZEMOVE:
			Utils::log_info("WM_ENTERSIZEMOVE received");
			break;

		case WM_EXITSIZEMOVE:
			Utils::log_info("WM_EXITSIZEMOVE received");
			break;

		case WM_SIZING:
			Utils::log_info("WM_SIZING received");
			break;

		case WM_ACTIVATE:
		{
			Utils::log_info(std::format("WM_ACTIVATE: wParam = {}", LOWORD(wParam)));

			if (LOWORD(wParam) == WA_INACTIVE && m_inputManager)
			{
				if (m_inputManager->getMouseState().isRelativeMode)
				{
					m_inputManager->setRelativeMouseMode(false);
					Utils::log_info("Window lost focus - disabled relative mouse mode");
				}
			}
		}
		break;

		case WM_KILLFOCUS:
		{
			Utils::log_info("WM_KILLFOCUS received");

			if (m_inputManager && m_inputManager->getMouseState().isRelativeMode)
			{
				m_inputManager->setRelativeMouseMode(false);
				Utils::log_info("Window lost focus - disabled relative mouse mode");
			}
		}
		break;

		case WM_CLOSE:
		{
			Utils::log_info("WM_CLOSE received");

			if (m_inputManager)
			{
				m_inputManager->setRelativeMouseMode(false);
			}
			if (m_closeCallback)
			{
				m_closeCallback();
			}
			PostQuitMessage(0);
		}
		break;

		case WM_KEYDOWN:
		{
			Utils::log_info(std::format("WM_KEYDOWN: key = {}", wParam));

			if (wParam == VK_F1)
			{
				if (m_inputManager)
				{
					bool currentMode = m_inputManager->getMouseState().isRelativeMode;
					m_inputManager->setRelativeMouseMode(!currentMode);
					Utils::log_info(std::format("Mouse relative mode: {}", !currentMode ? "ON" : "OFF"));
				}
			}

			if (wParam == VK_F4 && (GetKeyState(VK_MENU) & 0x8000))
			{
				Utils::log_info("Alt+F4 pressed");
				if (m_inputManager)
				{
					m_inputManager->setRelativeMouseMode(false);
				}
				PostQuitMessage(0);
				return 0;
			}
			else if (wParam == VK_ESCAPE)
			{
				Utils::log_info("ESC pressed");
				if (m_inputManager)
				{
					m_inputManager->setRelativeMouseMode(false);
				}
				PostQuitMessage(0);
				return 0;
			}
		}
		break;

		case WM_DESTROY:
		{
			Utils::log_info("WM_DESTROY received");
			PostQuitMessage(0);
		}
		break;

		default:
			// 鬆ｻ郢√↑繝｡繝・そ繝ｼ繧ｸ縺ｯ髯､螟悶＠縺ｦ繝ｭ繧ｰ蜃ｺ蜉・
			if (uMsg != WM_MOUSEMOVE && uMsg != WM_NCHITTEST && uMsg != WM_SETCURSOR &&
				uMsg != WM_GETTEXT && uMsg != WM_GETTEXTLENGTH && uMsg != WM_PAINT)
			{
				Utils::log_info(std::format("Unhandled message: 0x{:04x}", uMsg));
			}
			return DefWindowProcW(hWnd, uMsg, wParam, lParam);
		}

		return 0;
	}

	void Window::destroy() noexcept {
		if (m_inputManager)
		{
			m_inputManager->shutdown();
			m_inputManager.reset();
		}

		if (m_handle)
		{
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

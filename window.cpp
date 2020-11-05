#include "window.h"

// WindowClass defWindowClass(L"defWindowClass");

WindowClass::WindowClass(std::wstring className)
	: className(std::move(className)),
	  registered(false)
{
	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = &Window::WindowProc;
	wc.hInstance = GetModuleHandleW(nullptr);
	wc.lpszClassName = this->className.c_str();

	registered = RegisterClassExW(&wc);
}

WindowClass::~WindowClass()
{
	if (registered) UnregisterClassW(className.c_str(), GetModuleHandleW(nullptr));
}

LRESULT CALLBACK Window::WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_CREATE)
	{
		SetLastError(0);
		if (!SetWindowLongPtrW(
			hWnd,
			GWLP_USERDATA,
			(long long)reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams
		) && GetLastError()) return -1;
		return 0;
	}

	Window* const window = reinterpret_cast<Window*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
	if (window)
	{
		if (msg == WM_DESTROY)
		{
			window->hWnd = nullptr;
			return 0;
		}

		return window->wndProc(msg, wParam, lParam);
	}
	return DefWindowProcW(hWnd, msg, wParam, lParam);
}

Window::Window(const WindowClass& wc, DWORD style, DWORD exStyle, const wchar_t* name, HWND parent)
{
	if (!wc.registered) throw Exception("failed to register window class");
	hWnd = CreateWindowExW(
		exStyle,
		wc.className.c_str(),
		name,
		style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		parent,
		nullptr,
		GetModuleHandleW(nullptr),
		this
	);
	if (!hWnd) throw Exception("failed to create window");
	ShowWindow(hWnd, SW_SHOW);
}

void Window::update()
{
	MSG msg;
	if (GetMessageW(&msg, hWnd, 0, 0) == -1) throw Exception("failed to get message");
	TranslateMessage(&msg);
	DispatchMessageW(&msg);
}
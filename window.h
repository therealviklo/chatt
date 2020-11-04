#pragma once
#include "win.h"
#include "utils.h"

class Window;

class WindowClass
{
	friend Window;
private:
	std::wstring className;
	bool registered;
public:
	WindowClass(std::wstring className);
	~WindowClass();

	WindowClass(const WindowClass&) = delete;
	WindowClass& operator=(const WindowClass&) = delete;
};

class Window
{
	friend WindowClass;
public:
	EXCEPT(Exception);
private:
	UHandle<HWND, DestroyWindow> hWnd;
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
public:
	Window() noexcept = default;
	Window(const WindowClass& wc, DWORD style, DWORD exStyle, const wchar_t* name, HWND parent = nullptr);
	virtual ~Window() = default;

	virtual LRESULT wndProc(UINT msg, WPARAM wParam, LPARAM lParam) { return DefWindowProcW(hWnd, msg, wParam, lParam); }

	void update();

	constexpr operator HWND() noexcept { return hWnd; }
	constexpr operator bool() const noexcept { return hWnd; }
};

extern WindowClass defWindowClass;
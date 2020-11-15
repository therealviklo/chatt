#include "window.h"

WindowClass defWindowClass(L"defWindowClass", (HBRUSH)COLOR_BACKGROUND, LoadCursorW(nullptr, IDC_ARROW));

void updateAllWindows()
{
	MSG msg;
	if (GetMessageW(&msg, nullptr, 0, 0) == -1) throw Window::Exception("failed to get message");
	TranslateMessage(&msg);
	DispatchMessageW(&msg);
}

WindowClass::WindowClass(std::wstring className, HBRUSH backgroundColour, HCURSOR cursor)
	: className(std::move(className)),
	  registered(false)
{
	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = &Window::WindowProc;
	wc.hInstance = GetModuleHandleW(nullptr);
	wc.lpszClassName = this->className.c_str();
	wc.hbrBackground = backgroundColour + 1;
	wc.hCursor = cursor;

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
		switch (msg)
		{
			case WM_CLOSE:
			{
				window->hWnd = nullptr;
			}
			return 0;
			case WM_DESTROY:
			{
				/* Om fönstret har förstörts av något annat än window->hWnd:s destruktor så
				   ser det här till att destruktorn inte också gör det. (Annars gör det här
				   ingen skillnad.) */
				window->hWnd.resetNoClose();
			}
			return 0;
		}

		return window->wndProc(msg, wParam, lParam);
	}
	return DefWindowProcW(hWnd, msg, wParam, lParam);
}

Window::Window(
	const WindowClass& wc,
	DWORD style,
	DWORD exStyle,
	const wchar_t* name,
	HWND parent,
	int width,
	int height
)
{
	if (!wc.registered) throw Exception("failed to register window class");
	hWnd = CreateWindowExW(
		exStyle,
		wc.className.c_str(),
		name,
		style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		width,
		height,
		parent,
		nullptr,
		GetModuleHandleW(nullptr),
		this
	);
	if (!hWnd) throw Exception("failed to create window");
	ShowWindow(hWnd, SW_SHOW);
}

Window::Window(
	const WindowClass& wc,
	DWORD style,
	DWORD exStyle,
	const wchar_t* name,
	Menu&& menu,
	HWND parent,
	int width,
	int height
)
{
	if (!wc.registered) throw Exception("failed to register window class");
	hWnd = CreateWindowExW(
		exStyle,
		wc.className.c_str(),
		name,
		style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		width,
		height,
		parent,
		menu.menu,
		GetModuleHandleW(nullptr),
		this
	);
	if (!hWnd) throw Exception("failed to create window");
	menu.menu.resetNoClose();
	ShowWindow(hWnd, SW_SHOW);
}

void Window::update()
{
	MSG msg;
	if (GetMessageW(&msg, hWnd, 0, 0) == -1) throw Exception("failed to get message");
	TranslateMessage(&msg);
	DispatchMessageW(&msg);
}

LRESULT Control::subclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR idSubclass, DWORD_PTR refData)
{
	if (msg == WM_DESTROY)
	{
		reinterpret_cast<Control*>(refData)->hWnd.resetNoClose();
		return TRUE;
	}
	return reinterpret_cast<Control*>(refData)->proc(msg, wParam, lParam);
}

Control::Control(const wchar_t* wc, DWORD style, DWORD exStyle, const wchar_t* name, HWND parent)
{
	hWnd = CreateWindowExW(
		exStyle,
		wc,
		name,
		style ^ WS_VISIBLE ^ WS_CHILD,
		0,
		0,
		0,
		0,
		parent,
		nullptr,
		GetModuleHandleW(nullptr),
		nullptr
	);
	if (!hWnd) throw Exception("failed to create control");

	if (!SetWindowSubclass(hWnd, subclassProc, 1, (DWORD_PTR)this))
		throw Exception("failed to set control subclass");
}

std::wstring Control::getText()
{
	std::wstring s;
	const size_t size = SendMessageW(*this, WM_GETTEXTLENGTH, 0, 0);
	if (size)
	{
		s.resize(size);
		SendMessageW(*this, WM_GETTEXT, s.size(), reinterpret_cast<LPARAM>(&s[0]));
	}
	return s;
}

uint32_t IpAddress::getAddress()
{
	uint32_t addr = 0;
	SendMessageW(*this, IPM_GETADDRESS, 0, reinterpret_cast<LPARAM>(&addr));
	return addr;
}

void UpDown::setRange(int min, int max)
{
	SendMessageW(*this, UDM_SETRANGE32, min, max);
}

UpDown::UpDown(DWORD style, DWORD exStyle, HWND buddy, HWND parent)
	: UpDown(style, exStyle, parent)
{
	SendMessageW(*this, UDM_SETBUDDY, reinterpret_cast<WPARAM>(buddy), 0);
}

int UpDown::getValue()
{
	BOOL success = false;
	int ret = SendMessageW(*this, UDM_GETPOS32, 0, reinterpret_cast<LPARAM>(&success));
	if (success) throw Exception("Kunde inte hämta värde från nummerinmatare");
	return ret;
}

Menu::Menu(std::initializer_list<std::variant<MenuItem, SubMenu>> elements)
	: menu(CreateMenu())
{
	if (!menu) throw Exception("failed to create menu");

	for (auto& e : elements)
	{
		if (std::holds_alternative<MenuItem>(e))
		{
			if (!AppendMenuW(
				menu,
				MF_STRING,
				std::get<MenuItem>(e).second,
				std::get<MenuItem>(e).first.c_str()
			)) throw Exception("failed to add menu item");
		}
		else if (std::holds_alternative<SubMenu>(e))
		{
			if (!AppendMenuW(
				menu,
				MF_POPUP,
				(UINT_PTR)(HMENU)const_cast<Menu*>(&std::get<SubMenu>(e).second)->menu,
				std::get<SubMenu>(e).first.c_str()
			)) throw Exception("failed to add menu item");
			const_cast<Menu*>(&std::get<SubMenu>(e).second)->menu.resetNoClose();
		}
	}
}
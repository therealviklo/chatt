#pragma once
#include <variant>
#include "win.h"
#include <commctrl.h>
#include "utils.h"

// Klasser för att göra fönster.

void updateAllWindows();

// Forwarddeklaration
class Window;

/* RAII-wrapper för en window class (som inte är en C++-klass utan något
   som Windows använder). En global default-window-class deklareras senare
   i filen. */
class WindowClass
{
	friend Window;
private:
	std::wstring className;
	bool registered;
public:
	WindowClass(std::wstring className, HBRUSH backgroundColour, HCURSOR cursor);
	~WindowClass();

	WindowClass(const WindowClass&) = delete;
	WindowClass& operator=(const WindowClass&) = delete;
};

class Menu;

/* En klass som representerar ett generiskt fönster. Det är inte meningen att man direkt
   ska skapa instanser av klassen Window utan att man ska göra en underklass till Window
   och instantiera den (se gui.h). */
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
	Window(
		const WindowClass& wc,
		DWORD style,
		DWORD exStyle,
		const wchar_t* name,
		HWND parent = nullptr,
		int width = CW_USEDEFAULT,
		int height = CW_USEDEFAULT
	);
	Window(
		const WindowClass& wc,
		DWORD style,
		DWORD exStyle,
		const wchar_t* name,
		Menu&& menu,
		HWND parent = nullptr,
		int width = CW_USEDEFAULT,
		int height = CW_USEDEFAULT
	);
	virtual ~Window() = default;

	/* En virtuell funktion som får parametrarna från den "riktiga" WndProc:en. Det är meningen att underklasserna
	   ska överskrida den här funktionen. */
	virtual LRESULT wndProc(UINT msg, WPARAM wParam, LPARAM lParam) { return DefWindowProcW(hWnd, msg, wParam, lParam); }

	// Kör den här för att få fönstret att uppdatera. (Annars fryser det.)
	void update();

	constexpr operator HWND() noexcept { return hWnd; }
	constexpr operator bool() const noexcept { return hWnd; }
};

class Control
{
public:
	EXCEPT(Exception);
private:
	UHandle<HWND, DestroyWindow> hWnd;

	static LRESULT subclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR idSubclass, DWORD_PTR refData);
protected:
	virtual LRESULT proc(UINT msg, WPARAM wParam, LPARAM lParam) { return DefSubclassProc(*this, msg, wParam, lParam); }
public:
	Control(const wchar_t* wc, DWORD style, DWORD exStyle, const wchar_t* name, HWND parent);
	virtual ~Control() = default;

	std::wstring getText();

	constexpr operator HWND() noexcept { return hWnd; }
	constexpr operator bool() const noexcept { return hWnd; }
};

class EditControl : public Control
{
public:
	EditControl(DWORD style, DWORD exStyle, HWND parent)
		: Control(L"Edit", style, exStyle, nullptr, parent) {}
};

class Button : public Control
{
public:
	Button(const wchar_t* text, DWORD style, DWORD exStyle, HWND parent)
		: Control(L"Button", style, exStyle, text, parent) {}
};

class IpAddress : public Control
{
public:
	IpAddress(DWORD style, DWORD exStyle, HWND parent)
		: Control(L"SysIPAddress32", style, exStyle, nullptr, parent) {}

	uint32_t getAddress();
};

class UpDown : public Control
{
public:
	UpDown(DWORD style, DWORD exStyle, HWND parent)
		: Control(L"msctls_updown32", style, exStyle, nullptr, parent) {}
	UpDown(DWORD style, DWORD exStyle, HWND buddy, HWND parent);

	void setRange(int min, int max);
	int getValue();
};

class Label : public Control
{
public:
	Label(const wchar_t* text, DWORD style, DWORD exStyle, HWND parent)
		: Control(L"Static", style, exStyle, text, parent) {}
};

typedef std::pair<std::wstring, Menu> SubMenu;
typedef std::pair<std::wstring, UINT_PTR> MenuItem;
class Menu
{
	friend Window;
public:
	EXCEPT(Exception)
private:
	UHandle<HMENU, DestroyMenu> menu;
public:
	Menu(std::initializer_list<std::variant<MenuItem, SubMenu>> elements);
};

extern WindowClass defWindowClass;
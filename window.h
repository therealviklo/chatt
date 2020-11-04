#pragma once
#include "win.h"
#include "utils.h"

// Klasser för att göra fönster.

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
	WindowClass(std::wstring className);
	~WindowClass();

	WindowClass(const WindowClass&) = delete;
	WindowClass& operator=(const WindowClass&) = delete;
};

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
	Window(const WindowClass& wc, DWORD style, DWORD exStyle, const wchar_t* name, HWND parent = nullptr);
	virtual ~Window() = default;

	/* En virtuell funktion som får parametrarna från den "riktiga" WndProc:en. Det är meningen att underklasserna
	   ska överskrida den här funktionen. */
	virtual LRESULT wndProc(UINT msg, WPARAM wParam, LPARAM lParam) { return DefWindowProcW(hWnd, msg, wParam, lParam); }

	// Kör den här för att få fönstret att uppdatera. (Annars fryser det.)
	void update();

	constexpr operator HWND() noexcept { return hWnd; }
	constexpr operator bool() const noexcept { return hWnd; }
};

extern WindowClass defWindowClass;
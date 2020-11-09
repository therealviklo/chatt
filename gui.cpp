#include "gui.h"

MainWindow::MainWindow()
	: Window(
		defWindowClass,
		WS_OVERLAPPEDWINDOW,
		0,
		L"Chatt",
		Menu{MenuItem(L"Öppna", MenuId::open), 
			 MenuItem(L"Stäng", MenuId::close),
			 MenuItem(L"Anslut", MenuId::connect)}
	  ),
	  button(L"Yest", 0, 0, *this)
{
	SetWindowPos(button, 0, 0, 0, 50, 50, SWP_NOACTIVATE | SWP_NOZORDER);
}

LRESULT MainWindow::wndProc(UINT msg, WPARAM wParam, LPARAM lParam) 
{
	switch (msg)
	{
		case WM_COMMAND:
		{
			switch (HIWORD(wParam))
			{
				case BN_CLICKED:
				{
					if ((HWND)lParam == (HWND)button)
					{
						MessageBoxW(*this, L"HEJA", L"HE", 0);
					}
				}
			}
		}
		return 0;
	}
	return DefWindowProcW(*this, msg, wParam, lParam);
}
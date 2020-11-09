#include "gui.h"

void MainWindow::onResize(WORD width, WORD height)
{
	SetWindowPos(
		chatBox,
		nullptr,
		5,
		5,
		width - 5 - 5,
		height - 5 - 20 - 5 - 5,
		SWP_NOACTIVATE | SWP_NOZORDER
	);
	SetWindowPos(
		msgBox,
		nullptr,
		5,
		height - 5 - 20,
		width - 5 - 5 - 50 - 5,
		20,
		SWP_NOACTIVATE | SWP_NOZORDER
	);
	SetWindowPos(
		sendButton,
		nullptr,
		width - 5 - 50,
		height - 5 - 20,
		50,
		20,
		SWP_NOACTIVATE | SWP_NOZORDER
	);
}

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
	  chatBox(ES_MULTILINE | ES_READONLY | WS_VSCROLL, 0, *this),
	  msgBox(0, 0, *this),
	  sendButton(L"Skicka", 0, 0, *this)
{
	RECT windRect;
	if (!GetClientRect(*this, &windRect)) throw Exception("Kunde inte hämta fönsterstorlek");
	onResize(windRect.right - windRect.left, windRect.bottom - windRect.top);
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
					if ((HWND)lParam == (HWND)sendButton)
					{
						MessageBoxW(*this, L"HEJA", L"HE", 0);
					}
				}
			}
		}
		return 0;
		case WM_SIZE:
		{
			onResize(LOWORD(lParam), HIWORD(lParam));
		}
		return 0;
	}
	return DefWindowProcW(*this, msg, wParam, lParam);
}
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
	  ) {}

LRESULT MainWindow::wndProc(UINT msg, WPARAM wParam, LPARAM lParam) 
{
	switch (msg)
	{

	}
	return DefWindowProcW(*this, msg, wParam, lParam);
}
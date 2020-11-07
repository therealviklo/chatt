#include "gui.h"

MainWindow::MainWindow()
	: Window(
		defWindowClass,
		WS_OVERLAPPEDWINDOW,
		0,
		L"Chatt",
		Menu{SubMenu(L"Test2", Menu{MenuItem(L"Hej", 101)})}
	  ) {}

LRESULT MainWindow::wndProc(UINT msg, WPARAM wParam, LPARAM lParam) 
{
	switch (msg)
	{

	}
	return DefWindowProcW(*this, msg, wParam, lParam);
}
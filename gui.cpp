#include "gui.h"

MainWindow::MainWindow()
	: Window(
		defWindowClass,
		WS_OVERLAPPEDWINDOW,
		WS_EX_OVERLAPPEDWINDOW,
		L"Chatt"
	  ) {}

LRESULT MainWindow::wndProc(UINT msg, WPARAM wParam, LPARAM lParam) 
{
	switch (msg)
	{

	}
	return DefWindowProcW(*this, msg, wParam, lParam);
}
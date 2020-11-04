#pragma once
#include "window.h"
#include "cproto.h"

class MainWindow : public Window
{
private:
	MessageProcessor mp;
public:
	MainWindow();

	LRESULT wndProc(UINT msg, WPARAM wParam, LPARAM lParam) override;
};
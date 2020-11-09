#pragma once
#include <optional>
#include "window.h"
#include "cproto.h"

namespace MenuId
{
	enum 
	{
		open = 101,
		close = 102,
		connect = 103
	};
}

// GUI. (Inte riktigt klart än.)

class MainWindow : public Window
{
private:
	std::optional<MessageProcessor> mp;
public:
	MainWindow();

	LRESULT wndProc(UINT msg, WPARAM wParam, LPARAM lParam) override;
};
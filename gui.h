#pragma once
#include <optional>
#include "window.h"
#include "cproto.h"

// GUI. (Inte riktigt klart än.)

namespace MenuId
{
	enum 
	{
		open = 101,
		close = 102,
		connect = 103
	};
}

class MainWindow : public Window
{
private:
	EditControl chatBox;
	EditControl msgBox;
	Button sendButton;

	std::optional<MessageProcessor> mp;

	void onResize(WORD width, WORD height);
public:
	MainWindow();

	LRESULT wndProc(UINT msg, WPARAM wParam, LPARAM lParam) override;
};
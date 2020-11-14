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
	
	class ConnectWindow : public Window
	{
	private:
		MainWindow& mw;

		Label ipLabel;
		EditControl ip;
		Label portLabel;
		EditControl portBuddy;
		UpDown port;
		Button connButton;

		void onResize(WORD width, WORD height);
	public:
		ConnectWindow(HWND parent);
		
		LRESULT wndProc(UINT msg, WPARAM wParam, LPARAM lParam) override;
	};
	std::optional<ConnectWindow> connectWindow;

	void onResize(WORD width, WORD height);
public:
	MainWindow();

	LRESULT wndProc(UINT msg, WPARAM wParam, LPARAM lParam) override;
};
#include "gui.h"

void MainWindow::ConnectWindow::onResize(WORD width, WORD height)
{
	const int ipLabelX = 5;
	const int ipLabelY = 5;
	const int ipLabelW = 150;
	const int ipLabelH = 20;
	const int ipX = 5;
	const int ipY = ipLabelY + ipLabelH + 5;
	const int ipW = 150;
	const int ipH = 20;
	const int portLabelX = ipX + ipW + 5;
	const int portLabelY = 5;
	const int portLabelW = 100;
	const int portLabelH = 20;
	const int portBuddyX = ipX + ipW + 5;
	const int portBuddyY = portLabelY + portLabelH + 5;
	const int portBuddyW = 50;
	const int portBuddyH = 20;
	const int portX = portBuddyX + portBuddyW;
	const int portY = portBuddyY - 1;
	const int portW = 20;
	const int portH = 20 + 2;
	const int connButtonW = 100;
	const int connButtonH = 20;
	const int connButtonX = width - 5 - connButtonW;
	const int connButtonY = height - 5 - connButtonH;

	SetWindowPos(
		ipLabel,
		nullptr,
		ipLabelX,
		ipLabelY,
		ipLabelW,
		ipLabelH,
		SWP_NOACTIVATE | SWP_NOZORDER
	);
	SetWindowPos(
		ip,
		nullptr,
		ipX,
		ipY,
		ipW,
		ipH,
		SWP_NOACTIVATE | SWP_NOZORDER
	);
	SetWindowPos(
		portLabel,
		nullptr,
		portLabelX,
		portLabelY,
		portLabelW,
		portLabelH,
		SWP_NOACTIVATE | SWP_NOZORDER
	);
	SetWindowPos(
		portBuddy,
		nullptr,
		portBuddyX,
		portBuddyY,
		portBuddyW,
		portBuddyH,
		SWP_NOACTIVATE | SWP_NOZORDER
	);
	SetWindowPos(
		port,
		nullptr,
		portX,
		portY,
		portW,
		portH,
		SWP_NOACTIVATE | SWP_NOZORDER
	);
	SetWindowPos(
		connButton,
		nullptr,
		connButtonX,
		connButtonY,
		connButtonW,
		connButtonH,
		SWP_NOACTIVATE | SWP_NOZORDER
	);
}

MainWindow::ConnectWindow::ConnectWindow(HWND parent)
	: Window(
		defWindowClass,
		WS_OVERLAPPEDWINDOW ^ WS_MINIMIZEBOX ^ WS_MAXIMIZEBOX ^ WS_THICKFRAME,
		WS_EX_TOPMOST,
		L"Anslut",
		nullptr,
		250,
		150
	  ),
	  mw(*static_cast<MainWindow*>(reinterpret_cast<Window*>(GetWindowLongPtrW(parent, GWLP_USERDATA)))),
	  ipLabel(L"IP", 0, 0, *this),
	  ip(0, WS_EX_CLIENTEDGE, *this),
	  portLabel(L"Port", 0, 0, *this),
	  portBuddy(ES_NUMBER, WS_EX_CLIENTEDGE, *this),
	  port(UDS_HOTTRACK | UDS_SETBUDDYINT | UDS_NOTHOUSANDS, WS_EX_LEFT, portBuddy, *this),
	  connButton(L"Anslut", 0, 0, *this)
{
	port.setRange(1, 0xffff);
	SendMessageW(portBuddy, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(L"1"));
	SendMessageW(ip, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(L"127.0.0.1"));

	RECT windRect;
	if (!GetClientRect(*this, &windRect)) throw Exception("Kunde inte hämta fönsterstorlek");
	onResize(windRect.right - windRect.left, windRect.bottom - windRect.top);
}

LRESULT MainWindow::ConnectWindow::wndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_COMMAND:
		{
			switch (HIWORD(wParam))
			{
				case BN_CLICKED:
				{
					if ((HWND)lParam == (HWND)connButton)
					{
						if (mw.mp)
						{
							Addr addr{};
							try
							{
								addr = nameToAddr({wstringToString(ip.getText()), (unsigned short)port.getValue()});
							}
							catch (const std::exception& e)
							{
								MessageBoxW(*this, stringToWstring(e.what()).c_str(), L"Ogiltig adress", MB_ICONINFORMATION);
								break;
							}
							try
							{
								mw.mp->connect(addr);
							}
							catch (const std::exception& e)
							{
								MessageBoxW(*this, stringToWstring(e.what()).c_str(), L"Kan inte ansluta", MB_ICONINFORMATION);
							}
						}
						else
						{
							MessageBoxW(*this, L"Du måste öppna först!", L"Kan inte ansluta", MB_ICONINFORMATION);
						}
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
			 MenuItem(L"Anslut", MenuId::connect),
			 MenuItem(L"Adress", MenuId::address)}
	  ),
	  chatBox(ES_MULTILINE | ES_READONLY | WS_VSCROLL, WS_EX_CLIENTEDGE, *this),
	  msgBox(0, WS_EX_CLIENTEDGE, *this),
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
		case WM_MENUSELECT:
		{
			if (!(HIWORD(wParam) & MF_POPUP))
			{
				switch (LOWORD(wParam))
				{
					case MenuId::open:
					{
						selfName.emplace();
						try
						{
							mp.emplace(true, 0, &*selfName);
						}
						catch (...)
						{
							selfName.reset();
							throw;
						}
					}
					break;
					case MenuId::close:
					{
						mp.reset();
						selfName.reset();
					}
					break;
					case MenuId::connect:
					{
						connectWindow.emplace(*this);
					}
					break;
					case MenuId::address:
					{
						if (selfName)
						{
							MessageBoxW(
								*this,
								(L"IP: " + stringToWstring(selfName->ip) +
								L"\r\nPort: " + toWString(selfName->port)).c_str(),
								L"Adress",
								MB_ICONINFORMATION
							);
						}
						else
						{
							MessageBoxW(*this, L"Du måste öppna först", L"Har ingen adress", MB_ICONINFORMATION);
						}
					}
					break;
				}
			}
			else return DefWindowProcW(*this, msg, wParam, lParam);
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
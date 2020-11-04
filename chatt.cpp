#include <stdexcept>
#include <cstdio>
#include "win.h"
#include "utils.h"
#include "wsa.h"
#include "stun.h"
#include "cproto.h"
#include "gui.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	try
	{
		wsaHandler.initialise();

		MainWindow mw;
		while (mw) mw.update();
	}
	catch (const WSAException& e)
	{
		std::stringstream ss;
		ss << e.what();
		ss << "\r\nError code: ";
		ss << e.errCode;
		MessageBoxA(
			nullptr,
			ss.str().c_str(),
			"Error",
			MB_ICONERROR | MB_TASKMODAL
		);
	}
	catch (const std::exception& e)
	{
		MessageBoxA(nullptr, e.what(), "Error", MB_ICONERROR | MB_TASKMODAL);
	}
	catch (...)
	{
		MessageBoxW(nullptr, L"Unknown error", L"Error", MB_ICONERROR | MB_TASKMODAL);
	}
	return EXIT_SUCCESS;
}
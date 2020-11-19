#include <stdexcept>
#include <cstdio>
#include "win.h"
#include "utils.h"
#include "wsa.h"
#include "stun.h"
#include "cproto.h"
#include "gui.h"

// Huvudfilen

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	try // Fånga alla körtidsfel så att man kan visa en ruta med felet
	{
		wsaHandler.initialise();
		INITCOMMONCONTROLSEX icc{};
		icc.dwSize = sizeof(icc);
		icc.dwICC = ICC_INTERNET_CLASSES | ICC_UPDOWN_CLASS | ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES;
		InitCommonControlsEx(&icc);

		MainWindow mw;
		// EditControl ec(0, 0, mw);

		// SetWindowPos(ec, 0, 0, 0, 50, 50, SWP_NOACTIVATE | SWP_NOZORDER);

		while (mw) updateAllWindows();
	}
	catch (const WSAException& e) // WSAException har en felkod som man kan skriva ut
	{
		std::wstringstream ss;
		ss << stringToWstring(e.what());
		ss << L"\r\nFelkod: ";
		ss << e.errCode;
		MessageBoxW(
			nullptr,
			ss.str().c_str(),
			L"Kritiskt fel",
			MB_ICONERROR | MB_TASKMODAL
		);
		return EXIT_FAILURE;
	}
	catch (const std::exception& e) // Andra fel som ärver från std::exception
	{
		MessageBoxW(
			nullptr,
			stringToWstring(e.what()).c_str(),
			L"Kritiskt fel",
			MB_ICONERROR | MB_TASKMODAL
		);
		return EXIT_FAILURE;
	}
	catch (...) // För fel som inte ärver från std::exception (som de borde göra)
	{
		MessageBoxW(nullptr, L"Okänt fel", L"Kritiskt fel", MB_ICONERROR | MB_TASKMODAL);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

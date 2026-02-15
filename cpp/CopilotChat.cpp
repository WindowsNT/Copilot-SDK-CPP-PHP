#include <iostream>
#include ".\\copilot.hpp"
#pragma comment(lib,"wininet.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


std::shared_ptr<COPILOT> cop;
BOOL WINAPI HandlerRoutine(
    _In_ DWORD dwCtrlType
)
{
    if (dwCtrlType == CTRL_CLOSE_EVENT)
    {
        cop = nullptr;
        return TRUE;
    }
    return false;
}

int wmain()
{
	CoInitializeEx(NULL, COINIT_MULTITHREADED);

	// Console UTF and ANSI
    SetConsoleCtrlHandler(HandlerRoutine, TRUE);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
    SetConsoleOutputCP(CP_UTF8);

    wprintf(L"\033[36mCopilotChat using CopilotSDK - https://github.com/WindowsNT/Copilot-SDK-CPP-PHP\r\n\033[0m\r\n");
    wprintf(L"\033[33mReading SDK status ... \033[0m");
	auto status = COPILOT::Status();
}
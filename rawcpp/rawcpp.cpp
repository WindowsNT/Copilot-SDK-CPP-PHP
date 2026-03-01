// rawcpp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vector>
#include <mutex>
#include <queue>
#include <string>
#include <sys/types.h>
#ifdef LINUX
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif
#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wininet.lib")
#endif
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <condition_variable>
#include <stdlib.h>
#include "json.hpp"
#include "raw.hpp"

int brk = 0;


BOOL WINAPI MyConsoleHandler(DWORD ctrlType)
{
	switch (ctrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_CLOSE_EVENT:
		brk = 1;
		return TRUE;
	}
	return FALSE;
}

int wmain()
{
#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	SetConsoleCtrlHandler(MyConsoleHandler, TRUE);
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwMode = 0;
	GetConsoleMode(hOut, &dwMode);
	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(hOut, dwMode);
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);

#endif
	COPILOT_RAW raw("127.0.0.1", 3000, true);
//	COPILOT_RAW raw(L"f:\\copilot2\\copilot.exe", 3000, "your_token",1);
	raw.Ping();
	std::vector<std::shared_ptr<COPILOT_SESSION>> sessions;
	raw.Sessions(sessions);
	
	auto as = raw.AuthStatus();
	auto dup = as.dump();
	auto st = raw.Status();
	auto s1 = raw.CreateSession("gpt-4.1", true);
//	auto s1 = raw.CreateSession("phi:latest", true);
//	auto s1 = raw.CreateSession("gpt-5-mini", false);
//	auto m1 = raw.CreateMessage(s1, "Hello", 0);
	std::vector<std::wstring> files = { L"f:\\tp2imports\\365.jpg" };
	auto m1 = raw.CreateMessage(s1, "What do you see in this image?", 0, 0, 0, &files);
	auto m2 = raw.CreateMessage(s1, "Please tell me all numbers from 1 to 100", [&](std::string tok, long long ptr) -> HRESULT {
		std::cout << tok;
		if (brk)
		{
			brk = 0;
			return E_ABORT;
		}
		return S_OK;
		}, [&](std::string tok, long long ptr) -> HRESULT {
			std::cout << tok;
			return S_OK;
			}, 0);
	raw.Send(s1, m1);
	raw.Send(s1, m2);
	raw.Wait(s1, m2, 60000);
	raw.Wait(s1, m1, 60000);
	__nop();
}


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

int main()
{
#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	SetConsoleCtrlHandler(MyConsoleHandler, TRUE);
#endif
	COPILOT_RAW raw("127.0.0.1", 3000, true);
//	COPILOT_RAW raw(L"f:\\copilot2\\copilot.exe", 3000, "your_token");
	raw.Ping();
	std::vector<std::shared_ptr<SESSION>> sessions;
	raw.Sessions(sessions);
	
	auto as = raw.AuthStatus();
	auto dup = as.dump();
	auto ml = raw.ModelList();
	dup = ml.dump();
//	auto s1 = raw.CreateSession("gpt-4.1", true);
	auto s1 = raw.CreateSession("gpt-5-mini", true);
	raw.Send(s1, "Please tell me all numbers from 1 to 100", 60000, [&](std::string tok,long long ptr) -> HRESULT	{
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
		},0);
	__nop();
}


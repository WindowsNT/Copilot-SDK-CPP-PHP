// rawcpp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vector>
#include <mutex>
#include <queue>
#include <string>
#include <iomanip>
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
#include <CommCtrl.h>
#include <algorithm>
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

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

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
	COPILOT_RAW raw("127.0.0.1", 3000);
//	COPILOT_RAW raw(L"f:\\copilot2\\copilot.exe", 3000, "your_token",1);
	std::vector<std::shared_ptr<COPILOT_SESSION>> sessions;
	raw.Sessions(sessions);


	int iVersion = 0;
	raw.Ping(&iVersion);
	auto st = raw.Status();

	//	raw.SetMode(s1, COPILOT_RAW_MODE::INTERACTIVE);

	// Test file input and session compact
	if (0)
	{
		auto s1 = raw.CreateSession("gpt-4.1", nullptr);

		// Test raw API
		if (0)
		{
			// some raw stuff
			nlohmann::json j;
			j["jsonrpc"] = "2.0";
			j["id"] = raw.next();
			j["method"] = "session.agent.list";
			j["params"]["sessionId"] = s1->sessionId;
			auto r = raw.ret(j, true);
		}

		//	auto s1 = raw.CreateSession("phi:latest", true);
		std::vector<std::wstring> files = { L"f:\\tp2imports\\365.jpg" };
		auto m1 = raw.CreateMessage("What do you see in this image?", 0, 0, 0, &files);
		auto m2 = raw.CreateMessage("Tell me a short joke", [&](std::string tok, long long ptr) -> HRESULT {
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
			raw.Compact(s1);
			if (m1->completed_message && m2->completed_message)
				MessageBoxA(0, m1->completed_message->content.c_str(), m2->completed_message->content.c_str(), 0);
			raw.SwitchModel(s1, "gpt-5-mini");
			auto single = raw.One(s1, "Another one", 60000);
			MessageBoxA(0, single.c_str(), "Single", 0);
	}

	// User input test
	if (1)
	{
		COPILOT_SESSION_PARAMETERS spp;
		spp.user_ask_function = [&](nlohmann::json& j, std::string& resp, bool& free_form,long long cb) -> void {
			// auto question = j["params"]["question"].get<std::string>();
			// auto choices = j["params"]["choices"].get<std::vector<std::string>>();
			// auto free_form = j["params"]["free_form"].get<bool>();
			resp = "My name is Michael";
			};
		auto s1 = raw.CreateSession("gpt-4.1", &spp);
		auto r = raw.One(s1, "Ask the user his name", 60000);
		MessageBoxA(0, r.c_str(), "Information", 0);
	}

	// Tool test
	if (0)
	{
		std::vector<COPILOT_TOOL_PARAMETER> params = { {"city","City","City name","string",true}}; // name title description type required
		raw.AddTool("GetWeather", "Get the current weather for a city", "GetWeatherParams", params, [&](
			std::string session_id,
			std::string tool_id,
			std::vector<std::tuple<std::string, std::any>>& parameters)
			{
				nlohmann::json j;
				for (auto& p : parameters)
				{
					std::string name;
					std::any value;
					std::tie(name, value) = p;
					if (name == "city")
					{
						j["city"] = std::any_cast<std::string>(value);
					}
				}
				j["condition"] = "Sunny";
				j["temperature"] = "25C";
				// Or you can return a direct string, say "It is sunny".
				return j.dump();
			});
		auto s1 = raw.CreateSession("gpt-4.1", nullptr);
		auto m2 = raw.CreateMessage("What is the weather in Seattle?", [&](std::string tok, long long ptr) -> HRESULT {
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
			raw.Send(s1, m2);
			raw.Wait(s1, m2, 600000);
			std::string str = m2->completed_message->reasoningText.c_str();
			str += "\r\n\r\n";
			str += m2->completed_message->content.c_str();
			MessageBoxA(0, str.c_str(), "Information", 0);
	}
	__nop();
}


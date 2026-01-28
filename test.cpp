#include <iostream>
#include ".\\copilot.hpp"
#pragma comment(lib,"wininet.lib")

void TestOpenAI()
{
    COPILOT cop(L"", "Gemini", "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent", 0,
        L"your_api_key");
    cop.flg = CREATE_NEW_CONSOLE;
    cop.BeginInteractive();

    std::cout << "Enter your prompts, type exit to quit." << std::endl;
    for (;;)
    {
        std::wstring prompt;
        // get entire line
        printf("\033[92m");
        std::getline(std::wcin, prompt);
        printf("\033[0m");
        if (prompt == L"exit" || prompt == L"quit")
            break;
        auto ans = cop.PushPrompt(prompt, true, [](std::string tok, LPARAM lp)->HRESULT
            {
                COPILOT* cop = (COPILOT*)lp;
                std::wcout << cop->tou(tok.c_str());
                return S_OK;
            }, (LPARAM)&cop);
        std::wcout << std::endl;
    }
    cop.EndInteractive();

}


void TestLLama()
{
    COPILOT cop(L"f:\\llama\\run", "f:\\llama\\models\\mistral-7b-v0.1.Q2_K.gguf", "", 9991);
    cop.flg = CREATE_NEW_CONSOLE;
    cop.BeginInteractive();

	std::cout << "Enter your prompts, type exit to quit." << std::endl;
    for (;;)
    {
        std::wstring prompt;
        // get entire line
        printf("\033[92m");
        std::getline(std::wcin, prompt);
        printf("\033[0m");
        if (prompt == L"exit" || prompt == L"quit")
            break;
        auto ans = cop.PushPrompt(prompt, true, [](std::string tok, LPARAM lp)->HRESULT
            {
				COPILOT* cop = (COPILOT*)lp;
				std::wcout << cop->tou(tok.c_str());
                return S_OK;
            },(LPARAM)&cop);
		std::wcout << std::endl;
    }
    cop.EndInteractive();

}

void TestCopilot()
{
    COPILOT cop(L"f:\\copilot");
    cop.flg = CREATE_NEW_CONSOLE;

    std::vector<wchar_t> dll_path(1000);
    GetFullPathName(L".\\x64\\Debug\\dlltool.dll", 1000, dll_path.data(), 0);
    auto sl1 = cop.ChangeSlash(dll_path.data());
    auto dll_idx = cop.AddDll(sl1.c_str(), "pcall", "pdelete");
    cop.AddTool(dll_idx, "GetWeather", "Get the current weather for a city in a specific date", {
        {"city", "str", "Name of the city to get the weather for"},
        {"date", "int", "Date to get the weather for"}
        });

    cop.BeginInteractive();
    auto ans = cop.PushPrompt(L"Tell me the weather in Athens in 25 January 2026", true, [](std::string tok, LPARAM lp)->HRESULT
        {
            COPILOT* cop = (COPILOT*)lp;
            std::wcout << cop->tou(tok.c_str());
            return S_OK;
        }, (LPARAM)&cop);
    std::wstring s;
    for (auto& str : ans->strings)
        s += str;
    MessageBox(0, s.c_str(), 0, 0);
    cop.EndInteractive();

}

int main()
{

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);

//    TestOpenAI();
    TestLLama();
    TestCopilot();
}

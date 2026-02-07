#include <iostream>
#include ".\\copilot.hpp"
#pragma comment(lib,"wininet.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


// Change this 
#define YOUR_COPILOT_FOLDER COPILOT::GetDefaultCopilotfolder()

std::vector<wchar_t> dll_path(1000);
std::vector<wchar_t> image_path(1000);

/*
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
*/

void AskQuestion(COPILOT& cop,bool Tool = false,bool Image = false)
{
    if (Tool)
    {
        auto ans = cop.PushPrompt(L"Tell me the weather in Athens in 25 January 2026", true, [](std::string tok, LPARAM lp)->HRESULT
            {
                COPILOT* cop = (COPILOT*)lp;
                std::wcout << cop->tou(tok.c_str());
                return S_OK;
            }, (LPARAM)&cop);
        std::wstring s = ans->Collect();
        MessageBox(0, s.c_str(), 0, 0);
    }
	if (Image)
    {
        auto sl1 = cop.ChangeSlash(image_path.data());
        cop.PushAttachmentForNextPrompt(sl1.c_str());
        auto ans = cop.PushPrompt(L"Can you tell me what this image contains?", true, [](std::string tok, LPARAM lp)->HRESULT
            {
                COPILOT* cop = (COPILOT*)lp;
                std::wcout << cop->tou(tok.c_str());
                return S_OK;
            }, (LPARAM)&cop);
        std::wstring s = ans->Collect();
        MessageBox(0, s.c_str(), 0, 0);
    }
    if (1)
    {
        static int cnt = 0;
        auto ans = cop.PushPrompt(L"Tell me about WW1", true, [](std::string tok, LPARAM lp)->HRESULT
            {
                COPILOT* cop = (COPILOT*)lp;
                std::wcout << cop->tou(tok.c_str());
                cnt++;
                if (cnt > 30)
                    cop->CancelCurrent();
                return S_OK;
            }, (LPARAM)&cop);
        std::wstring s = ans->Collect();
        MessageBox(0, s.c_str(), 0, 0);
    }
}

void TestLLama()
{
	COPILOT_PARAMETERS cp;
	cp.folder = L"f:\\llama\\run";
	cp.model = "f:\\llama\\models\\mistral-7b-v0.1.Q2_K.gguf";
	cp.LLama_Port = 9991;   
    COPILOT cop(cp);
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
	COPILOT_PARAMETERS cp;
	cp.folder = YOUR_COPILOT_FOLDER;
	cp.system_message = "You are a helpful assistant that can answer questions and execute tools.";
    COPILOT cop(cp);

    auto sl1 = cop.ChangeSlash(dll_path.data());
    auto dll_idx = cop.AddDll(sl1.c_str(), "pcall", "pdelete");
    cop.AddTool(dll_idx, "GetWeather", "Get the current weather for a city in a specific date", {
        {"city", "str", "Name of the city to get the weather for"},
        {"date", "int", "Date to get the weather for"}
        });

    [[maybe_unused]] auto sdk_models = cop.copilot_model_list();
    cop.BeginInteractive();
    auto reply = cop.Ping();
    reply = cop.State();
    reply = cop.AuthState();
    AskQuestion(cop,true,true);
    cop.EndInteractive();
}


void TestOllama()
{
	auto model_list = COPILOT::ollama_list();
    COPILOT_PARAMETERS cp;
//    cp.custon_provider_type = "openai";    
//    cp.custom_provider_base_url = "http://localhost:11434/v1";
    cp.folder = YOUR_COPILOT_FOLDER;
    cp.model = "qwen3-coder:30b";
//    cp.model = "deepseek-r1:8b";
    COPILOT cop(cp);
    cop.BeginInteractive();
    AskQuestion(cop);
    cop.EndInteractive();
}

int main()
{

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);

    GetFullPathName(L".\\x64\\Debug\\dlltool.dll", 1000, dll_path.data(), 0);
    GetFullPathName(L".\\365.jpg", 1000, image_path.data(), 0);

    //    TestOpenAI();
//    TestLLama();
      TestCopilot();
//   TestOllama();
}

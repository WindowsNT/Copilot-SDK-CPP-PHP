#include ".\\copilot.hpp"

int main()
{
    COPILOT cop(L".\\copilot");
    cop.BeginInteractive();
    auto ans = cop.PushPrompt(L"Tell me a joke",true);
    std::wstring s;
	for (auto& str : ans->strings)
        s += str;
    MessageBox(0, s.c_str(), 0, 0);
    cop.EndInteractive();
}

# Copilot SDK for C++

Github released the [Copilot SDK](https://github.com/github/copilot-sdk) and here 's a C++ wrapper around it to be used in Windows.

# Installation
* Create a folder with python installed 
* pip install github-copilot-sdk
* Put [copilot.exe](https://github.com/github/copilot-cli/releases/) in that folder

# Usage
```cpp
#include "copilot.h"
#include <iostream>
COPILOT cop(L"c:\\copilot_folder");
cop.BeginInteractive();
auto ans = cop.PushPrompt(L"Tell me a joke",true);
std::wstring s;
for (auto& str : ans->strings)
    s += str;
MessageBox(0, s.c_str(), 0, 0);
cop.EndInteractive();
```

* PushPrompt's last parameter is whether to wait for the response. If false, then the "ans" structure includes a HANDLE event to be triggered when the response is ready.
* COPILOT constructor is
```cpp
COPILOT(std::wstring folder, std::string model = "gpt-4.1",std::string if_server = "")
```
* folder is where copilot.exe and python are located
* model is the model to use (you can get a list of models with static COPILOT::copilot_model_list() function
* if_server can be used to specify a remote copilot.exe running. You must still have the python folder though.

# License
MIT

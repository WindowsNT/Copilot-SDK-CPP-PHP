# Copilot SDK for C++

Github released the [Copilot SDK](https://github.com/github/copilot-sdk?tab=readme-ov-file) and here 's a C++ wrapper around it to be used in Windows.

# Installation
Create a folder with python installed 
pip install github-copilot-sdk
Put [copilot.exe](https://github.com/github/copilot-cli/releases/) in that folder

# Usage
Include the header file in your project and link against the `copilot.lib` library.
```cpp
#include "copilot.h"
#include <iostream>
COPILOT cop(L".\\copilot");
cop.BeginInteractive();
auto ans = cop.PushPrompt(L"Tell me a joke",true);
std::wstring s;
for (auto& str : ans->strings)
    s += str;
MessageBox(0, s.c_str(), 0, 0);
cop.EndInteractive();
```

PushPrompt's last parameter is whether to wait for the response. If false, then the "ans" structure includes a HANDLE event to be triggered when the response is ready.

# License
MIT

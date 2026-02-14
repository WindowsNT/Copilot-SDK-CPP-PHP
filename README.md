# Copilot/LLama/Ollama SDK for C++ and PHP

Github released the [Copilot SDK](https://github.com/github/copilot-sdk) and here 's a C++ wrapper around it to be used in Windows. This also allows to use a local LLama-based model through a local llama-server.
I'm already using it in [Turbo Play](https://www.turbo-play.com), [TurboIRC](https://www.turbo-play.com/apps.php) and other projects.

# Copilot Installation
* Create a folder with python installed 
* pip install github-copilot-sdk asyncio pywin32
* Put [copilot.exe](https://github.com/github/copilot-cli/releases/) in that folder

# LLama Installation
Create a folder where [llama-server](https://github.com/ggml-org/llama.cpp/releases) is located.

# Ollama 
Download and run [Ollama](https://ollama.com/), specify to make it visible to the network.

# Installation and Authentication check
```cpp
struct COPILOT_STATUS
{
	bool StatusValid = false;
	bool Installed = false;
	std::wstring folder;
	bool OllamaInstalled = false;
	bool Connected = false;
	bool Authenticated = false;
	std::vector<COPILOT_MODEL> models;
};
static COPILOT_STATUS Status(const wchar_t* folder,bool Refresh = false);
```

Call this method in a thread or a std::shared_future so you don't block the UI. If Installed is true and Authenticated is false, then run copilot.exe in a new console. If this method is called again and Refresh is false, it will return the cached status. 
If Refresh is true, it will refresh the status by checking the installation and authentication again.

# Usage
```cpp
struct COPILOT_PARAMETERS
{
	std::wstring folder;
	std::string model = "gpt-4.1";
	std::string remote_server;
	int LLama_Port = 0;
	std::wstring api_key;
	std::string reasoning_effort = "";
	std::string system_message = "";
	std::string custon_provider_type; 
	std::string custom_provider_base_url; //  
#ifdef _DEBUG
	bool Debug = 1;
#else
	bool Debug = 0;
#endif
};
```
Where
 - folder: folder where copilot.exe and python are located, or llama-server for local LLama models
 - model: model name, for Copilot use "gpt-4.1" or other supported models (copilot_model_list()) , for local LLama use the model name loaded in llama-server, for Ollama use the model name available in Ollama
 - remote_server: For LLama use "localhost"
 - LLama_Port: For LLama use the port where llama-server is running
 - api_key: API key for Copilot or other providers
 - reasoning_effort: optional parameter for Copilot, can be "low", "medium", "high", "xhigh"
 - custon_provider_type: For Ollama, can be "openai" "azure" etc
 - custom_provider_base_url: For Ollama, use "http://localhost:11434/v1"
 - Debug: enable debug output

```cpp
#include "copilot.h"
#include <iostream>
COPILOT_PARAMETERS cp;
cp.folder = L"f:\\copilot";
cp.system_message = "You are a helpful assistant that can answer questions and execute tools.";
COPILOT cop(cp);
cop.BeginInteractive();
auto ans = cop.PushPrompt(int Status,L"Tell me a joke",true, [](std::string tok, LPARAM lp)->HRESULT
        {
		    if (Status == 3)
			    return S_OK; 
            COPILOT* cop = (COPILOT*)lp;
            std::wcout << cop->tou(tok.c_str());
            return S_OK;
        }, (LPARAM)&cop);
std::wstring s = ans->Collect();
MessageBox(0, s.c_str(), 0, 0);
cop.EndInteractive();
```

* PushPormpt's Status is 1 for the answer tokens and 3 for reasoning tokens (if any).
* PushPrompt's true/false parameter is whether to wait for the response. If false, then the "ans" structure includes a HANDLE event to be triggered when the response is ready.
* You can provide a callback function to receive tokens as they arrive.


# Tool definition
```cpp
// Example of adding a tool from a dll

// DLL code
#include "json.hpp"
using json = nlohmann::json;
extern "C" {

	__declspec(dllexport)
		const char* pcall(const char* json_in)
	{
		json req = json::parse(json_in);
		json resp;
		resp["status"] = "ok";
		resp["temperature"] = "14C";

		std::string out = resp.dump();
		char* mem = (char*)std::malloc(out.size() + 1);
		memcpy(mem, out.c_str(), out.size() + 1);
		return mem;
	}
	
	__declspec(dllexport)
		const char* ask_user(const char* question)
	{
		json req = json::parse(question);
		// "question" -> the question
		// "choices" -> array of choices (if any)
		// "allowFreeForm" -> if true, the user can type a free form answer instead of choosing from choices

		json resp;
		resp["answer"] = "I don't have the answer to that right now";
		resp["wasFreeform"] = true;

		std::string out = resp.dump();
		char* mem = (char*)std::malloc(out.size() + 1);
		memcpy(mem, out.c_str(), out.size() + 1);
		return mem;
	}


	// free memory returned by pcall
	__declspec(dllexport)
		void pdelete(const char* p)
	{
		std::free((void*)p);
	}

}


// Main Code
std::vector<wchar_t> dll_path(1000);
GetFullPathName(L".\\x64\\Debug\\dlltool.dll", 1000, dll_path.data(), 0);
auto sl1 = cop.ChangeSlash(dll_path.data());
auto dll_idx = cop.AddDll(sl1.c_str(),"pcall","pdelete","ask_user");
cop.AddTool(dll_idx, "GetWeather", "Get the current weather for a city in a specific date",{
        {"city", "str", "Name of the city to get the weather for"},
        {"date", "int", "Date to get the weather for"}
    });
...
cop.BeginInteractive();
auto ans = cop.PushPrompt(L"Tell me the weather in Athens in 25 January 2026",true);
```

This adds a tool to Copilot that calls the pcall function in the dlltool.dll. The pcall function receives a json string with the tool parameters and must return a json string with the tool results.
Currently, it returns hardcoded "temperature": "14C", but you can modify it to call a weather API.

# Image Attachments
```cpp
 cop.PushAttachmentForNextPrompt("c://image.jpg");
```

This pushes an image attachment for the next prompt. You can push multiple attachments before a prompt.
The model must support attachments for this to work.

# Connect to LLama
```cpp
COPILOT_PARAMETERS cp;
cp.folder = L"f:\\llama\\run";
cp.model = "f:\\llama\\models\\mistral-7b-v0.1.Q2_K.gguf";
cp.LLama_Port = 9991;   
COPILOT cop(cp);
```

# Connect to Ollama
```cpp
COPILOT_PARAMETERS cp;
// You do not need the following if ollama is running to the default 11434 port
// cp.custon_provider_type = "openai";    
// cp.custom_provider_base_url = "http://localhost:11434/v1";
cp.folder = L"f:\\copilot";
cp.model = "qwen3-coder:30b";
COPILOT cop(cp);
``` 

# PHP
For running with PHP in a typical Linux PHP stack you do
* Install python and the copilot sdk with pip as before
* Put copilot CLI in a folder and make sure it's executable
* Run python server: 
```bash
python copilotworker.py
```
* Change the parameters in copilotworker.py to match your installation
```python
PORT = 8765
CLI_PATH = "/root/copilot"
```
* require "copilot.php" in your PHP code

```PHP
<?php
require_once "copilot.php";
$copilot = new Copilot("gpt-4.1",8765); 
echo $copilot->send("/models");
echo '<br><br>';
echo $copilot->send("/state");
echo '<br><br>';
echo $copilot->send("/authstate");
echo '<br><br>';
echo $copilot->ask("What is the capital of France?");
$copilot->kill();
?>
```


# Other stuff
* copilot_model_list() returns limited information, for full run-time description of the models from the SDK you can call std::vector<COPILOT_SDK_MODEL> ListModelsFromSDK().
* You can call Ping() to return "pong" to check if the connection with the SDK is working.
* You can call State() to get a state string (e.g. "connected").
* You can call AuthState() to get a "true" or "false".
# License
MIT

# Copilot/Ollama SDK for C++ and PHP

Github released the [Copilot SDK](https://github.com/github/copilot-sdk) and here 's a C++ wrapper around it to be used in Windows. This also allows to use a local LLama-based model through a local llama-server.
I'm already using it in [Turbo Play](https://www.turbo-play.com), [TurboIRC](https://apps.microsoft.com/detail/9PCQMH46GRQX?hl=en&gl=GR&ocid=pdpshare), [FaustChat](https://www.turbo-play.com/copilot.php?from=FaustChat) and other projects.

This is the new RAW only readme. If you want to see the old readme, open README_old.md.

# Ollama 
If you want to also use local models, download and run [Ollama](https://ollama.com/), specify to make it visible to the network. The models will be visible to your C++ code.


# Example 
```cpp
#include "raw.hpp"

// Use either a running copilot server 
//	COPILOT_RAW raw("127.0.0.1", 3000, true);

// Or a new one with your github access token
//	COPILOT_RAW raw(L"c:\\copilot\\copilot.exe", 3000, "your_token",1);

raw.Ping();
std::vector<std::shared_ptr<COPILOT_SESSION>> sessions;
raw.Sessions(sessions);
	
auto as = raw.AuthStatus();
auto st = raw.Status();
/*
struct COPILOT_SESSION_PARAMETERS
{
	bool Streaming = true;
	std::string reasoning_effort;
	std::string system_message;
};
*/
auto s1 = raw.CreateSession("gpt-4.1", nullptr); // use the default COPILOT_SESSION_PARAMETERS
//  Ollama also supported
//	auto s1 = raw.CreateSession("phi:latest");
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
raw.DestroySession(s1); // leaves files there
ra
```

# Tools
Tools are simply function callbacks. You tell the model "Hey, I know the answer to a specific question, so if you need, call my code".


```cpp
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
auto s1 = raw.CreateSession("gpt-4.1");
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

```

Raw mode doesn't yet support skills

Raw mode functions that do not exist in the Python SDK:
* `Compact()` to compact a session
* `Status()` contains also the account's premium quota percentages and usage
* `SetMode(...)` to set the current session mode, `COPILOT_RAW_MODE:: INTERACTIVE, PLAN, AUTOPILOT`.
* `DeleteSession` and `DestroySession` to delete a session with or without deleting the files used in it. 
* `ResumeSession` to resume an existing session (get all resumable sessions with `Sessions()`).

# CopilotChat
CopilotChat binary is a test command line app that you can use to test the SDK.
Command line parameters:
* -f <folder> : folder where copilot.exe (and python if not raw) is located. The default is `c:\ProgramData\933bd016-0397-42c9-b3e0-eaa7900ef53e`, 
* -m <model> : model name,  default is "gpt-5 mini"
* --token <token> : A github token to use. If not used, the default copilot authentication is used. If --raw is used, this is mandatory.
* --raw : Use the raw mode

Once CopilotChat is running, you can use the commands:
* /clipboard                : Pass the clipboard contents to the chat
* /compact                  : [raw mode] Compact the session
* /install or /update       : [python mode] Installs or updates Copilot. This downloads binaries available in www.turbo-play.com and runs pip to install prerequisites. 
* /file <file>              : Add an attachment for next prompt
* /auth                     : Runs copilot.exe for authentication if needed
* /models                   : Lists the available models
* /model <model_name>       : Changes the model to use
* /restart                  : Restart copilot
* /skill <folder>           : [python mode] Add a skills directory and restart Copilot
* /disabledskill "skill"    : [python mode] Add a disabled skill and restart Copilot
* /quota                    : Show your account quota (raw mode only)
* /save <N> <file>          : Save response N to file
* /status                   : Show Copilot Status
* /thinking                 : Turns thinking mode on/off for models that support thinking tokens.
* /quit or /exit            : Exits the application
 

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


# License
MIT

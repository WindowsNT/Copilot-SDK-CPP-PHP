# Copilot/Ollama SDK for C++ and PHP

Github released the [Copilot SDK](https://github.com/github/copilot-sdk) and here 's a C++ wrapper around it to be used in Windows. This also allows to use a local LLama-based model through a local llama-server.
I'm already using it in [Turbo Play](https://www.turbo-play.com), [TurboIRC](https://apps.microsoft.com/detail/9PCQMH46GRQX?hl=en&gl=GR&ocid=pdpshare), [FaustChat](https://www.turbo-play.com/copilot.php?from=FaustChat) and other projects.

This is the new RAW only readme.  RAW mode does not require python installed. If you want to see the old readme, open README_old.md.
Raw.hpp also compiles in linux with g++.

# Ollama 
If you want to also use local models, download and run [Ollama](https://ollama.com/), specify to make it visible to the network. The models will be visible to your C++ code.

# Initialize
```cpp
#include "raw.hpp"

// Use either a running copilot server 
//	COPILOT_RAW raw("127.0.0.1", 3000, true);

// Or a new one with your github access token
//	COPILOT_RAW raw(L"c:\\copilot\\copilot.exe", 3000, "your_token",1);
```

# Quick example
```cpp
auto s1 = raw.CreateSession("gpt-4.1");
auto r = raw.One(s1, "Tell me a short joke", 60000); // wait for maximum 1 minute
```

# Example 
```cpp
raw.Ping();
auto models = raw.CopilotModels();
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
	std::vector<std::wstring> skill_dirs;
	std::vector<std::wstring> disabled_skills;
	bool Infinite = true;
	std::function<void(nlohmann::json& j, std::string& resp, bool& free_form,long long cb)> user_ask_function;
	long long ask_function_callback = 0;
};
*/
auto s1 = raw.CreateSession("gpt-4.1", nullptr); // use the default COPILOT_SESSION_PARAMETERS if nullptr is passed
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
raw.DestroySession(s1); // leaves files there so session can be resumed later
raw.DeleteSession(s1); // deletes session
```

# User input
You can pass a function callback to the COPILOT_SESSION_PARAMETERS and this is called whenever the model wants to ask you a question.

```cpp
COPILOT_SESSION_PARAMETERS spp;
spp.user_ask_function = [&](nlohmann::json& j, std::string& resp, bool& free_form,long long cb) -> void {
	// auto question = j["params"]["question"].get<std::string>();
	// auto is_question_free_form = j["params"]["free_form"].get<bool>();
	// auto choices = j["params"]["choices"].get<std::vector<std::string>>(); // maybe not exists if is_question_free_form is true
	resp = "My name is Michael";
	free_form = true;
	};
auto s1 = raw.CreateSession("gpt-4.1", &spp);
auto r = raw.One(s1, "Ask the user his name", 60000);
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

# Other functions

* `Compact()` to compact a session
* `Status()` contains also the account's premium quota percentages and usage
* `SetMode(...)` to set the current session mode, `COPILOT_RAW_MODE:: INTERACTIVE, PLAN, AUTOPILOT`.
* `DeleteSession` and `DestroySession` to delete a session with or without deleting the files used in it. 
* `ResumeSession` to resume an existing session (get all resumable sessions with `Sessions()`).
* `SwitchModel` to switch model in an existing session.

# CopilotChat
CopilotChat binary is a test command line app that you can use to test the SDK.
Command line parameters:
* -f or --folder <folder> : folder where copilot.exe is located. 
* -m or --model  <model>  : model name,  default is "gpt-5 mini"
* -t or --token  <token>  : A github token to use. 

Once CopilotChat is running, you can use the commands:
* /clipboard                : Pass the clipboard contents to the chat
* /compact                  : Compact the session
* /file <file>              : Add an attachment for next prompt
* /models                   : Lists the available models
* /model <model_name>       : Changes the model to use
* /restart                  : Restart copilot
* /skill <folder>           : Add a skills directory and restart Copilot
* /disabledskill "skill"    : Add a disabled skill and restart Copilot
* /quota                    : Show your account quota 
* /save <N> <file>          : Save response N to file
* /status                   : Show Copilot Status
* /text <file>              : Pass text file contents as next prompt
* /thinking                 : Turns thinking mode on/off for models that support thinking tokens.
* /quit or /exit            : Exits the application
 

# PHP
Just require "cop.php" in your PHP code. You do not need python.

```PHP
<?php
require_once "cop.php";

$cop = new Copilot("your_token","/usr/local/bin/copilot",8765);
// or
$cop = new Copilot("","",8765); // run with an existing server
$m1 = $cop->Ping();
$m1 = $cop->AuthStatus();
$m1 = $cop->Quota();
$m1 = $cop->Sessions();

$s1 = new COPILOT_SESSION_PARAMETERS();
$s1->Streaming = true;
$s1->system_message = "You are a helpful assistant for testing the copilot cli.";
$session_id = $cop->CreateSession("gpt-4.1",$s1,true);
printf("Session ID: %s\n",$session_id); 
// Send message
$m1 = $cop->Prompt($session_id,"What is the capital of France?",true);
// End session
$x1 = $cop->EndSession($session_id,true);

?>
```


# License
MIT

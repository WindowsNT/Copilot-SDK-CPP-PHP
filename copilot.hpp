
// Copilot Class

#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ws2tcpip.h>
#include <wincrypt.h>   
#include <ShlObj.h>
#include <string>
#include <queue>
#include <mutex>
#include <functional>
#include <vector>
#include <map>
#include <sstream>
#include <wininet.h>
#include <dxgi.h>
#include <atlbase.h>
#include <dxgi1_2.h>
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "wininet.lib")

#include "stdinout2.h"
#include "rest.h"
#include "json.hpp"


const UINT VENDOR_NVIDIA = 0x10DE;
const UINT VENDOR_AMD = 0x1002;
const UINT VENDOR_INTEL = 0x8086;

struct GpuCaps
{
	bool hasNvidia = false;
	bool hasAmd = false;
	bool hasIntel = false;
	bool intelArc = false;
};

enum class LlamaBackend
{
	CUDA,
	Vulkan,
	SYCL,
	CPU
};



struct TOOL_PARAM
{
	std::string name;
	std::string type = "str";
	std::string desc;
};

struct TOOL_ENTRY
{
	std::string name;
	std::string desc;	
	std::vector<TOOL_PARAM> params;
};

struct DLL_LIST
{
	std::wstring dllpath;
	std::string callfunc;
	std::string deletefunc;
	std::vector<TOOL_ENTRY> tools;

};

class PushPopDirX
{
	std::vector<wchar_t> cd;
public:

	PushPopDirX(const wchar_t* f)
	{
		cd.resize(1000);
		GetCurrentDirectory(1000, cd.data());
		SetCurrentDirectory(f);
	}
	~PushPopDirX()
	{
		SetCurrentDirectory(cd.data());
	}
};


struct COPILOT_QUESTION
{
	std::wstring prompt;
	unsigned long long key = 0;
	HRESULT(__stdcall* cb1)(std::string token, LPARAM lp);
	LPARAM lpx = 0;
};

struct COPILOT_ANSWER
{
	std::vector<std::wstring> strings;
	HANDLE hEvent = 0;
	unsigned long long key = 0;
	HRESULT(__stdcall* cb1)(std::string token, LPARAM lp);
	LPARAM lpx = 0;
	~COPILOT_ANSWER()
	{
		if (hEvent)
			CloseHandle(hEvent);
		hEvent = 0;
	}
};
class COPILOT
{
	std::wstring folder;
	std::string model = "gpt-4.1";
	std::string if_server = "";
	int LLama = 0;
	std::wstring TempFile(const wchar_t* etx)
	{
		wchar_t path[1000] = {};
		GetTempPathW(1000, path);
		wchar_t fi[1000] = {};
		GetTempFileNameW(path, etx ? etx : L"tmp", 0, fi);
		DeleteFileW(fi);
		std::wstring s = fi;
		if (etx)
		{
			s += L".";
			s += etx;
		}
		return s;
	}

	HANDLE Run(const wchar_t* y, bool W, DWORD flg)
	{
		PROCESS_INFORMATION pInfo = { 0 };
		STARTUPINFO sInfo = { 0 };

		sInfo.cb = sizeof(sInfo);
		wchar_t yy[1000];
		swprintf_s(yy, 1000, L"%s", y);
		CreateProcess(0, yy, 0, 0, 0, flg, 0, 0, &sInfo, &pInfo);
		SetPriorityClass(pInfo.hProcess, IDLE_PRIORITY_CLASS);
		SetThreadPriority(pInfo.hThread, THREAD_PRIORITY_IDLE);
		if (W)
			WaitForSingleObject(pInfo.hProcess, INFINITE);
		else
		{
			CloseHandle(pInfo.hThread);
			return pInfo.hProcess;
		}
		DWORD ec = 0;
		GetExitCodeProcess(pInfo.hProcess, &ec);
		CloseHandle(pInfo.hProcess);
		CloseHandle(pInfo.hThread);
		return (HANDLE)(unsigned long long)ec;
	}

	std::shared_ptr<std::thread> interactiveThread;
	HANDLE hLLamaProcess = 0;
	std::recursive_mutex promptMutex;

	std::queue<COPILOT_QUESTION> Prompts;
	struct MESSAGE_AND_REPLY
	{
		bool System = 0;
		std::wstring message;
		std::wstring reply;
	};
	std::vector<MESSAGE_AND_REPLY> llama_history;
	float temperature = 0.7f;
	float top_p = 0.95f;
	float top_k = 40.0f;
	int seed = -1;
	float repeat_penalty = 1.1f;

	std::map<unsigned long long, std::shared_ptr<COPILOT_ANSWER>> Answers;


	void ReleaseAnswer(unsigned long long key)
	{
		std::lock_guard<std::recursive_mutex> lock(promptMutex);
		auto it = Answers.find(key);
		if (it != Answers.end())
		{
			Answers.erase(it);
		}
	}

	std::vector<DLL_LIST> dlls;
	std::wstring api_key;

public:



	static std::wstring tou(const char* s)
	{
		std::vector<wchar_t> buf(strlen(s) + 1);
		MultiByteToWideChar(CP_UTF8, 0, s, -1, buf.data(), (int)buf.size());
		return std::wstring(buf.data());
	}
	static std::string toc(const wchar_t* s)
	{
		std::vector<char> buf(wcslen(s) * 4 + 1);
		WideCharToMultiByte(CP_UTF8, 0, s, -1, buf.data(), (int)buf.size(), 0, 0);
		return std::string(buf.data());
	}
	static std::wstring ChangeSlash(const wchar_t* s)
	{
		std::wstring r = s;
		for (size_t i = 0; i < r.size(); i++)
		{
			if (r[i] == L'\\')
				r[i] = L'/';
		}
		return r;
	}


#ifdef _DEBUG
	DWORD flg = CREATE_NEW_CONSOLE;
#else
	DWORD flg = CREATE_NO_WINDOW;
#endif
	COPILOT(std::wstring folder, std::string model = "gpt-4.1",std::string if_server = "",int LLama = 0,const wchar_t* apikey = 0)
	{
		this->folder = folder;
		this->model = model;
		this->if_server = if_server;	
		this->LLama = LLama;
		if (apikey)
			this->api_key = apikey;
	}
	~COPILOT()
	{
		EndInteractive();
	}

	bool AddTool(size_t idll,std::string tool_name,std::string tool_desc,std::initializer_list<TOOL_PARAM> params)
	{
		if (idll >= dlls.size())
			return false;
		TOOL_ENTRY t;
		t.name = tool_name;
		t.desc = tool_desc;
		t.params = params;
		
		dlls[idll].tools.push_back(t);
		return true;
	}


	size_t AddDll(const std::wstring& dllpath, const std::string& callfunc, const std::string& deletefunc)
	{
		DLL_LIST d;
		d.dllpath = dllpath;
		d.callfunc = callfunc;
		d.deletefunc = deletefunc;
		dlls.push_back(d);
		return dlls.size() - 1;
	}	

	static std::vector<std::string> copilot_model_list() {
		return {
	"Claude Sonnet 4.5",
	"Claude Haiku 4.5",
	"Claude Opus 4.5",
	"Claude Sonnet 4",
	"GPT-5.2-Codex",
	"GPT-5.1-Codex-Max",
	"GPT-5.1-Codex",
	"GPT-5.2",
	"GPT-5.1",
	"GPT-5",
	"GPT-5.1-Codex-Mini",
	"GPT-5 mini",
	"GPT-4.1",
	"Gemini 3 Pro (Preview)",
		};
	};



	static GpuCaps DetectGpuCaps()
	{
		GpuCaps caps;

		CComPtr<IDXGIFactory1> factory;
		if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))
			return caps;

		for (UINT i = 0; ; ++i)
		{
			CComPtr<IDXGIAdapter1> adapter;
			if (factory->EnumAdapters1(i, &adapter) == DXGI_ERROR_NOT_FOUND)
				break;

			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			// Skip software adapter
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				continue;

			switch (desc.VendorId)
			{
			case VENDOR_NVIDIA:
				caps.hasNvidia = true;
				break;

			case VENDOR_AMD:
				caps.hasAmd = true;
				break;

			case VENDOR_INTEL:
				caps.hasIntel = true;
				if (desc.DedicatedVideoMemory > (2ull << 30))
					caps.intelArc = true;
				break;
			}
		}

		return caps;
	}


	static LlamaBackend ChooseBackend(const GpuCaps& caps)
	{
		if (caps.hasNvidia)
			return LlamaBackend::CUDA;

		if (caps.hasAmd)
			return LlamaBackend::Vulkan;

		if (caps.intelArc)
			return LlamaBackend::SYCL;

		if (caps.hasIntel)
			return LlamaBackend::Vulkan;

		return LlamaBackend::CPU;
	}


	static std::wstring GetDefaultCopilotfolder()
	{
		// Check registry for unchanged
		HKEY k = 0;
		std::wstring where;
		RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{5B2C72B7-E396-41CE-801D-EDA93EB4BA77}", 0, KEY_READ | KEY_WOW64_64KEY, &k);
		if (k)
		{
			wchar_t buf[MAX_PATH] = { 0 };
			DWORD sz = sizeof(buf);
			RegQueryValueExW(k, L"Where", 0, 0, (LPBYTE)buf, &sz);
			where = buf;
			RegCloseKey(k);
			if (where.length() > 2)
			{
				where += L"\\Copilot";
				return where;
			}
		}
		// Else, in programdata
		PWSTR p = 0;
		SHGetKnownFolderPath(FOLDERID_ProgramData, 0, 0, &p);
		std::wstring prjf;
		if (p)
		{
			wchar_t m[MAX_PATH] = { 0 };
			wcscpy_s(m, MAX_PATH, p);
			CoTaskMemFree(p);
			wcscat_s(m, MAX_PATH, L"\\933bd016-0397-42c9-b3e0-eaa7900ef53e");
			SHCreateDirectory(0, m);
			prjf = m;
		}
		return prjf;
	}


	static std::wstring GetDefaultLLamafolder()
	{
		// Check registry for unchanged
		HKEY k = 0;
		std::wstring where;
		RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{5B2C72B7-E396-41CE-801D-EDA93EB4BA77}", 0, KEY_READ | KEY_WOW64_64KEY, &k);
		if (k)
		{
			wchar_t buf[MAX_PATH] = { 0 };
			DWORD sz = sizeof(buf);
			RegQueryValueExW(k, L"Where", 0, 0, (LPBYTE)buf, &sz);
			where = buf;
			RegCloseKey(k);
			if (where.length() > 2)
			{
				where += L"\\LLama";
				return where;
			}
		}
		// Else, in programdata
		PWSTR p = 0;
		SHGetKnownFolderPath(FOLDERID_ProgramData, 0, 0, &p);
		std::wstring prjf;
		if (p)
		{
			wchar_t m[MAX_PATH] = { 0 };
			wcscpy_s(m, MAX_PATH, p);
			CoTaskMemFree(p);
			wcscat_s(m, MAX_PATH, L"\\059B885A-76DF-4436-8BD6-45FB92A14103");
			SHCreateDirectory(0, m);
			prjf = m;
		}
		return prjf;
	}

	bool IsInstalled()
	{
		wchar_t path[1000] = {};
		swprintf_s(path, 1000, L"%s\\copilot.exe", folder.c_str());
		return GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES;
	}




	std::shared_ptr<COPILOT_ANSWER> PushPrompt(const std::wstring& prompt,bool W, HRESULT(__stdcall* cb1)(std::string token, LPARAM lp) = 0,LPARAM lpx = 0)
	{
		auto answer = std::make_shared<COPILOT_ANSWER>();
		if (1)
		{
			std::lock_guard<std::recursive_mutex> lock(promptMutex);
			answer->hEvent = CreateEvent(0, FALSE, FALSE, 0);
			Sleep(50);
			answer->key = GetTickCount64();
			answer->cb1 = cb1;
			answer->lpx = lpx;
			Answers[answer->key] = answer;
			COPILOT_QUESTION q;
			q.key = answer->key;
			q.prompt = prompt;
			q.cb1 = cb1;
			q.lpx = lpx;
			Prompts.push(q);
		}
		if (W)
		{
			WaitForSingleObject(answer->hEvent, INFINITE);
			ReleaseAnswer(answer->key);
		}
		return answer;
	}


	void BeginInteractiveLLama()
	{
		if (interactiveThread)
			return;

		interactiveThread = std::make_shared <std::thread>([&]()
			{
				if (if_server.length() == 0)
				{
					std::vector<wchar_t> cmdline(1000);
					swprintf_s(cmdline.data(), 1000, L"%s\\llama-server.exe --n-gpu-layers 999 -m \"%S\" --port %i", folder.c_str(), model.c_str(), LLama);
					// Or cpu only
					GpuCaps caps = DetectGpuCaps();
					LlamaBackend backend = ChooseBackend(caps);
					if (backend == LlamaBackend::CPU)
					{
						swprintf_s(cmdline.data(), 1000, L"%s\\llama-server.exe  -m \"%S\" --port %i", folder.c_str(), model.c_str(), LLama);
					}
					STDINOUT2* io = new STDINOUT2();
					io->Prep(false);
					io->CreateChildProcess(folder.c_str(), cmdline.data(), flg == CREATE_NEW_CONSOLE ? true : false);
					if (1)
					{
						std::vector<char> buffer(4096);
						std::string output;
						for (;;)
						{
							DWORD read = 0;
							BOOL res = ReadFile(io->g_hChildStd_OUT_Rd, buffer.data(), (DWORD)buffer.size() - 1, &read, NULL);
							if (!res || read == 0)
								break;
							buffer[read] = 0;
							auto strx = std::string(buffer.data(), read);
							output += strx;
							if (output.find("erver is listening on") != std::string::npos)
								break;
						}
					}
					hLLamaProcess =io->piProcInfo.hProcess;

				}
				for (;;)
				{
					size_t sz = 0;
					if (1)
					{
						std::lock_guard<std::recursive_mutex> lock(promptMutex);
						sz = Prompts.size();
					}
					if (sz == 0)
					{
						Sleep(100);
						continue;
					}
					std::wstring r;
					std::lock_guard<std::recursive_mutex> lock(promptMutex);
					auto fr = Prompts.front();
					Prompts.pop();
					r = fr.prompt;
					if (r == L"exit" || r == L"quit")
					{
						SetEvent(Answers[fr.key]->hEvent);
						ReleaseAnswer(fr.key);
						// Terminate llama-server
						if (hLLamaProcess)
							TerminateProcess(hLLamaProcess, 0);
						hLLamaProcess = 0;
						break;
					}


					std::wstring Auth;
					std::wstring ver;
					if (api_key.length())
					{
						if (model == "Claude")
						{
							Auth = L"x-api-key: ";
							Auth += api_key.c_str();
							ver = L"anthropic-version: 2023-06-01";
						}
						else
						if (model == "Gemini")
						{
							Auth = L"x-goog-api-key: ";
							Auth += api_key.c_str();
						}
						else
						{
							Auth = L"Authorization: Bearer ";
							Auth += api_key.c_str();
						}
					}

					// Build buf
					nlohmann::json j;
					j["model"] = "model";
					j["temperature"] = temperature;
					if (fr.cb1)
						j["stream"] = true;
					j["top_k"] = top_k;
					j["top_p"] = top_p;
					j["repeat_penalty"] = repeat_penalty;
					j["seed"] = seed;
					nlohmann::json messages = nlohmann::json::array();
					for (auto& s : llama_history)
					{
						nlohmann::json mm;
						mm["role"] = "user";
						mm["content"] = toc(s.message.c_str()).c_str();
						messages.push_back(mm);
						nlohmann::json mr;
						mr["role"] = "assistant";
						mr["content"] = toc(s.reply.c_str()).c_str();
						messages.push_back(mr);
					}
					nlohmann::json mm;
					mm["role"] = "user";
					mm["content"] = toc(r.c_str()).c_str();
					messages.push_back(mm);
					j["messages"] = messages;

					auto buf = j.dump();



					RESTAPI::ihandle hr;
					RESTAPI::REST re;
					if (if_server.length() == 0)
					{
						re.Connect(L"localhost", false, (short)LLama);
						hr = re.RequestWithBuffer(L"/v1/chat/completions", L"POST", {}, buf.data(), buf.size());
					}
					else
					{
						std::wstring url = tou(if_server.c_str());
						url += L"/v1/chat/completions";

						std::wstring ContentType = L"Content-Type: application/json";
						std::wstring ContentLength;
						std::wstring Connection = L"Connection: close";
						wchar_t clbuf[100] = {};
						swprintf_s(clbuf, 100, L"Content-Length: %zu", buf.size());
						ContentLength = clbuf;

						hr = re.RequestWithBuffer(url.c_str(), L"POST", { ContentType,ContentLength,Connection,Auth,ver }, buf.data(), buf.size());
					}
					std::string response;


					std::vector<char> resp;
					struct D
					{
						std::vector<char>* resp;
						RESTAPI::memory_data_writer* w;
						COPILOT* pThis;
						COPILOT_QUESTION* fr = 0;
						std::string* response = 0;
					};
					RESTAPI::memory_data_writer w;

					if (fr.cb1)
					{
						D d;
						d.resp = &resp;
						d.pThis = this;
						d.w = &w;
						d.fr = &fr;
						d.response = &response;
						re.Read2(hr, w, [](size_t total_sent, size_t, void* lp) -> HRESULT
							{
								D* pThis = (D*)lp;
								std::vector<char>& g = pThis->w->GetP();
								auto ptr = g.data();
								ptr += pThis->resp->size();

								// copy it to resp
								auto old_size = pThis->resp->size();
								pThis->resp->resize(total_sent);
								memcpy(pThis->resp->data() + old_size, ptr, total_sent - old_size);

								// this is data: 
								for (;;)
								{
									auto str = strstr(ptr, "data: ");
									if (!str)
										return S_OK;
									str += 5;

									auto endline = strstr(str, "\n\n");
									if (!endline)
										return S_OK;
									std::string line(str, endline - str);

									// advance ptr
									ptr = endline + 2;

									if (line == "[DONE]" || line == " [DONE]")
									{
										std::lock_guard<std::recursive_mutex> lock(pThis->pThis->promptMutex);
										auto ans = pThis->pThis->Answers[pThis->fr->key];
										// fill response
										for (auto& s : ans->strings)
											*(pThis->response) += pThis->pThis->toc(s.c_str());
										SetEvent(ans->hEvent);
										pThis->pThis->ReleaseAnswer(pThis->fr->key);
										return S_OK;
									}
									try
									{
										nlohmann::json j = nlohmann::json::parse(line);
										if (j.contains("choices"))
										{
											auto& choices = j["choices"];
											if (choices.is_array() && choices.size())
											{
												auto& first = choices[0];
												if (first.contains("delta"))
												{
													auto& delta = first["delta"];
													if (delta.contains("content"))
													{
														auto content = delta["content"].get<std::string>();
														std::lock_guard<std::recursive_mutex> lock(pThis->pThis->promptMutex);
														auto ans = pThis->pThis->Answers[pThis->fr->key];
														ans->strings.push_back(pThis->pThis->tou(content.c_str()));
														// Call callback
														if (pThis->fr->cb1)
														{
															auto ef = pThis->fr->cb1(content, pThis->fr->lpx);
															if (FAILED(ef))
															{
																SetEvent(ans->hEvent);
																pThis->pThis->ReleaseAnswer(pThis->fr->key);
																return S_OK;
															}
														}
													}
												}
											}
										}
									}
									catch (...)
									{
									}
								}
							}, &d);
					}
					else
					{
						re.ReadToMemory(hr, resp);
						auto str = std::string(resp.data(), resp.size());
						try
						{
							nlohmann::json j = nlohmann::json::parse(str);
							if (j.contains("choices"))
							{
								auto& choices = j["choices"];
								if (choices.is_array() && choices.size())
								{
									auto& first = choices[0];
									if (first.contains("message"))
									{
										auto& message = first["message"];
										if (message.contains("content"))
										{
											response = message["content"].get<std::string>();
											std::lock_guard<std::recursive_mutex> lock(promptMutex);
											auto ans = Answers[fr.key];
											ans->strings.push_back(tou(response.c_str()));
											SetEvent(ans->hEvent);
											ReleaseAnswer(fr.key);
										}
									}
								}
							}
						}
						catch (...)
						{
						}
					}

					// Post entire history
					MESSAGE_AND_REPLY mr;
					mr.message = r;
					mr.reply = tou(response.c_str());
					llama_history.push_back(mr);
				}


			});

	}


	void BeginInteractive()
	{
		if (interactiveThread)
			return;
		if (LLama || api_key.length())
		{
			BeginInteractiveLLama();
			return;
		}
		interactiveThread = std::make_shared <std::thread> ([&]()
			{
				InteractiveCopilot([](LPARAM lp) -> COPILOT_QUESTION {
				
					COPILOT* pThis = (COPILOT*)lp;
					for (;;)
					{
						size_t sz = 0;
						if (1)
						{
							std::lock_guard<std::recursive_mutex> lock(pThis->promptMutex);
							sz = pThis->Prompts.size();
						}
						if (sz == 0)
						{
							Sleep(100);
							continue;
						}
						std::wstring r;
						std::lock_guard<std::recursive_mutex> lock(pThis->promptMutex);
						auto fr = pThis->Prompts.front();
						pThis->Prompts.pop();
						r = fr.prompt;
						if (r == L"exit" || r == L"quit")
							SetEvent(pThis->Answers[fr.key]->hEvent);
						COPILOT_QUESTION rr;
						rr.key = fr.key;
						rr.prompt = r;
						rr.cb1 = fr.cb1;
						rr.lpx = fr.lpx;
						return rr;
					}

					}, [](std::wstring response, unsigned long long key,LPARAM lp, bool End) 
						{
							COPILOT* pThis = (COPILOT*)lp;
							std::lock_guard<std::recursive_mutex> lock(pThis->promptMutex);
							if (End)
							{
								SetEvent(pThis->Answers[key]->hEvent);
								pThis->ReleaseAnswer(key);
								return;
							}
							pThis->Answers[key]->strings.push_back(response);
							if (pThis->Answers[key]->cb1)
								pThis->Answers[key]->cb1(pThis->toc(response.c_str()), pThis->Answers[key]->lpx);

						}, (LPARAM)this);
			});
	}

	void EndInteractive()
	{
		if (interactiveThread)
		{
			PushPrompt(L"exit", true);
			interactiveThread->join();
			interactiveThread.reset();
			interactiveThread = nullptr;
		}
	}

	void InteractiveCopilot(std::function<COPILOT_QUESTION(LPARAM lp)> pro,std::function<void(std::wstring, unsigned long long key,LPARAM lp,bool End)> cb,LPARAM lp)
	{	
		const char* py = R"(
# import pdb
import asyncio
import random
import sys
import os
import time
import ctypes
import json
from copilot import CopilotClient
from copilot.tools import define_tool
from copilot.generated.session_events import SessionEventType
from pydantic import BaseModel, Field

%s

async def main():
    %s
    await client.start()

    session = await client.create_session({
        "model": "%s",
        "streaming": True,
        "tools": [%s],
    })
    print("Model: ", "%s")

    a = 1
    def handle_event(event):
        nonlocal a
        if event.type == SessionEventType.ASSISTANT_MESSAGE_DELTA:
            with open("%s" + str(a) + ".txt", "w", encoding="utf-8") as f:
                f.write(event.data.delta_content)
                f.flush()
                # also to display
                print(event.data.delta_content, end="")
                a = a + 1

    session.on(handle_event)

    b = 1
    while True:
        fii = "%s" + str(b) + ".txt"
        if not os.path.exists(fii):
            time.sleep(1)
            continue
        time.sleep(1)
        with open(fii, "r",encoding="utf-8") as f:
            user_input = f.read().strip()
        # delete the file after reading
        os.remove(fii)
        # trim crlf at the end
        # pdb.set_trace()
        user_input = user_input.rstrip("\r\n")
        print("\033[92m",user_input,"\033[0m")
        if user_input == "exit":
            print("Exiting interactive session.")
            break
        if user_input == "quit":
            print("Exiting interactive session.")
            break
        await session.send_and_wait({"prompt": user_input})
        print("");
        b += 1
        # Indicate end of response
        with open("%s" + str(a) + ".txt", "w", encoding="utf-8") as f:
            f.write("--end--")
            f.flush()
            a = a + 1
    
asyncio.run(main())


)";

		PushPopDirX ppd(folder.c_str());
		auto tf = TempFile(L"py");
		auto tin = TempFile(nullptr);
		auto tout = TempFile(nullptr);
		std::vector<char> data(1000000);

		std::string cli = "client = CopilotClient()";
		if (if_server.length() > 0)
		{
			cli = R"(client = CopilotClient({  "cli_url": ")" + if_server + R"(" }))";
		}

		std::string dll_entries;
		std::string tools;
		for (size_t iDLL = 0 ; iDLL < dlls.size() ;iDLL++)
		{
			auto& d = dlls[iDLL];
			std::vector<char> d1(10000);
			sprintf_s(d1.data(),10000,R"(dll%zi = ctypes.CDLL("%s")
dll%zi.pcall.argtypes = (ctypes.c_char_p,)
dll%zi.pcall.restype  = ctypes.c_void_p
dll%zi.pdelete.argtypes = (ctypes.c_void_p,)
def call_cpp%zi(obj):
    s = json.dumps(obj).encode("utf-8")
    p = dll%zi.pcall(s)
    result = ctypes.string_at(p).decode("utf-8")
    dll%zi.pdelete(p)
    return json.loads(result)
)", iDLL + 1,toc(d.dllpath.c_str()).c_str(), iDLL + 1, iDLL + 1,iDLL + 1,iDLL + 1,iDLL + 1,iDLL + 1);
			dll_entries += d1.data();
			dll_entries += "\r\n";

			// Tools for this dll
			for (size_t iTool = 0; iTool < d.tools.size(); iTool++)
			{
				auto& t = d.tools[iTool];
				sprintf_s(d1.data(), 1000, R"(class tool%zi%zi_params(BaseModel):)", iDLL + 1, iTool + 1);
				dll_entries += d1.data();
				dll_entries += "\r\n";
				for (size_t iParam = 0; iParam < t.params.size(); iParam++)
				{
					auto& p = t.params[iParam];
					sprintf_s(d1.data(), 1000, R"(    %s: %s = Field(description="%s"))", p.name.c_str(), p.type.c_str(), p.desc.c_str());
					dll_entries += d1.data();
					dll_entries += "\r\n";
				}

				tools += t.name;
				if (iTool != d.tools.size() - 1)
					tools += ",";

				dll_entries += "\r\n";
				sprintf_s(d1.data(), 1000, R"(@define_tool(description="%s")
async def %s(params: tool%zi%zi_params) -> dict:)",t.desc.c_str(),t.name.c_str(), iDLL + 1, iTool + 1);
				dll_entries += d1.data();
				dll_entries += "\r\n";

				for (size_t iParam = 0; iParam < t.params.size(); iParam++)
				{
					auto& p = t.params[iParam];
					sprintf_s(d1.data(), 1000, R"(    %s = params.%s)",p.name.c_str(),p.name.c_str());
					dll_entries += d1.data();
					dll_entries += "\r\n";
				}

				sprintf_s(d1.data(), 1000, R"(    resp = await asyncio.to_thread(call_cpp%zi,{)", iDLL + 1);
				dll_entries += d1.data();

				for (size_t iParam = 0; iParam < t.params.size(); iParam++)
				{
					auto& p = t.params[iParam];
					sprintf_s(d1.data(), 1000, R"("%s": %s)",p.name.c_str(),p.name.c_str());
					dll_entries += d1.data();
					if (iParam != t.params.size() - 1)
						dll_entries += ",";
				}

				sprintf_s(d1.data(), 1000, R"(}))");
				dll_entries += d1.data();
				dll_entries += "\r\n";
				dll_entries += "    return resp";
				dll_entries += "\r\n";
			}

		}


		sprintf_s(data.data(), data.size(), py, dll_entries.c_str(),cli.c_str(), model.c_str(),tools.c_str(), model.c_str(), toc(ChangeSlash(tout.c_str()).c_str()).c_str(), toc(ChangeSlash(tin.c_str()).c_str()).c_str(), toc(ChangeSlash(tout.c_str()).c_str()).c_str());

		FILE* f = nullptr;
		_wfopen_s(&f, tf.c_str(), L"wb");
		fwrite(data.data(), 1, strlen(data.data()), f);
		fclose(f);
		std::wstring cmd = L"python \"" + tf + L"\"";

		auto hi = Run(cmd.c_str(), false, flg);
		int a = 1, b = 1;
		for (;;)
		{
			// If ended ?
			DWORD ec = 0;
			GetExitCodeProcess(hi, &ec);
			if (ec != STILL_ACTIVE)
				break;

			auto q = pro(lp);
			std::string prompt = toc(q.prompt.c_str());
			// write it to tin
			{
				std::wstring fn = tin + std::to_wstring(b) + L".txt";
				FILE* f = nullptr;
				_wfopen_s(&f, fn.c_str(), L"wb");
				if (f)
				{
					fwrite(prompt.c_str(), 1, prompt.size(), f);
					fclose(f);
					b++;
				}
			}

			// Now read response
			for (int sleepcount = 0;  sleepcount < 500 ;)
			{
				// If ended ?
				DWORD ec = 0;
				GetExitCodeProcess(hi, &ec);
				if (ec != STILL_ACTIVE)
					break;
				auto fn = tout + std::to_wstring(a) + L".txt";
				FILE* f = nullptr;
				_wfopen_s(&f, fn.c_str(), L"rb");
				if (f)
				{
					fseek(f, 0, SEEK_END);
					auto sz = ftell(f);
					fseek(f, 0, SEEK_SET);
					std::vector<char> outdata(sz + 1);
					fread(outdata.data(), 1, sz, f);
					outdata[sz] = 0;
					fclose(f);
					DeleteFileW(fn.c_str());
					std::wstring o = tou(outdata.data());
					cb(o,q.key, lp, o == L"--end--");
					a++;
					if (o == L"--end--")
						break;
				}
				else
				{
					Sleep(100);
					sleepcount++;
				}
			}
		}
		CloseHandle(hi);
		DeleteFileW(tf.c_str());

	}

};


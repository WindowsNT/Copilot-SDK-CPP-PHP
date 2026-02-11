
// Copilot Class

#pragma once
#include <string>
#include <queue>
#include <mutex>
#include <functional>
#include <vector>
#include <map>
#include <any>
#include <sstream>


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <WS2tcpip.h>
#include <wincrypt.h>   
#include <ShlObj.h>
#include <shellapi.h>
#include <wininet.h>
#include <dxgi.h>
#include <atlbase.h>
#undef min
#undef max
#include <dxgi1_2.h>
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "ws2_32.lib")
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


inline HRESULT OllamaRunning = S_FALSE;

HRESULT CopUpdate(HWND,int What); // 0 Install 1 Update 2 Remove 3 Check

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
	std::string ask_user_func;
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

struct COPILOT_ATTACHMENT
{
	std::wstring path;
	std::string type = "file";
};

struct COPILOT_QUESTION
{
	std::wstring prompt;
	std::vector<COPILOT_ATTACHMENT> attachments;
	unsigned long long key = 0;
	HRESULT(__stdcall* cb1)(int Status,std::string token, LPARAM lp);
	LPARAM lpx = 0;
};


struct COPILOT_ANSWER
{
	std::vector<std::wstring> strings;
	std::vector<std::wstring> reasoning;
	std::wstring Collect()
	{
		std::wstring r;
		for (auto& s : strings)
			r += s;
		return r;
	}
	std::wstring CollectReasoning()
	{
		std::wstring r;
		for (auto& s : reasoning)
			r += s;
		return r;
	}
	HANDLE hEvent = 0;
	unsigned long long key = 0;
	HRESULT(__stdcall* cb1)(int Status,std::string token, LPARAM lp);
	LPARAM lpx = 0;
	~COPILOT_ANSWER()
	{
		if (hEvent)
			CloseHandle(hEvent);
		hEvent = 0;
	}
};



struct COPILOT_MODEL
{
	std::string id;
	std::string fullname;
	float rate = 0.0f;
	bool SupportsVision = 0;
	bool Ollama = 0;
};

struct COPILOT_STATUS
{
	bool StatusValid = false;
	bool Installed = false;
	std::shared_future<HRESULT> NeedUpdate;
	std::wstring folder;
	bool OllamaInstalled = false;
	bool Connected = false;
	bool Authenticated = false;
	std::vector<COPILOT_MODEL> models;
};


struct COPILOT_SDK_MODEL
{
	std::string id;
	std::string name;
	bool SupportsVision = false;
	std::vector<std::string> supportedvisionmediatypes;
	int MaxPromptTokens = 0;
	int MaxContextWindowTokens = 0;
	int MaxPromptImages = 0;
	int MaxPromptImageSize = 0;
	std::string Terms;
	float BillingMultiplier = 0.0f;
};

struct COPILOT_PARAMETERS
{
	std::wstring folder;
	std::string model = "gpt-4.1";
	std::string remote_server;
	int LLama_Port = 0;
	std::wstring api_key;
	std::string reasoning_effort = "";
	std::string system_message = "";
	std::string custon_provider_type; // example  openai for ollama
	std::string custom_provider_base_url; //  "http://localhost:11434/v1"; for Ollama
#ifdef _DEBUG
	bool Debug = 1;
#else
	bool Debug = 0;
#endif
};

class COPILOT
{
	COPILOT_PARAMETERS cp;

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

	
	std::shared_ptr<std::thread> interactiveThread;
	HANDLE hProcess = 0;
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
	std::vector<COPILOT_ATTACHMENT>  NextAttachments;


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
	HANDLE hFM = 0, hFO = 0, hEI = 0, hEO = 0, hEC = 0;
	void* p1 = 0;
	void* p2 = 0;

	void CloseAllHandles()
	{
		if (p2)
			UnmapViewOfFile(p2);
		p2 = 0;
		if (p1)
			UnmapViewOfFile(p1);
		p1 = 0;


		if (hFM)
		{
			CloseHandle(hFM);
			hFM = 0;
		}
		if (hFO)
		{
			CloseHandle(hFO);
			hFO = 0;
		}
		if (hEI)
		{
			CloseHandle(hEI);
			hEI = 0;
		}
		if (hEO)
		{
			CloseHandle(hEO);
			hEO = 0;
		}
		if (hEC)
		{
			CloseHandle(hEC);
			hEC = 0;
		}
	}

public:

	std::any user_data;
	COPILOT_PARAMETERS& GetParameters()
	{
		return cp;
	}
	static HANDLE Run(const wchar_t* y, bool W, DWORD flgx)
	{
		PROCESS_INFORMATION pInfo = { 0 };
		STARTUPINFO sInfo = { 0 };

		sInfo.cb = sizeof(sInfo);
		wchar_t yy[1000];
		swprintf_s(yy, 1000, L"%s", y);
		CreateProcess(0, yy, 0, 0, 0, flgx, 0, 0, &sInfo, &pInfo);
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

	static void DetectOllamaRunning()
	{
		if (OllamaRunning == S_FALSE)
		{
			std::thread tx([&]()
				{
					TryOllamaRunning();
				});
			tx.detach();
		}

	}


	COPILOT(COPILOT_PARAMETERS& cx,bool NoOllama = false)
	{
		cp = cx;
		if (NoOllama == false)
			DetectOllamaRunning();
	}
	~COPILOT()
	{
		EndInteractive();
	}

#pragma comment(lib,"Comctl32.lib")
	static void ShowStatus(bool HasInstaller,const wchar_t* folder,bool Refresh = false,HWND hParent = 0)
	{
		auto def_folder = COPILOT::GetDefaultCopilotfolder();
		if (!folder)
			folder = def_folder.c_str();
		if (!wcslen(folder))
			folder = def_folder.c_str();
		auto st = Status(folder, Refresh);
		static bool Reshow = false;
		auto getst = [&]() -> std::wstring
			{
				std::wstring s;
				if (st.Installed)
				{
					s += std::wstring(L"Installed: ") + std::wstring(L"Yes.\r\n");
					if (st.Connected)
						s += std::wstring(L"Status: ") + std::wstring(L"Connected.\r\n");
					else
						s += std::wstring(L"Status: ") + std::wstring(L"Disconnected.\r\n");
					if (st.Authenticated)
						s += std::wstring(L"Authentication: ") + std::wstring(L"Authenticated.\r\n");
					else
						s += std::wstring(L"Authentication: ") + std::wstring(L"Not Authenticated.\r\n");
					if (st.NeedUpdate.valid() && st.NeedUpdate.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
					{
						if (st.NeedUpdate.get() == E_PENDING)
							s += std::wstring(L"Update Available.\r\n");
					}

					s += L"\r\n\r\n";
					if (!st.models.empty())
						s += L"Models\r\n----------------------------------------------------\r\n";
					for (auto& l : st.models)
					{
						wchar_t buf[200] = {};
						swprintf_s(buf, 100, L"%.2f - %S\r\n", l.rate, l.fullname.c_str());
						s += buf;
					}
				}
				else
					s = L"Copilot is not installed.";
				return s;
			};

		TASKDIALOGCONFIG tdc = {};
		tdc.cbSize = sizeof(tdc);
		tdc.hwndParent = hParent;
		tdc.pszWindowTitle = L"Copilot Status";
		tdc.pszMainInstruction = L"Copilot Status";
		auto status = getst();
		tdc.pszContent = status.c_str();
		TASKDIALOG_BUTTON buttons[10] = {};
		if (!st.Installed)
		{
			if (HasInstaller)
			{
				buttons[0].pszButtonText = L"Install";
				buttons[0].nButtonID = 103;
				buttons[1].pszButtonText = L"Close";
				buttons[1].nButtonID = IDCANCEL;
				tdc.pButtons = buttons;
				tdc.cButtons = 2;
			}
			else
			{
				buttons[0].pszButtonText = L"Close";
				buttons[0].nButtonID = IDCANCEL;
				tdc.pButtons = buttons;
				tdc.cButtons = 1;
			}
		}
		else
		{
			if (HasInstaller)
			{
				int ni = 0;
				if (st.NeedUpdate.valid() && st.NeedUpdate.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
				{
					auto hr = st.NeedUpdate.get();
					if (hr == E_PENDING)
					{ 
						buttons[ni].pszButtonText = L"Update";
						buttons[ni].nButtonID = 103;
						ni++;
					}
				}
				buttons[ni].pszButtonText = L"Installation Folder";
				buttons[ni].nButtonID = 104;
				ni++;
				buttons[ni].pszButtonText = L"Run CLI";
				if (!st.Authenticated)
					buttons[ni].pszButtonText = L"Authenticate";
				buttons[ni].nButtonID = 102;
				ni++;
				buttons[ni].pszButtonText = L"Remove";
				buttons[ni].nButtonID = 105;
				ni++;
				buttons[ni].pszButtonText = L"Refresh";
				buttons[ni].nButtonID = 101;
				ni++;
				buttons[ni].pszButtonText = L"Close";
				buttons[ni].nButtonID = IDCANCEL;
				tdc.pButtons = buttons;
				tdc.cButtons = ni + 1;
			}
			else
			{
				buttons[0].pszButtonText = L"Installation Folder";
				buttons[0].nButtonID = 104;
				buttons[1].pszButtonText = L"Run CLI";
				if (!st.Authenticated)
					buttons[1].pszButtonText = L"Authenticate";
				buttons[1].nButtonID = 102;
				buttons[2].pszButtonText = L"Refresh";
				buttons[2].nButtonID = 101;
				buttons[3].pszButtonText = L"Close";
				buttons[3].nButtonID = IDCANCEL;
				tdc.pButtons = buttons;
				tdc.cButtons = 4;
			}
		}
		struct P
		{
			COPILOT_STATUS* pst = 0;
			std::wstring folder;
			std::wstring* pStatus;
			std::function<std::wstring()> getStatus;
		};
		P p;
		p.folder = folder;
		p.pst = &st;
		p.pStatus = &status;
		p.getStatus = getst;
		tdc.lpCallbackData = (LPARAM) & p;
		tdc.pfCallback = [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM, LONG_PTR lpData)->HRESULT
		{
			P* p = (P*)lpData;
			if (msg == TDN_CREATED)
			{
				return S_FALSE;
			}
			if (msg == TDN_BUTTON_CLICKED)
			{
				int id = (int)wParam;
				if (id == 104)
				{
					ShellExecute(0, L"open", p->pst->folder.c_str(), 0, 0, SW_SHOW);
					return S_FALSE;
				}
				if (id == 103)
				{
					if (FAILED(CopUpdate(hwnd,p->pst->Installed ? 1 : 0)))
						return S_FALSE;
					Reshow = 1;
					return S_OK;
				}
				if (id == 105)
				{
					if (FAILED(CopUpdate(hwnd, 2)))
						return S_FALSE;
					Reshow = 1;
					return S_OK;
				}
				if (id == 102)
				{
					PushPopDirX pp(p->folder.c_str());
					auto cop_exe = std::wstring(p->pst->folder) + L"\\copilot.exe";
					COPILOT::Run(cop_exe.c_str(), false, CREATE_NEW_CONSOLE);
					return S_FALSE;
				}
				if (id == 101)
				{
					*p->pst = COPILOT::Status(p->folder.c_str(), true);
					*p->pStatus = p->getStatus();
					SendMessage(hwnd, TDM_SET_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)p->pStatus->c_str());
					return S_FALSE;
				}
				if (id == IDCANCEL)
					return S_OK;
				return S_FALSE;
			}
			return S_OK;
		};
		for (;;)
		{
			TaskDialogIndirect(&tdc, 0, 0, 0);
			if (!Reshow)
				break;
			Reshow = 0;
			ShowStatus(HasInstaller,folder, true, hParent);
			break;
		}
	}

	static COPILOT_STATUS Status(const wchar_t* folder,bool Refresh = false)
	{
		static COPILOT_STATUS s;
		if (Refresh)
			s.StatusValid = 0;
		if (s.StatusValid && s.folder == folder)
			return s;

		s.StatusValid = true;
		wchar_t path[1000] = {};
		swprintf_s(path, 1000, L"%s\\copilot.exe", folder);
		if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES)
		{
			s.Installed = false;
			return s;
		}
		s.folder = folder;

		COPILOT_PARAMETERS cp;
		cp.folder = folder;
		s.Installed = true;
		s.NeedUpdate = std::async([]() -> HRESULT
			{
				auto hr = CopUpdate(0, 3);
				return hr;
			});

		cp.Debug = false;
		TryOllamaRunning();
		if (OllamaRunning == S_OK)
			s.OllamaInstalled = true;
		COPILOT cop(cp, true);
		cop.BeginInteractive();
		auto conn = cop.State();
		s.Connected = conn == L"connected";
		auto auth = cop.AuthState();
		s.Authenticated = auth == L"true";
		s.models.clear();
		if (s.Authenticated)
			s.models = cop.copilot_model_list();
		if (s.OllamaInstalled)
		{
			auto ollama_models = ollama_list();
			for(auto& o : ollama_models)
				s.models.push_back(o);
		}
		cop.EndInteractive();
		return s;
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


	size_t AddDll(const std::wstring& dllpath, const std::string& callfunc, const std::string& deletefunc,const std::string& ask_user)
	{
		DLL_LIST d;
		d.dllpath = dllpath;
		d.callfunc = callfunc;
		d.deletefunc = deletefunc;
		d.ask_user_func = ask_user;
		dlls.push_back(d);
		return dlls.size() - 1;
	}	

	static void TryOllamaRunning()
	{
		if (OllamaRunning != S_FALSE)
			return;
		SOCKET x = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (x == INVALID_SOCKET)
		{
			OllamaRunning = E_FAIL;
			return;
		}
		sockaddr_in sa = { 0 };
		sa.sin_family = AF_INET;
		sa.sin_port = htons(11434);
		inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr);
		int res = connect(x, (sockaddr*)&sa, sizeof(sa));
		closesocket(x);
		if (res == SOCKET_ERROR)
			OllamaRunning = E_FAIL;
		else
			OllamaRunning = S_OK;
	}

	static std::vector<COPILOT_MODEL> ollama_list()
	{
		//http://localhost:11434/api/tags
		RESTAPI::REST r;
		r.Connect(L"localhost", false, 11434);
		RESTAPI::memory_data_provider d(0, 0);
		auto ji = r.Request2(L"/api/tags",d,true,L"GET");
		std::vector<char> dd;
		r.ReadToMemory(ji,dd);
		if (dd.size() == 0)
			return {};
		std::string s(dd.data(), dd.size());
		auto jx = nlohmann::json::parse(s);
		std::vector<COPILOT_MODEL> models;
		for (auto& item : jx["models"])
		{
			std::string name = item["model"];
			COPILOT_MODEL m;
			m.id = name;
			m.fullname = name;
			m.rate = 0.0f;
			m.Ollama = 1;
			models.push_back(m);
		}
		return models;
	}


	std::vector<COPILOT_MODEL> copilot_model_list() {

		auto sdk_models = ListModelsFromSDK();
		std::vector<COPILOT_MODEL> models;
		for (const auto& sdk_model : sdk_models)
		{
			COPILOT_MODEL m;
			m.fullname = sdk_model.name;
			m.id = sdk_model.id;
			m.rate = sdk_model.BillingMultiplier;
			m.SupportsVision = sdk_model.SupportsVision;
			m.Ollama = 0;
			models.push_back(m);
		}
		return models;


/*		return {
			{"GPT-4.1",0,0},
			{"GPT-5-Mini",0,0},

			{"GPT-5",1,1},
			{"GPT-5.1",1,1},
			{"GPT-5.1-Codex-Mini",0.33f,1},
			{"GPT-5.1-Codex",1,1},
			{"GPT-5.1-Codex-Max",1,1},
			{"GPT-5.2",1,1},
			{"GPT-5.2-Codex",1,1},

			{"Claude-Haiku-4.5",0.33f,1},
			{"Claude-Sonnet-4",1,1},
			{"Claude-Sonnet-4.5",1,1},
			{"Claude-Opus-4.5",3.0f,1},

			{ "Gemini-3-Pro-Preview",1,1 },
			
		};
		*/
	};

	/*
		"Claude Sonnet 4.5",
	"Claude Haiku 4.5",
	"Claude Opus 4.5",
	"Claude Sonnet 4",
	"Gemini 3 Pro (Preview)",

	*/


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

	bool IsCopilotInstalled()
	{
		wchar_t path[1000] = {};
		swprintf_s(path, 1000, L"%s\\copilot.exe", cp.folder.c_str());
		return GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES;
	}


	void CancelCurrent()
	{
		if (hEC)
			SetEvent(hEC);
	}


	

	void PushAttachmentForNextPrompt(const std::wstring& path)
	{
		COPILOT_ATTACHMENT a;
		a.path = path;
		NextAttachments.push_back(a);
	}

	bool CheckProcess = true;
	std::shared_ptr<COPILOT_ANSWER> PushPrompt(const std::wstring& prompt,bool W, HRESULT(__stdcall* cb1)(int Status,std::string token, LPARAM lp) = 0,LPARAM lpx = 0)
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
			q.attachments = NextAttachments;
			NextAttachments.clear();
			q.prompt = prompt;
			q.cb1 = cb1;
			q.lpx = lpx;
			Prompts.push(q);
		}
		if (W)
		{
			for (;;)
			{
				auto wi = WaitForSingleObject(answer->hEvent, 1000);
				if (wi == WAIT_OBJECT_0)
				{
					ReleaseAnswer(answer->key);
					break;
				}
				// Check process 
				if (CheckProcess)
				{
					DWORD ec = 0;
					GetExitCodeProcess(hProcess, &ec);
					if (ec != STILL_ACTIVE)
					{
						ReleaseAnswer(answer->key);
						break;
					}
				}
			}
		}
		return answer;
	}


	void BeginInteractiveLLama()
	{
		if (interactiveThread)
			return;

		interactiveThread = std::make_shared <std::thread>([&]()
			{
				if (cp.remote_server.length() == 0)
				{
					std::vector<wchar_t> cmdline(1000);
					swprintf_s(cmdline.data(), 1000, L"%s\\llama-server.exe --n-gpu-layers 999 -m \"%S\" --port %i", cp.folder.c_str(), cp.model.c_str(), cp.LLama_Port);
					// Or cpu only
					GpuCaps caps = DetectGpuCaps();
					LlamaBackend backend = ChooseBackend(caps);
					if (backend == LlamaBackend::CPU)
						swprintf_s(cmdline.data(), 1000, L"%s\\llama-server.exe  -m \"%S\" --port %i", cp.folder.c_str(), cp.model.c_str(), cp.LLama_Port);
					STDINOUT2* io = new STDINOUT2();
					io->Prep(false);
					io->CreateChildProcess(cp.folder.c_str(), cmdline.data(), cp.Debug);
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
					hProcess =io->piProcInfo.hProcess;

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
					if (r == L"/exit" || r == L"/quit")
					{
						SetEvent(Answers[fr.key]->hEvent);
						ReleaseAnswer(fr.key);
						// Terminate llama-server
						if (hProcess)
							TerminateProcess(hProcess, 0);
						hProcess = 0;
						break;
					}


					std::wstring Auth;
					std::wstring ver;
					if (cp.api_key.length())
					{
						if (cp.model == "Claude")
						{
							Auth = L"x-api-key: ";
							Auth += cp.api_key.c_str();
							ver = L"anthropic-version: 2023-06-01";
						}
						else
						if (cp.model == "Gemini")
						{
							Auth = L"x-goog-api-key: ";
							Auth += cp.api_key.c_str();
						}
						else
						{
							Auth = L"Authorization: Bearer ";
							Auth += cp.api_key.c_str();
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
					if (cp.remote_server.length() == 0)
					{
						re.Connect(L"localhost", false, (short)cp.LLama_Port);
						hr = re.RequestWithBuffer(L"/v1/chat/completions", L"POST", {}, buf.data(), buf.size());
					}
					else
					{
						std::wstring url = tou(cp.remote_server.c_str());
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
															auto ef = pThis->fr->cb1(1,content, pThis->fr->lpx);
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
		if (cp.LLama_Port && cp.api_key.length())
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
						if (r == L"/exit" || r == L"/quit")
							SetEvent(pThis->Answers[fr.key]->hEvent);
						COPILOT_QUESTION rr;
						rr.key = fr.key;
						rr.prompt = r;
						rr.attachments = fr.attachments;
						rr.cb1 = fr.cb1;
						rr.lpx = fr.lpx;
						return rr;
					}

					}, [](int Status,std::wstring response, unsigned long long key,LPARAM lp, bool End) 
						{
							COPILOT* pThis = (COPILOT*)lp;
							std::lock_guard<std::recursive_mutex> lock(pThis->promptMutex);
							if (End)
							{
								SetEvent(pThis->Answers[key]->hEvent);
								pThis->ReleaseAnswer(key);
								return;
							}
							if (Status == 3)
								pThis->Answers[key]->reasoning.push_back(response);
							else
								pThis->Answers[key]->strings.push_back(response);
							if (pThis->Answers[key]->cb1)
								pThis->Answers[key]->cb1(Status,pThis->toc(response.c_str()), pThis->Answers[key]->lpx);

						}, (LPARAM)this);
			});
	}

	void EndInteractive(bool PushedExit = false)
	{
		if (interactiveThread)
		{
			if (!PushedExit)	
				PushPrompt(L"/exit", true);
			interactiveThread->join();
			interactiveThread.reset();
			interactiveThread = nullptr;
		}
		CloseAllHandles();
	}

	std::wstring Ping()
	{
		auto ans = PushPrompt(L"/ping", true);
		if (!ans)
			return L"";
		return ans->Collect();
	}
	std::wstring State()
	{
		auto ans = PushPrompt(L"/state", true);
		if (!ans)
			return L"";
		return ans->Collect();
	}
	std::wstring AuthState()
	{
		auto ans = PushPrompt(L"/authstate", true);
		if (!ans)
			return L"";
		return ans->Collect();
	}

	std::vector<COPILOT_SDK_MODEL> ModelsFromJ(std::string s)
	{
		try
		{
			auto jx = nlohmann::json::parse(s);
			std::vector<COPILOT_SDK_MODEL> models;
			for (auto& item : jx)
			{
				COPILOT_SDK_MODEL m;
				m.id = item["id"].get<std::string>();
				m.name = item["name"].get<std::string>();
				if (item.contains("capabilities") && item["capabilities"].contains("limits"))
				{
					auto& limits = item["capabilities"]["limits"];
					if (limits.contains("max_prompt_tokens"))
						m.MaxPromptTokens = limits["max_prompt_tokens"].get<int>();
					if (limits.contains("max_context_window_tokens"))
						m.MaxContextWindowTokens = limits["max_context_window_tokens"].get<int>();
					if (limits.contains("vision"))
					{
						auto& vision = limits["vision"];
						if (vision.contains("supported_media_types"))
						{
							m.SupportsVision = true;
							for (auto& mt : vision["supported_media_types"])
								m.supportedvisionmediatypes.push_back(mt.get<std::string>());
						}
						if (vision.contains("max_prompt_images"))
							m.MaxPromptImages = vision["max_prompt_images"].get<int>();
						if (vision.contains("max_prompt_image_size"))
							m.MaxPromptImageSize = vision["max_prompt_image_size"].get<int>();
					}
					// policy
					if (item.contains("policy"))
					{
						auto& policy = item["policy"];
						if (policy.contains("terms"))
							m.Terms = policy["terms"].get<std::string>();
					}

					// billing multiplier
					if (item.contains("billing") && item["billing"].contains("multiplier"))
						m.BillingMultiplier = item["billing"]["multiplier"].get<float>();

					models.push_back(m);
				}
			}
			return models;
		}
		catch (...)
		{
			return {};
		}
	}

	std::vector<COPILOT_SDK_MODEL> ListModelsBeforeConnection()
	{
		const char* py = R"(
import sys
import asyncio
import json
from dataclasses import asdict

from copilot import CopilotClient
client = CopilotClient()
async def main():
    await client.start()
    models = await client.list_models()
    j = json.dumps([asdict(m) for m in models], indent=2, ensure_ascii=False)
    print(j)
    with open(sys.argv[1], 'w') as f:
        print(j, file=f) 
    
if len(sys.argv) <= 1:
    exit()
asyncio.run(main())
 
)";

		PushPopDirX ppd(cp.folder.c_str());
		auto tf = TempFile(L"py");
		std::vector<char> data(1000000);
		// Write py to file
		HANDLE h = CreateFileW(tf.c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		if (h != INVALID_HANDLE_VALUE)
		{
			DWORD written = 0;
			WriteFile(h, py, (DWORD)strlen(py), &written, 0);
			CloseHandle(h);
		}
		else
		{
			return {};
		}
		auto tf2 = TempFile(L"json");
		std::wstring cmd = L"python \"";
		cmd += tf.c_str();
		cmd += L"\" \"";
		cmd += tf2.c_str();
		cmd += L"\"";
		Run(cmd.c_str(), true, CREATE_NO_WINDOW);
		// Read json from file
		HANDLE h2 = CreateFileW(tf2.c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (h2 != INVALID_HANDLE_VALUE)
		{
			DWORD read = 0;
			std::vector<char> buffer(1000000);
			ReadFile(h2, buffer.data(), (DWORD)buffer.size() - 1, &read, 0);
			CloseHandle(h2);
			if (read > 0)
			{
				buffer[read] = 0;
				std::string s(buffer.data(), read);
				return ModelsFromJ(s);
			}
		}
		return { };
	}

	std::vector<COPILOT_SDK_MODEL> ListModelsFromSDK()
	{
		if (interactiveThread == nullptr)
			return ListModelsBeforeConnection();
		std::vector<COPILOT_SDK_MODEL> models;
		auto ans = PushPrompt(L"/models", true);
		if (!ans)
			return models;
		std::wstring r = ans->Collect();
		auto s = toc(r.c_str());
		return ModelsFromJ(s);
	}

	void InteractiveCopilot(std::function<COPILOT_QUESTION(LPARAM lp)> pro,std::function<void(int Status,std::wstring, unsigned long long key,LPARAM lp,bool End)> cb,LPARAM lp)
	{	
		const char* py = R"(
import pdb
import asyncio
import random
import sys
import struct
import os
import time
import ctypes
import win32event
import json
from dataclasses import asdict
from multiprocessing.shared_memory import SharedMemory
from copilot import CopilotClient
from copilot.tools import define_tool
from copilot.generated.session_events import SessionEventType
from pydantic import BaseModel, Field

%s

def is_json_direct(myjson,key):
    try:
        buf2 = json.loads(myjson)
        buf = buf2[key]
    except ValueError as e:
        return False
    except KeyError as e:
        return False
    return True

async def main():
    %s
    shm_in = SharedMemory(name="shm_in_%S", create=True, size=1024*1024)
    shm_out = SharedMemory(name="shm_out_%S", create=True, size=1024*1024)
    ev_in = win32event.CreateEvent(None, 0, 0, "ev_in_%S")
    ev_out = win32event.CreateEvent(None, 0, 0, "ev_out_%S")
    ev_cancel = win32event.CreateEvent(None, 0, 0, "ev_cancel_%S")
    buf = shm_out.buf
    struct.pack_into("<I", buf, 0, 0)         # write_index
    struct.pack_into("<I", buf, 4, 0)         # read_index
    struct.pack_into("<I", buf, 8, 1024*1024)  # capacity
    await client.start()

%s

    session = await client.create_session(session_config)
    print("Model: ", session_config['model'])

    def ring_write_final(payload: bytes):
        buf = shm_out.buf
        w = struct.unpack_from("<I", buf, 0)[0]
        r = struct.unpack_from("<I", buf, 4)[0]
	    
        capacity = struct.unpack_from("<I", buf, 8)[0]
        data_off = 12
	    
        msg_len = 4 + len(payload)
	    
        # free space
        if w >= r:
            free = capacity - (w - r)
        else:
            free = r - w
	    
        if free <= msg_len:
            # FULL drop
            return False
	    
        pos = data_off + w;
        # write size
        struct.pack_into("<I", buf, pos, len(payload))
        pos += 4	
        # payload (wrap safe)
        end = min(len(payload), capacity - (w + 4))
        buf[pos:pos+end] = payload[:end]
        if end < len(payload):
            buf[data_off:data_off+(len(payload)-end)] = payload[end:]
	    
        # advance write
        struct.pack_into("<I", buf, 0, (w + msg_len) %% capacity)
        return True

    def ring_write_status(payload: bytes, status: int):
        # first byte of payload is status
        payload = bytes([status]) + payload
        return ring_write_final(payload)

    def ring_write(payload: bytes):
        return ring_write_status(payload,1)

    start_reasoning = 0
    def handle_event(event):
        nonlocal start_reasoning
        if event.type == SessionEventType.ASSISTANT_REASONING:
            if (start_reasoning == 1):
                print("\033[0m")
                start_reasoning = 0
        if event.type == SessionEventType.ASSISTANT_REASONING_DELTA:
            payload = event.data.delta_content.encode("utf-8")
            if (start_reasoning == 0):
                print("\033[35m",end='')
                start_reasoning = 1
            print(event.data.delta_content,end='')
            ring_write_status(payload,3)
            # if ev_cancel is set, stop
            wait = win32event.WaitForSingleObject(ev_cancel, 0)
            if wait == win32event.WAIT_OBJECT_0:
                asyncio.get_running_loop().create_task(session.abort())
            win32event.SetEvent(ev_out)
        if event.type == SessionEventType.ASSISTANT_MESSAGE_DELTA:
            if (start_reasoning == 1):
                print("\033[0m")
                start_reasoning = 0
            payload = event.data.delta_content.encode("utf-8")
            # print it
            print(event.data.delta_content, end='', flush=True)
            ring_write(payload)
            # if ev_cancel is set, stop
            wait = win32event.WaitForSingleObject(ev_cancel, 0)
            if wait == win32event.WAIT_OBJECT_0:
                asyncio.get_running_loop().create_task(session.abort())
            # set event
            win32event.SetEvent(ev_out)

    session.on(handle_event)

    loop = asyncio.get_running_loop()
    while True:
        await loop.run_in_executor(
            None,
            win32event.WaitForSingleObject,
            ev_in,
            win32event.INFINITE
            )
        # first 4 bytes = size
        buf = shm_in.buf
        size = struct.unpack_from("<I", buf, 0)[0]
        if size == 0 or size > len(buf) - 4:
            continue  # corrupted / empty
        payload = bytes(buf[4:4+size])
        user_input = payload.decode("utf-8").rstrip("\r\n")
        if user_input == "/exit":
            print("\033[91m",user_input,"\033[0m",sep='')
            print("Exiting interactive session.")
            break
        if user_input == "/quit":
            print("\033[91m",user_input,"\033[0m",sep='')
            print("Exiting interactive session.")
            break
        if user_input == "/ping":
            print("\033[91m",user_input,"\033[0m",sep='')
            pong = await client.ping("")
            ring_write(bytes(pong.message, 'utf-8'))
            end_payload = "--end--".encode("utf-8")
            ring_write_status(end_payload,2)
            win32event.SetEvent(ev_out)
            continue
        if user_input == "/authstate":
            print("\033[91m",user_input,"\033[0m",sep='')
            pong = await client.get_auth_status()
            ring_write(bytes(json.dumps(pong.isAuthenticated), 'utf-8'))
            end_payload = "--end--".encode("utf-8")
            ring_write_status(end_payload,2)
            win32event.SetEvent(ev_out)
            continue
        if user_input == "/state":
            print("\033[91m",user_input,"\033[0m",sep='')
            pong = client.get_state()
            ring_write(bytes(pong, 'utf-8'))
            end_payload = "--end--".encode("utf-8")
            ring_write_status(end_payload,2)
            win32event.SetEvent(ev_out)
            continue
        if user_input == "/models":
            print("\033[91m",user_input,"\033[0m",sep='')
            models = await client.list_models()
            payload = json.dumps([asdict(m) for m in models], indent=2, ensure_ascii=False)
            ring_write(bytes(payload, 'utf-8'))
            end_payload = "--end--".encode("utf-8")
            ring_write_status(end_payload,2)
            win32event.SetEvent(ev_out)
            continue
        print("\033[92m",user_input,"\033[0m",end='',sep='')
        print('')
        # check if user passed a direct message
        isjd = is_json_direct(user_input,"direct")
        if isjd:
			# parse direct message
             await session.send_and_wait(json.loads(user_input))
        else:
            await session.send_and_wait({"prompt": user_input}, timeout=None)
        # also send --end--
        end_payload = "--end--".encode("utf-8")
        print("");
        ring_write_status(end_payload,2)
        win32event.SetEvent(ev_out)

    shm_in.close()
    shm_out.close()
    shm_in.unlink()
    shm_out.unlink()       
    
asyncio.run(main())



)";

		PushPopDirX ppd(cp.folder.c_str());
		auto tf = TempFile(L"py");
#ifdef _DEBUG
//		tf = L"r:\\1.py";
#endif
		std::vector<char> data(1000000);

		CLSID clsid = {};
		CoCreateGuid(&clsid);
		wchar_t clsid_buf[100] = {};
		StringFromGUID2(clsid, clsid_buf, 100);

		std::string cli = "client = CopilotClient()";
		if (cp.remote_server.length() > 0)
		{
			cli = R"(client = CopilotClient({  "cli_url": ")" + cp.remote_server + R"(" }))";
		}


		std::string dll_entries;
		std::vector<std::string> tools_array;
		for (size_t iDLL = 0 ; iDLL < dlls.size() ;iDLL++)
		{
			auto& d = dlls[iDLL];
			std::vector<char> d1(10000);
			sprintf_s(d1.data(),10000,R"(dll%zi = ctypes.CDLL("%s")
dll%zi.%s.argtypes = (ctypes.c_char_p,)
dll%zi.%s.restype  = ctypes.c_void_p
dll%zi.ask_user.argtypes = (ctypes.c_char_p,)
dll%zi.ask_user.restype  = ctypes.c_void_p
dll%zi.%s.argtypes = (ctypes.c_void_p,)
def call_cpp%zi(obj):
    s = json.dumps(obj).encode("utf-8")
    p = dll%zi.%s(s)
    result = ctypes.string_at(p).decode("utf-8")
    dll%zi.%s(p)
    return json.loads(result)
def au_cpp%zi(obj):
    s = json.dumps(obj).encode("utf-8")
    p = dll%zi.%s(s)
    result = ctypes.string_at(p).decode("utf-8")
    dll%zi.%s(p)
    return json.loads(result)
)", iDLL + 1,toc(d.dllpath.c_str()).c_str(),

		iDLL + 1, d.callfunc.c_str(),iDLL + 1, d.callfunc.c_str(),iDLL + 1, iDLL + 1, iDLL + 1,d.deletefunc.c_str(),

			iDLL + 1,iDLL + 1, d.callfunc.c_str(),iDLL + 1,d.deletefunc.c_str(),
				iDLL + 1, iDLL + 1, d.ask_user_func.c_str(), iDLL + 1, d.deletefunc.c_str()
				);
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

				tools_array.push_back(t.name);

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


		std::vector<char> config(10000);
		// init
		sprintf_s(config.data() + strlen(config.data()), 10000 - strlen(config.data()), "    session_config = {");

		// model
		sprintf_s(config.data() + strlen(config.data()), 10000 - strlen(config.data()), R"(
        "model": "%s",
        "streaming": True,)", cp.model.c_str());

		// Dlls ask_user any
		for (size_t i = 0; i < dlls.size(); i++)
		{
			auto& d = dlls[i];
			if (d.ask_user_func.length())
			{
				sprintf_s(config.data() + strlen(config.data()), 10000 - strlen(config.data()), R"(
		"on_user_input_request": au_cpp%zi,)", i + 1);
				break;
			}
		}

		// reasoning_effort
		if (cp.reasoning_effort.length())
		{
			sprintf_s(config.data() + strlen(config.data()), 10000 - strlen(config.data()), R"(
        "reasoning_effort": "%s",)", cp.reasoning_effort.c_str());
		}

		if (cp.system_message.length())
		{
			sprintf_s(config.data() + strlen(config.data()), 10000 - strlen(config.data()), R"(
        "system_message": {"content": "%s"},)", cp.system_message.c_str());
		}

		// Tools
		if (tools_array.size())
		{
			sprintf_s(config.data() + strlen(config.data()), 10000 - strlen(config.data()), R"(
        "tools": [)");
			for (size_t i = 0; i < tools_array.size(); i++)
			{
				sprintf_s(config.data() + strlen(config.data()), 10000 - strlen(config.data()), R"(
            %s)", tools_array[i].c_str());
				if (i != tools_array.size() - 1)
					sprintf_s(config.data() + strlen(config.data()), 10000 - strlen(config.data()), ",");
			}
			sprintf_s(config.data() + strlen(config.data()), 10000 - strlen(config.data()), R"(
        ],)");
		}

		if (cp.custon_provider_type.length() > 2)
		{
			// build something like 
			/*
			"provider": {
        "type": "...",
        "base_url": "...",  # Ollama endpoint
        "api_key": "..."
    },
	*/

			sprintf_s(config.data() + strlen(config.data()), 10000 - strlen(config.data()), R"(
        "provider": {
            "type": "%s",)", cp.custon_provider_type.c_str());
			if (cp.custom_provider_base_url.length() > 0)
			{
				sprintf_s(config.data() + strlen(config.data()), 10000 - strlen(config.data()), R"(
            "base_url": "%s",)", cp.custom_provider_base_url.c_str());
			}
			if (cp.api_key.length() > 0)
			{
				sprintf_s(config.data() + strlen(config.data()), 10000 - strlen(config.data()), R"(
            "api_key": "%S",)", cp.api_key.c_str());
			}
			// end provider
			sprintf_s(config.data() + strlen(config.data()), 10000 - strlen(config.data()), R"(
        },)");
		}

		// end
		sprintf_s(config.data() + strlen(config.data()), 10000 - strlen(config.data()), R"(
    })");


		sprintf_s(data.data(), data.size(), py, dll_entries.c_str(),cli.c_str(), 
			clsid_buf, clsid_buf, clsid_buf, clsid_buf, clsid_buf,
			config.data()
			);

		FILE* f = nullptr;
		_wfopen_s(&f, tf.c_str(), L"wb");
		fwrite(data.data(), 1, strlen(data.data()), f);
		fclose(f);
		std::wstring cmd = L"python \"" + tf + L"\"";

		hProcess = Run(cmd.c_str(), false, cp.Debug ? CREATE_NEW_CONSOLE : CREATE_NO_WINDOW);

		// Get the shared memory and events
	
		// 5 tries
		for (int tr = 0; tr < 15; tr++)
		{
			if (!hFM)
				hFM = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, (L"shm_in_" + std::wstring(clsid_buf)).c_str());
			if (!hFO)
				hFO = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, (L"shm_out_" + std::wstring(clsid_buf)).c_str());
			if (!hEI)
				hEI = OpenEventW(EVENT_ALL_ACCESS, FALSE, (L"ev_in_" + std::wstring(clsid_buf)).c_str());
			if (!hEO)
				hEO = OpenEventW(EVENT_ALL_ACCESS, FALSE, (L"ev_out_" + std::wstring(clsid_buf)).c_str());
			if (!hEC)
				hEC = OpenEventW(EVENT_ALL_ACCESS, FALSE, (L"ev_cancel_" + std::wstring(clsid_buf)).c_str());
			if (!hFM || !hFO || !hEI || !hEO || !hEC)
			{
				Sleep(1000);
	
			}
			else
				break;
		}
		if (!hFM || !hFO || !hEI || !hEO || !hEC)
		{
			CloseAllHandles();
			if (hProcess)
			{
				TerminateProcess(hProcess, 0);
				CloseHandle(hProcess);
			}
			hProcess = 0;
#ifndef _DEBUG
			DeleteFileW(tf.c_str());
#endif
			return;
		}
		p1 = MapViewOfFile(hFM, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		p2 = MapViewOfFile(hFO, FILE_MAP_ALL_ACCESS, 0, 0, 0);


		for (; p1 && p2 ;)
		{
			// If ended ?
			if (CheckProcess)
			{
				DWORD ec = 0;
				GetExitCodeProcess(hProcess, &ec);
				if (ec != STILL_ACTIVE)
					break;
			}
			auto q = pro(lp);
			std::string prompt = toc(q.prompt.c_str());
			if (q.attachments.size())
			{
				// We must pass a direct json with 
				nlohmann::json j;
				j["direct"] = true;
				j["prompt"] = prompt;
				nlohmann::json attachments = nlohmann::json::array();
				for (auto& a : q.attachments)
				{
					nlohmann::json ja;
					ja["path"] = toc(a.path.c_str());
					ja["type"] = a.type;
					attachments.push_back(ja);
				}
				j["attachments"] = attachments;
				prompt = j.dump();
			}

			// Write it to shared memory
			if (1)
			{
				auto buf = (char*)p1;
				size_t sz = prompt.size();
				if (sz > (1024*1024 - 10))					
					sz = 1024 * 1024 - 10;
				// First 4 bytes = size
				*(unsigned int*)buf = (unsigned int)sz;
				memcpy(buf + 4, prompt.c_str(), sz);
				// Signal event
				SetEvent(hEI);
			}

			if (prompt == "/exit" || prompt == "/quit")
				break;

			if (1)
			{
				uint8_t* base = (uint8_t*)p2;

				uint32_t* write_idx = (uint32_t*)(base + 0);
				uint32_t* read_idx = (uint32_t*)(base + 4);
				uint32_t  capacity = *(uint32_t*)(base + 8);
				uint8_t* data = base + 12;
				bool Ended = false;
				for (; !Ended;)
				{
					// Wait for output event
					for (;;)
					{
						auto wai = WaitForSingleObject(hEO, 1000);
						if (wai == WAIT_OBJECT_0)
							break;
						// Check Process
						if (CheckProcess)
						{
							DWORD ec = 0;
							GetExitCodeProcess(hProcess, &ec);
							if (ec != STILL_ACTIVE)
							{
								Ended = true;
								break;
							}
						}

					}

					if (1)
					{
						// Read shared memory
						while (*read_idx != *write_idx)
						{
							// read size
							uint32_t size;
							memcpy(&size, data + *read_idx, 4);
							*read_idx = (*read_idx + 4) % capacity;

							// read payload
							std::vector<char> buf(size);

							uint32_t first = std::min(size, capacity - *read_idx);
							memcpy(buf.data(), data + *read_idx, first);

							if (first < size)
								memcpy(buf.data() + first, data, size - first);

							*read_idx = (*read_idx + size) % capacity;

							unsigned char StatusByte = buf[0];
							std::string msg(buf.data() + 1, buf.size() - 1);

							if (StatusByte == 3)
							{
								// reasoning
								cb(3, tou(msg.c_str()), q.key, lp, false);
								continue;
							}

							// ---- termination check ----
							if (msg == "--end--")
							{
								// Call callback
								cb(2,tou(msg.c_str()), q.key, lp, true);
								Ended = true;
								break;
							}
							// Call callback
							cb(1,tou(msg.c_str()), q.key, lp, false);
						}
					}
				}
			}
		}

		CloseHandle(hProcess);
#ifndef _DEBUG
		DeleteFileW(tf.c_str());
#endif
	}

};


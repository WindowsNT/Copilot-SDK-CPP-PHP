//
#ifndef _WIN32
typedef int SOCKET;
#endif

struct COMPLETED_MESSAGE
{
	std::string content;
	std::string messageId;
	std::string reasoningText;
};

struct MESSAGE
{
	std::string content;
	int Ack = 0;

};

struct PENDING_MESSAGE
{
	std::string m;
	std::string id;
	std::function<HRESULT(std::string& reasoning, long long ptr)> reasoning_callback = nullptr;
	std::function<HRESULT(std::string& msg, long long ptr)> messaging_callback = nullptr;
	long long ptr = 0;
	std::shared_ptr<COMPLETED_MESSAGE> completed_message;
	std::vector<std::shared_ptr<MESSAGE>> reasoning_messages;
	std::vector<std::shared_ptr<MESSAGE>> output_messages;
};


struct COPILOT_SESSION
{
	std::string sessionId;
	std::string cwd;
	std::string title;
	std::string updatedAt;
	std::string model;
	std::shared_ptr<PENDING_MESSAGE> pending_message;
	std::vector<std::shared_ptr<PENDING_MESSAGE>> pending_messages;
};

class COPILOT_RAW
{
	std::string host;
	int port = 0;
	SOCKET x = 0;
	int nid = 1;
	bool Headless = 1;

	std::string next()
	{
		return std::to_string(nid++);
	}
	std::vector<std::shared_ptr<COPILOT_SESSION>> all_sessions;


	int ssend(const char* data, int len)
	{
		int total_sent = 0;
		while (total_sent < len)
		{
			int sent = send(x, data + total_sent, len - total_sent, 0);
			if (sent == 0 || sent == -1)
				return -1;
			total_sent += sent;
		}
		return total_sent;
	}

	std::vector<std::string> responses;
	std::recursive_mutex response_mutex;

	std::mutex m;
	std::condition_variable cv;
	bool ready = false;

	std::shared_ptr<std::thread> wait_thread;



	std::vector<std::string> ExtractResponsesFromBuffer(std::vector<char>& b)
	{
		std::vector<std::string> rs;

		// Find Content-Length: XXX\r\n\r\n
		for (;;)
		{
			auto c1 = std::search(b.begin(), b.end(), "Content-Length: ", "Content-Length: " + 16);
			if (c1 == b.end())
				break;
			c1 += 16;
			auto c2 = std::search(c1, b.end(), "\r\n\r\n", "\r\n\r\n" + 4);
			if (c2 == b.end())
				break;
			std::string len_str(c1, c2);
			int content_length = std::stoi(len_str);
			auto content_start = c2 + 4;
			// This is invalid because it seeks after end, fix it
			if (std::distance(content_start, b.end()) < content_length)
				break;
			std::string content(content_start, content_start + content_length);
			rs.push_back(content);
			b.erase(b.begin(), content_start + content_length);
		}
		return rs;
	}

/*	HRESULT GetAFullResponse()
	{
		std::vector<char> buf = pending;
		pending.clear();
		auto cs = buf.size();
		buf.resize(cs + 4096);
		int HasFoundLength = 0;
		for(;;)
		{
			int received = recv(x, buf.data() + cs, 4096, 0);
			if (received == SOCKET_ERROR)
				return E_ABORT;
		buf.resize(cs + received);
		if (HasFoundLength == 0)
		{
			// Check if there's a Content-Length header and if we have received the full response
			size_t header_end = response.find("\r\n\r\n");
			if (header_end != std::string::npos)
			{
				size_t content_length_pos = response.find("Content-Length: ");
				if (content_length_pos != std::string::npos)
				{
					size_t content_length_end = response.find("\r\n", content_length_pos);
					if (content_length_end != std::string::npos)
					{
						std::string content_length_str = response.substr(content_length_pos + 16, content_length_end - (content_length_pos + 16));
						int content_length = std::stoi(content_length_str);
						size_t total_response_size = header_end + 4 + content_length;
						MustContentLength = total_response_size;
						if (response.size() >= total_response_size)
						{
							response = response.substr(header_end + 4, content_length);
							break;
						}
					}
				}
			}
		}
			else
			{
				if (response.find("\n") != std::string::npos)
					break;
			}
		}


	}
	*/

	void GoPending(std::shared_ptr<COPILOT_SESSION> s)
	{
		if (s->pending_messages.size())
		{
			auto pm = s->pending_messages[0];
			s->pending_messages.erase(s->pending_messages.begin());
			if (s->pending_message)
			{
				s->pending_message = nullptr;
			}
			Send(s, pm);
		}
	}

	void WaitThread()
	{
		std::vector<char> pending;
		for (;;)
		{
			std::vector<char> buf(4096);
			int received = recv(x, buf.data(), (int)buf.size(), 0);
			if (received == SOCKET_ERROR || received == 0)
				break;
			buf.resize(received);
			pending.insert(pending.end(), buf.begin(), buf.end());

			auto responses2 = ExtractResponsesFromBuffer(pending);

			for (auto response : responses2)
			{
				std::lock_guard<std::recursive_mutex> lock(response_mutex);
#ifdef _DEBUG
				OutputDebugStringA(response.c_str());
				OutputDebugStringA("\n");
#endif

				// Parse response for "method" session.event
				if (1)
				{
					nlohmann::json r;
					try
					{
						r = nlohmann::json::parse(response.c_str(), response.c_str() + response.size());

						// see if the method is session.event and if it has params.event.type session.start, if so, extract the sessionId and cwd
						if (r.contains("method") && r["method"] == "session.event" && r.contains("params") && r["params"].contains("event") && r["params"]["event"].contains("type"))
						{
							auto data = r["params"]["event"]["data"];
							std::string sessionId, cwd;
							if (r["params"].contains("sessionId"))
								sessionId = r["params"]["sessionId"].get<std::string>();
							std::string evtype = r["params"]["event"]["type"].get<std::string>();
							if (evtype == "assistant.reasoning_delta")
							{
								bool F = 0;
								for (auto& s : all_sessions)
								{
									if (s->sessionId == sessionId)
									{
										auto m = std::make_shared<MESSAGE>();
										m->content = data["deltaContent"].get<std::string>();
										if (s->pending_message)
											s->pending_message->reasoning_messages.push_back(m);
										F = 1;
										break;
									}
								}
								if (F)
									continue;
							}
							if (evtype == "assistant.message_delta")
							{
								bool F = 0;
								for (auto& s : all_sessions)
								{
									if (s->sessionId == sessionId)
									{
										auto m = std::make_shared<MESSAGE>();
										m->content = data["deltaContent"].get<std::string>();
										if (s->pending_message)
											s->pending_message->output_messages.push_back(m);
										F = 1;
										break;
									}
								}
								if (F)
									continue;
							}
							if (evtype == "assistant.message")
							{
								bool F = 0;
								for (auto& s : all_sessions)
								{
									if (s->sessionId == sessionId)
									{
										if (s->pending_message)
										{
											if (!s->pending_message->completed_message)
												s->pending_message->completed_message = std::make_shared<COMPLETED_MESSAGE>();
											s->pending_message->completed_message->content = data["content"].get<std::string>();
											s->pending_message->completed_message->messageId = data["messageId"].get<std::string>();
											if (data.contains("reasoningText"))
												s->pending_message->completed_message->reasoningText = data["reasoningText"].get<std::string>();
											s->pending_message = 0;
										}
										GoPending(s);
										F = 1;
										break;
									}
								}
								if (F)
									continue;
							}
							if (evtype == "session.idle")
							{
								for (auto& s : all_sessions)
								{
									if (s->sessionId == sessionId)
									{
										GoPending(s);
										break;
									}
								}
							}

							if (evtype == "session.start")
							{
								auto data = r["params"]["event"]["data"];
								std::string sessionId, cwd;
								if (data.contains("sessionId"))
									sessionId = data["sessionId"].get<std::string>();
								if (data.contains("context") && data["context"].contains("cwd"))
									cwd = data["context"]["cwd"].get<std::string>();
								// Update all_sessions

								bool F = 0;
								for (auto& s : all_sessions)
								{
									if (s->sessionId == sessionId)
									{
										F = 1;
										s->cwd = cwd;
										break;
									}
								}
								if (!F)
								{
									auto s = std::make_shared<COPILOT_SESSION>();
									s->sessionId = sessionId;
									s->cwd = cwd;
									all_sessions.push_back(s);
								}
							}
						}
					}
					catch (std::exception& e)
					{
						[[maybe_unused]] auto wh = e.what();
						// Not a valid JSON, ignore
						return;
					}
				}

				responses.push_back(response);
				std::lock_guard<std::mutex> lock2(m);
				ready = true;
				cv.notify_one();
			}
		}

	}

	nlohmann::json ret(nlohmann::json& j,bool w)
	{
		auto send = j.dump();
		if (!Headless)
			send += "\n";
#ifdef _DEBUG
		OutputDebugStringA(send.c_str());
		if (Headless)
			OutputDebugStringA("\n");
#endif

		if (Headless)
		{
			// Must also send Content-Length: XXX \r\ndata
			std::string header = "Content-Length: " + std::to_string(send.length()) + "\r\n\r\n";
			send = header + send;
		}

		ssend(send.c_str(), (int)send.length());
		if (!w)
			return {};

		auto id = j["id"];

		std::unique_lock<std::mutex> lock(m);
		cv.wait(lock, [&] { return ready; });
		ready = false;
		std::string response;
		{
			bool F = 0;
			for (;;)
			{
				if (F)
					break;
				std::lock_guard<std::recursive_mutex> lock(response_mutex);
				for (size_t ir = 0 ; ir < responses.size() ; ir++)
				{
					auto& r = responses[ir];
					nlohmann::json rr;
					try
					{
						rr = nlohmann::json::parse(r.c_str(), r.c_str() + r.size());
					}
					catch (...)
					{
						continue;
					}
					if (rr.contains("id") && rr["id"] == id)
					{
						response = r;
						responses.erase(responses.begin(), responses.begin() + ir + 1);
						F = 1;
						break;
					}
				}
				Sleep(100);
			}
		}
		return nlohmann::json::parse(response.c_str(), response.c_str() + response.size());
	}

public:

	nlohmann::json Init()
	{
		// {"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":1}}
		if (Headless)
			return {};
		nlohmann::json j;
		j["jsonrpc"] = "2.0";
		j["id"] = next();
		j["method"] = "initialize";
		j["params"]["protocolVersion"] = 1;
		auto r = ret(j,true);
		return r;
	}

nlohmann::json AuthStatus()
	{
	nlohmann::json j;
	j["jsonrpc"] = "2.0";
	j["id"] = next();
	j["method"] = "auth.getStatus";
	auto r = ret(j, true);
	return r;	
	}

	nlohmann::json ModelList()
	{
		nlohmann::json j;
		j["jsonrpc"] = "2.0";
		j["id"] = next();
		j["method"] = "models.list";
		auto r = ret(j,true);
		return r;
	}

	nlohmann::json Ping()
	{
		nlohmann::json j;
		j["jsonrpc"] = "2.0";
		j["id"] = next();
		j["method"] = "ping";
		auto r = ret(j,true);
		return r;
	}

	nlohmann::json DestroySession(std::shared_ptr<COPILOT_SESSION> s)
	{
		if (!s)
			return {};
		nlohmann::json j;
		j["jsonrpc"] = "2.0";
		j["id"] = next();
		j["method"] = "session.destroy";
		j["params"]["sessionId"] = s->sessionId;
		auto r = ret(j,true);
		s->sessionId.clear();
		return r;
	}


	void Abort(std::shared_ptr<COPILOT_SESSION> s)
	{
		if (!s)
			return;
		nlohmann::json j;
		j["jsonrpc"] = "2.0";
		j["id"] = next();
		j["method"] = "session.abort";
		j["params"]["sessionId"] = s->sessionId;
		ret(j,false);
	}


	void ExecuteCallbacks(std::shared_ptr<COPILOT_SESSION> se,std::shared_ptr<PENDING_MESSAGE> s)
	{
		for (size_t im = 0; im < s->reasoning_messages.size(); im++)
		{
			if (!s->reasoning_callback)
				break;
			auto rm = s->reasoning_messages[im];
			if (rm)
			{
				if (rm->Ack == 1)
					continue;
				rm->Ack = 1;
				std::string r = rm->content;
				rm->content.clear();
				auto hr = s->reasoning_callback(r, s->ptr);
				if (FAILED(hr))
					Abort(se);
			}
		}
		for (size_t im = 0; im < s->output_messages.size(); im++)
		{
			if (!s->messaging_callback)
				break;
			auto rm = s->output_messages[im];
			if (rm)
			{
				if (rm->Ack == 1)
					continue;
				std::string m = rm->content;
				rm->Ack = 1;
				auto hr = s->messaging_callback(m, s->ptr);
				if (FAILED(hr))
					Abort(se);
			}
		}
	}

	int Wait(std::shared_ptr<COPILOT_SESSION> s,std::shared_ptr<PENDING_MESSAGE> msg, int WaitMs = 0)
	{
		if (!s)
			return -2;
		if (!msg)
			return -3;
		int waited = 0;
		for (;;)
		{
			if (1)
			{
				std::lock_guard<std::recursive_mutex> lock(response_mutex);
				if (msg->completed_message)
					return 0;
			}
			Sleep(100);
			waited += 100;
			if (WaitMs > 0 && waited >= WaitMs)
				return -1;

			ExecuteCallbacks(s, msg);
		}
	}

	std::shared_ptr<PENDING_MESSAGE> CreateMessage(std::shared_ptr<COPILOT_SESSION> s, const char* message, std::function<HRESULT(std::string& reasoning, long long ptr)> reasoning_callback = nullptr, std::function<HRESULT(std::string& msg, long long ptr)> messaging_callback = nullptr, long long ptr = 0)
	{
		auto pm = std::make_shared<PENDING_MESSAGE>();
		pm->m = message;
		pm->reasoning_callback = reasoning_callback;
		pm->messaging_callback = messaging_callback;
		pm->ptr = ptr;
		return pm;
	}
	
	void Send(std::shared_ptr<COPILOT_SESSION> s, std::shared_ptr<PENDING_MESSAGE> wh)
	{
		if (!s)
			return;
		if (s->sessionId.empty())
			return;
		// {"jsonrpc":"2.0","id":"11443832-6639-43f5-b202-8803c8156e46","method":"session.send","params":{"sessionId":"17a317f3-3faf-4401-b1dc-3d918c64a2f4","prompt":"What is 2+2?","attachments":null,"mode":null}}

		if (1)
		{
			bool HasPending = 0;

			if (1)
			{
				std::lock_guard<std::recursive_mutex> lock(response_mutex);
				if (s->pending_message)
					HasPending = 1;
			}
			if (HasPending)
			{
				// Not idle, queue the message
				if (1)
				{
					std::lock_guard<std::recursive_mutex> lock(response_mutex);
					s->pending_messages.push_back(wh);
				}
				return;
			}
			else
			{
				responses.clear();
			}
		}

		std::shared_ptr<PENDING_MESSAGE> pm = wh;
		nlohmann::json j;
		j["jsonrpc"] = "2.0";
		j["id"] = next();
		j["method"] = "session.send";
		j["params"]["sessionId"] = s->sessionId;
		j["params"]["prompt"] = pm->m;
		pm->id = j["id"];
		s->pending_message = pm;
		ret(j,false);
		return;
	}


	nlohmann::json Sessions(std::vector<std::shared_ptr<COPILOT_SESSION>>& sessions)
	{
		nlohmann::json j;
		j["jsonrpc"] = "2.0";
		j["id"] = next();
		j["method"] = "session/list";
		if (Headless)
			j["method"] = "session.list";
		j["params"] = nlohmann::json::object();
		auto r = ret(j,true);

		// it's in r["result"]["sessions"]
		for (auto& s : r["result"]["sessions"])
		{
			COPILOT_SESSION se;
			if (s.contains("sessionId"))
				se.sessionId = s["sessionId"].get<std::string>();
			if (s.contains("cwd"))
				se.cwd = s["cwd"].get<std::string>();
			if (s.contains("title"))
				se.title = s["title"].get<std::string>();
			if (s.contains("updatedAt"))
				se.updatedAt = s["updatedAt"].get<std::string>();
			auto se2 = std::make_shared<COPILOT_SESSION>(se);
			sessions.push_back(se2);
		}
		all_sessions = sessions;
		return r;
	}


	std::shared_ptr<COPILOT_SESSION> CreateSession(const char* model_id,bool Streaming)
	{
		// {"jsonrpc":"2.0","id":"62587fbb-6596-4144-8014-62403b7d6a97","method":"session.create","params":{"model":"gpt-5-mini","requestPermission":true,"envValueMode":"direct"}}
		nlohmann::json j;
		j["jsonrpc"] = "2.0";
		j["id"] = next();
		j["method"] = "session.create";
		auto params = nlohmann::json::object();
		params["model"] = model_id;
		params["requestPermission"] = true;
		params["envValueMode"] = "direct";
		params["streaming"] = Streaming;
		j["params"] = params;
		auto old_num_sessions = all_sessions.size();
		ret(j,false);
		for (int tries = 0 ; tries < 100 ; tries++)
		{
			Sleep(100);
			std::lock_guard<std::recursive_mutex> lock(response_mutex);
			if (old_num_sessions < all_sessions.size())
			{
				auto se = all_sessions.back();
				se->model = model_id;
				return se;
			}
		}
		return nullptr;
	}


#ifdef _WIN32
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

	HANDLE hProcess = 0;
	int debug = 0;
	COPILOT_RAW(const wchar_t* path_to_cli, int port,const char* auth_token,int Debug)
	{
		debug = Debug;
		std::vector<wchar_t> cmd(1000);
		GUID tok = {};
		CoCreateGuid(&tok);
		std::vector<wchar_t> toks(100);
		StringFromGUID2(tok, toks.data(), (int)toks.size());
		// remove first and last character from toks.data() which are { and }
		toks.erase(std::remove(toks.begin(), toks.end(), '{'), toks.end());
		toks.erase(std::remove(toks.begin(), toks.end(), '}'), toks.end());

		swprintf_s(cmd.data(), 1000, L"\"%s\" --no-auto-update --auth-token-env %s --acp --port %d --log-level info --headless", path_to_cli,toks.data(), port);
		if (debug == 2)
			swprintf_s(cmd.data(), 1000, L"cmd /k \"%s\" --no-auto-update --auth-token-env %s --acp --port %d --log-level info --headless", path_to_cli, toks.data(), port);
		PROCESS_INFORMATION pi = {};
		STARTUPINFO si = {};
		si.cb = sizeof(STARTUPINFO);


		// Create environment variable with the name of toks.data() and value of auth_token
		auto base = GetEnvironmentStringsW();

		std::wstring envBlock;
		for (auto p = base; *p; p += wcslen((wchar_t*)p) + 1)
		{
			envBlock.append((wchar_t*)p);
			envBlock.push_back('\0');
		}
		FreeEnvironmentStringsW((LPWCH)base);

		envBlock += toks.data();
		envBlock += L"=";
		envBlock += tou(auth_token);
		envBlock.push_back('\0');

		envBlock.push_back('\0');

		DWORD flg = CREATE_UNICODE_ENVIRONMENT;
		if (debug == 0)
			flg |= CREATE_NO_WINDOW;
		else
			flg |= CREATE_NEW_CONSOLE;
		CreateProcess(0, cmd.data(), nullptr, nullptr, FALSE, flg,envBlock.data(), nullptr, &si, &pi);
		hProcess = pi.hProcess;
		if (pi.hThread)
			CloseHandle(pi.hThread);
		Headless = true;
		host = "127.0.0.1";
		this->port = port;
		Start();
	}
#endif

	void Start()
	{
		x = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (x == 0 || x == -1)
		{
			return;
		}
		sockaddr_in sa = { 0 };
		sa.sin_family = AF_INET;
		sa.sin_port = htons(port);
#pragma warning(disable:4996)
		sa.sin_addr.s_addr = inet_addr(host.c_str());
#pragma warning(default:4996)

		int res = connect(x, (sockaddr*)&sa, sizeof(sa));
		if (res == -1)
		{
			closesocket(x);
			x = 0;
			return;
		}
		wait_thread = std::make_shared<std::thread>(&COPILOT_RAW::WaitThread, this);

	}

	COPILOT_RAW(const char* ip, int port,bool hl)
	{
		Headless = hl;
		host = ip;
		this->port = port;
		Start();
	}

	~COPILOT_RAW()
	{
		if (x)
		{
			closesocket(x);
			x = 0;
		}
		if (wait_thread && wait_thread->joinable())
			wait_thread->join();
		wait_thread = 0;
#ifdef _WIN32
		if (hProcess)
		{
			TerminateProcess(hProcess, 0);
			CloseHandle(hProcess);
			hProcess = 0;
		}
#endif
	}
};

//

struct COMPLETED_MESSAGE
{
	std::string content;
	std::string messageId;
	std::string reasoningText;
};

struct REASONING_MESSAGE
{
	std::string content;

};

struct SESSION
{
	std::string sessionId;
	std::string cwd;
	std::string title;
	std::string updatedAt;
	std::shared_ptr<COMPLETED_MESSAGE> completed_messsage;
	std::vector<std::shared_ptr<REASONING_MESSAGE>> reasoning_messages;
};

class COPILOT_RAW
{
	std::string host;
	int port = 0;
	SOCKET x = 0;
	int nid = 1;
	bool Headless = 1;

	int next()
	{
		return nid++;
	}
	std::vector<std::shared_ptr<SESSION>> all_sessions;


	std::string guidid()
	{
		// Generate a random GUID
		GUID guid;
		CoCreateGuid(&guid);
		char guid_str[64];
		snprintf(guid_str, sizeof(guid_str),
			"%08x-%04x-%04x-%04x-%012llx",
			guid.Data1, guid.Data2, guid.Data3,
			(guid.Data4[0] << 8) | guid.Data4[1],
			((unsigned long long)guid.Data4[2] << 40) |
			((unsigned long long)guid.Data4[3] << 32) |
			((unsigned long long)guid.Data4[4] << 24) |
			((unsigned long long)guid.Data4[5] << 16) |
			((unsigned long long)guid.Data4[6] << 8) |
			guid.Data4[7]);
		return std::string(guid_str);	
	}


	int ssend(const char* data, int len)
	{
		int total_sent = 0;
		while (total_sent < len)
		{
			int sent = send(x, data + total_sent, len - total_sent, 0);
			if (sent == SOCKET_ERROR)
				return SOCKET_ERROR;
			total_sent += sent;
		}
		return total_sent;
	}

	std::queue<std::string> responses;
	std::recursive_mutex response_mutex;
	HANDLE hResponseEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
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
							if (evtype == "assistant.reasoning")
							{
								bool F = 0;
								for (auto& s : all_sessions)
								{
									if (s->sessionId == sessionId)
									{
										auto m = std::make_shared<REASONING_MESSAGE>();
										m->content = data["content"].get<std::string>();
										s->reasoning_messages.push_back(m);
										break;
									}
								}
							}
							if (evtype == "assistant.message")
							{
								bool F = 0;
								for (auto& s : all_sessions)
								{
									if (s->sessionId == sessionId)
									{
										s->completed_messsage = std::make_shared<COMPLETED_MESSAGE>();
										s->completed_messsage->content = data["content"].get<std::string>();
										s->completed_messsage->messageId = data["messageId"].get<std::string>();
										s->completed_messsage->reasoningText = data["reasoningText"].get<std::string>();
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
									auto s = std::make_shared<SESSION>();
									s->sessionId = sessionId;
									s->cwd = cwd;
									all_sessions.push_back(s);
								}
							}
						}
					}
					catch (...)
					{
						// Not a valid JSON, ignore
						return;
					}
				}

				responses.push(response);
				SetEvent(hResponseEvent);
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
		WaitForSingleObject(hResponseEvent, INFINITE);
		std::string response;
		{
			std::lock_guard<std::recursive_mutex> lock(response_mutex);
			if (!responses.empty())
			{
				response = responses.front();
				responses.pop();
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

	nlohmann::json EraseSession(std::shared_ptr<SESSION> s)
	{
		if (!s)
			return {};
		nlohmann::json j;
		j["jsonrpc"] = "2.0";
		j["id"] = next();
		j["method"] = "session.destroy";
		j["params"]["sessionId"] = s->sessionId;
		auto r = ret(j,true);
		return r;
	}


	void Abort(std::shared_ptr<SESSION> s)
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

	void Send(std::shared_ptr<SESSION> s, const char* message,int WaitMs = 0,std::function<HRESULT(std::string& reasoning)> reasoning_callback = nullptr)
	{
		// {"jsonrpc":"2.0","id":"11443832-6639-43f5-b202-8803c8156e46","method":"session.send","params":{"sessionId":"17a317f3-3faf-4401-b1dc-3d918c64a2f4","prompt":"What is 2+2?","attachments":null,"mode":null}}
		
		s->completed_messsage = nullptr;
		nlohmann::json j;
		j["jsonrpc"] = "2.0";
		j["id"] = guidid();
		j["method"] = "session.send";
		j["params"]["sessionId"] = s->sessionId;
		j["params"]["prompt"] = message;
		ret(j, false);

		if (WaitMs > 0)
		{
			int waited = 0;
			while (waited < WaitMs)
			{
				Sleep(100);
				waited += 100;
				std::lock_guard<std::recursive_mutex> lock(response_mutex);
				if (s->completed_messsage)
					break;
				if (reasoning_callback && s->reasoning_messages.size())
				{
					auto rm = s->reasoning_messages.back();
					if (rm)
					{
						std::string r = rm->content;
						rm->content.clear();
						auto hr = reasoning_callback(r);
						s->reasoning_messages.pop_back();
						if (FAILED(hr))
							Abort(s);
					}
				}
			}
		}

	}


	nlohmann::json Sessions(std::vector<std::shared_ptr<SESSION>>& sessions)
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
			SESSION se;
			if (s.contains("sessionId"))
				se.sessionId = s["sessionId"].get<std::string>();
			if (s.contains("cwd"))
				se.cwd = s["cwd"].get<std::string>();
			if (s.contains("title"))
				se.title = s["title"].get<std::string>();
			if (s.contains("updatedAt"))
				se.updatedAt = s["updatedAt"].get<std::string>();
			auto se2 = std::make_shared<SESSION>(se);
			sessions.push_back(se2);
		}
		all_sessions = sessions;
		return r;
	}


	std::shared_ptr<SESSION> CreateSession(const char* model_id,bool Streaming)
	{
		// {"jsonrpc":"2.0","id":"62587fbb-6596-4144-8014-62403b7d6a97","method":"session.create","params":{"model":"gpt-5-mini","requestPermission":true,"envValueMode":"direct"}}
		nlohmann::json j;
		j["jsonrpc"] = "2.0";
		auto ng = guidid();
		j["id"] = ng;
		j["method"] = "session.create";
		auto params = nlohmann::json::object();
		params["model"] = model_id;
		params["requestPermission"] = true;
		params["envValueMode"] = "direct";
		params["streaming"] = Streaming;
		j["params"] = params;
		auto old_num_sessions = all_sessions.size();
		ret(j,false);
			// wait for a response that has the "id" of that
		for (int tries = 0 ; tries < 100 ; tries++)
		{
			Sleep(100);
			std::lock_guard<std::recursive_mutex> lock(response_mutex);
			if (old_num_sessions < all_sessions.size())
				return all_sessions.back();
		}
		return nullptr;
	}


	COPILOT_RAW(const char* ip, int port,bool hl)
	{
		Headless = hl;
		host = ip;
		this->port = port;
		x = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (x == INVALID_SOCKET)
		{
			printf("socket failed: %d\n", WSAGetLastError());
			return;
		}
		sockaddr_in sa = { 0 };
		sa.sin_family = AF_INET;
		sa.sin_port = htons(port);
#pragma warning(disable:4996)
		sa.sin_addr.s_addr = inet_addr(ip);
#pragma warning(default:4996)
			
		int res = connect(x, (sockaddr*)&sa, sizeof(sa));
		if (res == SOCKET_ERROR)
		{
			printf("connect failed: %d\n", WSAGetLastError());
			closesocket(x);
			x = 0;
			return;
		}
		printf("Connected to %s:%d\n", ip, port);
		wait_thread = std::make_shared<std::thread>(&COPILOT_RAW::WaitThread, this);
	}
};

//
#ifndef _WIN32
typedef int SOCKET;
#endif

#include <sstream>
#include <wininet.h>
#include "rest.h"

inline static HRESULT OllamaRunning2;

struct COMPLETED_MESSAGE
{
	std::string content;
	std::string messageId;
	std::string reasoningText;
};

struct COPILOT_MESSAGE
{
	std::string content;
	int Ack = 0;

};

struct COPILOT_RAW_SDK_MODEL
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

struct COPILOT_RAW_MODEL
{
	std::string id;
	std::string fullname;
	float rate = 0.0f;
	bool SupportsVision = 0;
	bool Ollama = 0;
};

struct COPILOT_RAW_STATUS
{
	bool StatusValid = false;
	bool Installed = false;
	std::wstring folder;
	bool OllamaInstalled = false;
	bool Connected = false;
	bool Authenticated = false;
	std::vector<COPILOT_RAW_MODEL> models;
};

struct PENDING_MESSAGE
{
	std::string m;
	std::vector<std::wstring> attachments;
	std::string id;
	std::function<HRESULT(std::string& reasoning, long long ptr)> reasoning_callback = nullptr;
	std::function<HRESULT(std::string& msg, long long ptr)> messaging_callback = nullptr;
	long long ptr = 0;
	std::shared_ptr<COMPLETED_MESSAGE> completed_message;
	std::vector<std::shared_ptr<COPILOT_MESSAGE>> reasoning_messages;
	std::vector<std::shared_ptr<COPILOT_MESSAGE>> output_messages;
};


struct COPILOT_SESSION
{
	SOCKET ollama = 0;
	bool OllamaStreaming = false;
	std::string ollama_context;
	std::shared_ptr<std::thread> OllamaThread;
	std::string sessionId;
	std::string cwd;
	std::string title;
	std::string model;
	std::shared_ptr<PENDING_MESSAGE> pending_message;
	std::vector<std::shared_ptr<PENDING_MESSAGE>> pending_messages;

	int nid = 1;

	std::vector<std::string> ExtractChunkedResponses(std::vector<char>& b)
	{
		// Might not have ContentLength, but instead be chunked with \r\n after each chunk, and end with \r\n0\r\n\r\n
		/*
		
		// This is like this

		HTTP/1.1 200 OK
Content-Type: application/json; charset=utf-8
Date: Sun, 01 Mar 2026 08:32:15 GMT
Transfer-Encoding: chunked
\r\n
876
{"model":"phi:latest","created_at":"2026-03-01T08:32:15.1913205Z","response":" Here are the numbers from 1 to 100:\n\n```\n1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100\n```\n\n","done":true,"done_reason":"stop","context":[11964,25,317,8537,1022,257,11040,2836,290,281,11666,4430,8796,13,383,8796,3607,7613,7429,284,262,2836,6,82,2683,13,198,12982,25,4222,1560,502,477,3146,422,220,16,284,220,3064,198,48902,25,3423,389,262,3146,422,220,16,284,220,3064,25,198,198,15506,63,198,16,11,220,17,11,220,18,11,220,19,11,220,20,11,220,21,11,220,22,11,220,23,11,220,24,11,220,940,11,220,1157,11,220,1065,11,220,1485,11,220,1415,11,220,1314,11,220,1433,11,220,1558,11,220,1507,11,220,1129,11,220,1238,11,220,2481,11,220,1828,11,220,1954,11,220,1731,11,220,1495,11,220,2075,11,220,1983,11,220,2078,11,220,1959,11,220,1270,11,220,3132,11,220,2624,11,220,2091,11,220,2682,11,220,2327,11,220,2623,11,220,2718,11,220,2548,11,220,2670,11,220,1821,11,220,3901,11,220,3682,11,220,3559,11,220,2598,11,220,2231,11,220,3510,11,220,2857,11,220,2780,11,220,2920,11,220,1120,11,220,4349,11,220,4309,11,220,4310,11,220,4051,11,220,2816,11,220,3980,11,220,3553,11,220,3365,11,220,3270,11,220,1899,11,220,5333,11,220,5237,11,220,5066,11,220,2414,11,220,2996,11,220,2791,11,220,3134,11,220,3104,11,220,3388,11,220,2154,11,220,4869,11,220,4761,11,220,4790,11,220,4524,11,220,2425,11,220,4304,11,220,3324,11,220,3695,11,220,3720,11,220,1795,11,220,6659,11,220,6469,11,220,5999,11,220,5705,11,220,5332,11,220,4521,11,220,5774,11,220,3459,11,220,4531,11,220,3829,11,220,6420,11,220,5892,11,220,6052,11,220,5824,11,220,3865,11,220,4846,11,220,5607,11,220,4089,11,220,2079,11,220,3064,198,15506,63,628],"total_duration":2334862500,"load_duration":55095900,"prompt_eval_count":43,"prompt_eval_duration":15410800,"eval_count":218,"eval_duration":2145463700}
size
{...}
0

		*/
		b.push_back(0); // Null terminate for easier searching
		std::vector<std::string> rs;
		for (int trr = 0 ; ; trr++)
		{
			auto c1 = strstr(b.data(), "\r\n\r\n");
			if (trr > 0)
			{
				c1 = b.data();
				if (c1 == nullptr)
					break;
			}
			else
			{
				if (c1 == nullptr)
					break;
				c1 += 4;
			}
			std::string len_str(c1);
			int chunk_size = std::stoi(len_str, nullptr, 16);
			if (chunk_size == 0)
			{
				// End of chunks
				b.clear();
				break;
			}
			// go to next line
			auto c2 = strstr(c1, "{");
			if (c2 == nullptr)
				break;
			std::string chunk(c2, c2 + chunk_size);
			rs.push_back(chunk);

			std::vector<char> newb;
			auto chunk_end = c2 + chunk_size + 2; // skip \r\n after chunk
			if (*chunk_end == 0)
				chunk_end++;
			while (*chunk_end)
				{
				newb.push_back(*chunk_end);
				chunk_end++;
			}
			b = std::move(newb);
			if (strlen(b.data()) < 4)
			{
				b.clear();
				break;
			}
		}

		return rs;
	}


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



	void OllThread()
	{

		std::vector<char> pending;

		for (;;)
		{
			std::vector<char> buf(4096);
			int received = recv(ollama, buf.data(), (int)buf.size(), 0);
			if (received == SOCKET_ERROR || received == 0)
			{
				// Connection closed or error
				return;
			}
			buf.resize(received);
			pending.insert(pending.end(), buf.begin(), buf.end());

			auto responses2 = ExtractResponsesFromBuffer(pending);
			if (responses2.empty())
				responses2 = ExtractChunkedResponses(pending);
			for (auto& r : responses2)
			{
				try
				{
					nlohmann::json j = nlohmann::json::parse(r);
					if (j.contains("response"))
					{
						std::string content = j["response"].get<std::string>();
						if (pending_message)
						{
							pending_message->output_messages.push_back(std::make_shared<COPILOT_MESSAGE>(COPILOT_MESSAGE{ content }));
						}
					}

					// and context
					if (j.contains("context"))
					{
						ollama_context = j["context"].dump();
					}

				}
				catch (...)
				{
				}
			}
			if (pending_message)
			{
				auto m2 = std::make_shared<COMPLETED_MESSAGE>();
				for (auto& m : pending_message->output_messages)
				{
					m2->content += m->content;
				}
				pending_message->completed_message = m2;
			}
		}
	}

	int ssend(const char* data, int len)
	{
		int total_sent = 0;
		while (total_sent < len)
		{
			int sent = send(ollama, data + total_sent, len - total_sent, 0);
			if (sent == 0 || sent == -1)
				return -1;
			total_sent += sent;
		}
		return total_sent;
	}


	void OllamaSend(std::shared_ptr<PENDING_MESSAGE> wh)
	{
		if (!ollama || !wh)
			return;

		/*
		* POST /api/generate HTTP/1.1
Host: localhost:11434
Content-Type: application/json
Content-Length: ...
Connection: keep-alive

{
 "model":"llama3",
 "prompt":"Hello",
 "stream":true
}
		*/

		std::string build;
		build += "POST /api/generate HTTP/1.1\r\n";
		build += "Host: localhost:11434\r\n";
		build += "Content-Type: application/json\r\n";
		std::string body;
		body += "{";
		body += "\"model\":\"";
		body += model;
		body += "\",";
		body += "\"prompt\":\"";
		body += wh->m;
		body += "\",";
		std::string id = std::to_string(nid++);
		body += "\"id\":\"";
		body += id;
		body += "\",";

		if (ollama_context.size())	
		{
			body += "\"context\":";
			body += ollama_context;
			body += ",";
		}

		//if (OllamaStreaming)
	//		body += "\"stream\":true";
	//	else
			body += "\"stream\":false";
		body += "}";
		build += "Content-Length: " + std::to_string(body.size()) + "\r\n";
		build += "Connection: keep-alive\r\n\r\n";
		build += body;

		pending_message = wh;
		pending_message->id = id;
		ssend(build.c_str(), (int)build.size());
		for (int tries = 0; tries < 100; tries++)
		{
			if (pending_message->completed_message)
				break;
			Sleep(100);
		}
	}

	void OllamaOff()
	{
		if (ollama)
			closesocket(ollama);
		ollama = 0;
		if (OllamaThread && OllamaThread->joinable())
		{
			OllamaThread->join();
		}
		OllamaThread = nullptr;
	}
	virtual ~COPILOT_SESSION()
	{
		OllamaOff();
	}
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
										auto m = std::make_shared<COPILOT_MESSAGE>();
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
										auto m = std::make_shared<COPILOT_MESSAGE>();
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
		if (s->ollama)
		{
			s->OllamaOff();
			return {};
		}
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

	std::shared_ptr<PENDING_MESSAGE> CreateMessage(std::shared_ptr<COPILOT_SESSION> s, const char* message, std::function<HRESULT(std::string& reasoning, long long ptr)> reasoning_callback = nullptr, std::function<HRESULT(std::string& msg, long long ptr)> messaging_callback = nullptr, long long ptr = 0,std::vector<std::wstring>* atts = 0)
	{
		auto pm = std::make_shared<PENDING_MESSAGE>();
		pm->m = message;
		if (atts)
			pm->attachments = *atts;
		pm->reasoning_callback = reasoning_callback;
		pm->messaging_callback = messaging_callback;
		pm->ptr = ptr;
		return pm;
	}

	void SendOllama(std::shared_ptr<COPILOT_SESSION> s, std::shared_ptr<PENDING_MESSAGE> wh)
	{
		if (!s)
			return;
		if (!s->ollama)
			return;

		s->OllamaSend(wh);

	}

	void Send(std::shared_ptr<COPILOT_SESSION> s, std::shared_ptr<PENDING_MESSAGE> wh)
	{
		if (!s)
			return;
		if (s->ollama)
		{
			SendOllama(s, wh);
			return;
		}
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
		if (pm->attachments.size())
		{
			j["params"]["attachments"] = nlohmann::json::array();
			for (auto& a : pm->attachments)
			{
				nlohmann::json o;
				o["type"] = "file";
				o["path"] = toc(ChangeSlash(a.c_str()).c_str());
				j["params"]["attachments"].push_back(o);
			}
		}
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
			auto se2 = std::make_shared<COPILOT_SESSION>(se);
			sessions.push_back(se2);
		}
		all_sessions = sessions;
		return r;
	}

	static std::vector<COPILOT_RAW_SDK_MODEL> ModelsFromJ(std::string s)
	{
		try
		{
			auto jx = nlohmann::json::parse(s);
			std::vector<COPILOT_RAW_SDK_MODEL> models;
			for (auto& item : jx)
			{
				COPILOT_RAW_SDK_MODEL m;
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


	static void TryOllamaRunning()
	{
		if (OllamaRunning2 != S_FALSE)
			return;
		SOCKET x = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (x == INVALID_SOCKET)
		{
			OllamaRunning2 = E_FAIL;
			return;
		}
		sockaddr_in sa = { 0 };
		sa.sin_family = AF_INET;
		sa.sin_port = htons(11434);
#pragma warning(disable:4996)
		sa.sin_addr.s_addr = inet_addr("127.0.0.1");
#pragma warning(default:4996)
		int res = connect(x, (sockaddr*)&sa, sizeof(sa));
		closesocket(x);
		if (res == SOCKET_ERROR)
			OllamaRunning2 = E_FAIL;
		else
			OllamaRunning2 = S_OK;
	}

	static void DetectOllamaRunning()
	{
		if (OllamaRunning2 == S_FALSE)
		{
			std::thread tx([&]()
				{
					TryOllamaRunning();
				});
			tx.detach();
		}

	}


	static std::vector<COPILOT_RAW_MODEL> ollama_list()
	{
		//http://localhost:11434/api/tags
		RESTAPI::REST r;
		r.Connect(L"localhost", false, 11434);
		RESTAPI::memory_data_provider d(0, 0);
		auto ji = r.Request2(L"/api/tags", d, true, L"GET");
		std::vector<char> dd;
		r.ReadToMemory(ji, dd);
		if (dd.size() == 0)
			return {};
		std::string s(dd.data(), dd.size());
		auto jx = nlohmann::json::parse(s);
		std::vector<COPILOT_RAW_MODEL> models;
		for (auto& item : jx["models"])
		{
			std::string name = item["model"];
			COPILOT_RAW_MODEL m;
			m.id = name;
			m.fullname = name;
			m.rate = 0.0f;
			m.Ollama = 1;
			models.push_back(m);
		}
		return models;
	}

	std::vector<COPILOT_RAW_MODEL> CopilotModels()
	{
		auto mod = ModelList();
		auto sdk_models = ModelsFromJ(mod["result"]["models"].dump());
		std::vector<COPILOT_RAW_MODEL> models;
		for (const auto& sdk_model : sdk_models)
		{
			COPILOT_RAW_MODEL m;
			m.fullname = sdk_model.name;
			m.id = sdk_model.id;
			m.rate = sdk_model.BillingMultiplier;
			m.SupportsVision = sdk_model.SupportsVision;
			m.Ollama = 0;
			models.push_back(m);
		}
		return models;
	}

	COPILOT_RAW_STATUS Status()
	{
		COPILOT_RAW_STATUS s;
		s.Installed = 1;
		s.folder = cli_path;
		s.Connected = 1;
		auto auth = AuthStatus();
		// this is json
		auto du = auth.dump();
		if (auth.contains("result"))
		{
			if (auth["result"].contains("isAuthenticated"))
			{
				bool isAuthenticated = auth["result"]["isAuthenticated"].get<bool>();
				s.Authenticated = isAuthenticated;
			}
		}

		s.models = CopilotModels();
		if (OllamaRunning2 == S_OK)
		{
			auto ollama_models = ollama_list();
			s.models.insert(s.models.end(), ollama_models.begin(), ollama_models.end());
		}
		return s;
	}


	std::shared_ptr<COPILOT_SESSION> CreateOllamaSession(const char* model_id, bool Streaming)
	{
		if (OllamaRunning2 != S_OK)
			return nullptr;
		auto ollama_models = ollama_list();
		bool F = 0;
		for (auto& m : ollama_models)
		{
			if (m.id == model_id)
			{
				F = 1;
				break;
			}
		}
		if (!F)
			return nullptr;

		auto se = std::make_shared<COPILOT_SESSION>();
		se->model = model_id;
		se->ollama = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (se->ollama == INVALID_SOCKET)
			return nullptr;

		sockaddr_in sa = { 0 };
		sa.sin_family = AF_INET;
		sa.sin_port = htons(11434);
#pragma warning(disable:4996)
		sa.sin_addr.s_addr = inet_addr("127.0.0.1");
#pragma warning(default:4996)
		int res = connect(se->ollama, (sockaddr*)&sa, sizeof(sa));
		if (res == SOCKET_ERROR)
		{
			closesocket(se->ollama);
			return nullptr;
		}
		se->OllamaStreaming = Streaming;
		se->OllamaThread = std::make_shared<std::thread>(&COPILOT_SESSION::OllThread, se.get());
		return se;
	}

	std::shared_ptr<COPILOT_SESSION> CreateSession(const char* model_id,bool Streaming)
	{
		auto models = CopilotModels();
		bool F = 0;
		for (auto& m : models)
		{
			if (m.id == model_id)
			{
				F = 1;
				break;
			}
		}
		if (!F)
			return CreateOllamaSession(model_id, Streaming);

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


	HANDLE hProcess = 0;
	int debug = 0;
	std::wstring cli_path;
	COPILOT_RAW(const wchar_t* path_to_cli, int port,const char* auth_token,int Debug)
	{
		debug = Debug;
		DetectOllamaRunning();
		cli_path = path_to_cli;
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
			swprintf_s(cmd.data(), 1000, L"cmd.exe /L \"%s\" --no-auto-update --auth-token-env %s --acp --port %d --log-level info --headless", path_to_cli, toks.data(), port);
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
		DetectOllamaRunning();
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

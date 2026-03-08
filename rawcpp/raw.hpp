//
#ifndef _WIN32
typedef int SOCKET;
#endif

#include <sstream>
#include <any>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <iomanip>
#include <condition_variable>
#include <functional>
#ifdef _WIN32
#include <wininet.h>
#include <wincred.h>
#include "rest.h"
#else
// linux
#include "json.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef int SOCKET;
typedef int HRESULT;
void closesocket(int s)
{
	close(s);
}
#define S_OK 0
#define E_ABORT -1
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define E_FAIL -2
#define S_FALSE 1
#define Sleep(x) usleep((x)*1000)
#define OutputDebugStringA(x) do { std::cout << x; } while(0)
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#define FAILED(hr) (((HRESULT)(hr)) < 0)
#endif


inline static HRESULT OllamaRunning2;

struct COMPLETED_MESSAGE
{
	std::string content;
	std::string messageId;
	std::string reasoningText;
};

struct COPILOT_RAW_QUOTA
{
	int entitlementRequests = 0;
	int usedRequests = 0;
	float remainingPercentage = 0.0f;
	int overage = 0;
	bool overageAllowedWithExhaustedQuota = 0;
	std::string resetDate;
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
#ifdef _WIN32
	std::wstring folder;
#else
	std::string folder;
#endif
	bool OllamaInstalled = false;
	bool Connected = false;
	bool Authenticated = false;
	std::map<std::string,COPILOT_RAW_QUOTA> quota;
	std::vector<COPILOT_RAW_MODEL> models;
};

struct PENDING_MESSAGE
{
	bool Sent = 0;
	std::string m;
#ifdef _WIN32
	std::vector<std::wstring> attachments;
#else
	std::vector<std::string> attachments;
#endif
	std::string id;
	std::function<HRESULT(std::string& reasoning, long long ptr)> reasoning_callback = nullptr;
	std::function<HRESULT(std::string& msg, long long ptr)> messaging_callback = nullptr;
	long long ptr = 0;
	std::shared_ptr<COMPLETED_MESSAGE> completed_message;
	std::vector<std::shared_ptr<COPILOT_MESSAGE>> reasoning_messages;
	std::vector<std::shared_ptr<COPILOT_MESSAGE>> output_messages;
};


struct COPILOT_TOOL_PARAMETER
{
	std::string name;
	std::string title;
	std::string description;
	std::string typ;
	bool Required = false;
};
struct COPILOT_TOOL
{
	std::string name;
	std::string description;
	std::string title;
	std::vector<COPILOT_TOOL_PARAMETER> parameters;
	std::function<std::string(
		std::string session_id,
		std::string tool_id,
		std::vector<std::tuple<std::string, std::any>>& parameters
		)> foo;
};

struct COPILOT_SESSION_PARAMETERS
{
	bool Streaming = true;
	std::string reasoning_effort;
	std::string system_message;
#ifdef _WIN32
	std::vector<std::wstring> skill_dirs;
	std::vector<std::wstring> disabled_skills;
#else
	std::vector<std::string> skill_dirs;
	std::vector<std::string> disabled_skills;
#endif
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
	std::any user_data;



	std::vector<std::string> ExtractChunkedResponses(std::vector<char>& b,bool& F)
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
				F = 1;
				break;
			}

			// Do we have enough data for the chunk?
			if (strlen(c1) < chunk_size + 2) // +2 for \r\n after chunk
				break;

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
			if (newb.empty())
				break;
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

			bool F = 0;
			auto responses2 = ExtractResponsesFromBuffer(pending);
			if (responses2.empty())
				responses2 = ExtractChunkedResponses(pending, F);
			else
				F = 1;
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
			if (pending_message && F)
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


	static std::string encode_for_json(const std::string& s)
	{
		std::ostringstream o;

		for (unsigned char c : s)
		{
			switch (c)
			{
			case '"':  o << "\\\""; break;
			case '\\': o << "\\\\"; break;
			case '\b': o << "\\b";  break;
			case '\f': o << "\\f";  break;
			case '\n': o << "\\n";  break;
			case '\r': o << "\\r";  break;
			case '\t': o << "\\t";  break;

			default:
				if (c < 0x20)
				{
					o << "\\u"
						<< std::hex << std::uppercase
						<< std::setw(4) << std::setfill('0')
						<< (int)c;
				}
				else
				{
					o << c;
				}
			}
		}

		return o.str();
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
		auto m = encode_for_json(wh->m);
		body += m;
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


enum struct COPILOT_RAW_MODE
{
	INTERACTIVE = 0,
	PLAN = 1,
	AUTOPILOT = 2,
};

class COPILOT_RAW
{
	std::string host;
	int port = 0;
	SOCKET x = 0;
	int nid = 1;

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

						if (r.contains("method") && r["method"] == "tool.call")
						{
							auto& params = r["params"];
							auto sessionId = params["sessionId"].get<std::string>();
							auto toolCallId = params["toolCallId"].get<std::string>();
							auto toolName = params["toolName"].get<std::string>();
							std::vector<std::tuple<std::string, std::any>> parameters;
							for (auto& a : params["arguments"].items())
							{
								auto paramName = a.key();
								std::any paramvalue;
								// if string or number or bool, store as string, else store as json
								if (a.value().is_string())
									paramvalue = a.value().get<std::string>();
								else if (a.value().is_number())
									paramvalue = std::to_string(a.value().get<double>());
								else if (a.value().is_boolean())
									paramvalue = a.value().get<bool>();
								else
									paramvalue = a.value().dump();
								parameters.push_back({ paramName, paramvalue });
							}

							for (auto& t : tools)
							{
								if (t->name == toolName)
								{
									if (t->foo)
									{
										auto str = t->foo(sessionId, toolCallId, parameters);
										// Send immediately {"jsonrpc":"2.0","id":1,"result":{"result":{"textResultForLlm":"...","resultType":"success"}}}
										nlohmann::json j;
										j["jsonrpc"] = "2.0";
										j["id"] = r["id"];
										// encode str for json
										//str = COPILOT_SESSION::encode_for_json(str);
										j["result"]["result"]["textResultForLlm"] = str;
										j["result"]["result"]["resultType"] = "success";
										ret(j, false);
									}
									break;
								}
							}
						}

						if (r.contains("method") && r["method"] == "permission.request")
						{
							// send immediately {"jsonrpc":"2.0","id":0,"result":{"result":{"kind":"approved"}}}
							nlohmann::json j;
							j["jsonrpc"] = "2.0";
							j["id"] = r["id"];
							j["result"]["result"]["kind"] = "approved";
							ret(j, false);
							continue;
						}

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
							if (evtype == "assistant.message" && data.contains("content") && data["content"].get<std::string>().length() > 0)
							{
								bool F = 0;
								for (auto& s : all_sessions)
								{
									if (s->sessionId == sessionId)
									{
										if (s->pending_message)
										{
											ExecuteCallbacks(s,s->pending_message);
											if (!s->pending_message->completed_message)
												s->pending_message->completed_message = std::make_shared<COMPLETED_MESSAGE>();
											s->pending_message->completed_message->content = data["content"].get<std::string>();
											s->pending_message->completed_message->messageId = data["messageId"].get<std::string>();
											if (data.contains("reasoningText"))
												s->pending_message->completed_message->reasoningText = data["reasoningText"].get<std::string>();
											s->pending_message = 0;
										}
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
										// Send the next one
										std::lock_guard<std::recursive_mutex> lock(response_mutex);
										if (s->pending_message)
											s->pending_message = nullptr;
										if (s->pending_messages.size())
										{
											auto pm = s->pending_messages[0];
											s->pending_messages.erase(s->pending_messages.begin());
											Send(s, pm);
										}
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
				ready = true;
				cv.notify_one();
			}
		}
		PendingComplete();
		x = 0;
	}

	void PendingComplete(std::shared_ptr<COPILOT_SESSION> s = nullptr)
	{
		// All pending messages should be marked as completed with empty content
		if (!s)
		{
			for (auto& j : all_sessions)
			{
				PendingComplete(j);
			}
			return;
		}
		if (s)
		{
			if (s->pending_messages.size())
			{
				std::lock_guard<std::recursive_mutex> lock(response_mutex);
				for (auto& m : s->pending_messages)
				{
					if (!m->completed_message)
						m->completed_message = std::make_shared<COMPLETED_MESSAGE>();
					m->completed_message->content.clear();
				}
			}
		}

	}
public:

	std::string next()
	{
		return std::to_string(nid++);
	}

	nlohmann::json ret(nlohmann::json& j,bool w)
	{
		auto send = j.dump();
#ifdef _DEBUG
		OutputDebugStringA(send.c_str());
		OutputDebugStringA("\n");
#endif

		if (1)
		{
			// Must also send Content-Length: XXX \r\ndata
			std::string header = "Content-Length: " + std::to_string(send.length()) + "\r\n\r\n";
			send = header + send;
		}

		ssend(send.c_str(), (int)send.length());
		if (!w)
			return {};
		if (!x)
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
	std::vector<std::shared_ptr<COPILOT_TOOL>> tools;

public:


#pragma comment(lib,"Comctl32.lib")
	void AddTool(std::string name, std::string desc, std::string title,std::vector<COPILOT_TOOL_PARAMETER> params,std::function<std::string(std::string session_id,
		std::string tool_id,
		std::vector<std::tuple<std::string, std::any>>& parameters
		)> foo)
	{
		std::shared_ptr<COPILOT_TOOL> t = std::make_shared<COPILOT_TOOL>();
		t->name = name;
		t->description = desc;
		t->title = title;
		t->parameters = params;
		t->foo = foo;
		tools.push_back(t);
	}

#ifdef _WIN32
	void ShowStatus(HWND hParent, COPILOT_RAW_STATUS* u = 0)
	{
		auto st = u ? *u : Status();
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
					s += L"\r\n\r\n";
					if (!st.models.empty())
						s += L"Models\r\n----------------------------------------------------\r\n";
					for (auto& l : st.models)
					{
						wchar_t buf[200] = {};
						swprintf_s(buf, 100, L"%6.2f - %S\r\n", l.rate, l.fullname.c_str());
						s += buf;
					}
					// Quota
					if (st.quota.size() && st.quota.find("premium_interactions") != st.quota.end())
					{
						s += L"\r\n\r\nAccount Quota\r\n----------------------------------------------------\r\n";
						auto& q = st.quota["premium_interactions"];
						wchar_t buf[200] = {};
						swprintf_s(buf, 100, L"%6.2f - Premium Percentage Usaged (%d/%d)\r\n", 100.0f - q.remainingPercentage,q.usedRequests,q.entitlementRequests);
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
		tdc.dwFlags = TDF_ENABLE_HYPERLINKS;
		tdc.pszWindowTitle = L"Copilot Status";
		tdc.pszMainInstruction = L"Copilot Status";
		auto status = getst();
		tdc.pszContent = status.c_str();
		std::wstring footer;
		if (st.Installed)
		{
			footer += L"Copilot <a href=\"https://github.com/settings/copilot/features\">account</a>";
			footer += L", <a href=\"https://github.com/settings/models\">models";
			footer += L"</a>, <a href=\"https://github.com/settings/connections/applications";
			footer += L"\">authorized applications</a>";
			footer += L".\r\n";
			footer += L"<a href=\"https://www.turbo-play.com/copilot.php\">Learn more</a>. ";
			footer += L"<a href=\"https://www.turbo-play.com/aireport.php\">Report AI Abuse</a>. ";
			tdc.pszFooter = footer.c_str();
		}
		else
			tdc.pszFooter = L"View your Copilot <a href=\"https://github.com/settings/copilot/features\">account</a> and <a href=\"https://github.com/settings/models\">models</a>.\r\n<a href=\"https://www.turbo-play.com/copilot.php\">Learn more</a>.";
		TASKDIALOG_BUTTON buttons[10] = {};
		if (1)
		{
			int nid = 0;
			if (!st.Authenticated && client_id.length() && client_secret.length())
			{
				buttons[nid].pszButtonText = L"Authenticate";
				buttons[nid].nButtonID = 102;
				nid++;
			}
			buttons[nid].pszButtonText = L"Close";
			buttons[nid].nButtonID = IDCANCEL;
			tdc.pButtons = buttons;
			tdc.cButtons = nid + 1;
		}
		tdc.lpCallbackData = (LONG_PTR)this;

		tdc.pfCallback = [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LONG_PTR xx )->HRESULT
			{
				auto p = (COPILOT_RAW*)xx;
				if (msg == TDN_CREATED)
				{
					return S_FALSE;
				}
				if (msg == TDN_HYPERLINK_CLICKED)
				{
					std::wstring link = (LPCWSTR)lParam;
					ShellExecute(0, L"open", (const wchar_t*)lParam, 0, 0, SW_SHOW);
					return S_FALSE;
				}
				if (msg == TDN_BUTTON_CLICKED)
				{
					int id = (int)wParam;
					if (id == 102)
					{
						// Authenticate
						if (p->client_id.length() && p->client_secret.length())
						{
							auto token = GetAccessToken(hwnd, p->client_id.c_str(), p->client_secret.c_str());
							void CopReturnedToken(std::string);
							CopReturnedToken(token);
							return S_OK;
						}
					}
					if (id == IDCANCEL)
						return S_OK;
					return S_FALSE;
				}
				return S_OK;
			};
		TaskDialogIndirect(&tdc, 0, 0, 0);
	}
#endif

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

	nlohmann::json Ping(int* pVersion = 0)
	{
		nlohmann::json j;
		j["jsonrpc"] = "2.0";
		j["id"] = next();
		j["method"] = "ping";
		auto r = ret(j,true);
		if (pVersion && r.contains("result") && r["result"].contains("protocolVersion"))
		{
			*pVersion = r["result"]["protocolVersion"].get<int>();
		}
		return r;
	}

	nlohmann::json Compact(std::shared_ptr<COPILOT_SESSION> s)
	{
		if (!s)
			return {};
		nlohmann::json j;
		j["jsonrpc"] = "2.0";
		j["id"] = next();
		j["method"] = "session.compaction.compact";
		j["params"]["sessionId"] = s->sessionId;
		return ret(j,true);
	}


	nlohmann::json SetMode(std::shared_ptr<COPILOT_SESSION> s,COPILOT_RAW_MODE m)
	{
		nlohmann::json j;
		j["jsonrpc"] = "2.0";
		j["id"] = next();
		j["method"] = "session.mode.set";
		j["params"]["sessionId"] = s->sessionId;
		if (m == COPILOT_RAW_MODE::INTERACTIVE)
			j["params"]["mode"] = "interactive";
		else if (m == COPILOT_RAW_MODE::PLAN)
			j["params"]["mode"] = "plan";
		else if (m == COPILOT_RAW_MODE::AUTOPILOT)
			j["params"]["mode"] = "autopilot";	
		auto r = ret(j,true);
		return r;
	}

	nlohmann::json Quota()
	{
		nlohmann::json j;
		j["jsonrpc"] = "2.0";
		j["id"] = next();
		j["method"] = "account.getQuota";
		auto r = ret(j, true);
		return r;
	}

	nlohmann::json DeleteSession(std::shared_ptr<COPILOT_SESSION> s)
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
		j["method"] = "session.delete";
		j["params"]["sessionId"] = s->sessionId;
		auto r = ret(j, true);
		s->sessionId.clear();
		return r;
	}

	nlohmann::json SwitchModel(std::shared_ptr<COPILOT_SESSION> s, const char* modelId)
	{
		if (!s)
			return {};
		if (s->ollama)
			return {};
		nlohmann::json j;
		j["jsonrpc"] = "2.0";
		j["id"] = next();
		j["method"] = "session.model.switchTo";
		j["params"]["sessionId"] = s->sessionId;
		j["params"]["modelId"] = modelId;
		auto r = ret(j, true);
		return r;
	}

	nlohmann::json ResumeSession(std::shared_ptr<COPILOT_SESSION> s)
	{
		if (!s)
			return {};
		if (s->ollama)
			return {};
		nlohmann::json j;
		j["jsonrpc"] = "2.0";
		j["id"] = next();
		j["method"] = "session.resume";
		j["params"]["sessionId"] = s->sessionId;
		auto r = ret(j, true);
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
		PendingComplete(s);
	}

#ifdef _WIN32
	static void ToClip(HWND hhx, const wchar_t* t, bool Empty)
	{
		if (OpenClipboard(hhx))
		{
			if (Empty)
				EmptyClipboard();
			HGLOBAL hGG = GlobalAlloc(GMEM_FIXED, wcslen(t) * 2 + 100);
			void* pp = GlobalLock(hGG);
			wcscpy_s((wchar_t*)pp, wcslen(t) + 10, t);
			SetClipboardData(CF_UNICODETEXT, hGG);
			GlobalUnlock(hGG);
			CloseClipboard();
		}
	}

	static bool SaveToken(const wchar_t* token, wchar_t* Target, wchar_t* Username)
	{
		CREDENTIAL cred = { 0 };
		cred.Type = CRED_TYPE_GENERIC;
		cred.TargetName = Target;
		cred.CredentialBlobSize = (DWORD)(wcslen(token) * sizeof(wchar_t));
		cred.CredentialBlob = (LPBYTE)token;
		cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
		cred.UserName = Username;

		return CredWrite(&cred, 0);
	}


	static std::wstring LoadToken(const wchar_t* Target)
	{
		PCREDENTIAL cred;
		if (CredRead(Target, CRED_TYPE_GENERIC, 0, &cred))
		{
			auto sz = cred->CredentialBlobSize / sizeof(wchar_t);
			std::wstring r = std::wstring((wchar_t*)cred->CredentialBlob, sz);
			CredFree(cred);
			return r;
		}
		return L"";
	}

	static bool DeleteToken(const wchar_t* Target)
	{
		if (CredDeleteW(Target, CRED_TYPE_GENERIC, 0))
			return true;
		return false;
	}

	static std::string GetAccessToken(HWND hParent, const char* client_id, const char* client_secret)
	{
		// 1. Request verification code POST https://github.com/login/device/code
		RESTAPI::REST r;
		std::wstring acc = L"Accept: application/json";
		r.Connect(L"github.com", true);
		std::string d2;
		d2 += "client_id=";
		d2 += client_id;
		d2 += "&";
		//	d2 += "scope=";
		//	d2 += ""

		auto r2 = r.RequestWithBuffer(L"/login/device/code", L"POST", { acc }, d2.data(), d2.length());
		std::vector<char> d;
		r.ReadToMemory(r2, d);
		d.resize(d.size() + 1);
		try
		{
			nlohmann::json x = nlohmann::json::parse(d.data());

			struct R
			{
				const char* client_id;
				const char* client_secret;
				std::string device_code;
				std::string user_code;
				nlohmann::json* x;
				std::string token;
				bool Cancelled = false;
				std::shared_ptr<std::thread> j;
			};
			R r;
			r.client_id = client_id;
			r.client_secret = client_secret;
			r.x = &x;

			auto verification_uri = x["verification_uri"].get<std::string>();
			auto device_code = x["device_code"].get<std::string>();
			auto user_code = x["user_code"].get<std::string>();
			r.device_code = device_code;
			r.user_code = user_code;


			TASKDIALOGCONFIG tdc = {};
			tdc.cbSize = sizeof(tdc);
			tdc.hwndParent = hParent;
			tdc.dwFlags = TDF_ENABLE_HYPERLINKS | TDF_CALLBACK_TIMER;
			tdc.pszWindowTitle = L"Copilot Authentication";
			tdc.pszMainInstruction = L"GitHub Authentication Required";
			std::wstring str = L"Please authenticate to github by going to\r\n\r\n<a href=\"";
			str += tou(verification_uri.c_str()) + L"\">";
			str += tou(verification_uri.c_str());
			str += L"</a>\r\n\r\nand use the following code:\r\n\r\n";
			str += tou(user_code.c_str());
			tdc.pszContent = str.c_str();
			TASKDIALOG_BUTTON buttons[10] = {};
			buttons[0].nButtonID = 100;
			buttons[0].pszButtonText = L"Copy code to clipboard";
			buttons[1].nButtonID = 101;
			buttons[1].pszButtonText = L"Go to authentication URL";
			buttons[2].nButtonID = IDCANCEL;
			buttons[2].pszButtonText = L"Cancel";
			tdc.cButtons = 3;
			tdc.pButtons = buttons;
			tdc.pfCallback = [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LONG_PTR lpData) -> HRESULT
				{
					R* x = (R*)lpData;
					if (msg == TDN_TIMER)
					{
						if (x->token.length())
						{
							SendMessage(hwnd, TDM_CLICK_BUTTON, IDCANCEL, 0);
							return S_OK;
						}
						else
						{
							return S_FALSE;
						}
					}
					if (msg == TDN_CREATED)
					{
						auto thr = [](R* r)
							{
								try
								{
									for (int i = 0; i < 100; i++)
									{
										// POST https://github.com/login/oauth/access_token
										RESTAPI::REST r2;
										r2.Connect(L"github.com", true);
										std::string d2;
										d2 += "client_id=";
										d2 += r->client_id;
										d2 += "&";
										d2 += "device_code=";
										d2 += r->device_code;
										d2 += "&";
										d2 += "grant_type=urn:ietf:params:oauth:grant-type:device_code";

										std::wstring acc = L"Accept: application/json";
										auto r3 = r2.RequestWithBuffer(L"/login/oauth/access_token", L"POST", { acc }, d2.data(), d2.length());
										std::vector<char> d;
										r2.ReadToMemory(r3, d);
										d.resize(d.size() + 1);
										nlohmann::json root2 = nlohmann::json::parse(d.data());
										// is there an "access_token" ?
										if (root2.contains("access_token"))
										{
											std::string access_token = root2["access_token"].get<std::string>();
											r->token = access_token;
											break;
										}
										Sleep(8000);
										if (r->Cancelled)
										{
											break;
										}
									}
								}
								catch (...)
								{
									r->token = " ";
								}
							};
						x->j = std::make_shared<std::thread>(thr, x);
					}
					if (msg == TDN_HYPERLINK_CLICKED)
					{
						// open the link in default browser
						ShellExecute(0, L"open", (const wchar_t*)lParam, 0, 0, SW_SHOWNORMAL);
					}
					if (msg == TDN_BUTTON_CLICKED)
					{
						if (wParam == 100)
						{
							std::string user_code = x->user_code;
							std::wstring wcode = tou(user_code.c_str());
							ToClip(hwnd, wcode.c_str(), true);
							return S_FALSE;
						}
						if (wParam == 101)
						{
							std::string verification_uri = x->x->operator[]("verification_uri").get<std::string>();
							ShellExecute(0, L"open", tou(verification_uri.c_str()).c_str(), 0, 0, SW_SHOWNORMAL);
							return S_FALSE;
						}
						if (wParam == IDCANCEL)
						{
							x->Cancelled = true;
							x->j->join();
							return S_OK;
						}
					}
					return S_FALSE;
				};
			tdc.lpCallbackData = (LPARAM)&r;
			TaskDialogIndirect(&tdc, 0, 0, 0);

			if (r.token.length() > 2)
				return r.token;
		}
		catch (...)
		{
			return "";
		}
		return "";
	}

	void Authenticate(HWND hwnd = 0)
	{
		auto token = GetAccessToken(hwnd, client_id.c_str(), client_secret.c_str());
		void CopReturnedToken(std::string);
		CopReturnedToken(token);
	}
#endif



	void ExecuteCallbacks(std::shared_ptr<COPILOT_SESSION> se,std::shared_ptr<PENDING_MESSAGE> s)
	{
		std::lock_guard<std::recursive_mutex> lock(response_mutex);
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
				//rm->content.clear();
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
			ExecuteCallbacks(s, msg);
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

		}
	}

	std::string One(std::shared_ptr<COPILOT_SESSION> s, const char* message, int WaitMs = 0)
	{
		auto pm = CreateMessage(message);
		Send(s, pm);
		int r = Wait(s, pm, WaitMs);
		if (r == 0)
			return pm->completed_message->content;
		return "";
	}


#ifdef _WIN32
	std::shared_ptr<PENDING_MESSAGE> CreateMessage(const char* message, std::function<HRESULT(std::string& reasoning, long long ptr)> reasoning_callback = nullptr, std::function<HRESULT(std::string& msg, long long ptr)> messaging_callback = nullptr, long long ptr = 0, std::vector<std::wstring>* atts = 0)
#else
	std::shared_ptr<PENDING_MESSAGE> CreateMessage(const char* message, std::function<HRESULT(std::string& reasoning, long long ptr)> reasoning_callback = nullptr, std::function<HRESULT(std::string& msg, long long ptr)> messaging_callback = nullptr, long long ptr = 0, std::vector<std::string>* atts = 0)
#endif
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
		pm->Sent = 1;
		ret(j,false);
		return;
	}


	nlohmann::json Sessions(std::vector<std::shared_ptr<COPILOT_SESSION>>& sessions)
	{
		nlohmann::json j;
		j["jsonrpc"] = "2.0";
		j["id"] = next();
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


#ifdef _WIN32
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
#endif

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

#ifdef _WIN32
	std::wstring cli_path;
#else
	std::string cli_path;
#endif

	COPILOT_RAW_STATUS Status()
	{
		COPILOT_RAW_STATUS s;
		s.Installed = 1;
		if (!x)
			s.Installed = 0;
		if (!s.Installed)
			return s;
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
#ifdef _WIN32
		if (OllamaRunning2 == S_OK)
		{
			auto ollama_models = ollama_list();
			s.models.insert(s.models.end(), ollama_models.begin(), ollama_models.end());
		}
#endif


		auto j = Quota();
		try
		{
			if (j.contains("result"))
			{
				// entitlementRequests
				COPILOT_RAW_QUOTA q;
				for (auto wh : { "chat","completions","premium_interactions" })
				{
					q.entitlementRequests = j["result"]["quotaSnapshots"][wh]["entitlementRequests"].get<int>();
					q.overage = j["result"]["quotaSnapshots"][wh]["overage"].get<int>();
					q.usedRequests = j["result"]["quotaSnapshots"][wh]["usedRequests"].get<int>();
					q.remainingPercentage = j["result"]["quotaSnapshots"][wh]["remainingPercentage"].get<float>();
					q.overageAllowedWithExhaustedQuota = j["result"]["quotaSnapshots"][wh]["overageAllowedWithExhaustedQuota"].get<bool>();
					q.resetDate = j["result"]["quotaSnapshots"][wh]["resetDate"].get<std::string>();
					s.quota[wh] = q;
				}
			}

		}
		catch (...)
		{
		}
		return s;
	}


#ifdef _WIN32
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
#endif

	std::shared_ptr<COPILOT_SESSION> CreateSession(const char* model_id, COPILOT_SESSION_PARAMETERS* sp = 0)
	{
		COPILOT_SESSION_PARAMETERS default_params;
		if (!sp)
			sp = &default_params;
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
#ifdef _WIN32
		if (!F)
			return CreateOllamaSession(model_id, sp->Streaming);
#else
		if (!F)
			return nullptr;
#endif

		// {"jsonrpc":"2.0","id":"62587fbb-6596-4144-8014-62403b7d6a97","method":"session.create","params":{"model":"gpt-5-mini","requestPermission":true,"envValueMode":"direct"}}
		nlohmann::json j;
		j["jsonrpc"] = "2.0";
		j["id"] = next();
		j["method"] = "session.create";
		auto params = nlohmann::json::object();
		params["model"] = model_id;
		params["requestPermission"] = true;
		params["envValueMode"] = "direct";
		params["streaming"] = sp->Streaming;
		if (sp->system_message.length())
			params["system_message"] = sp->system_message;
		if (sp->reasoning_effort.length())
			params["reasoning_effort"] = sp->reasoning_effort;
		if (sp->skill_dirs.size())
		{
			params["skill_directories"] = nlohmann::json::array();
			for (auto& sd : sp->skill_dirs)
				params["skill_directories"].push_back(toc(sd.c_str()));
		}
		if (sp->disabled_skills.size())
		{
			params["disabled_skills"] = nlohmann::json::array();
			for (auto& ds : sp->disabled_skills)
				params["disabled_skills"].push_back(toc(ds.c_str()));
		}
		j["params"] = params;
		if (tools.size())
		{
			j["params"]["tools"] = nlohmann::json::array();
			for (auto& t : tools)
			{
				nlohmann::json to;
				to["name"] = t->name;
				to["description"] = t->description;
				to["parameters"] = nlohmann::json::object();
				to["parameters"]["properties"] = nlohmann::json::object();
				for (auto& p : t->parameters)
				{
					to["parameters"]["properties"][p.name] = nlohmann::json::object();
					to["parameters"]["properties"][p.name]["description"] = p.description;
					to["parameters"]["properties"][p.name]["title"] = p.title;
					to["parameters"]["properties"][p.name]["type"] = p.typ;
				}
				// Required parameters
				to["parameters"]["required"] = nlohmann::json::array();
				for (auto& rp : t->parameters)
				{
					if (rp.Required)
						to["parameters"]["required"].push_back(rp.name);
				}

				to["parameters"]["type"] = "object";
				to["parameters"]["title"] = t->title;

				j["params"]["tools"].push_back(to);
			}
		}


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

#ifndef _WIN32
	static std::string tou(const char* s)
	{
		return s;
	}
	static std::string toc(const char* s)
	{
		return s;
	}
	static std::string ChangeSlash(const char* s)
	{
		return s;
	}


#endif


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
	std::string client_id;
	std::string client_secret;
#ifndef _WIN32
	COPILOT_RAW(const char* path_to_cli, int port, const char* auth_token, int Debug = 0, const char* clientid = 0, const char* clientsecret = 0)
#else
	COPILOT_RAW(const wchar_t* path_to_cli, int port, const char* auth_token, int Debug = 0, const char* clientid = 0, const char* clientsecret = 0)
#endif
	{
		debug = Debug;
		DetectOllamaRunning();
		if (clientid)
			client_id = clientid;
		if (clientsecret)
			client_secret = clientsecret;
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
			swprintf_s(cmd.data(), 1000, L"\"%s\" --no-auto-update --auth-token-env %s --acp --port %d --log-level all --headless", path_to_cli, toks.data(), port);
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
			x = 0;
			return;
		}
		sockaddr_in sa = { 0 };
		sa.sin_family = AF_INET;
		sa.sin_port = htons((u_short)port);
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

	COPILOT_RAW(const char* ip, int port)
	{
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

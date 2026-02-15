#include <Windows.h>
#include "json.hpp"
using json = nlohmann::json;

extern "C" {

	// returns heap allocated char* (caller frees)
	__declspec(dllexport)
		const char* pcall(const char* json_in)
	{
		json req = json::parse(json_in);
		json resp;
		resp["city"] = req["city"];
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
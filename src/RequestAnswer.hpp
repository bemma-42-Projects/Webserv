#pragma once
#include <string>
#include "Request.hpp"

class RequestAnswer
{
	public:
		RequestAnswer();
		~RequestAnswer();
		static std::string	answer(Request &request);
		static std::string	methodGet(Request &request);
		static std::string	getIfFile(Request &request);
		static std::string	getIfDir(Request &request);

	private:
};

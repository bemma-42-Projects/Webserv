#pragma once
#include <string>
#include "Request.hpp"

class RequestAnswer
{
	public:
		RequestAnswer();
		~RequestAnswer();
		std::string	answer(Request &request);
		std::string	methodGet(Request &request);

	private:
		// Attributes
};

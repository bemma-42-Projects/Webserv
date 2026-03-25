#pragma once
#include <string>
#include "Request.hpp"

class RequestAnswer
{
	public:
		RequestAnswer(Request request);
		~RequestAnswer();
		int			setAnswer();
		int			methodGet();
		int			getIfFile(std::string file);
		int			getIfDir();
		std::string	getAnswer();
		int			getError();

	private:
		std::string	answer_;
		int			error_;
		Request		request_;
};

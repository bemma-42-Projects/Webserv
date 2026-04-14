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
		int			methodDelete();
		int			methodCGI();
		bool		isCgi();
		int			getIfFile(std::string file);
		int			getIfDir();
		std::string	getAnswer();
		int			getError();
		std::string findIndex(Location loc);

	private:
		std::string	answer_;
		int			error_;
		Request		request_;
		std::string	cgi_interpreter_;
};
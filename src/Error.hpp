#pragma once
#include <string>
#include "LocationConfig.hpp"

class Error 
{
	public:
		Error();
		~Error();
		//static void	setError(int code, std::string message);
		static std::string	ErrorPage();
		static std::string	AnswerError(int code, std::string message, std::map<int, std::string>	pageError);
		static std::string Itoa(int nbr);


	private:
		static int			code_;
		static std::string	message_;
		//static LocationConfig 	*loc_;
		static std::map<int, std::string>	page_error_;
		//static std::string	error_page_;
		//static std::string	answer_error;
};

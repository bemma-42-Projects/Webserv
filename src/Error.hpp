#pragma once
#include <string>

class Error 
{
	public:
		Error();
		~Error();
		void	setError(int code, std::string message);
		void	ErrorPage();
		void	AnswerError();


	private:
		int			code_;
		std::string	message_;
		std::string error_page_;
		std::string	answer_error;
};

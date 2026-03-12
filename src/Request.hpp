#pragma once
#include <string>

class Request {

	public:
		Request(char *buffer);
		~Request();
		std::string getRequest() const;
		bool		complete();
		void		parsingHttp();

	private:
		std::string	request_;
		std::string	methods_;
};

std::ostream& operator<<(std::ostream& out, const Request& request);
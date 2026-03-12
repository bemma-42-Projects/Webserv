#pragma once
#include <string>

class Request {

	public:
		Request(char *buffer);
		~Request();
		std::string getRequest() const;
		int	complete();

	private:
		std::string	request_;
};

std::ostream& operator<<(std::ostream& out, const Request& request);
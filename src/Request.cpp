#include "Request.hpp"
#include <iostream>
#include <exception>

Request::Request(char *buffer)
{
	request_ = buffer;
}

Request::~Request(){}


std::ostream& operator<<(std::ostream& out, const Request& request)
{
	out << request.getRequest(); 
    return out;
}


std::string Request::getRequest() const
{
	return(request_);
}

//verifie qu'il y a "\r\n\r\n" cad que la requet soit complete
bool	Request::complete()
{
	if (request_.find("\r\n\r\n") != std::string::npos)
	{
		std::cout << "Request complete" << std::endl;
		return (true);
	}
	std::cerr << "Request not complete" << std::endl;
	return (false);
}

void	Request::parsingHttp()
{
	if (complete() == false)
		return ;
}
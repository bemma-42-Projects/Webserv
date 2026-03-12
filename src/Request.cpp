#include <Request.hpp>
#include <iostream>

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

int	Request::complete()
{
	
}
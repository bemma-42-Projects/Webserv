#include "Error.hpp"
#include <sstream>

Error::Error(){}

Error::~Error(){}

void	Error::setError(int code, std::string message)
{
	code_ = code;
	message_ = message;
}


std::string itoa(int nbr)
{
	std::stringstream ss;
    
    ss << nbr;
    std::string str = ss.str();
	return str;
}

void	Error::ErrorPage()
{	
	std::string	code_str = itoa(code_);
	error_page_ = "<html>"
		"<head><title>" + code_str + ' ' + message_ + " </title></head>"
		"<body>"
		"<center><h1>" + code_str + ' ' + message_ + " </h1></center>"
		"<hr><center>Webserv/1.0</center>"
		"</body>"
		"</html>";
}


void	Error::AnswerError()
{
	std::string header = "HTTP/1.1" + ' ' + itoa(code_);
	header += ' ' + message_ + "\r\n";
	std::string content_type = "text/html";

	header += "Content-Type: " + content_type + "\r\n";
	header += "Content-Length: " + itoa(error_page_.length()) + "\r\n";
	header += "\r\n";

	//std::cout << "header = " << header << std::endl;

	answer_error = header + error_page_;
}
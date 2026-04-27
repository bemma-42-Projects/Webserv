#include "Error.hpp"
#include <sstream>

Error::Error(){}

Error::~Error(){}

//void	Error::setError(int code, std::string message)
//{
//	code_ = code;
//	message_ = message;
//}


int         Error::code_ = 0;
std::string Error::message_ = "";

std::string Error::Itoa(int nbr)
{
	std::stringstream ss;
    
    ss << nbr;
    std::string str = ss.str();
	return str;
}

std::string	Error::ErrorPage()
{	
	std::string	code_str = Itoa(code_);
	return ("<html>"
		"<head><title>" + code_str + " " + message_ + " </title></head>"
		"<body>"
		"<center><h1>" + code_str + " " + message_ + " </h1></center>"
		"<hr><center>Webserv/1.0</center>"
		"</body>"
		"</html>");
}


std::string	Error::AnswerError(int code, std::string message)
{
	code_ = code;
	message_ = message;
	std::string error_page = ErrorPage();
	std::string header = "HTTP/1.1 " + Itoa(code_);
	header += " " + message_ + "\r\n";
	std::string content_type = "text/html";

	header += "Content-Type: " + content_type + "\r\n";
	header += "Content-Length: " + Itoa(error_page.length()) + "\r\n";
	header += "\r\n";

	//std::cout << "header = " << header << std::endl;

	//answer_error = header + error_page;
	return (header + error_page);
}
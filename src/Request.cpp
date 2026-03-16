#include "Request.hpp"
#include <iostream>
#include <exception>
#include <algorithm>
#include <sstream>

Request::Request(char *buffer)
{
	request_ = buffer;
}

Request::~Request(){}


std::ostream& operator<<(std::ostream& out, const Request& request)
{
	out << "Request: " << request.getRequest() << "\n"
		<< "Method: " << request.getMethod() << "\n"
		<< "Path: " << request.getPath() << "\n"
		<< "Version: " << request.getVersion() << "\n";
		//<< "Headers: " << request.getHeaders() << "\n"; 

	out << "Headers: \n";
	
	const std::map<std::string, std::string>& headers = request.getHeaders();
	std::map<std::string, std::string>::const_iterator i;

	for (i = headers.begin(); i != headers.end(); ++i) {
		out << "	Header: " << i->first  // La clé (ex: "Content-Type")
				<< " | Valeur: " << i->second // La valeur (ex: "text/html")
				<< "\n";
	}
	out << "Body: " << request.getBody() << "\n";
    return out;
}

std::string Request::getRequest() const
{
	return(request_);
}

std::string Request::getMethod() const
{
	return(method_);
}

std::string Request::getPath() const
{
	return(path_);
}

std::string Request::getVersion() const
{
	return(version_);
}

std::map<std::string, std::string> Request::getHeaders() const
{
	return(headers_);
}

std::string Request::getBody() const
{
	return(body_);
}

//verifie qu'il y a "\r\n\r\n" cad que la requet soit complete
bool	Request::complete()
{
	size_t	end = request_.find("\r\n\r\n");
	if (end == std::string::npos)
	{
		std::cerr << "Request not complete" << std::endl;
		return false;
	}
	size_t it = request_.find("Content-Length:");
	if (it == std::string::npos)
		return true;
	it += 16;
	std::string	tmp = request_.substr(it, end);
	size_t	len;
	std::stringstream ss(tmp);
    ss >> len;
	while (request_[end] == '\r' || request_[end] == '\n')
		++end;
	if (request_.size() - end != len)
	{
		std::cout << "Request not complete" << std::endl;
		return false;
	}
	std::cout << tmp << std::endl;
	return true;
}

//parse la premier ligne et implemente la class (methode chemin version)
int	Request::initFistLine()
{
	size_t	begin = 0;
	size_t last = request_.find("\r\n");
	if (last == std::string::npos)
	{
		std::cout << "Request not have method or path or version" << std::endl;
		return 2;
	}
	size_t	it = request_.find(" ", begin);
	if (it == std::string::npos || it >= last)
		return 1;
	method_ = request_.substr(begin, it);
	//probablement a voir plus tard
	if (method_ != "GET" && method_ != "POST" && method_ != "DELETE")
	{
		std::cout << "501 Not Implemented" << std::endl;
		return 1;
	}
	begin = request_.find("/", it);
	if (begin == std::string::npos || begin != (it + 1))
		return 1;
	it = request_.find(" ", begin);
	if (it == std::string::npos || it >= last)
		return 1;
	path_ = request_.substr(begin, it - begin);
	begin = request_.find("HTTP", it);
	if (begin == std::string::npos || begin != (it + 1))
		return 1;
	version_ = request_.substr(begin, last - begin);

	//std::cout << method_ << "\n" << path_ << "\n" << version_ << std::endl;

	return 0;
}

//initialise la map avec le header
int	Request::initHeader()
{
	size_t last = request_.find("\r\n\r\n");
	if (last == std::string::npos)
	{
		std::cout << "Probleme with header" << std::endl;
		return 1;
	}
	size_t end = 0;
	while (end < last)
	{
		size_t	begin = request_.find("\r\n", end);
		if (begin == std::string::npos)
		{
			std::cout << "Probleme with header" << std::endl;
			return 1;
		}
		if (begin == last)
			break;
		begin += 2;
		end = begin;
		size_t it = request_.find(":", begin);
		if (it == std::string::npos || it >= last)
			return 1;
		std::string cle = request_.substr(begin, it - begin);
		if (it +2 >= last || request_[it + 1] != ' ')
		{
			std::cout << "Error header" << std::endl;
			return 1;
		}
		begin = it + 2;
		size_t	end = request_.find("\r\n", begin);
		std::string value = request_.substr(begin, end - begin);
		headers_.insert(std::pair<std::string, std::string>(cle, value));
	}
	return 0;
}

//verifie que le body exist et initialise le body de la class
int	Request::initBody()
{
	std::map<std::string, std::string>::const_iterator it = headers_.find("Content-Length");
	if (it == headers_.end())
	{
		//std::cout << "not body" << std::endl;
		body_ = "\0";
		return 0;
	}
	std::string value = it->second;
	size_t begin = request_.find("\r\n\r\n");
	if (begin == std::string::npos)
	{
		std::cout << "Probleme with body" << std::endl;
		return 1;
	}
	size_t	len;
	std::stringstream ss(value);
    ss >> len;
	// attention a la limite sinon renvoir "413 Request Entity Too Large"
	while (request_[begin] == '\r' || request_[begin] == '\n')
		++begin;
	if (request_.size() - begin != len)
	{
		std::cout << "the size of the body don't is good" << std::endl;
		return 1;
	}
	//std::cout << "the size is good" << std::endl;
	body_ = request_.substr(begin, len);
	return 0;
}

int	Request::parsingHttp()
{
	if (complete() == false)
		return 1; //continuer la lecture
	int res = initFistLine();
	if (res == 1) //attention a ne pas rappeler la fonction pour verifier les sortie
	{
		std::cout << "erreur 400" << std::endl;
		return 1;
	}
	else if (res == 2)
	{
		std::cout << "erreur ligne 1" << std::endl;
		return;
	}
	if (initHeader() == 1)
		return 1 ;
	if (initBody() == 1)
		return 1;
	std::cout << "\n--------------------------------------------------------\n" << std::endl;
	return 0;
}




int main()
{
	const char *buffer = "POST /upload HTTP/1.1\r\n"
	    "Host: localhost:8080\r\n"
	    "Content-Type: application/x-www-form-urlencoded\r\n"
	    "Content-Length: 27\r\n"
	    "\r\n\r\n" // Ligne vide importante entre headers et body
	    "name=Gemini&project=webserv";


	Request file((char *)buffer);
	if (file.parsingHttp() == 1)
	{
		std::cerr << "Error" << std::endl;
		return 1;
	}
	std::cout << file << std::endl;
	
}


//header fait normalement, a verifier, verifier les caracteres avec content-length
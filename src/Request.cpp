#include "Request.hpp"
#include <iostream>
#include <exception>
#include <algorithm>

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
		out << "Header: " << i->first  // La clé (ex: "Content-Type")
				<< " | Valeur: " << i->second // La valeur (ex: "text/html")
				<< "\n";
	}
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



//verifie qu'il y a "\r\n\r\n" cad que la requet soit complete
bool	Request::complete()
{
	if (request_.find("\r\n\r\n") != std::string::npos)
	{
		std::cout << "Request complete" << std::endl;
		return true;
	}
	std::cerr << "Request not complete" << std::endl;
	return false;
}

//parse la premier ligne et implemente la class (methode chemin version)
int	Request::fistLine()
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
	if (method_ != "GET" && method_ != "POST" && method_ != "DELETE")
	{
		std::cout << "Method not valide" << std::endl;
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

	std::cout << method_ << "\n" << path_ << "\n" << version_ << std::endl;

	return 0;
}

int	Request::header()
{
	size_t last = request_.find("\r\n\r\n");
	if (last == std::string::npos)
	{
		std::cout << "Probleme with header" << std::endl;
		return 1;
	}
	size_t tmp = 0;
	while (tmp < last)
	{
		size_t	begin = request_.find("\r\n", tmp);
		if (begin == std::string::npos)
		{
			std::cout << "Probleme with header" << std::endl;
			return 1;
		}
		if (begin == last)
			break;
		begin += 2;
		tmp = begin;
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
		std::string value = request_.substr(begin, last - begin);
		headers_.insert(std::pair<std::string, std::string>(cle, value));
		
	}

		// On définit le type de l'itérateur pour plus de clarté
	//std::map<std::string, std::string>::const_iterator i;

	//for (i = headers_.begin(); i != headers_.end(); ++i) {
	//	std::cout << "Header: " << i->first  // La clé (ex: "Content-Type")
	//			<< " | Valeur: " << i->second // La valeur (ex: "text/html")
	//			<< std::endl;
	//}

	return 0;
	//faire pareil pour la valeur et implementer la map ex::ages["Bob"] = 30;
	//faire une boucle

}

void	Request::parsingHttp()
{
	if (complete() == false)
		return; //continuer la lecture
	if (fistLine() == 1) //attention a ne pas rappeler la fonction pour verifier les sortie
	{
		std::cout << "erreur 400" << std::endl;
		return;
	}
	//else if (setFistLine() == 2)
	//{
	//	std::cout << "erreur ligne 1" << std::endl;
	//	return;
	//}
	if (header() == 1)
		return;
	std::cout << "\n--------------------------------------------------------\n" << std::endl;
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
	file.parsingHttp();
	std::cout << file << std::endl;
	
}


//header fait normalement, a verifier, verifier les caracteres avec content-length
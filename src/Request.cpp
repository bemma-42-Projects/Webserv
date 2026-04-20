#include "Request.hpp"
#include <iostream>
#include <exception>
#include <algorithm>
#include <sstream>
//#include <fcntl.h>    // pour open
//#include <unistd.h>   // pour read, close
//#include <sys/stat.h> // pour stat
#include "Config.hpp"
#include "RequestAnswer.hpp"
#include "Error.hpp"
//#include <dirent.h>


Request::Request()
{}

Request::Request(char *buffer)
{
	request_ = buffer;
	error_ = 0;
}

Request::~Request(){}


std::ostream& operator<<(std::ostream& out, const Request& request)
{
	out << "Request: " << request.getRequest() << "\n"
		<< "Method: " << request.getMethod() << "\n"
		<< "Path: " << request.getPath() << "\n"
		<< "Version: " << request.getVersion() << "\n";

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

std::string Request::getUrlPath() const
{
	return(url_path_);
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

int	Request::getError() const
{
	return error_;
}

Location	Request::getLocation() const
{
	return location_;
}


//verifie qu'il y a "\r\n\r\n" cad que la requet soit complete
//!!! ne pouvoir lire et parser qu'un certain nombre de body en meme temps pour l'espace memoir
bool	Request::complete()
{
	size_t	end = request_.find("\r\n\r\n");
	if (end == std::string::npos)
		return false;//requette non complet
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

	if (request_.size() - end < len)
	{
		//error_ = 413;
		return false;
	}
	return true;
}


void	Request::splitUri_()
{
	size_t	question_mark_position = this->raw_uri_.find('?');

	if (question_mark_position != std::string::npos)
	{
		this->url_path_ = this->raw_uri_.substr(0, question_mark_position);
		this->query_string_ = this->raw_uri_.substr(question_mark_position + 1);
	}
	else
		this->query_string_ = "";
}

//parse la premier ligne et implemente la class (methode chemin version)
int	Request::initFistLine()
{
	size_t	begin = 0;
	size_t last = request_.find("\r\n");
	if (last == std::string::npos)
		return 1;

	size_t	it = request_.find(" ", begin);
	if (it == std::string::npos || it >= last)
		return 1;

	method_ = request_.substr(begin, it);
	if (method_ != "GET" && method_ != "POST" && method_ != "DELETE")
		return 2; //501 Not Implemented

	
	begin = request_.find("/", it);
	if (begin == std::string::npos || begin != (it + 1))
		return 1;

	it = request_.find(" ", begin);
	if (it == std::string::npos || it >= last)
		return 1;

	url_path_ = request_.substr(begin, it - begin);
	std::cout << url_path_ << std::endl;

	this->raw_uri_ = url_path_;
	this->splitUri_();

	//path_ = Config::getRoot() + url_path_;//avoir a peut etre supprimer
	
	
	begin = request_.find("HTTP", it);
	if (begin == std::string::npos || begin != (it + 1))
		return 1;
	version_ = request_.substr(begin, last - begin);
	return 0;
}

//initialise la map avec le header
int	Request::initHeader()
{
	size_t last = request_.find("\r\n\r\n");
	if (last == std::string::npos)
		return 1;
	size_t end = 0;
	while (end < last)
	{
		size_t	begin = request_.find("\r\n", end);
		if (begin == std::string::npos)
			return 1;
		if (begin == last)
			break;
		begin += 2;
		end = begin;
		size_t it = request_.find(":", begin);
		if (it == std::string::npos || it >= last)
			return 1;
		std::string cle = request_.substr(begin, it - begin);
		if (it +2 >= last || request_[it + 1] != ' ')
			return 1;
		begin = it + 2;
		size_t	end = request_.find("\r\n", begin);
		std::string value = request_.substr(begin, end - begin);
		headers_.insert(std::pair<std::string, std::string>(cle, value));
	}
	if (headers_.find("Host") == headers_.end() 
		|| (method_ == "POST" && headers_.find("Content-Length") == headers_.end()))
		return 1;
	if (headers_.find("Content-Type") == headers_.end())
		headers_.insert(std::pair<std::string, std::string>("Content-Type", "application/octet-stream"));
	
	return 0;
}

//verifie que le body exist et initialise le body de la class
int	Request::initBody()
{
	std::map<std::string, std::string>::const_iterator it = headers_.find("Content-Length");
	if (it == headers_.end())
	{
		body_ = "\0";
		return 0;
	}
	std::string value = it->second;
	size_t begin = request_.find("\r\n\r\n");
	if (begin == std::string::npos)
		return 1;
	size_t	len;
	std::stringstream ss(value);
    ss >> len;
	if (len > Config::getBodySize())
		return 1;
	while (request_[begin] == '\r' || request_[begin] == '\n')
		++begin;
	if (request_.size() - begin != len)
		return 1;
	body_ = request_.substr(begin, len);
	return 0;
}

int	Request::checkOfLocation()
{
	Location* loc = Config::matchLocation(url_path_);
	if (loc == NULL)
		return 1;
	location_ = *loc;
	std::cout << location_.getPath() << std::endl;
	std::vector<std::string> allowedMethods = location_.getAllowedMethods();
	if (std::find(allowedMethods.begin(), allowedMethods.end(), method_)
			== allowedMethods.end())
		return 2;
	return 0;
}


int	Request::parsingHttp()
{
	if (complete() == false)
		return 2; //continuer la lecture
	int res = initFistLine();
	if (res == 1)
	{
		error_ = 401;
		return 0;
	}
	else if (res == 2)
	{
		error_ = 501;
		return 0;
	}
	if (initHeader() == 1)
	{
		error_ = 402;
		return 0 ;
	}
	int	body =  initBody();
	if (body == 1)
	{
		error_ = 413;
		return 0;
	}
	int checkLoc = checkOfLocation();
	if (checkLoc == 1)
	{
		error_ = 404;
		return 0;
	}
	else if (checkLoc == 2)
	{
		error_ = 405;
		return 0;
	}
	std::string	root = location_.getRoot();
	if (!url_path_.empty() && url_path_[0] == '/')
		path_ = root + url_path_;
	else
		path_ = root + "/" + url_path_; 
	std::cout << "\n--------------------------------------------------------\n" << std::endl;
	return 1;
}

std::string	Request::getRequestUri() const
{
	return (this->raw_uri_);
}

std::string	Request::getQueryString() const
{
	return (this->query_string_);
}

std::string	Request::getContentType() const
{
	std::map<std::string, std::string>::const_iterator it = headers_.find("Content-Type");

	if (it != headers_.end())
		return (it->second);
	return ("");
}

std::string	Request::getContentLength() const
{
	std::map<std::string, std::string>::const_iterator it = headers_.find("Content-Length");

	if (it != headers_.end())
		return (it->second);
	return ("0");
}

std::string	Request::getHost() const
{
	std::map<std::string, std::string>::const_iterator it = headers_.find("Host");

	if (it != headers_.end())
	{
		std::string	host_raw = it->second;
		size_t		pos = host_raw.find(':');

		if (pos != std::string::npos)
			return (host_raw.substr(0, pos));
		return (host_raw);
	}
	return ("localhost");
}

std::string Request::getPort() const
{
	std::map<std::string, std::string>::const_iterator it = headers_.find("Host");

	if (it != headers_.end())
	{
		std::string	host_raw = it->second;
		size_t		pos = host_raw.find(':');

		if (pos != std::string::npos)
			return (host_raw.substr(pos + 1));
		return ("80");
	}
	return ("80");
}

void	Request::setClientIP(const std::string &ip)
{
	this->client_ip_ = ip;
}

std::string	Request::getClientIP() const
{
	return (this->client_ip_);
}

//void	Request::setError(int error)
//{
//	error_ = error;
//}

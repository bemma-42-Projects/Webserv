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
#include "Location.hpp"
//#include <dirent.h>


Request::Request()
{}

Request::Request(char *buffer)
{
	this->request_ = buffer;
	this->error_ = 0;
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
	return (this->request_);
}

std::string Request::getMethod() const
{
	return (this->method_);
}

std::string Request::getPath() const
{
	return (this->path_);
}

std::string Request::getUrlPath() const
{
	return (this->url_path_);
}

std::string Request::getVersion() const
{
	return (this->version_);
}

std::map<std::string, std::string> Request::getHeaders() const
{
	return (this->headers_);
}

std::string Request::getBody() const
{
	return (this->body_);
}

int	Request::getError() const
{
	return (this->error_);
}

Location	Request::getLocation() const
{
	return (this->location_);
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
	std::map<std::string, std::string>::const_iterator it = this->headers_.find("Content-Type");

	if (it != this->headers_.end())
		return (it->second);
	return ("");
}

std::string	Request::getContentLength() const
{
	std::map<std::string, std::string>::const_iterator it = this->headers_.find("Content-Length");

	if (it != this->headers_.end())
		return (it->second);
	return ("0");
}

std::string	Request::getHost() const
{
	std::map<std::string, std::string>::const_iterator it = this->headers_.find("Host");

	if (it != this->headers_.end())
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
	std::map<std::string, std::string>::const_iterator it = this->headers_.find("Host");

	if (it != this->headers_.end())
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

//verifie qu'il y a "\r\n\r\n" cad que la requet soit complete
//!!! ne pouvoir lire et parser qu'un certain nombre de body en meme temps pour l'espace memoir
bool	Request::complete()
{
	size_t	end = this->request_.find("\r\n\r\n");
	if (end == std::string::npos)
		return false;//requette non complet
	// std::cout << "test" <<std::endl;
	size_t it = this->request_.find("Content-Length:");
	if (it == std::string::npos)
		return true;
	it += 15;
	std::string	tmp = this->request_.substr(it, end);
	size_t	len;
	std::stringstream ss(tmp);
    ss >> len;
	//while (request_[end] == '\r' || request_[end] == '\n')
	//	++end;
	end += 4;
	//std::cout << "test" <<std::endl;

	//std::cout << request_.substr(end) << std::endl;
	//std::cout << this->request_.size() - end << " < " << len << std::endl;
	if (this->request_.size() - end < len)
	{
		//error_ = 413;
		return (false);
	}
	// std::cout << "test" <<std::endl;
	return (true);
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
	size_t last = this->request_.find("\r\n");
	if (last == std::string::npos)
		return (1);

	size_t	it = this->request_.find(" ", begin);
	if (it == std::string::npos || it >= last)
		return (1);

	this->method_ = this->request_.substr(begin, it);
	if (this->method_ != "GET" && this->method_ != "POST" && this->method_ != "DELETE")
		return (2); //501 Not Implemented

	
	begin = this->request_.find("/", it);
	if (begin == std::string::npos || begin != (it + 1))
		return (1);

	it = this->request_.find(" ", begin);
	if (it == std::string::npos || it >= last)
		return (1);

	this->url_path_ = this->request_.substr(begin, it - begin);
	//std::cout << this->url_path_ << std::endl;

	this->raw_uri_ = this->url_path_;
	this->splitUri_();

	//path_ = Config::getRoot() + url_path_;//avoir a peut etre supprimer
	
	
	begin = this->request_.find("HTTP", it);
	if (begin == std::string::npos || begin != (it + 1))
		return (1);
	this->version_ = this->request_.substr(begin, last - begin);
	return (0);
}

//initialise la map avec le header
int	Request::initHeader()
{
	size_t last = this->request_.find("\r\n\r\n");
	if (last == std::string::npos)
		return (1);
	size_t end = 0;
	while (end < last)
	{
		size_t	begin = this->request_.find("\r\n", end);
		if (begin == std::string::npos)
			return (1);
		if (begin == last)
			break;
		begin += 2;
		end = begin;
		size_t it = this->request_.find(":", begin);
		if (it == std::string::npos || it >= last)
			return (1);
		std::string key = this->request_.substr(begin, it - begin);
		if (it +2 >= last || request_[it + 1] != ' ')
			return (1);
		begin = it + 2;
		size_t	end = this->request_.find("\r\n", begin);
		std::string value = this->request_.substr(begin, end - begin);
		this->headers_.insert(std::pair<std::string, std::string>(key, value));
	}
	if (this->headers_.find("Host") == this->headers_.end() 
		|| (this->method_ == "POST" && this->headers_.find("Content-Length") == this->headers_.end()))
		return (1);
	if (this->headers_.find("Content-Type") == this->headers_.end())
		this->headers_.insert(std::pair<std::string, std::string>("Content-Type", "application/octet-stream"));
	
	return (0);
}

//verifie que le body exist et initialise le body de la class
int	Request::initBody()
{
	std::map<std::string, std::string>::const_iterator it = this->headers_.find("Content-Length");
	if (it == this->headers_.end())
	{
		//this->body_ = "\0";
		this->body_ = "";
		return (0);
	}
	// s'il n'y a pas de Content-Length, on considère qu'il n'y a pas de corps
	if (it == this->headers_.end())
	{
		this->body_ = "";
		return (0);
	}

	//size_t begin = this->request_.find("\r\n\r\n");
	//if (begin == std::string::npos)
	size_t	header_end = this->request_.find("\r\n\r\n");
	size_t	body_start = header_end + 4;

	size_t				expected_len = 0;

	std::string value = it->second;
	std::stringstream	ss(value);
	ss >> expected_len;

	if (expected_len > Config::getBodySize())
	{
		std::cout << "[DEBUG] Body trop grand: " << expected_len << std::endl;
		return (1);
	}

	size_t	received_len = this->request_.size() - body_start;
	if (received_len < expected_len)
	{
		std::cout << "[DEBUG] Body incomplet: " << received_len << "/" << expected_len << std::endl;
		return (2);
	}
	//while (this->request_[begin] == '\r' || this->request_[begin] == '\n')
	//	++begin;
	//if (this->request_.size() - begin != len)
	//	return (1);
	this->body_ = this->request_.substr(body_start, expected_len);
	return (0);
}

int	Request::checkOfLocation()
{
	Location* loc = Config::matchLocation(this->url_path_);
	if (loc == NULL)
		return (1);
	this->location_ = *loc;
	// std::cout << location_.getPath() << std::endl;
	std::vector<std::string> allowedMethods = this->location_.getAllowedMethods();
	if (std::find(allowedMethods.begin(), allowedMethods.end(), this->method_)
			== allowedMethods.end())
		return (2);
	return (0);
}

// on parse tout ce qu'on a accumule jusqu'a present
// pas le dernier morceau de requete
ParsingStatus	Request::parsingHttp(const std::string &raw_data)
{
	//this->request_ = raw_data;
	this->request_ += raw_data;
	
	//if (complete() == false)
	//	return (PARSING_INCOMPLETE); //continuer la lecture

	size_t	header_end = this->request_.find("\r\n\r\n");
	if (header_end == std::string::npos)
		return (PARSING_INCOMPLETE);

	int res = initFistLine();

	if (res == 1)
	{
		std::cout << "[DEBUG] Echec FirstLine. Code: " << res << std::endl;
		this->error_ = 401;
		return (PARSING_FAILED);
	}
	else if (res == 2)
	{
		std::cout << "[DEBUG] Echec FirstLine. Code: " << res << std::endl;
		this->error_ = 501;
        return (PARSING_FAILED);
	}
	if (initHeader() == 1)
	{
		std::cout << "[DEBUG] Echec Headers" << std::endl;
		this->error_ = 402;
		return (PARSING_FAILED);
	}
	int	body = initBody();
	if (body == 1)
	{
		this->error_ = 413;
		return (PARSING_FAILED);
	}
	if (body == 2)
        return (PARSING_INCOMPLETE);

	int checkLoc = checkOfLocation();
	if (checkLoc == 1)
	{
		std::cout << "[DEBUG] Echec Location. Code: " << checkLoc << std::endl;
		this->error_ = 404;
		return (PARSING_FAILED);
	}
	else if (checkLoc == 2)
	{
		std::cout << "[DEBUG] Echec Location. Code: " << checkLoc << std::endl;
		this->error_ = 405;
		return (PARSING_FAILED);
	}
	
	std::string	root = this->location_.getRoot();
	if (!(this->url_path_.empty()) && this->url_path_[0] == '/')
		this->path_ = root + this->url_path_;
	else
		this->path_ = root + "/" + this->url_path_;

	if (this->path_[this->path_.length() - 1] == '/')
	{
		std::vector<std::string> indexes = this->location_.getIndex();
		if (!indexes.empty())
			this->path_ += indexes[0];
	}
	//std::cout << "\n--------------------------------------------------------\n" << std::endl;
	//std::cout << root << std::endl;
	//std::cout << this->url_path_ << std::endl;
	//std::cout << this->path_ << std::endl;
	return (PARSING_SUCCESS);
}

//void	Request::setError(int error)
//{
//	error_ = error;
//}
/*
int main()
{
	//try{
		Config::location();

		const char *buffer = 
		"POST /uploads HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Content-Type: multipart/form-data; boundary=boundary123\r\n"
		"Content-Length: 162\r\n"
		"\r\n"
		"--boundary123\r\n"
		"Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n"
		"Ceci est le contenu de mon fichier !\r\n"
		"--boundary123--";
	
		Request file((char *)buffer);
		int res = file.parsingHttp();
		if (res == 0)
		{
			std::cout << "error " << file.getError() << std::endl;
			return 0;
		}
		else if (res == 2)
		{
			std::cout << "requette non complete" << std::endl;
			return 0;
		}
		// std::cout << "parsing good, locatio = " << file.getLocation().getRoot() << std::endl;
		// std::cout << file << std::endl;
		RequestAnswer answer(file);
		// std::cout << "test " << std::endl;
		if (answer.setAnswer() == 1)
			std::cout << "anser =" << answer.getAnswer() << std::endl;
		
	//}
	//catch(std::exception &e)
	//{
	//	std::cerr << "error : " << e.what() << std::endl;
	//	//Error::setError(e.what());
	//}
}
*/

void	Request::clear()
{
	this->request_.clear();
	this->method_.clear();
	this->path_.clear();
	this->url_path_.clear();
	this->version_.clear();
	this->headers_.clear();
	this->body_.clear();
	this->error_ = 0;
	this->location_ = Location();
	this->raw_uri_.clear();
	this->query_string_.clear();
	this->client_ip_.clear();
}
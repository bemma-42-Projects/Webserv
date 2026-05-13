#include "Request.hpp"
#include <iostream>
#include <exception>
#include <algorithm>
#include <sstream>
#include "RequestAnswer.hpp"
#include "Error.hpp"
#include "ServerConfig.hpp"
#include "parsingconf.hpp"

Request::Request()
{}

Request::Request(char *buffer, ServerConfig& server) : server_(&server)
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
		out << "	Header: " << i->first
				<< " | Valeur: " << i->second
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

std::string Request::getErrorMessage() const
{
	return message_error_;
}

const LocationConfig	&Request::getLocation() const
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

const ServerConfig*	Request::getServer() const
{
	return server_;
}

//verifie qu'il y a "\r\n\r\n" cad que la requet soit complete
//!!! ne pouvoir lire et parser qu'un certain nombre de body en meme temps pour l'espace memoir
bool	Request::complete()
{
	size_t	end = this->request_.find("\r\n\r\n");
	if (end == std::string::npos)
		return false;
	size_t it = this->request_.find("Content-Length:");
	if (it == std::string::npos)
		return true;
	it += 15;
	std::string	tmp = this->request_.substr(it, end);
	size_t	len;
	std::stringstream ss(tmp);
    ss >> len;
	end += 4;
	if (request_.size() - end < len)
		return false;
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
	size_t last = this->request_.find("\r\n");
	if (last == std::string::npos)
		return (1);

	size_t	it = this->request_.find(" ", begin);
	if (it == std::string::npos || it >= last)
		return (1);

	this->method_ = this->request_.substr(begin, it);
	if (this->method_ != "GET" && this->method_ != "POST" && this->method_ != "DELETE" && this->method_ != "HEAD")
		return (2);

	begin = this->request_.find("/", it);
	if (begin == std::string::npos || begin != (it + 1))
		return (1);

	it = this->request_.find(" ", begin);
	if (it == std::string::npos || it >= last)
		return (1);

	this->url_path_ = this->request_.substr(begin, it - begin);

	this->raw_uri_ = this->url_path_;
	this->splitUri_();
	
	begin = this->request_.find("HTTP", it);
	if (begin == std::string::npos || begin != (it + 1))
		return (1);
	this->version_ = this->request_.substr(begin, last - begin);
	return (0);
}

// initialise la map avec le header
int Request::initHeader()
{
    size_t last = this->request_.find("\r\n\r\n");
    if (last == std::string::npos)
        return (1);

    size_t current_pos = 0;
    
    while (current_pos < last)
    {
        size_t begin = this->request_.find("\r\n", current_pos);
        if (begin == std::string::npos)
            return (1);
        if (begin == last)
            break;
            
        begin += 2;
        size_t it = this->request_.find(":", begin);
        if (it == std::string::npos || it >= last)
            return (1);
            
        std::string key = this->request_.substr(begin, it - begin);
        
        if (it + 2 >= last || request_[it + 1] != ' ')
            return (1);
            
        begin = it + 2;
        
        size_t value_end = this->request_.find("\r\n", begin);
        if (value_end == std::string::npos)
            return (1);
            
        std::string value = this->request_.substr(begin, value_end - begin);
        this->headers_.insert(std::pair<std::string, std::string>(key, value));

        current_pos = value_end; 
    }


   	if (headers_.find("Host") == headers_.end())
        return (1);

    if (method_ == "POST" 
        && headers_.find("Content-Length") == headers_.end() 
        && headers_.find("Transfer-Encoding") == headers_.end())
    {
        return (1);
    }

    if (headers_.find("Content-Type") == headers_.end())
        headers_.insert(std::pair<std::string, std::string>("Content-Type", "application/octet-stream"));
        
    return (0);
}

int Request::initBody()
{
    if (headers_.count("Transfer-Encoding") && headers_["Transfer-Encoding"] == "chunked") 
    {
        size_t headers_end = request_.find("\r\n\r\n");
        if (headers_end == std::string::npos) 
            return 1;

        std::string raw_body = request_.substr(headers_end + 4);

        if (raw_body.find("0\r\n\r\n") != std::string::npos) 
        {
            body_.clear();
            return 0;
        }
        return 3;
    }

    std::map<std::string, std::string>::const_iterator it = headers_.find("Content-Length");
    
    if (it == headers_.end())
    {
        body_.clear(); 
        return 0;
    }

    std::string value = it->second;
    size_t  len;
    std::stringstream ss(value);
    ss >> len;

    if (len == 0) {
        body_.clear();
        return 0; 
    }

    size_t begin = request_.find("\r\n\r\n");
    if (begin == std::string::npos)
        return 1;
    begin += 4; 

    if (len > location_.getClientMaxBodySize())
        return 2;

    if (request_.size() - begin < len)
        return 1;

    body_ = request_.substr(begin, len);
    
    return 0;
}



const	LocationConfig	*Request::matchExtensionLocation() const
{
	if (this->server_ == NULL)
        return (NULL);

	const std::vector<LocationConfig> &all_locs = server_->getLocations();

    for (size_t i = 0; i < all_locs.size(); ++i)
    {
        std::string loc_name = all_locs[i].getPath();

        if (loc_name.size() > 1 && loc_name[0] == '.')
        {
            if (url_path_.size() >= loc_name.size()
				&& url_path_.substr(this->url_path_.size() - loc_name.size()) == loc_name)
			{
				return (&all_locs[i]);
			}
        }
    }
	return (NULL);
}

int Request::checkOfLocation()
{
    
    if (this->server_ == NULL) {
        return 1;
    }

    const LocationConfig* loc = server_->matchLocation(url_path_);
	/*if (loc->getReturn().first == 301 || loc->getReturn().first == 302)
	{
		return_ = loc->getReturn();
		return (3);
	}
	*/

    if (loc == NULL || loc->getPath() == "/" ) {
		if (!url_path_.empty() && url_path_[url_path_.size() -1] != '/')
		{
			std::string	retry_path = url_path_ + "/";
			const LocationConfig	*retry_loc = server_->matchLocation(retry_path);
			if (retry_loc != NULL && retry_loc->getPath() != "/" )
				loc = retry_loc;
		}
    }

	if (loc == NULL)
		return (1);

	const	LocationConfig	*ext_loc = matchExtensionLocation();
	if (ext_loc != NULL)
		loc = ext_loc;

    location_ = *loc;
    
    std::set<std::string> allowedMethods = location_.getAllowedMethods();
    
	if (std::find(allowedMethods.begin(), allowedMethods.end(), method_) == allowedMethods.end()) {
        return (2);
    }

	std::string root = location_.getRoot();
	if (method_ == "POST" && location_.getAllowedUpload() == true)
	{
		if (!location_.getUploadPath().empty())
			root = location_.getUploadPath();
		else
			return 1;
	}
	std::string	loc_p = location_.getPath();
	std::string url = url_path_;
	std::string	clean_loc = loc_p;

	if (clean_loc.size() > 1 && clean_loc[clean_loc.size() - 1] == '/')
		clean_loc.erase(clean_loc.size() - 1);

	std::string	clean_url = url;
	if (clean_url.size() > 1 && clean_url[clean_url.size() - 1] == '/')
		clean_url.erase(clean_url.size() - 1);

	std::string	remaining = "";

	if (clean_url.find(clean_loc) == 0)
	{
		if (url.size() > clean_loc.size())
		{
			remaining = url.substr(clean_loc.size());
			if (remaining.size() > 0 && remaining[0] == '/')
				remaining = remaining.substr(1);
		}
	}
	if (!root.empty() && root[root.size() - 1] == '/')
		this->path_ = root + remaining;
	else
		this->path_ = root + '/' + remaining;
	if (this->path_.size() > 1 && this->path_[this->path_.size() - 1] == '/')
		this->path_.erase(this->path_.size() - 1);
    return (0);
}

void	Request::setServerConfig(const ServerConfig *server)
{
	this->server_ = server;
}

// on parse tout ce qu'on a accumule jusqu'a present
ParsingStatus	Request::parsingHttp(const std::string &raw_data)
{
	this->request_ = raw_data;

	if (complete() == false)
		return (PARSING_INCOMPLETE);
	
	int res = initFistLine();

	if (res == 1)
	{
		error_ = 400;
		message_error_ = "Bad Request";
		return (PARSING_FAILED);
	}
	else if (res == 2)
	{
		error_ = 501;
		message_error_ = "Not Implemented";
		return (PARSING_FAILED);
	}
	
	int checkLoc = checkOfLocation();
	if (checkLoc == 1)
	{
		error_ = 404;
		message_error_ = "Not Found";
		return (PARSING_FAILED);
	}
	else if (checkLoc == 2)
	{
		error_ = 405;
		message_error_ = "Method Not Allowed";
		return (PARSING_FAILED);
	}
	else if (checkLoc == 3)
	{
		return (PARSING_SUCCESS);
	}
	if (initHeader() == 1)
	{
        error_ = 400;
        message_error_ = "Bad Request";
        return (PARSING_FAILED);
	}
	int	body =  initBody();
	if (body == 1)
	{
		error_ = 400;
        message_error_ = "Bad Request";
		return (PARSING_FAILED);
	}
	else if (body == 2)
	{
		error_ = 413;
		message_error_ = "Payload Too Large";
		return (PARSING_FAILED);
	}
	else if (body == 3) {
        return (PARSING_INCOMPLETE); 
    }
	return (PARSING_SUCCESS);

}

void	Request::clear()
{
	this->request_.clear();
    this->method_.clear();
    this->path_.clear();
    this->url_path_.clear();
    this->version_.clear();
    this->body_.clear();
    this->message_error_.clear();
    this->raw_uri_.clear();
    this->query_string_.clear();
	this->headers_.clear();
	this->error_ = 0;
    this->location_ = LocationConfig(); 
    this->server_ = NULL;
}
#include "ServerConfig.hpp"
#include "parsingconf.hpp"

ServerConfig::ServerConfig() {
	autoindex_ = -1;
	client_max_body_size_ = 0;
	allowed_upload_ = -1;
	
}

const std::vector<Listen>&  ServerConfig::getListen() const {
	return (listen_);
}
	
const std::vector<std::string>&  ServerConfig::getServerName() const {
	return (server_name_);
}

const std::map<int, std::string>& ServerConfig::getErrorPage() const {
	return (error_page_);
}

const std::string& ServerConfig::getRoot() const {
	return (root_);
}

size_t ServerConfig::getClientMaxBodySize() const {
	return (client_max_body_size_);
}

const std::vector<std::string>& ServerConfig::getIndex() const {
	return (index_);
}

const std::pair<int, std::string>& ServerConfig::getReturn() const {
	return (return_);
}

const std::vector<LocationConfig>& ServerConfig::getLocations() const {
	return (locations_);
}

int ServerConfig::getAutoIndex() const {
	return (autoindex_);
}

LocationConfig&	ServerConfig::getLastLocation() {
	if (locations_.empty()) {
		throw std::runtime_error("Tentative d'accès à une location dans un serveur vide !");
	}
	return (locations_.back());
}

const std::set<std::string>&	ServerConfig::getAllowedMethods() const {
	return (allowed_methods_);
}

const std::string&	ServerConfig::getUploadPath() const {
	return (upload_path_);
}

int	ServerConfig::getAllowedUpload() const {
	return (allowed_upload_);
}

const std::map<std::string, std::string>&	ServerConfig::getCgiHandler() const {
	return (cgi_handler_);
}

void	ServerConfig::setRoot(const std::string& str) {
	this->root_ = str;
}

void	ServerConfig::setIndex(const std::vector<std::string>& index) {
	this->index_ = index;
}

void	ServerConfig::setAutoIndex(int allow) {
	autoindex_ = allow;
}

void	ServerConfig::setClientMaxBodySize(size_t size) {
	this->client_max_body_size_ = size;
}

void	ServerConfig::setServerName(const std::vector<std::string>& names) {
	server_name_ = names;
}

void	ServerConfig::setReturn(int code, const std::string& url) {
	return_.first = code;
	return_.second = url;
}

void	ServerConfig::setAllowedMethods(const std::set<std::string>& methods) {
	allowed_methods_ = methods;
}

void	ServerConfig::setUploadPath(const std::string& upload_path) {
	upload_path_ = upload_path;
}

void	ServerConfig::setAllowedUpload(int allow) {
	allowed_upload_ = allow;
}

void	ServerConfig::setCgiHandler(const std::string& ext, const std::string& path) {
	cgi_handler_[ext] = path;
}

void	ServerConfig::addLocation(const LocationConfig& loc) {
	locations_.push_back(loc);
}

void	ServerConfig::addListen(const std::string& ip, int port) {
	Listen newlisten;
	newlisten.ip = ip;
	newlisten.port = port;
	this->listen_.push_back(newlisten);
}

void	ServerConfig::addErrorPage(int code, const std::string& path) {
	error_page_[code] = path;
}

void ServerConfig::finalize() {

	if (this->root_.empty())
		this->root_ = "./src/www";

	if (this->autoindex_ == -1)
		this->autoindex_ = false;

	if (this->client_max_body_size_ == 0)
		this->client_max_body_size_ = 1000000;

	if (this->allowed_methods_.empty()) {
		this->allowed_methods_.insert("GET");
		// this->allowed_methods_.insert("POST"); a voir
	}

	if (this->allowed_upload_ == -1)
		this->allowed_upload_ = false;

	if (this->listen_.empty()) {
		Listen newLis;
		listen_.push_back(newLis);
	}

	if (this->index_.empty())
		index_.push_back("index.html");

	if (this->upload_path_.empty() && this->allowed_upload_ == true)
		this->allowed_upload_ = false; 

	// if (this->locations_.empty) a voir avec romane 

	for (size_t i = 0; i < locations_.size(); i++) {

		if (locations_[i].getRoot().empty())
			locations_[i].setRoot(combineRootUri(this->root_, locations_[i].getPath())[0]);

		if (locations_[i].getAutoIndex() == -1)
			locations_[i].setAutoIndex(this->autoindex_);

		if (locations_[i].getAllowedMethods().empty())
			locations_[i].setAllowedMethods(this->allowed_methods_);

		if (locations_[i].getAllowedUpload() == -1)
			locations_[i].setAllowedUpload(this->allowed_upload_);

		if (locations_[i].getUploadPath().empty()) {
			locations_[i].setUploadPath(this->upload_path_);
			if (locations_[i].getUploadPath().empty() && locations_[i].getAllowedUpload() == true)
				locations_[i].setAllowedUpload(false);
		}
		if (locations_[i].getIndex().empty())
			locations_[i].setIndex(this->index_);

		if (locations_[i].getErrorPage().empty() && !this->error_page_.empty()) {
			std::map<int, std::string>::const_iterator it;
			for (it = this->error_page_.begin(); it != this->error_page_.end(); it++) {
				locations_[i].addErrorPage(it->first, it->second);
			}
		}
		
		if (locations_[i].getClientMaxBodySize() == 0)
			locations_[i].setClientMaxBodySize(this->client_max_body_size_);
	}
}

std::ostream& operator<<(std::ostream &stream, const ServerConfig& srv) {
	if (!srv.getListen().empty()) {
		stream << "Listen : ";
		for (size_t i = 0; i < srv.getListen().size(); i++)
		{
			if (!srv.getListen()[i].ip.empty())
				stream << "IP:" << srv.getListen()[i].ip << " ";
			if (srv.getListen()[i].port >= 0 && srv.getListen()[i].port < 65536)
				stream << "Port:" << srv.getListen()[i].port;
			std::cout << " | ";
		}
		stream << std::endl;
	}

	if (!srv.getServerName().empty()) {
		stream << "Server name: ";
		for (size_t i = 0; i < srv.getServerName().size() ; i++)
		{
			stream << srv.getServerName()[i] << " ";
		}
		stream << std::endl;
	}

	stream << "Error pages: ";
	std::map<int, std::string>::const_iterator it;
	for (it = srv.getErrorPage().begin(); it != srv.getErrorPage().end(); it++) {
		stream << "Error:" << it->first << " Page:" << it->second << "  |  ";
	}
	stream << std::endl;

	stream << "Cgi: ";
	std::map<std::string, std::string>::const_iterator ite;
	for (ite = srv.getCgiHandler().begin(); ite != srv.getCgiHandler().end(); ite++) {
		stream << "Ext:" << ite->first << " Path:" << ite->second << "  |  ";
	}
	stream << std::endl;

	if (srv.getReturn().first != 0) { // On vconst_érifie si un code est défini
		stream << "Return: " << srv.getReturn().first;
		if (!srv.getReturn().second.empty()) {
			stream << " (" << srv.getReturn().second << ")";
		}
		stream << std::endl;
	}

	stream << "Auto index: " << srv.getAutoIndex() << std::endl;
	stream << "Client max body size: " << srv.getClientMaxBodySize() << std::endl;
	if (!srv.getRoot().empty())
		stream << "Root: " << srv.getRoot() << std::endl;
	if (!srv.getIndex().empty()) {
		stream << "Index: ";
		for (size_t i = 0; i < srv.getIndex().size(); i++)
		{
			stream << srv.getIndex()[i] << " ";
		}
		
	}
	stream << std::endl;
	if (!srv.getAllowedMethods().empty()) {
		stream << "Allowed methods: ";

		std::set<std::string>::const_iterator it;
		for (it = srv.getAllowedMethods().begin(); it != srv.getAllowedMethods().end(); it++)
		{
			stream << *it << " ";
		}
		std::cout << std::endl;
	}

	if (!srv.getLocations().empty()) {
		stream << std::endl;
		stream << std::endl;
		stream << "Locations: ";
		for (size_t i = 0; i < srv.getLocations().size(); i++)
		{
			stream << srv.getLocations()[i] << " ";
		}
	}
	
	
	return (stream);
}


//cherche la location par raport au path 
//int	Request::parsingHttp()
LocationConfig* ServerConfig::matchLocation(std::string requestPath) 
{
	std::cout << "test regdfh " << std::endl;
    LocationConfig* bestMatch = NULL;
    size_t longestLen = 0;
    std::vector<LocationConfig>::iterator it;

    for (it = locations_.begin(); it != locations_.end(); ++it)
    {
		std::string locPath = it->getPath();
		std::cout << "test " << std::endl;
        
        if (requestPath.find(locPath) == 0) 
        {
            if (locPath.length() > longestLen) 
            {
                longestLen = locPath.length();
                bestMatch = &(*it);
            }
        }
    }
	std::cout << "test 4 " << std::endl;
    return bestMatch;
}
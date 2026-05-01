#include "LocationConfig.hpp"


LocationConfig::LocationConfig() {
	autoindex_ = false;
	allowed_upload_ = false;
	client_max_body_size_ = 0;
}

const std::string&	LocationConfig::getPath() const {
	return (path_);
}

const std::string&	LocationConfig::getRoot() const {
	return (root_);
}

const std::vector<std::string>&	LocationConfig::getIndex() const {
	return (index_);
}

bool	LocationConfig::getAutoindex() const {
	return (autoindex_);
}

const std::pair<int, std::string>&	LocationConfig::getReturn() const {
	return (return_);
}

const std::vector<std::string>&	LocationConfig::getAllowedMethods() const {
	return (allowed_methods_);
}

bool	LocationConfig::getAllowedUpload() const {
	return (allowed_upload_);
}

const std::string&	LocationConfig::getUploadPath() const {
	return (upload_path_);
}


size_t	LocationConfig::getClientMaxBodySize() const {
	return (client_max_body_size_);
}

const std::map<std::string, std::string>&	LocationConfig::getCgi() const {
	return (cgi_);
}

const std::map<int, std::string>&	LocationConfig::getErrorPage() const {
	return (error_page_);
}

void	LocationConfig::setPath(const std::string& path_loc) {
	path_ = path_loc;
}

void	LocationConfig::setRoot(const std::string& str) {
	this->root_ = str;
}

void	LocationConfig::setIndex(const std::vector<std::string>& index) {
	this->index_ = index;
}

void	LocationConfig::setClientMaxBodySize(size_t size) {
	this->client_max_body_size_ = size;
}


void	LocationConfig::setAllowedUpload(bool allow) {
	allowed_upload_ = allow;
}

void	LocationConfig::setAutoIndex(bool allow) {
	autoindex_ = allow;
}

void	LocationConfig::setUploadPath(const std::string& upload_path) {
	upload_path_ = upload_path;
}

void	LocationConfig::setAllowedMethods(const std::vector<std::string>& methods) {
	allowed_methods_ = methods;
}

void	LocationConfig::setReturn(int code, const std::string& url) {
	return_.first = code;
	return_.second = url;
}

void	LocationConfig::addErrorPage(int code, const std::string& path) {
	error_page_[code] = path;
}

std::ostream& operator<<(std::ostream &stream, const LocationConfig& loc) {
	std::cout << std::endl;

	stream << "Path: " << loc.getPath() << std::endl;

	stream << "Error pages: ";
	std::map<int, std::string>::const_iterator it;
	for (it = loc.getErrorPage().begin(); it != loc.getErrorPage().end(); it++) {
		stream << "Error:" << it->first << " Page:" << it->second << "  |  ";
	}
	stream << std::endl;

	if (loc.getReturn().first != 0) { // On vérifie si un code est défini
		stream << "Return: " << loc.getReturn().first;
		if (!loc.getReturn().second.empty()) {
			stream << " (" << loc.getReturn().second << ")";
		}
		stream << std::endl;
	}

	stream << "Auto index: " << loc.getAutoindex() << std::endl;
	stream << "Client max body size: " << loc.getClientMaxBodySize() << std::endl;
	if (!loc.getRoot().empty())
		stream << "Root: " << loc.getRoot() << std::endl;

	if (!loc.getIndex().empty()) {
		stream << "Index: ";
		for (size_t i = 0; i < loc.getIndex().size(); i++)
		{
			stream << loc.getIndex()[i] << " ";
		}
		std::cout << std::endl;
	}
	
	if (!loc.getAllowedMethods().empty()) {
		stream << "Allowed methods: ";
		for (size_t i = 0; i < loc.getAllowedMethods().size(); i++)
		{
			stream << loc.getAllowedMethods()[i] << " ";
		}
		std::cout << std::endl;
	}
	
	stream << "Allowed upload: " << loc.getAllowedUpload() << std::endl;

	if (!loc.getUploadPath().empty())
		stream << "Upload path: " << loc.getUploadPath() << std::endl;
	return stream;
}
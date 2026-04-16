#include "LocationConfig.hpp"

LocationConfig::LocationConfig() {
	autoindex_ = false;
	allowed_upload_ = false;
	client_max_body_size_ = 0;
}

const std::string&	LocationConfig::getUri() const {
	return (uri_);
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
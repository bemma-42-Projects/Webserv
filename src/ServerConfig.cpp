#include "ServerConfig.hpp"

ServerConfig::ServerConfig() {
	autoindex_ = false;
	client_max_body_size_ = 1048576;
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

bool ServerConfig::getAutoindex() const {
	return (autoindex_);
}
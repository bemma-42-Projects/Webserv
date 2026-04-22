#include "ServerConfig.hpp"

ServerConfig::ServerConfig() {
	autoindex_ = false;
	client_max_body_size_ = 1048576;
	listen_[0].ip = "0.0.0.0";
	listen_[0].port = 80;
	root_ = "";
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

void	ServerConfig::setRoot(const std::string& str) {
	this->root_ = str;
}


std::ostream& operator<<(std::ostream &stream, const ServerConfig& srv) {
	stream << "Listen :";
	for (size_t i = 0; i < srv.getListen().size(); i++)
	{
		stream << std::endl;
		if (!srv.getListen()[i].ip.empty())
			stream << "IP:" << srv.getListen()[i].ip << " ";
		if (srv.getListen()[i].port >= 0 && srv.getListen()[i].port < 65536)
			stream << "Port:" << srv.getListen()[i].port;
	}

	stream << std::endl;
	stream << "Server name: ";
	for (size_t i = 0; i < srv.getServerName().size(); i++)
	{
		stream << srv.getServerName()[i] << " ";
	}
	stream << std::endl;
	// stream << "Error page: ";
	// for (size_t i = 0; i < srv.getErrorPage().size(); i++)
	// {
	// 	stream << "Error:" << srv.getErrorPage()[i].
	// }
}
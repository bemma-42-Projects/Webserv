#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <map>
#include <utility>

#include "LocationConfig.hpp"

struct Listen {
	std::string	ip;
	int			port;
	Listen() : ip("0.0.0.0"), port(80) {}
};

class ServerConfig {
public:

	ServerConfig();
	const std::vector<Listen>&				getListen() const;
	const std::vector<std::string>&			getServerName() const;
	const std::map<int, std::string>&		getErrorPage() const;
	const std::string&						getRoot() const;
	size_t									getClientMaxBodySize() const;
	const std::vector<std::string>&			getIndex() const;
	const std::pair<int, std::string>&		getReturn() const;
	const std::vector<LocationConfig>&		getLocations() const;
	bool									getAutoindex() const;

private:
	std::vector<Listen>				listen_;
	std::vector<std::string>		server_name_;
	std::map<int, std::string>		error_page_;
	std::string						root_;
	size_t							client_max_body_size_;
	std::vector<std::string>		index_;
	std::pair<int, std::string>		return_;
	std::vector<LocationConfig>		locations_;
	bool							autoindex_;

};


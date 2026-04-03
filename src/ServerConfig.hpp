#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <map>

#include "LocationConfig.hpp"

class ServerConfig {

	std::vector<std::string> listen;
	std::vector<std::string> server_name;
	std::map<int, std::string> error_page;
	std::string root;
	size_t client_max_body_size;
	std::vector<std::string> index;
	std::map<int, std::string> return_;
	std::vector<LocationConfig> locations;
	bool autoindex;
};
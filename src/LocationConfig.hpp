#pragma once


#include <string>
#include <vector>
#include <iostream>
#include <map>

class LocationConfig {
	std::vector<LocationConfig> locations;
	std::string uri;
	std::string root;
	std::vector<std::string> index;
	bool autoindex;
	std::vector<std::string> return_;
	std::vector<std::string> allowed_methods;
};
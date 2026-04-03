#pragma once


#include <string>
#include <vector>
#include <iostream>
#include <map>

class LocationConfig {

	std::string uri;
	std::string root;
	std::vector<std::string> index;
	bool autoindex;
	std::map<int, std::string> return_;
	std::vector<std::string> allowed_methods;
	bool allowed_upload;
	std::string upload_path;
};

//il manquerait config cgi
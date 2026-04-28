#pragma once
#include <string>
#include <vector>
#include <map>

class Location {


public:
	Location();
	Location(std::string path, std::string root, 
		std::string upload_path, std::vector<std::string> index, 
		bool autoindex, std::vector<std::string> methods);
	~Location();
	std::vector<std::string>	getIndex();
	bool						getAutoindex();
	std::string					getRoot();
	std::string					getPath();
	std::vector<std::string>	getAllowedMethods();
	std::map<int, std::string>	getError();

private:

    std::string					path_;            // "/downloads"
    std::string					root_;            // "./data"
    std::string					upload_path_;    // "./data/tmp"
    std::vector<std::string>	index_;           // "secret_list.html"
    bool						autoindex_;       // true
    std::vector<std::string>	allowed_methods_; // ["GET", "POST"]
	std::map<int, std::string>	error_;
};
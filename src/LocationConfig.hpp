#pragma once


#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <utility>
#include "ServerConfig.hpp"

class ServerConfig;

class LocationConfig {
public:

	LocationConfig();
	LocationConfig(ServerConfig conf);
	const std::string&							getPath() const;
	const std::string&							getRoot() const;
	const std::vector<std::string>&				getIndex() const;
	bool										getAutoindex() const;
	const std::pair<int, std::string>&			getReturn() const;
	const std::vector<std::string>&				getAllowedMethods() const;
	bool										getAllowedUpload() const;
	const std::string&							getUploadPath() const;
	const std::map<std::string, std::string>&	getCgi() const;
	size_t										getClientMaxBodySize() const;
	const std::map<int, std::string>&			getErrorPage() const;

	void										setPath(const std::string& path_loc);
	void										setRootLoc(const std::string& str);
	void										setIndex(const std::vector<std::string>& index);
	void										setClientMaxBodySize(size_t size);
	void										setAllowedUpload(bool allow);
	void										setAutoIndex(bool allow);
	void										setUploadPath(const std::string& upload_path);
	void										setAllowedMethods(const std::vector<std::string>& methods);
	void										setReturn(int code, const std::string& url);

	void										addErrorPage(int code, const std::string& path);
private:
	std::string							path_;
	std::string							root_;
	std::vector<std::string>			index_;
	bool								autoindex_;
	std::pair<int, std::string>			return_;
	std::vector<std::string>			allowed_methods_;
	bool								allowed_upload_;
	std::string							upload_path_;
	std::map<std::string, std::string>	cgi_;
	size_t								client_max_body_size_;
	std::map<int, std::string>			error_page_;
};

std::ostream& operator<<(std::ostream &stream, const LocationConfig& loc);
//il manquerait config cgi
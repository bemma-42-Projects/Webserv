#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <utility>
#include <set>

#include "ServerConfig.hpp"

class	ServerConfig;

class	LocationConfig {
public:

	LocationConfig();
	const std::string&							getPath() const;
	const std::string&							getRoot() const;
	const std::vector<std::string>&				getIndex() const;
	int											getAutoIndex() const;
	const std::pair<int, std::string>&			getReturn() const;
	const std::set<std::string>&				getAllowedMethods() const;
	int											getAllowedUpload() const;
	const std::string&							getUploadPath() const;
	size_t										getClientMaxBodySize() const;
	const std::map<int, std::string>&			getErrorPage() const;
	const std::map<std::string, std::string>&	getCgiHandler() const;
	const std::string&							getAlias() const;

	void										setPath(const std::string& path_loc);
	void										setRoot(const std::string& str);
	void										setIndex(const std::vector<std::string>& index);
	void										setClientMaxBodySize(size_t size);
	void										setAllowedUpload(int allow);
	void										setAutoIndex(int allow);
	void										setUploadPath(const std::string& upload_path);
	void										setAllowedMethods(const std::set<std::string>& methods);
	void										setReturn(int code, const std::string& url);
	void										setCgiHandler(const std::string& ext, const std::string& path);
	void										setAlias(const std::string& str);

	void										addErrorPage(int code, const std::string& path);
private:
	std::string							path_;
	std::string							root_;
	std::string							alias_;
	std::vector<std::string>			index_;
	int									autoindex_;
	std::pair<int, std::string>			return_;
	std::set<std::string>				allowed_methods_;
	int									allowed_upload_;
	std::string							upload_path_;
	size_t								client_max_body_size_;
	std::map<int, std::string>			error_page_;
	std::map<std::string, std::string>	cgi_handler_;
};

std::ostream&	operator<<(std::ostream &stream, const LocationConfig& loc);

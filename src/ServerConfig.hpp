#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <ostream>
#include <map>
#include <utility>
#include <set>

#include "LocationConfig.hpp"

class	LocationConfig;

struct	Listen {
	std::string	ip;
	int			port;
	Listen() : ip("0.0.0.0"), port(80) {}
};


class	ServerConfig {
public:

	ServerConfig();
	const std::vector<Listen>&							getListen() const;
	const std::vector<std::string>&						getServerName() const;
	const std::map<int, std::string>&					getErrorPage() const;
	const std::string&									getRoot() const;
	size_t												getClientMaxBodySize() const;
	const std::vector<std::string>&						getIndex() const;
	const std::pair<int, std::string>&					getReturn() const;
	const std::vector<LocationConfig>&					getLocations() const;
	int													getAutoIndex() const;
	LocationConfig&										getLastLocation();
	const std::set<std::string>&						getAllowedMethods() const;
	const std::string&									getUploadPath() const;
	int													getAllowedUpload() const;
	const std::map<std::string, std::string>&			getCgiHandler() const;

	void									setRoot(const std::string& str);
	void									setIndex(const std::vector<std::string>& index);
	void									setClientMaxBodySize(size_t size);
	void									setAutoIndex(int allow);
	void									setServerName(const std::vector<std::string>& names);
	void									setReturn(int code, const std::string& url);
	void									setAllowedMethods(const std::set<std::string>& methods);
	void									setUploadPath(const std::string& upload_path);
	void									setAllowedUpload(int allow);
	void									setCgiHandler(const std::string& ext, const std::string& path);

	void									addLocation(const LocationConfig& loc);
	void									addListen(const std::string& ip, int port);
	void									addErrorPage(int code, const std::string& path);

	const LocationConfig 					*matchLocation(std::string requestPath) const;
	void									finalize();

private:
	std::vector<Listen>					listen_;
	std::vector<std::string>			server_name_;
	std::map<int, std::string>			error_page_;
	std::string							root_;
	size_t								client_max_body_size_;
	std::vector<std::string>			index_;
	std::set<std::string>				allowed_methods_;
	int									allowed_upload_;
	std::string							upload_path_;
	std::pair<int, std::string>			return_;
	std::vector<LocationConfig>			locations_;
	int									autoindex_;
	std::map<std::string, std::string>	cgi_handler_;

};

std::ostream&	operator<<(std::ostream &stream, const ServerConfig& srv);


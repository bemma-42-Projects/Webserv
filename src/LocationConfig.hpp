#pragma once


#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <utility>

class LocationConfig {
public:

	LocationConfig();
	const std::string&							getUri() const;
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

private:
	std::string							uri_;
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

//il manquerait config cgi
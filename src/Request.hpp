#pragma once
#include <string>
#include <map>
#include "ServerConfig.hpp"
#include <utility>

enum	ParsingStatus {
	PARSING_FAILED = 0,
	PARSING_SUCCESS = 1,
	PARSING_INCOMPLETE = 2
};

class Request {

	public:
		Request();
		Request(char *buffer, ServerConfig& server);
		~Request();
		std::string 						getRequest() const;
		std::string 						getMethod() const;
		std::string 						getPath() const;
		std::string 						getUrlPath() const;
		std::string 						getVersion() const;
		std::map<std::string, std::string>	getHeaders() const;
		std::string 						getBody() const;
		int									getError() const;
		std::string 						getErrorMessage() const;
		const LocationConfig				&getLocation() const;
		const ServerConfig*					getServer() const;
		std::string							getClientIP() const;
		//int								requestHttp();							
		ParsingStatus						parsingHttp(const std::string &raw_data);
		bool								complete();
		int									initFistLine();
		int									initHeader();
		int									initBody();
		int									checkOfLocation();
		void								setServerConfig(const ServerConfig *server);
		const LocationConfig				*matchExtensionLocation() const;
		//void								setError(int error);

		//std::string						answer();
		//std::string						methodGet();
		void								splitUri_();
		std::string							getRequestUri() const;
		std::string							getQueryString() const;
		std::string							getContentType() const;
		std::string							getContentLength() const;
		std::string							getHost() const;
		std::string							getPort() const;
		void								setClientIP(const std::string &ip);
		void								clear();

	private:
		std::string							request_;
		std::string							method_;
		std::string							path_;
		std::string							url_path_;
		std::string							version_;
		std::map<std::string, std::string>	headers_;
		std::string							body_;
		int									error_;
		std::string							message_error_;
		LocationConfig						location_;
		std::string							raw_uri_;
		std::string							query_string_;
		std::string							client_ip_;
		const ServerConfig*					server_;
		std::pair<int, std::string>			return_;

};

std::ostream& operator<<(std::ostream& out, const Request& request);
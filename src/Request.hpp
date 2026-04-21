#pragma once
#include <string>
#include <map>
#include "Config.hpp"
#include "Location.hpp"

class Request {

	public:
		Request();
		Request(char *buffer);
		~Request();
		std::string 						getRequest() const;
		std::string 						getMethod() const;
		std::string 						getPath() const;
		std::string 						getUrlPath() const;
		std::string 						getVersion() const;
		std::map<std::string, std::string>	getHeaders() const;
		std::string 						getBody() const;
		int									getError() const;
		Location							getLocation() const;
		//int									requestHttp();							
		int									parsingHttp(const std::string &raw_data);
		bool								complete();
		int									initFistLine();
		int									initHeader();
		int									initBody();
		int									checkOfLocation();
		//void								setError(int error);

		//std::string							answer();
		//std::string							methodGet();
		void								splitUri_();
		std::string							getRequestUri() const;
		std::string							getQueryString() const;
		std::string							getContentType() const;
		std::string							getContentLength() const;
		std::string							getHost() const;
		std::string							getPort() const;
		void								setClientIP(const std::string &ip);
		std::string							getClientIP() const;

	private:
		std::string							request_;
		std::string							method_;
		std::string							path_;
		std::string							url_path_;
		std::string							version_;
		std::map<std::string, std::string>	headers_;
		std::string							body_;
		int									error_;
		Location							location_;
		std::string							raw_uri_;
		std::string							query_string_;
		std::string							client_ip_;
};

std::ostream& operator<<(std::ostream& out, const Request& request);
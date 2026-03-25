#pragma once
#include <string>
#include <map>

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
		//int									requestHttp();							
		int									parsingHttp();
		bool								complete();
		int									initFistLine();
		int									initHeader();
		int									initBody();
		//std::string							answer();
		//std::string							methodGet();

	private:
		std::string							request_;
		std::string							method_;
		std::string							path_;
		std::string							url_path_;
		std::string							version_;
		std::map<std::string, std::string>	headers_;
		std::string							body_;
		int									error_;
};

std::ostream& operator<<(std::ostream& out, const Request& request);
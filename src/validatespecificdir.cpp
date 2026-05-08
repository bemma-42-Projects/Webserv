
#include "parsingconf.hpp"

//fais les check pour le port ex:8080
int validatePort(std::string port_str) {
	if (port_str.empty())
		throw std::runtime_error("port is empty");

	for (size_t i = 0; i < port_str.size(); i++) {
		if (!isdigit(port_str[i]))
			throw std::runtime_error("invalid port: '" + port_str + "' contains non-digits");
	}
	char *end;
	long port = strtol(port_str.c_str(), &end, 10 );
	if (port >= 0 && port <= 65535)
		return (port);
	throw std::runtime_error("port out of range");
}

//fais tous les check concernant l'ip donc soit localhost soit qqch comme 127.0.0.1
void validateIP(std::string str) {
	if (str == "localhost")
		return ;

	size_t i = 0;
	int count = 0;

	while (i < str.size()) {
		size_t start = i;
		while (i < str.size() && isdigit(str[i]))
			i++;

		if (start == i)
			throw std::runtime_error("invalid host: empty octet in IP '" + str + "'");

		std::string number(str, start, i - start);
		if (number.size() > 3)
			throw std::runtime_error("invalid host: octet '" + number + "' is too long");

		int val = atoi(number.c_str());
		if (val < 0 || val > 255)
			throw std::runtime_error("invalid host: octet '" + number + "' is out of range (0-255)");
		
		if (i < str.size()) {
			if (str[i] != '.') 
				throw std::runtime_error("invalid host: unexpected character '" + std::string(1, str[i]) + "' in IP");
			count++;
			i++;
			if (i == str.size())
				throw std::runtime_error("invalid host: IP address cannot end with a dot");
		}
		
	}
	if (count != 3)
		throw std::runtime_error("invalid host: '" + str + "' is not a valid IPv4 address");
}

//pour listen valide la premiere ip adresse
void validateOneArg(std::string str, ServerConfig& srv) {
	if (str.empty())	
		throw std::runtime_error("listen: empty argument");
	size_t pos = str.find(':');
	if (pos == str.npos) { // le cas ou il y a que l'IP ou que le port

		if (str.find('.') != std::string::npos || str == "localhost") {
			validateIP(str);

			if (str == "localhost")
				str = "127.0.0.1";
			srv.addListen(str, 80);
			return ;
		}

		int prt = validatePort(str);
		if (prt != -1) {
			srv.addListen("0.0.0.0", prt);
			return ;
		}
	}

	std::string ip_str = str.substr(0, pos);
	validateIP(ip_str);
	if (ip_str == "localhost")
		ip_str = "127.0.0.1";

	std::string port_str = str.substr(pos + 1);
	if (port_str.empty())
		throw std::runtime_error("listen: missing port after ':' in '" + str + "'");
	int portres = validatePort(port_str);
	srv.addListen(ip_str, portres);

}

void validateListen(std::vector<std::string> args, State state, ServerConfig& srv) {
	if (args.size() < 1 || args.size() > 1)
		throw std::runtime_error("directive 'listen' requires exactly 1 argument");
	if (state != IN_SERVER)
		throw std::runtime_error("directive 'listen' is only allowed in server block");
	validateOneArg(args[0], srv);
}

std::string combineRootUri(std::string root, std::string uri) {
	std::string res;
	if (root.empty())
		return (res);
	if (uri.empty())
		return (res);

	// Si le root finit par un slash, on l'enlève pour éviter le double slash avec l'URI
	if (root[root.size() - 1] == '/') {
		root.erase(root.size() - 1);
	}

	// Si l'URI ne commence pas par un slash, on en ajoute un
	if (uri[0] != '/') {
		uri = "/" + uri;
	}
	res = root + uri;
	return (res);
}

void validateRoot(std::vector<std::string> args) {
	if (args.size() != 1)
		throw std::runtime_error("directive 'root' requires exactly 1 argument");

	if (args[0].empty())
		throw std::runtime_error("directive 'root' has an empty argument");

	if (access(args[0].c_str(), F_OK) == -1)
		throw std::runtime_error("root '" + args[0] + "': path does not exist");

	if (access(args[0].c_str(), R_OK | X_OK) == -1)
		throw std::runtime_error("root '" + args[0] + "': permission denied (read/execute required)");

	struct stat sb;
	if (stat(args[0].c_str(), &sb) == -1)
		throw std::runtime_error("root '" + args[0] + "': failed to get file status (stat)");

	if (!S_ISDIR(sb.st_mode))
		throw std::runtime_error("root '" + args[0] + "': is not a directory");

}

void validateClientMaxBodySize(std::vector<std::string> args) {
	if (args.size() != 1)
		throw std::runtime_error("directive 'client_max_body_size' requires exactly 1 argument");

	if (args[0].empty())
		throw std::runtime_error("directive 'client_max_body_size' is empty");

	size_t i = 0;
	while (i < args[0].size() && isdigit(args[0][i]))
		i++;
	if (!isdigit(args[0][i]) && i < args[0].size())
		throw std::runtime_error("client_max_body_size: must be a digit");

	// if (i < args[0].size())
	// 	throw std::runtime_error("client_max_body_size: '" + s + "' must start with a number");
	// 	// return (false);

	// if (i < s.size()) {
	// 	if (i != s.size() - 1)
	// 		throw std::runtime_error("client_max_body_size: invalid format '" + s + "'");

	// 	char unit = toupper(s[i]);
	// 	if (unit != 'K' && unit != 'M' && unit != 'G')
	// 		throw std::runtime_error("client_max_body_size: unknown unit '" + std::string(1, s[i]) + "' (use K, M, or G)");
	// }
	long long val = std::atoll(args[0].c_str());
	if (val < 0)
		throw std::runtime_error("client_max_body_size: value must be positive");
	if (val > 2147483647)
		throw std::runtime_error("client_max_body_size: value is too large");

}

bool isErrorCode(std::string code) {
	if (code == "400" || code == "403" || code == "404" || code == "405" || code == "413" || code == "500"
			|| code == "501" || code == "502" || code == "503" || code == "504" || code == "413" || code == "414" || code == "408")
		return (true);
	return (false);
}

void validateErrorPage(std::vector<std::string> args) {
	if (args.size() < 2)
		throw std::runtime_error("directive 'error_page' requires at least 2 arguments (code and path)");


	for (size_t i = 0; i < args.size() - 1; i++) {
		if (isErrorCode(args[i]) == false)
			throw std::runtime_error("error_page: '" + args[i] + "' is not a valid HTTP error code (300-599)");
	}

	if (args.back().empty())
		throw std::runtime_error("error_page: path is empty");

	if (access(args.back().c_str(), F_OK) == -1)
		throw std::runtime_error("error_page path '" + args.back() + "': does not exist");

	if (access(args.back().c_str(), R_OK) == -1)
		throw std::runtime_error("error_page path '" + args.back() + "': permission denied (read required)");

	struct stat sb;
	if (stat(args.back().c_str(), &sb) == -1)
		throw std::runtime_error("error_page path '" + args.back() + "': stat failed");

	if (!S_ISREG(sb.st_mode))
		throw std::runtime_error("error_page path '" + args.back() + "': is not a regular file");

}

void isValidUrl(const std::string& url) {
	if (url.empty())
		throw std::runtime_error("return: URL or message is empty");

	if (url[0] == '/')
		return ;

	if (url.find("http://") == 0 && url.length() > 7)
		return ;

	if (url.find("https://") == 0 && url.length() > 8)
		return ;

	throw std::runtime_error("return: '" + url + "' is not a valid URL (must start with / or http)");
}

void validateReturn(std::vector<std::string> args) {
	if (args.size() < 1 || args.size() > 2)
		throw std::runtime_error("directive 'return' requires 1 or 2 arguments");

	if (args.size() == 1) {
		if (args[0] == "200" || args[0] == "201" || args[0] == "204" || args[0] == "301" 
			|| args[0] == "302" || isErrorCode(args[0]) == true)
		{
			if (args[0] == "301" || args[0] == "302")
				throw std::runtime_error("return: status " + args[0] + " requires a redirection URL");

			return ;
		}
		isValidUrl(args[0]);
	}
	else if (args.size() == 2) {
		if (args[0] != "200" && args[0] != "201" && args[0] != "204" && args[0] != "301" 
			&& args[0] != "302" && isErrorCode(args[0]) == false)
			throw std::runtime_error("return: first argument '" + args[0] + "' must be a valid status code");

		if (args[0] == "301" || args[0] == "302") {
			isValidUrl(args[1]);
		}
		else {
			if (args[1].empty())
				throw std::runtime_error("return: second argument (body/url) cannot be empty");
		}
		
	}
	
}

void  validateIndex(std::vector<std::string> args) {
	if (args.size() < 1)
		throw std::runtime_error("directive 'index' requires at least one argument");

	for (size_t i = 0; i < args.size(); i++) {
		if (args[i].empty())
			throw std::runtime_error("index: empty argument found");
		if (args[i][0] == '/' && (i + 1) != args.size())
			throw std::runtime_error("index: '" + args[i] + "' is an absolute path and must be the last argument");

		if (args[i][args[i].size() - 1] == '/') {
			throw std::runtime_error("index: '" + args[i] + "' cannot be a directory (must be a file)");
		}
	}
}

void validateAutoIndex(std::vector<std::string> args, State state, ServerConfig& srv) {
	if (args.size() != 1)
		throw std::runtime_error("directive 'autoindex' requires exactly 1 argument (on/off)");

	bool value;
	if (args[0] == "on")
		value = true;
	else if (args[0] == "off")
		value = false;
	else
		throw std::runtime_error("autoindex: invalid value '" + args[0] + "' (must be 'on' or 'off')");

	if (state == IN_SERVER)
		srv.setAutoIndex(value);

	else if (state == IN_LOCATION) {
		if (srv.getLocations().empty())
			throw std::runtime_error("autoindex: no location context found");
		srv.getLastLocation().setAutoIndex(value);
	}
	else
		throw std::runtime_error("autoindex: directive is not allowed in this context");
}

void validateAllowedMethods(std::vector<std::string> args, ServerConfig& srv, State state) {
	if (args.empty())
		throw std::runtime_error("directive 'allowed_methods' is empty");
	if (args.size() > 3)
		throw std::runtime_error("directive 'allowed_methods' has too many arguments (max 3: GET, POST, DELETE)");
	for (size_t i = 0; i < args.size(); i++) {
		if (args[i] != "GET" && args[i] != "POST" && args[i] != "DELETE")
			throw std::runtime_error("allowed_methods: unknown method '" + args[i] + "' (only GET, POST, DELETE are supported)");
	}
	std::set<std::string> setMethods(args.begin(), args.end());
	if (state == IN_SERVER)
		srv.setAllowedMethods(setMethods);

	else if (state == IN_LOCATION) {
		if (srv.getLocations().empty())
			throw std::runtime_error("allowed_methods: no location context found");
		srv.getLastLocation().setAllowedMethods(setMethods);
	}
	else
		throw std::runtime_error("allowed_methods: directive not allowed in this context");
}

void validateAllowedUpload(std::vector<std::string> args, ServerConfig& srv, State state) {
	if (args.size() != 1)
		throw std::runtime_error("directive 'allowed_upload' requires exactly 1 argument (on/off)");
	bool value;
	if (args[0] == "on")
		value = true;
	else if (args[0] == "off")
		value = false;
	else
		throw std::runtime_error("allowed_upload: invalid value '" + args[0] + "' (must be 'on' or 'off')");

	if (state == IN_SERVER)
		srv.setAllowedUpload(value);

	else if (state == IN_LOCATION) {
		if (srv.getLocations().empty()) 
			throw std::runtime_error("allowed_upload: no location context found");
		
		srv.getLastLocation().setAllowedUpload(value);
	}
	else 
		throw std::runtime_error("allowed_upload: directive not allowed in this context");
}

void validateUploadPath(std::vector<std::string> args,ServerConfig& srv, State state) {
	if (args.size() != 1)
		throw std::runtime_error("directive 'upload_path' requires exactly 1 argument");

	if (args[0].empty())
		throw std::runtime_error("upload_path: argument is empty");

	if (access(args[0].c_str(), F_OK) == -1)
		throw std::runtime_error("upload_path '" + args[0] + "': directory does not exist");

	if (access(args[0].c_str(), W_OK) == -1)
		throw std::runtime_error("upload_path '" + args[0] + "': permission denied (write access required)");

	struct stat sb;
	if (stat(args[0].c_str(), &sb) == -1)
		throw std::runtime_error("upload_path '" + args[0] + "': stat failed");

	if (!S_ISDIR(sb.st_mode))
		throw std::runtime_error("upload_path '" + args[0] + "': is not a directory");

	if (state == IN_SERVER) 
		srv.setUploadPath(args[0]);

	else if (state == IN_LOCATION) {
		if (srv.getLocations().empty()) 
			throw std::runtime_error("upload_path: no location context found");

		srv.getLastLocation().setUploadPath(args[0]);
	}
	else 
		throw std::runtime_error("upload_path: directive not allowed in this context");
}

void validateServerName(std::vector<std::string> args) {
	if (args.empty())
		throw std::runtime_error("directive 'server_name' requires at least one argument");

	for (size_t i = 0; i < args.size(); i++) {
		if (args[i].empty()) 
			throw std::runtime_error("server_name: one of the names is empty");

		for (size_t j = 0; j < args[i].size(); j++) {
			if (!isalnum(args[i][j]) && args[i][j] != '.' && args[i][j] != '-' && args[i][j] != '_') {
				std::string err = "server_name: invalid character '";
				err += args[i][j];
				err += "' found in '";
				err += args[i];
				err += "'";
				throw std::runtime_error(err);

			}
		}
	}
}

void validateCgi(std::vector<std::string> args, ServerConfig& srv, State state) {
	if (args.size() != 2)
		throw std::runtime_error("directive 'cgi' requires exactly 2 arguments (extension and executable path)");

	if (args[0].empty() || args[0][0] != '.')
		throw std::runtime_error("cgi: extension '" + args[0] + "' must start with a dot (e.g., .php)");

	struct stat sb;
	if (stat(args[1].c_str(), &sb) == -1)
		throw std::runtime_error("cgi executable '" + args[1] + "': does not exist");

	if (!(sb.st_mode & S_IXUSR))
		throw std::runtime_error("cgi executable '" + args[1] + "': permission denied (execution bit not set)");

	if (!S_ISREG(sb.st_mode))
		throw std::runtime_error("cgi executable '" + args[1] + "': is not a regular file");
	
	if (state == IN_SERVER) 
		srv.setCgiHandler(args[0], args[1]);

	else if (state == IN_LOCATION) {
		if (srv.getLocations().empty()) 
			throw std::runtime_error("cgi: no location context found");
		srv.getLastLocation().setCgiHandler(args[0], args[1]);
	}
	else 
		throw std::runtime_error("cgi: directive not allowed in this context");
}

bool validateSpecificDirective(std::string name, std::vector<std::string> args, State state, ServerConfig& srv) {
	
	try {
		if (name == "listen") {
			validateListen(args, state, srv);
			return (true);
		}

		else if (name == "root") {
			validateRoot(args);

			if (state == IN_SERVER) {
				srv.setRoot(args[0]);
				return (true);
			}
			else if (state == IN_LOCATION) {
				if (srv.getLocations().empty()) {
					return (false);
				}
				srv.getLastLocation().setRoot(args[0]);
				return (true);
			}
		}

		else if (name == "server_name") {
			validateServerName(args);
			srv.setServerName(args);
			return (true);
		}

		else if (name == "client_max_body_size") {
			validateClientMaxBodySize(args);
			char *end;
			size_t size = strtol(args[0].c_str(), &end, 10);
			if (state == IN_SERVER) {
				srv.setClientMaxBodySize(size);
				return (true);
			}
			else if (state == IN_LOCATION) {
				if (srv.getLocations().empty())
					return (false);
				srv.getLastLocation().setClientMaxBodySize(size);
				return (true);
			}
			return (false);
		}

		else if (name == "error_page") {
			validateErrorPage(args);
			std::string path = args.back();
			for (size_t i = 0; i < args.size() - 1; i++) {
				int code = std::atoi(args[i].c_str());
				if (state == IN_SERVER) 
					srv.addErrorPage(code, path);
				else if (state == IN_LOCATION && !srv.getLocations().empty())
					srv.getLastLocation().addErrorPage(code, path);
				else
					return (false);
			}
			return (true);
		}

		else if (name == "index") {
			validateIndex(args);
			if (state == IN_SERVER) {
				srv.setIndex(args);
				return (true);
			}
			else if (state == IN_LOCATION && !srv.getLocations().empty()) {
				srv.getLastLocation().setIndex(args);
				return (true);
			}
		}

		else if (name == "return") {
			validateReturn(args);
			int code = 0;
			std::string url = "";
			if (args.size() == 1) {
				if (args[0] == "200" || args[0] == "201" || args[0] == "204" || args[0] == "301" 
						|| args[0] == "302" || isErrorCode(args[0]) == true)
					code = std::atoi(args[0].c_str());
				else {
					code = 302;
					url = args[0];
				}
			}
			else if (args.size() == 2) {
				code = std::atoi(args[0].c_str());
				url = args[1];
			}
			if (state == IN_SERVER) {
				srv.setReturn(code, url);
				return (true);
			}
			else if (state == IN_LOCATION && !srv.getLocations().empty()) {
				srv.getLastLocation().setReturn(code, url);
				return (true);
			}
		}

		else if (name == "autoindex") {
			validateAutoIndex(args,state, srv);
			return (true); 
		}

		else if (name == "allowed_methods") {
			validateAllowedMethods(args, srv, state);
			return (true);
		}

		else if (name == "allowed_upload") {
			validateAllowedUpload(args, srv, state);
			return (true);
		}

		else if (name == "upload_path") {
			validateUploadPath(args, srv, state);
			return (true);
		}

		else if (name == "cgi") {
			validateCgi(args, srv, state);
			return (true);
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Configuration Error: " << e.what() << std::endl;
		return (false);
	}
	return (false);
}

#include "parsingconf.hpp"

int validatePort(std::string port_str) {
	if (port_str.empty())
		return (-1);

	for (size_t i = 0; i < port_str.size(); i++) {
		if (!isdigit(port_str[i]))
			return (-1);
	}
	char *end;
	long port = strtol(port_str.c_str(), &end, 10 );
	if (port >= 0 && port <= 65535)
		return (port);
	return (-1);
}


bool validateIP(std::string str) {
	if (str == "localhost")
		return (true);

	size_t i = 0;
	int count = 0;

	while (i < str.size()) {
		size_t start = i;
		while (i < str.size() && isdigit(str[i]))
			i++;
		if (start == i)
			return (false);
		std::string number(str, start, i - start);
		if (number.size() > 3)
			return (false);
		int val = atoi(number.c_str());
		if (val < 0 || val > 255)
			return (false);
		
		if (i < str.size()) {
			if (str[i] != '.') 
				return (false);
			count++;
			i++;
			if (i == str.size())
				return (false);
		}
		
	}
	if (count != 3)
		return (false);
	return (true);
}

//pour listen valide la premiere ip adresse
bool validateOneArg(std::string str, ServerConfig& srv) {
	if (str.empty())	
		return (false);
	size_t pos = str.find(':');
	if (pos == str.npos) { // le cas ou il y a que l'IP ou que le port

		if (str.find('.') != std::string::npos || str == "localhost") {
			if (validateIP(str) == true) {
				if (str == "localhost")
					str = "127.0.0.1";
				srv.addListen(str, 80);
				return (true);
			}
		}
		int prt = validatePort(str);
		if (prt != -1) {
			srv.addListen("0.0.0.0", prt);
			return (true);
		}
		return (false);
	}

	std::string ip_str = str.substr(0, pos);
	if (validateIP(ip_str) == false)
		return (false);
	if (ip_str == "localhost")
		ip_str = "127.0.0.1";
	if (str[pos] == ':')
		pos++;
	std::string port_str = str.substr(pos, str.size() - pos);
	int portres = validatePort(port_str);
	if (portres == -1)
		return (false);
	srv.addListen(ip_str, portres);
	return (true);
}

bool validateListen(std::vector<std::string> args, State state, ServerConfig& srv) {
	if (args.size() < 1 || args.size() > 1)
		return (false);
	if (state != IN_SERVER)
		return (false);
	if (validateOneArg(args[0], srv) == false)
		return (false);
	// std::cout << "listen = good" << std::endl;
	return (true);
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

bool validateRoot(std::vector<std::string> args) {
	if (args.size() != 1)
	{
		std::cout << "pas bon nb d'argument" << std::endl;
		return (false);
	}
	if (args[0].empty())
		return (false);
	if (access(args[0].c_str(), F_OK) == -1)
	{
		std::cout << "le dossier n'existe pas" << std::endl;
		return (false);
	}
	if (access(args[0].c_str(), R_OK | X_OK) == -1)
	{
		std::cout << "je n'arrive pas a lire " << std::endl;
		return (false);
	}
	struct stat sb;
	if (stat(args[0].c_str(), &sb) == -1)
	{
		std::cout << "stat pas bon" << std::endl;
		return (false);
	}
	if (!S_ISDIR(sb.st_mode))
	{
		std::cout << "C'est pas un dossier" << std::endl;
		return (false);
	}
	std::cout << "Root = good" << std::endl;
	return (true);
}

bool validateClientMaxBodySize(std::vector<std::string> args) {
	if (args.size() != 1)
		return (false);
	if (args[0].empty())
		return (false);
	size_t i = 0;
	while (i < args[0].size() && isdigit(args[0][i]))
		i++;
	if (i < args[0].size())
		return (false);
	// std::cout << "ClientMaxBodySize = good" << std::endl;
	return (true);
}

bool isErrorCode(std::string code) {
	if (code == "400" || code == "403" || code == "404" || code == "405" || code == "413" || code == "500"
			|| code == "501" || code == "502" || code == "503" || code == "504" || code == "413" || code == "414" || code == "408")
		return (true);
	return (false);
}

bool validateErrorPage(std::vector<std::string> args) {
	if (args.size() < 2)
	{
		// std::cout << "pas bon nombre d'argument" << std::endl;
		return (false);
	}

	for (size_t i = 0; i < args.size() - 1; i++) {
		if (isErrorCode(args[i]) == false) {
			// std::cout << "mauvais code" << std::endl;
			return (false);
		}
	}
	if (args.back().empty())
	{
		// std::cout << "emplacement vide" << std::endl;
		return (false);
	}
	if (access(args.back().c_str(), F_OK) == -1) {
		// std::cout << "je ne trouve pas" << std::endl;
		return (false);
	}
	if (access(args.back().c_str(), R_OK) == -1) {
		// std::cout << "je ne trouve pas" << std::endl;
		return (false);
	}
	struct stat sb;
	if (stat(args.back().c_str(), &sb) == -1)
	{
		// std::cout << "stat pas bon" << std::endl;
		return (false);
	}
	if (!S_ISREG(sb.st_mode))
	{
		// std::cout << "C'est pas un fichier" << std::endl;
		return (false);
	}
	// std::cout << "ErrorPage = good" << std::endl;
	return (true);
}

bool isValidUrl(const std::string& url) {
	if (url.empty())
		return (false);

	if (url[0] == '/')
		return (true);

	if (url.find("http://") == 0 && url.length() > 7)
		return (true);

	if (url.find("https://") == 0 && url.length() > 8)
		return (true);

	return (false);
}

bool validateReturn(std::vector<std::string> args) {
	if (args.size() < 1 || args.size() > 2)
	{
		// std::cout << "args mauvais" << std::endl;
		return (false);
	}
	if (args.size() == 1) {
		if (args[0] == "200" || args[0] == "201" || args[0] == "204" || args[0] == "301" 
			|| args[0] == "302" || isErrorCode(args[0]) == true)
		{
			if (args[0] == "301" || args[0] == "302") {
				// std::cout << "redir doit avoir url" << std::endl;
				return (false);
			}
			return (true);
		}
		if (isValidUrl(args[0]))
			return (true);
		return (false);
	}
	else if (args.size() == 2) {
		if (args[0] != "200" && args[0] != "201" && args[0] != "204" && args[0] != "301" 
			&& args[0] != "302" && isErrorCode(args[0]) == false)
		{
			// std::cout << "mauvais code erreur" << std::endl;
			return (false);
		}
		if (args[1].empty())
			return (false);
	}
	
	// std::cout << "Return = good" << std::endl;
	return (true);
	
}

bool validateIndex(std::vector<std::string> args) {
	if (args.size() < 1)
		return (false);
	for (size_t i = 0; i < args.size(); i++) {
		if (args[i][0] == '/' && (i + 1) != args.size())
			return (false);
	}
	// std::cout << "Index = good" << std::endl;
	return (true);
}

bool validateAutoIndex(std::vector<std::string> args, State state, ServerConfig& srv) {
	if (args.size() != 1)
		return (false);
	bool value;
	if (args[0] == "on")
		value = true;
	else if (args[0] == "off")
		value = false;
	else
		return (false);

	if (state == IN_SERVER) {
		srv.setAutoIndex(value);
		return (true);
	}
	else if (state == IN_LOCATION) {
		if (srv.getLocations().empty())
			return (false);

		srv.getLastLocation().setAutoIndex(value);
		return (true);
	}
	return (false);
}

bool validateAllowedMethods(std::vector<std::string> args, ServerConfig& srv, State state) {
	if (args.size() < 1 || args.size() > 3)
		return (false);
	for (size_t i = 0; i < args.size(); i++) {
		if (args[i] != "GET" && args[i] != "POST" && args[i] != "DELETE")
			return (false);
	}
	std::set<std::string> setMethods(args.begin(), args.end());
	if (state == IN_SERVER) {
		srv.setAllowedMethods(setMethods);
		return (true);
	}
	else if (state == IN_LOCATION) {
		if (!srv.getLocations().empty()) {
			srv.getLastLocation().setAllowedMethods(setMethods);
			return (true);
		}
	}
	return (false);
}

bool validateAllowedUpload(std::vector<std::string> args, ServerConfig& srv, State state) {
	if (args.size() != 1)
		return (false);
	bool value;
	if (args[0] == "on")
		value = true;
	else if (args[0] == "off")
		value = false;
	else
		return (false);
	if (state == IN_SERVER) {
		srv.setAllowedUpload(value);
		return (true);
	}
	else if (state == IN_LOCATION) {
		if (!srv.getLocations().empty()) {
			srv.getLastLocation().setAllowedUpload(value);
			return (true);
		}
	}
	return (false);
}

bool validateUploadPath(std::vector<std::string> args,ServerConfig& srv, State state) {
	if (args.size() != 1 || args[0].empty())
		return (false);

	if (access(args[0].c_str(), F_OK) == -1)
		return (false);

	if (access(args[0].c_str(), W_OK) == -1)
		return (false);

	struct stat sb;
	if (stat(args[0].c_str(), &sb) == -1)
		return (false);
	if (!S_ISDIR(sb.st_mode))
		return (false);

	if (state == IN_SERVER) {
		srv.setUploadPath(args[0]);
		return (true);
	}
	else if (state == IN_LOCATION) {
		if (!srv.getLocations().empty()) {
			srv.getLastLocation().setUploadPath(args[0]);
			return (true);
		}
	}
	return (false);
}

bool validateServerName(std::vector<std::string> args) {
	if (args.size() < 1) {
		// std::cout << "c'est la" << std::endl;
		return (false);
	}
	for (size_t i = 0; i < args.size(); i++) {
		if (args[i].empty()) {
			// std::cout << "c'est la" << std::endl;
			return (false);
		}

		for (size_t j = 0; j < args[i].size(); j++) {
			if (!isalnum(args[i][j]) && args[i][j] != '.' && args[i][j] != '-' && args[i][j] != '_') {
				// std::cout << "c'est la" << std::endl;
				return (false);
			}
		}
	}
	return (true);
}

bool validateCgi(std::vector<std::string> args, ServerConfig& srv, State state) {
	if (args.size() != 2)
		return (false);

	if (args[0].empty() || args[0][0] != '.')
		return (false);

	struct stat sb;
	if (stat(args[1].c_str(), &sb) == -1)
		return (false);

	if (!(sb.st_mode & S_IXUSR))
		return (false);
	if (!S_ISREG(sb.st_mode))
		return (false);
	
	if (state == IN_SERVER) {
		srv.setCgiHandler(args[0], args[1]);
		return (true);
	}
	else if (state == IN_LOCATION) {
		if (!srv.getLocations().empty()) {
			srv.getLastLocation().setCgiHandler(args[0], args[1]);
			return (true);
		}
	}
	return (false);
}

bool validateSpecificDirective(std::string name, std::vector<std::string> args, State state, ServerConfig& srv) {
	if (name == "listen")
	{
		// std::cout << "LISTEN:" << std::endl;
		return (validateListen(args, state, srv));
		
	}

	else if (name == "root")
	{
	 if (validateRoot(args) == true) {
		if (state == IN_SERVER) {
			srv.setRoot(args[0]);
			return (true);
		}
		else if (state == IN_LOCATION) {
			if (srv.getLocations().empty()) {
				// std::cerr << "pas de location" << std::endl;
				return (false);
			}
			// std::vector<std::string> res = combineRootUri(args[0], srv.getLastLocation().getPath());
			// if (validateRoot(args)) {
				srv.getLastLocation().setRoot(args[0]);
				return (true);
			// }
		}
	 }
		// std::cout << "ROOT:" << std::endl;
		return (false);
	}

	else if (name == "server_name") {
		if (validateServerName(args) == true) {
			srv.setServerName(args);
			return (true);
		}
		// std::cout << "c'est la" << std::endl;
		return (false);
	}

	else if (name == "client_max_body_size") {
		if (validateClientMaxBodySize(args) == true) {
			char *end;
			size_t size = strtol(args[0].c_str(), &end, 10);
			if (state == IN_SERVER) {
				srv.setClientMaxBodySize(size);
				return (true);
			}
			else if (state == IN_LOCATION) {
				if (srv.getLocations().empty()) {
					// std::cerr << "pas de location" << std::endl;
					return (false);
				}
				srv.getLastLocation().setClientMaxBodySize(size);
				return (true);
			}
			return (false);
		}
		// std::cout << "CLIENT_MAX_BODY_SIZE:" << std::endl;
		return (false);
	}

	else if (name == "error_page") {
		if (validateErrorPage(args)) {
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
		return (false);
	}

	else if (name == "index") {
		if (validateIndex(args)) {
			if (state == IN_SERVER) {
				srv.setIndex(args);
				return (true);
			}
			else if (state == IN_LOCATION && !srv.getLocations().empty()) {
				srv.getLastLocation().setIndex(args);
				return (true);
			}
		}
		return (false);
	}

	else if (name == "return") {
		if (validateReturn(args)) {
			int code = 0;
			std::string url = "";
			if (args.size() == 1) {
				if (args[0] == "200" || args[0] == "201" || args[0] == "204" || args[0] == "301" 
						|| args[0] == "302" || isErrorCode(args[0]) == true)
				{
					code = std::atoi(args[0].c_str());
				}
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
		return (false);
	}
	else if (name == "autoindex")
		return (validateAutoIndex(args,state, srv));

	else if (name == "allowed_methods")
		return (validateAllowedMethods(args, srv, state));
	
	else if (name == "allowed_upload")
		return (validateAllowedUpload(args, srv, state));
	
	else if (name == "upload_path")
		return (validateUploadPath(args, srv, state));
	
	else if (name == "cgi")
		return (validateCgi(args, srv, state));
	return (false);
	
}
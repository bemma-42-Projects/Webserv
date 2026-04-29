#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <errno.h>
// #include <cstring>
#include <iostream>
#include <unistd.h>
#include <vector>
#include <cctype>
#include <stack>
#include <cstdlib>

#include <sys/stat.h>

#include "ServerConfig.hpp"

// permet de savoir dans quel bloc je suis je ne l'utilise que pour validate pour l'instant
enum State {
	OUTSIDE,
	IN_SERVER,
	IN_LOCATION
};


//cette fonction sert juste a lire un fichier et a le stocker dans une string
std::string readFile(const char *path) {

	int fd = open(path, O_RDONLY);

	if (fd == -1)
	{
		return ("fail");
	}

	char buffer[1024];
	ssize_t bytes = read(fd, buffer, 1024);
	std::string result;

	while (bytes > 0) 
	{
		result.append(buffer, bytes);
		bytes = read(fd, buffer, 1024);
	}
	if (bytes < 0)
	{
		close(fd);
		return ("fail");
	}
	close(fd);
	return (result);
}

//cette fonction sert a separer chacun de mes mots et separateur (separe avant token)
std::vector<std::string> tokenizeConfig(std::string str) {

	std::vector<std::string> res;

	for (size_t i = 0; i < str.length(); )
	{
		while (i < str.length() && std::isspace((unsigned char)str[i]) != 0)
			i++;
		if (i >= str.length())
			break ;
		if (str[i] == '#')
		{
			while ( i < str.length() && str[i] != '\n')
				i++;
		}
		else if (str[i] == '{' || str[i] == '}' || str[i] == ';')
		{
			res.push_back(str.substr(i, 1));
			i++;
		}
		else if (str[i] != '{' && str[i] != '}' && str[i] != ';' && std::isspace((unsigned char)str[i]) == 0)
		{
			size_t word = str.find_first_of("{}; \t\n\v\f\r", i);
			if (word == str.npos)
				word = str.length();
			
			std::string newstring = str.substr(i, (word - i));
			res.push_back(newstring);
			i = word;
		}
	}
	return (res);
}

bool isSimpleDirective(std::string name) {
	if (name == "listen" || name == "root" || name == "client_max_body_size" || name == "server_name"
			|| name == "error_page" || name == "index" || name == "return" || name == "autoindex"
			|| name == "allowed_methods" || name == "upload_path" || name == "allowed_upload")
		return (true);
	return (false);
}

bool directiveIsAllowed(std::string name, State state) {
	if (state == IN_SERVER && (name == "listen" || name == "root" || name == "client_max_body_size"
			|| name == "server_name" || name == "error_page" || name == "index" || name == "return"
			|| name == "autoindex"))
		return (true);
	else if (state == IN_LOCATION && (name == "root" || name == "index" || name == "autoindex"
			|| name == "return" || name == "allowed_methods" || name == "upload_path"
			|| name == "allowed_upload" || name == "client_max_body_size" || name == "error_page"))
		return (true);
	return (false);
}

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
	if (pos == str.npos) {

		if (str.find('.') == std::string::npos)
			return (validatePort(str));
		return (validateIP(str));
	}

	std::string ip_str = str.substr(0, pos);
	if (validateIP(ip_str) == false)
		return (false);
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

	if (state != IN_SERVER)
		return (false);
	for (size_t i = 0; i < args.size() ; i++ ) {
		if (validateOneArg(args[i], srv) == false)
			return (false);
	}
	std::cout << "listen = good" << std::endl;
	return (true);
}

bool validateRoot(std::vector<std::string> args) {
	if (args.size() != 1)
	{
		// std::cout << "pas bon nb d'argument" << std::endl;
		return (false);
	}
	if (args[0].empty())
		return (false);
	if (access(args[0].c_str(), F_OK) == -1)
	{
		// std::cout << "le dossier n'existe pas" << std::endl;
		return (false);
	}
	if (access(args[0].c_str(), R_OK) == -1)
	{
		// std::cout << "je n'arrive pas a lire " << std::endl;
		return (false);
	}
	struct stat sb;
	if (stat(args[0].c_str(), &sb) == -1)
	{
		// std::cout << "stat pas bon" << std::endl;
		return (false);
	}
	if (!S_ISDIR(sb.st_mode))
	{
		// std::cout << "C'est pas un dossier" << std::endl;
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
	std::cout << "ClientMaxBodySize = good" << std::endl;
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
		std::cout << "pas bon nombre d'argument" << std::endl;
		return (false);
	}

	for (size_t i = 0; i < args.size() - 1; i++) {
		if (isErrorCode(args[i]) == false) {
			std::cout << "mauvais code" << std::endl;
			return (false);
		}
	}
	if (args.back().empty())
	{
		std::cout << "emplacement vide" << std::endl;
		return (false);
	}
	if (access(args.back().c_str(), F_OK) == -1) {
		std::cout << "je ne trouve pas" << std::endl;
		return (false);
	}
	if (access(args.back().c_str(), R_OK) == -1) {
		std::cout << "je ne trouve pas" << std::endl;
		return (false);
	} 
	std::cout << "ErrorPage = good" << std::endl;
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
		std::cout << "args mauvais" << std::endl;
		return (false);
	}
	if (args.size() == 1) {
		if (args[0] == "200" || args[0] == "201" || args[0] == "204" || args[0] == "301" 
			|| args[0] == "302" || isErrorCode(args[0]) == true)
		{
			if (args[0] == "301" || args[0] == "302") {
				std::cout << "redir doit avoir url" << std::endl;
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
			std::cout << "mauvais code erreur" << std::endl;
			return (false);
		}
		if (args[1].empty())
			return (false);
	}
	
	std::cout << "Return = good" << std::endl;
	return (true);
	
}

bool validateIndex(std::vector<std::string> args) {
	for (size_t i = 0; i < args.size(); i++) {
		if (args[i][0] == '/' && (i + 1) != args.size())
			return (false);
	}
	std::cout << "Index = good" << std::endl;
	return (true);
}

bool validateAutoIndex(std::vector<std::string> args, State state, ServerConfig& srv) {
	if (args.size() != 1)
		return (false);
	if (args[0] == "on" || args[0] == "off") {
		if (state == IN_SERVER && args[0] == "on") {
			srv.setAutoIndex(true);
			return (true);
		}
		else if (state == IN_LOCATION && args[0] == "on") {
			if (srv.getLocations().empty()) {
				std::cerr << "pas de location" << std::endl;
				return (false);
			}
			srv.getLastLocation().setAutoIndex(true);
			return (true);
		}
		else if (args[0] == "off")
			return (true);
	}
	return (false);
}

bool validateAllowedMethods(std::vector<std::string> args, ServerConfig& srv) {
	if (args.size() < 1 || args.size() > 3)
		return (false);
	for (size_t i = 0; i < args.size(); i++) {
		if (args[i] != "GET" && args[i] != "POST" && args[i] != "DELETE")
			return (false);
	}
	if (!srv.getLocations().empty()) {
		srv.getLastLocation().setAllowedMethods(args);
		std::cout << "AllowedMethods = good" << std::endl;
		return (true);
	}
	return (false);
}

bool validateAllowedUpload(std::vector<std::string> args, ServerConfig& srv) {
	if (args.size() != 1)
		return (false);
	if (!srv.getLocations().empty()) {
		if (args[0] == "on") {
			srv.getLastLocation().setAllowedUpload(true);
			return (true);
		}
		else if (args[0] == "off") {
			return (true);
		}
	}
	// std::cout << "AllowedUpload = good" << std::endl;
	return (false);
}

bool validateUploadPath(std::vector<std::string> args,ServerConfig& srv) {
	if (args.size() != 1)
		return (false);
	if (args[0].empty())
		return (false);
	if (access(args[0].c_str(), F_OK) == -1)
	{
		// std::cout << "le dossier n'existe pas" << std::endl;
		return (false);
	}
	if (access(args[0].c_str(), W_OK) == -1)
	{
		// std::cout << "je n'arrive pas a lire " << std::endl;
		return (false);
	}
	struct stat sb;
	if (stat(args[0].c_str(), &sb) == -1)
	{
		// std::cout << "stat pas bon" << std::endl;
		return (false);
	}
	if (!S_ISDIR(sb.st_mode))
	{
		// std::cout << "C'est pas un dossier" << std::endl;
		return (false);
	}
	std::cout << "UploadPath = good" << std::endl;
	if (!srv.getLocations().empty()) {
		srv.getLastLocation().setUploadPath(args[0]);
		return (true);
	}
	return (false);
}

//on va check sur quelle directive on est et en fonction aller check si tout est bon pour
// la directive
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
				std::cerr << "pas de location" << std::endl;
				return (false);
			}
			srv.getLastLocation().setRootLoc(args[0]);
			return (true);
		}

	 }
		// std::cout << "ROOT:" << std::endl;
		return (false);
	}

	else if (name == "server_name") {
		srv.setServerName(args);
		return (true);
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
					std::cerr << "pas de location" << std::endl;
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
		return (validateAllowedMethods(args, srv));
	
	else if (name == "allowed_upload")
		return (validateAllowedUpload(args, srv));
	
	else if (name == "upload_path")
		return (validateUploadPath(args, srv));
	
	return (false);
	
}

//validation globale des directive on va juste check si les attentes communes a 
//toutes les directives sont respecter 
bool validateOneDirective(std::vector<std::string> tokens, size_t& i, State state, ServerConfig& srv) {
	std::vector<std::string> args;
	std::string name = tokens[i];

	if (directiveIsAllowed(name, state) == false)
	{
		std::cout << "Pas dans le bon bloc..." << std::endl; 
		return (false);
	}
	i++;
	for ( ;i < tokens.size() && tokens[i] != ";" ;i++)
	{
		if (tokens[i] == "}" || tokens[i] == "{")
			return (false);
		if (isSimpleDirective(tokens[i]) == true)
			return (false);
		args.push_back(tokens[i]);
	}
	if (args.size() == 0)
		return (false);
	if (i >= tokens.size() || tokens[i] != ";")
		return (false);
	if (validateSpecificDirective(name, args, state, srv) == false)
		return (false);
	return (true);
}


// fonction qui valide la structure du fichier de config (pour l'instant elle check si le 
// nb d'accolade est bon, si les blocs sont bien fait qu'il n'y a pas de location dans location
// etc, je ne check pas pour l'instant les directives et les ;)
bool validateStructure(std::vector<std::string> tokens, std::vector<ServerConfig> &all_servers) {

	State state = OUTSIDE;
	std::stack<std::string> context;
	for (size_t i = 0; i < tokens.size() ; i++)
	{
		
		if (tokens[i] == "server")
		{
			if (state != OUTSIDE)
				return (false);
			else if (i + 1 >= tokens.size() || tokens[i + 1] != "{")
				return (false);
			ServerConfig server;
			all_servers.push_back(server);
			context.push("server");
			state = IN_SERVER;
			i++;
		}
		else if (tokens[i] == "location")
		{
			if (state != IN_SERVER)
			{
				std::cout << "c'est pas bon ici " << state << std::endl;
				return (false);
			}
			if (i + 1 >= tokens.size() || tokens[i + 1].find_first_of("{}") != std::string::npos)
				return (false);
			if (i + 2 >= tokens.size() || tokens[i + 2] != "{")
				return (false);
			LocationConfig location;
			location.setPath(tokens[i + 1]);
			all_servers.back().addLocation(location);
			context.push("location");
			state = IN_LOCATION;
			i += 2;
		}
		else if (tokens[i] == "{")
		{
			std::cout << "c'est pas bon ici" << std::endl;
			return (false);
		}
		else if (tokens[i] == "}")
		{
			if (context.empty())
			{
				std::cout << "c'est pas bon ici" << std::endl;
				return (false);
			}
			context.pop();
			if (context.empty())
				state = OUTSIDE;
			else {
				if (context.top() == "server")
					state = IN_SERVER;
				else if (context.top() == "location")
					state = IN_LOCATION;
			}
			
			
		}
		else if (isSimpleDirective(tokens[i]) == true)
		{
			if (all_servers.empty()) {
				std::cout << "directive hors bloc server" << std::endl;
				return (false);
			}

			// std::cout << "         Je suis sur une directive!" << std::endl;
			if (validateOneDirective(tokens, i, state, all_servers.back()) == false)
			{
				std::cout << "c'est pas bon ici" << std::endl;
				return (false);
			}
		}
		
	}
	if (state == OUTSIDE && context.empty())
		return (true);
	std::cout << "c'est pas bon ici" << std::endl;
	return (false);
}

int main(int argc, char **argv) {
	(void)argc;

	std::vector<ServerConfig> all_configs;
	std::string text = readFile(argv[1]);
	std::vector<std::string> res = tokenizeConfig(text);
	// try {
	// 	validateStructure(res, all_configs);
	// }
	// catch (const std::exception& e) {
	// 	std::cerr << "Erreur fatale de configuration : " << e.what() << std::endl;
	// 	return (1); 
	// }
	if (validateStructure(res, all_configs) == false)
		std::cout << "Erreur bad configuration" << std::endl;
	else 
		std::cout << "Everything's good!" << std::endl;

	for (size_t i = 0; i < all_configs.size(); i++) {

		std::cout << std::endl << std::endl << "Serveur " << i << ";" << std::endl;
		std::cout << all_configs[i] << std::endl << std::endl;
	}
}

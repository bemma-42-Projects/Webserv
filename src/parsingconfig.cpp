#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <errno.h>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <vector>
#include <cctype>
#include <stack>
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

bool parsePort(std::string port_str) {
	if (port_str.empty())
		return (false);

	for (size_t i = 0; i < port_str.size(); i++) {
		if (!isdigit(port_str[i]))
			return (false);
	}

	// A REMPLACER (stol : C++11)
	//long port = stol(port_str, 0, 10 );
	//if (port >= 0 && port <= 65535)
	//	return (true);
	return (false);
}

bool parseIp(std::string ip) {
	if (ip.empty())
		return (false);
	return (true);
}

bool validateListen(std::vector<std::string> args) {

	for (size_t i = 0, nb_pv = 0; i < args.size() ; i++ )
	{
		nb_pv = 0;
		for (size_t y = 0; y < args[i].size() ;y++)
		{
			size_t pos_colon = args[i].find(':'); //colon = : en anglais

			if (pos_colon == std::string::npos)
			{
				if (parsePort(args[i]) == 0)
					return (false);
				if (parseIp(args[i]) == 0)
					return (false);
			}
			if (isdigit(args[i][y]) == 0 && args[i][y] != ':' && args[i][y] != '.')
			{
				std::cout << "je ne suis pas un digit n'y un :" << std::endl;
				return (false);

			}
			else if (args[i][y] == ':')
			{
				if (nb_pv == 1)
					return (false);
				nb_pv++;
			}
		}
	}
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
	// std::cout << "--le path est bon!" << std::endl;
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
	std::cout << "GOOD!" << std::endl;
	return (true);
}

bool validateSpecificDirective(std::string name, std::vector<std::string> args) {
	if (name == "listen")
	{
		// std::cout << "LISTEN:" << std::endl;
		return (validateListen(args));
	}
	else if (name == "root")
	{
		// std::cout << "ROOT:" << std::endl;
		return (validateRoot(args));
	}
	else if (name == "server_name") {
		// std::cout << "SERVER_NAME:" << std::endl;
		return (true);
	}
	else if (name == "client_max_body_size") {
		// std::cout << "CLIENT_MAX_BODY_SIZE:" << std::endl;
		return (validateClientMaxBodySize(args));
	}
	else if (name == "error_page") {
		std::cout << "ERROR_PAGE:" << std::endl;
		return (validateErrorPage(args));
	}
	return (false);
	
}


bool validateOneDirective(std::vector<std::string> tokens, size_t& i, State state) {
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
	if (validateSpecificDirective(name, args) == false)
		return (false);
	return (true);
}


// fonction qui valide la structure du fichier de config (pour l'instant elle check si le 
// nb d'accolade est bon, si les blocs sont bien fait qu'il n'y a pas de location dans location
// etc, je ne check pas pour l'instant les directives et les ;)
bool validateStructure(std::vector<std::string> tokens) {

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
			context.push("server");
			state = IN_SERVER;
			i++;
		}
		else if (tokens[i] == "location")
		{
			if (state != IN_SERVER)
				return (false);
			if (i + 1 >= tokens.size() || tokens[i + 1].find_first_of("{}") != std::string::npos)
				return (false);
			if (i + 2 >= tokens.size() || tokens[i + 2] != "{")
				return (false);
			context.push("location");
			state = IN_LOCATION;
			i += 2;
		}
		else if (tokens[i] == "{")
			return (false);
		else if (tokens[i] == "}")
		{
			if (context.empty())
				return (false);
			context.pop();
			if (context.empty())
				state = OUTSIDE;
			else if (context.top() == "server")
				state = IN_SERVER;
			else if (context.top() == "location")
				state = IN_LOCATION;
			
		}
		else if (isSimpleDirective(tokens[i]) == true)
		{

			// std::cout << "         Je suis sur une directive!" << std::endl;
			if (validateOneDirective(tokens, i, state) == false)
				return (false);
		}
		
	}
	if (state == OUTSIDE && context.empty())
		return (true);
	return (false);
}


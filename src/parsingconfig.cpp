
#include "parsingconf.hpp"
#include "RequestAnswer.hpp"
#include "Request.hpp"
#include "Error.hpp"

//cette fonction sert juste a lire un fichier et a le stocker dans une string
std::string readFile(const char *path) {

	int fd = open(path, O_RDONLY);

	if (fd == -1)
		throw std::runtime_error("Impossible d'ouvrir le fichier");

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
		throw std::runtime_error("Impossible d'ouvrir le fichier");
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
			|| name == "allowed_methods" || name == "upload_path" || name == "allowed_upload" || name == "cgi_handler")
		return (true);
	return (false);
}

bool directiveIsAllowed(std::string name, State state) {
	if (state == IN_SERVER && (name == "listen" || name == "root" || name == "client_max_body_size"
			|| name == "server_name" || name == "error_page" || name == "allowed_methods" || name == "index" 
			|| name == "return" || name == "autoindex" || name == "allowed_upload" || name == "upload_path" || name == "cgi_handler"))
		return (true);
	else if (state == IN_LOCATION && (name == "root" || name == "index" || name == "autoindex"
			|| name == "return" || name == "allowed_methods" || name == "upload_path"
			|| name == "allowed_upload" || name == "client_max_body_size" || name == "error_page" || name == "cgi_handler"))
		return (true);
	return (false);
}


//validation globale des directive on va juste check si les attentes communes a 
//toutes les directives sont respecter 
bool validateOneDirective(std::vector<std::string> tokens, size_t& i, State state, ServerConfig& srv) {
	std::vector<std::string> args;
	std::string name = tokens[i];

	if (directiveIsAllowed(name, state) == false)
	{
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
	{
		return (false);
	}
	return (true);
}


// fonction qui valide la structure du fichier de config (pour l'instant elle check si le 
// nb d'accolade est bon, si les blocs sont bien fait qu'il n'y a pas de location dans location
// etc, je ne check pas pour l'instant les directives et les ;)
bool validateStructure(std::vector<std::string> &tokens, std::vector<ServerConfig> &all_servers) {
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
			return (false);
		}
		else if (tokens[i] == "}")
		{
			if (context.empty())
			{
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
				return (false);
			}
			if (validateOneDirective(tokens, i, state, all_servers.back()) == false)
			{
				return (false);
			}
		}
		else
            return (false);
	}
	if (state == OUTSIDE && context.empty())
		return (true);
	return (false);
}

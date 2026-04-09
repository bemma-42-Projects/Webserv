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

bool validateListen(std::vector<std::string> args) {
	for (size_t i = 0, nb_pv = 0; i < args.size() ; i++ )
	{
		nb_pv = 0;
		for (size_t y = 0; y < args[i].size() ;y++)
		{
			if (isdigit(args[i][y]) == 0 && args[i][y] != ':')
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
		// if (nb_pv != 2 && nb_pv != 0)
		// {
			
		// 	return (false);
		// }
	}
	return (true);
}

bool validateSpecificDirective(std::string name, std::vector<std::string> args) {
	if (name == "listen")
	{
		std::cout << "Je suis un listen" << std::endl;
		return (validateListen(args));
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
	if (i >= tokens.size())
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

			std::cout << "         Je suis sur une directive!" << std::endl;
			if (validateOneDirective(tokens, i, state) == false)
				return (false);
		}
		
	}
	if (state == OUTSIDE && context.empty())
		return (true);
	return (false);
}

int main(int argc, char **argv) {
	(void)argc;
	std::string text = readFile(argv[1]);
	// std::cout << text << std::endl << std::endl;
	std::vector<std::string> res = tokenizeConfig(text);
	// for (size_t len = 0; len < res.size(); len++) {
	// 	std::cout << "|" << res[len] << "|" << std::endl; 
	// }
	if (validateStructure(res) == false)
		std::cout << "Erreur bad configuration" << std::endl;
	else 
		std::cout << "Everything's good!" << std::endl; 
}


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

int isSimpleDirective(std::string name) {
	if (name == "listen" || name == "root" || name == "client_max_body_size" || name == "server_name"
			|| name == "error_page" || name == "index" || name == "return" || name == "autoindex"
			|| name == "allowed_methods" || name == "upload_path" || name == "allowed_upload")
		return (1);
	return (0);
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
	for (size_t len = 0; len < res.size(); len++) {
		std::cout << "|" << res[len] << "|" << std::endl; 
	}
	if (validateStructure(res) == false)
		std::cout << "Erreur bad configuration" << std::endl;
	else 
		std::cout << "Everything's good!" << std::endl; 
}


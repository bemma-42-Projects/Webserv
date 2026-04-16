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

bool validateStructure(std::vector<std::string> tokens) {
	int count_brace = 0;

	for (size_t i = 0; i < tokens.size() ; i++)
	{
		// if (count_brace < 0)
		// 	return (false);
		if (tokens[i] == "server")
		{
			if (tokens[i + 1] != "{")
				return (false);
		}
		else if (tokens[i] == "location")
		{
			if (tokens[i + 1].find_first_of("{}") != std::string::npos|| tokens[i + 2] != "{")
				return (false); 
		}
		if (tokens[i] == "{")
			count_brace++;
		else if (tokens[i] == "}")
			count_brace--;
	}
	if (count_brace != 0)
		return (false);
	return (true);
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


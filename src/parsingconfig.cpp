
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
void validateOneDirective(std::vector<std::string> tokens, size_t& i, State state, ServerConfig& srv) {
	std::vector<std::string> args;
	std::string name = tokens[i];

	if (directiveIsAllowed(name, state) == false)
	{
		throw std::runtime_error("directive '" + name + "' is not allowed in this context");
		// std::cout << "Pas dans le bon bloc..." << std::endl; 
		// return (false);
	}
	i++;
	for ( ;i < tokens.size() && tokens[i] != ";" ;i++)
	{
		if (tokens[i] == "}" || tokens[i] == "{")
			throw std::runtime_error("directive '" + name + "': unexpected '" + tokens[i] + "' found before ';'");

		if (isSimpleDirective(tokens[i]) == true)
			throw std::runtime_error("directive '" + name + "': missing ';' before '" + tokens[i] + "'");

		args.push_back(tokens[i]);
	}
	if (args.empty())
		throw std::runtime_error("directive '" + name + "' requires at least one argument");

	if (i >= tokens.size() || tokens[i] != ";")
		throw std::runtime_error("directive '" + name + "': missing termination ';' at end of line");

	validateSpecificDirective(name, args, state, srv);

}

// fonction qui valide la structure du fichier de config (pour l'instant elle check si le 
// nb d'accolade est bon, si les blocs sont bien fait qu'il n'y a pas de location dans location
// etc, je ne check pas pour l'instant les directives et les ;)
void validateStructure(std::vector<std::string> &tokens, std::vector<ServerConfig> &all_servers) {
	State state = OUTSIDE;
	std::stack<std::string> context;

	for (size_t i = 0; i < tokens.size() ; i++)
	{
		
		if (tokens[i] == "server")
		{
			if (state != OUTSIDE)
				throw std::runtime_error("structure: 'server' block cannot be nested inside another block");
			else if (i + 1 >= tokens.size() || tokens[i + 1] != "{")
				throw std::runtime_error("structure: expected '{' after 'server'");
			ServerConfig server;
			all_servers.push_back(server);
			context.push("server");
			state = IN_SERVER;
			i++;
		}
		else if (tokens[i] == "location")
		{
			if (state != IN_SERVER)
				throw std::runtime_error("structure: 'location' must be inside a 'server' block");

			if (i + 1 >= tokens.size() || tokens[i + 1].find_first_of("{}") != std::string::npos)
				throw std::runtime_error("structure: 'location' requires a valid path before '{'");

			if (i + 2 >= tokens.size() || tokens[i + 2] != "{")
				throw std::runtime_error("structure: expected '{' after location path");

			LocationConfig location;
			location.setPath(tokens[i + 1]);
			all_servers.back().addLocation(location);
			context.push("location");
			state = IN_LOCATION;
			i += 2;
		}
		else if (tokens[i] == "{")
			throw std::runtime_error("structure: unexpected '{' (check your block opening syntax)");
		else if (tokens[i] == "}") {
			if (context.empty())
				throw std::runtime_error("structure: unexpected closing brace '}' (no block open)");

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
		else if (isSimpleDirective(tokens[i]) == true) {
			if (all_servers.empty())
				throw std::runtime_error("structure: directive '" + tokens[i] + "' found outside of any 'server' block");

			validateOneDirective(tokens, i, state, all_servers.back());
		}
		else
			throw std::runtime_error("structure: unknown token '" + tokens[i] + "'");
	}
	if (state != OUTSIDE && !context.empty())
		throw std::runtime_error("structure: reached end of file with unclosed '" + context.top() + "' block");
}


// int main(int argc, char **argv) {
// 	if (argc != 2)
// 		return (1);

// 	std::vector<ServerConfig> all_configs;
// 	std::string text = readFile(argv[1]);
// 	std::vector<std::string> res = tokenizeConfig(text);
// 	if (validateStructure(res, all_configs) == false) {
// 		// std::cout << "Erreur bad configuration" << std::endl;
// 		return (1);
// 	}
// 	else 
// 		std::cout << "Everything's good!" << std::endl;

// 	for (size_t i = 0; i < all_configs.size(); i++) 
// 		all_configs[i].finalize();


// 	for (size_t i = 0; i < all_configs.size(); i++) {

// 		std::cout << std::endl << std::endl << "Serveur " << i << ";" << std::endl;
// 		std::cout << all_configs[i] << std::endl << std::endl;
// 	}

// 	// std::cout << all_configs[0].getLocations()[0].getPath() << std::endl;

// 	const char *buffer = 
// 	"POST /upload HTTP/1.1\r\n"
// 	"Host: localhost:8080\r\n"
// 	"Content-Type: multipart/form-data; boundary=boundary123\r\n"
// 	"Content-Length: 162\r\n"
// 	"\r\n"
// 	"--boundary123\r\n"
// 	"Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n"
// 	"Content-Type: text/plain\r\n"
// 	"\r\n"
// 	"Ceci est le contenu de ton fichier !\r\n"
// 	"--boundary123--";
	
// 	Request file((char *)buffer, all_configs[0]);
// 	int result = file.parsingHttp();
// 	if (result == 0)
// 	{
// 		std::cout << "error " << file.getError() << std::endl;
// 		std::cout << Error::AnswerError(file.getError(), file.getErrorMessage(), all_configs[0].getErrorPage());
// 		return 0;
// 	}
// 	else if (result == 2)
// 	{
// 		std::cout << "requette non complete" << std::endl;
// 		return 0;
// 	}
// 	// std::cout << file << std::endl;
// 	RequestAnswer answer(file);
// 	if (answer.setAnswer() == 1) {}
// 		// std::cout << "anser =" << answer.getAnswer() << std::endl;

// }

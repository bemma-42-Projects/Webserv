#include "Server.hpp"
#include <iostream>
#include <stdexcept>
#include <iostream>
#include <csignal>
#include "RequestAnswer.hpp"

bool g_running = true;

void    handle_sigint(int sig) {
    (void)sig;
    std::cout << "\nSIGINT received. Shutting down server gracefully..." << std::endl;
    g_running = false;
}

int main() {
	Config::location();

    
    std::signal(SIGINT, handle_sigint);
    //std::signal(SIGQUIT, handle_sigint);

    try {
		Server  webServer;
        webServer.init();
        webServer.run();
    }
    catch(const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return (1);
    }
    return (0);
}

/*
// MAIN JULIEN CGI
int main()
{
	Config::location();

		const char *buffer = "GET /cgi-bin/test.php?name=julien&project=webserv HTTP/1.1\r\n"
			"Host: localhost:8080\r\n"
			"Connection: close\r\n"
			"\r\n";
	
		Request file((char *)buffer);
		file.setClientIP("127.0.0.1");

		int res = file.parsingHttp();

		if (res == 0)
		{
			std::cout << "Erreur de parsing : " << file.getError() << std::endl;
			return 0;
		}
		else if (res == 2)
		{
			std::cout << "Requête incomplète" << std::endl;
			return 0;
		}

		std::cout << "--- Debug Routing ---" << std::endl;
        std::cout << "URL demandée : " << file.getUrlPath() << std::endl;
        std::cout << "Root de la location : " << file.getLocation().getRoot() << std::endl;
        std::cout << "Path : " << file.getPath() << std::endl;

		RequestAnswer answer(file);

		if (answer.setAnswer() == 1)
        {
            std::cout << "Réponse du serveur : " << std::endl;
			std::cout << answer.getAnswer() << std::endl;
		}
        else
        {
            std::cout << "Erreur lors de la génération de la réponse : " << answer.getError() << std::endl;
        }
        return (0);
}
*/

// MAIN ROMANE
/*
int main()
{
	//try{
		Config::location();

		const char *buffer = "GET /Makefile HTTP/1.1\r\n"
			"Host: localhost:8080\r\n"
			//"Content-Type: application/x-www-form-urlencoded\r\n"
			"Content-Length: 27\r\n"
			"\r\n\r\n" // Ligne vide importante entre headers et body
			"name=Gemini&project=webserv";
	
	
		Request file((char *)buffer);
		int res = file.parsingHttp();
		if (res == 0)
		{
			std::cout << "error " << file.getError() << std::endl;
			return 0;
		}
		else if (res == 2)
		{
			std::cout << "requette non complete" << std::endl;
			return 0;
		}
		std::cout << file.getLocation().getRoot() << std::endl;
		//std::cout << file << std::endl;
		RequestAnswer answer(file);
		std::cout << "test " << std::endl;
		if (answer.setAnswer() == 1)
			std::cout << "anser =" << answer.getAnswer() << std::endl;
		
	//}
	//catch(std::exception &e)
	//{
	//	std::cerr << "error : " << e.what() << std::endl;
	//	//Error::setError(e.what());
	//}
}
*/

// MAIN EMMA
/*
int main(int argc, char **argv) {
	(void)argc;

	std::vector<ServerConfig> all_configs;
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

*/
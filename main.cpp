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

/*
int main() {
    Server  webServer;
    std::signal(SIGINT, handle_sigint);
    std::signal(SIGQUIT, handle_sigint);

    try {
        webServer.init();
        webServer.run();
    }
    catch(const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return (1);
    }
    return (0);
}
*/

int main()
{
	Config::location();

		const char *buffer = "GET /cgi-bin/test.php HTTP/1.1\r\n"
			"Host: localhost:8080\r\n"
			"Connection: close\r\n"
			"\r\n";
	
		Request file((char *)buffer);
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
	//}
	//catch(std::exception &e)
	//{
	//	std::cerr << "error : " << e.what() << std::endl;
	//	//Error::setError(e.what());
	//}
}
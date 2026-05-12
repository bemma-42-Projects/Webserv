#include "Server.hpp"
#include "parsingconfig.hpp"
#include "ServerConfig.hpp"
#include "Error.hpp"
#include <iostream>
#include <stdexcept>
#include <csignal>
#include <vector>

bool g_running = true;

void    handle_sigint(int sig) {
    (void)sig;
    std::cout << "\nSIGINT received. Shutting down server gracefully..." << std::endl;
    g_running = false;
}

int main(int argc, char **argv) {
    if (argc != 2)
    {
        std::cerr << "Usage: ./webserv <config_file>" << std::endl;
        return (1);
    }
    std::signal(SIGINT, handle_sigint);

    try {
		std::cout << "[INFO] Parsing configuration file: " << argv[1] << std::endl;
		std::string text = readFile(argv[1]);
		std::vector<std::string> res = tokenizeConfig(text);
	
		std::vector<ServerConfig> all_configs;
		
		if (validateStructure(res, all_configs) == false)
		{
			std::cout << "Fatal error: Invalid configuration file." << std::endl;
			return (1);
		}
		std::cout << "Everything's good!" << std::endl;

		for (size_t i = 0; i < all_configs.size(); i++) 
			all_configs[i].finalize();
	// for (size_t i = 0; i < all_configs.size(); i++) {

	// 	std::cout << std::endl << std::endl << "Serveur " << i << ";" << std::endl;
	// 	std::cout << all_configs[i] << std::endl << std::endl;
	// }
	//std::cout << "testtttt" << std::endl;
	std::cout << "--> DEBUG MAIN: Taille de all_configs avant Server = " << all_configs.size() << std::endl;

	std::cout << "[INFO] Initializing server..." << std::endl;
	/*
	if (all_configs.empty())
		return (1);
	if (all_configs[0].getLocations().empty())
		return (1);
	// if (all_configs[1].get)
	std::cout << all_configs[0].getLocations()[0].getPath() << std::endl;
	std::cout << "end" << std::endl;
	*/
		Server  webServer(all_configs);
        webServer.init();
        webServer.run();
    }
    catch(const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return (1);
    }
    return (0);
}

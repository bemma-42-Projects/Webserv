#include "Server.hpp"
#include <iostream>
#include <stdexcept>
#include <iostream>
#include <csignal>

bool g_running = true;

void    handle_sigint(int sig) {
    (void)sig;
    std::cout << "\nSIGINT received. Shutting down server gracefully..." << std::endl;
    g_running = false;
}

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

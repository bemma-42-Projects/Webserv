#include "Server.hpp"
#include <iostream>
#include <stdexcept>

int main(void) {
    Server  webServer;

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

#include <sys/socket.h> // For socket functions
#include <netinet/in.h> // For sockaddr_in
#include <cstdlib>      // For exit() and EXIT_FAILURE
#include <iostream>     // For cout
#include <unistd.h>     // For read

int main() {
    // Create a socket (IPv4, TCP)
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        std::cout << "Failed to create socket. errno: " << errno << std::endl;
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cout << "setsockopt failed. errno: " << errno << std::endl;
        exit(EXIT_FAILURE);
    }

    // Listen to port 4243 on any address
    sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = INADDR_ANY;
    // htons is necessary to convert a number to network byte order
    sockaddr.sin_port = htons(4243);

    if (bind(sockfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
        std::cout << "Failed to bind port 4243. errno: " << errno << std::endl;
        exit(EXIT_FAILURE);
    }
    
    // Start listening. Hold at most 10 connections in the queue
    if (listen(sockfd, 10) < 0) {
        std::cout << "Failed to listen on socket. errno: " << errno << std::endl;
        exit(EXIT_FAILURE);
    }

    // Grab a connection from the queue
    auto addrlen = sizeof(sockaddr);
    int connection = accept(sockfd, (struct sockaddr *)&sockaddr, (socklen_t *)&addrlen);
    if (connection < 0) {
        std::cout << "Failed to grab connection. errno: " << errno << std::endl;
        exit(EXIT_FAILURE);
    }

    // Read from the connection
    char buffer[100] = {0};
    auto bytesRead = read(connection, buffer, 100);
    std::cout << "The message was: " << buffer;

    // Send a message to the connection
    std::string response = "Good talking to you \n";
    send(connection, response.c_str(), response.size(), 0);

    // Close the connection
    close(connection);
    close(sockfd);
}
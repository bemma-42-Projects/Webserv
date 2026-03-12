#define _POSIX_C_SOURCE 200112L
#include <string>           
#include <unistd.h>         
#include <sys/types.h>      
#include <sys/socket.h>     
#include <netinet/in.h>     
#include <arpa/inet.h>      
#include <netdb.h>          
#include <fcntl.h>          
#include <map>              
#include <ctime>            
#include <iostream>         
#include "Client.hpp"       
#include "SystemError.hpp"  
#include "GaiError.hpp"     
#define PORT "8080"         
#define BACKLOG 10          
#define MAX_TIMEOUT 10

int main(void)
{
    struct addrinfo         hints;                          
    struct addrinfo         *res;                           
    struct addrinfo         *p;                             
    int                     status;                         
    char                    ip_buffer[INET6_ADDRSTRLEN];    
    int                     sockfd = -1;                    
    int                     yes = 1;                        
    const std::string       port_str = PORT; 
    try {
        std::memset(&hints, 0, sizeof hints);   
        hints.ai_family = AF_UNSPEC;            
        hints.ai_socktype = SOCK_STREAM;        
        hints.ai_flags = AI_PASSIVE;            
        if ((status = getaddrinfo(NULL, port_str.c_str(), &hints, &res)) != 0) {
            throw GaiError("DNS/Setup Error", status);
        }
        std::cout << "Booting up server on port " << port_str << "..." << std::endl;
        for (p = res; p != NULL; p = p->ai_next) {
            void                *addr;  
            std::string         ipver;  
            struct sockaddr_in  *ipv4;  
            struct sockaddr_in6 *ipv6;  
            if (p->ai_family == AF_INET) {  
                ipv4 = reinterpret_cast<struct sockaddr_in *>(p->ai_addr);  
                addr = &(ipv4->sin_addr);   
                ipver = "IPv4";             
            } else {                        
                ipv6 = reinterpret_cast<struct sockaddr_in6 *>(p->ai_addr); 
                addr = &(ipv6->sin6_addr);  
                ipver = "IPv6";             
            }
            inet_ntop(p->ai_family, addr, ip_buffer, sizeof(ip_buffer));
            std::string ipstr(ip_buffer);
            std::cout << "Local interface found -> " << ipver << ": " << ipstr << std::endl;
            sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (sockfd == -1) {
                std::cerr << "Failed to create socket: " << std::strerror(errno) << ". Moving to next..." << std::endl;
                continue ;
            }
            std::cout << "Socket successfully created!" << std::endl;
            setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
            std::cout << "Attempting to bind to port " << port_str << "..." << std::endl;
            if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                std::cerr << "Bind failed: " << std::strerror(errno) << " Closing socket..." << std::endl;
                close(sockfd);
                continue ;
            }
            std::cout << "Successfully bound to " << ipstr << " on port " << port_str << "!" << std::endl;
            break ;
        }
        freeaddrinfo(res);
        if (p == NULL) {
            throw SystemError("Fatal error: Failed to bind to any of the local interfaces");
        }
        std::cout << "Setting up the listener..." << std::endl;
        if (listen(sockfd, BACKLOG) == -1) {
            throw SystemError("Fatal error: listen() failed");
        }
        std::cout << "Server is now actively listening on port " << port_str << "! (Backlog: 10)" << std::endl;
        std::cout << "Entering the main server loop..." << std::endl;
        std::map<int, Client> clients;
        while (true) {
            time_t current_time = std::time(NULL);
            for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ) {
                Client &client = it->second;
                if (std::difftime(current_time, client.getLastActivity()) > MAX_TIMEOUT) {
                    std::cout << "Client on socket " << client.getSocketFd() << " timed out due to inactivity. Closing connection." << std::endl;
                    client.setState(Client::DISCONNECTED);
                    close(client.getSocketFd());
                    clients.erase(it++);
                }
                else {
                    ++it;
                }
            }
            std::cout << "Waiting for incoming connections... (Program is blocked here)" << std::endl;
            struct sockaddr_storage client_addr;
            socklen_t               addr_size;
            int                     client_fd;
            addr_size = sizeof(client_addr);
            client_fd = accept(sockfd, reinterpret_cast<struct sockaddr *>(&client_addr), &addr_size);
                if (client_fd == -1) {
                std::cerr << "Error: accept() failed: " << std::strerror(errno) << std::endl;
                continue ;
            }
            Client  client(client_fd, client_addr);
            client.updateLastActivity();
            client.setState(Client::READING_REQUEST);
            clients[client_fd] = client;
            std::cout << "CONNECTION ACCEPTED!" << std::endl;
            std::cout << "Client IP: " << clients[client_fd].getIp() << std::endl;
            std::cout << "Communication is now open on new socket: " << clients[client_fd].getSocketFd() << std::endl;
            std::cout << "Listening socket " << sockfd << " is still active in the background." << std::endl;
            char buffer[1024];
            std::memset(buffer, 0, sizeof(buffer));
            ssize_t bytes_received = recv(client.getSocketFd(), buffer, sizeof(buffer) - 1, 0);
            if (bytes_received < 0) {
                std::cerr << "Error reading from socket." << std::endl;
            } else if (bytes_received == 0) {
                std::cout << "Client unexpectedly closed the connection." << std::endl;
                clients[client_fd].setState(Client::DISCONNECTED);
                close(client.getSocketFd());
                clients.erase(client.getSocketFd());
            } else {
                Client &current_client = clients[client_fd];
                current_client.updateLastActivity();
                std::cout << "--- RECEIVED " << bytes_received << " BYTES FROM CLIENT ---" << std::endl;
                std::string received_data(buffer, bytes_received);
                std::cout << received_data << std::endl;
                std::cout << "--------------------------------------" << std::endl;
                current_client.setState(Client::PROCESSING);
                std::string response = "Good talking to you!\n";
                current_client.setState(Client::WRITING_RESPONSE);
                ssize_t bytes_sent = send(client_fd, response.c_str(), response.size(), 0);
                if (bytes_sent < 0) {
                    std::cerr << "Error: sendind response to " << client_fd << std::endl;
                } else {
                    std::cout << "Successfully sent " << bytes_sent << " bytes back to the client " << client_fd << std::endl;
                    clients[client_fd].updateLastActivity();
                }
                clients[client_fd].setState(Client::DISCONNECTED); 
                close(client_fd);
                clients.erase(client_fd);
            }
        }
    }
    catch (const std::exception &e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        if (sockfd != -1) {
            close(sockfd);
        }
        return (1);
    }
    std::cout << "Shutting down the server (sockfd)." << std::endl;
    if (sockfd != -1) {
        close(sockfd);
    }
    return (0);
}

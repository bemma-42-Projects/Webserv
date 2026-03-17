#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "Server.hpp"
#include "SystemError.hpp"
#include "GaiError.hpp"

# define PORT "8080"
# define BACKLOG 128

# define MAX_TIMEOUT 10

Server::Server() : _server_socket(-1), _epoll_fd(-1) {

}
    
Server::~Server()
{
    if (_server_socket != -1)
        close(_server_socket);
    if (_epoll_fd != -1)
        close(_epoll_fd);
}

void    Server::_setNonBlocking(int fd) {
    int flags;

    flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		throw SystemError("fcntl(F_GETFL) failed");
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		throw SystemError("fcntl(F_SETFL) failed");
}

void    Server::_initAddrinfoParams(struct addrinfo &addrinfo_params) {
    memset(&addrinfo_params, 0, sizeof(addrinfo_params));
	addrinfo_params.ai_family = AF_UNSPEC;
	addrinfo_params.ai_socktype = SOCK_STREAM;
	addrinfo_params.ai_flags = AI_PASSIVE;
}

struct addrinfo *Server::_getAddrInfo(const std::string &port_str)
{
    struct addrinfo		addrinfo_params;
	struct addrinfo		*res;
	int					status;

    _initAddrinfoParams(addrinfo_params);
    if ((status = getaddrinfo(NULL, port_str.c_str(), &addrinfo_params, &res)) != 0) {
		throw GaiError("DNS/Setup Error", status);
	}
	std::cout << "Booting up server on port " << port_str << "..." << std::endl;
    return (res);
}

void    Server::_printInterface(struct addrinfo *p, char *ip_buffer) {
    void				*addr;
	std::string			ipver;
	struct sockaddr_in	*ipv4;
	struct sockaddr_in6	*ipv6;

	if (p->ai_family == AF_INET)
	{
		ipv4 = reinterpret_cast<struct sockaddr_in *>(p->ai_addr);
		addr = &(ipv4->sin_addr);
		ipver = "IPv4";
	}
	else
	{
		ipv6 = reinterpret_cast<struct sockaddr_in6 *>(p->ai_addr); 
		addr = &(ipv6->sin6_addr);
		ipver = "IPv6";
	}
	inet_ntop(p->ai_family, addr, ip_buffer, sizeof(ip_buffer));
	std::string ipstr(ip_buffer);
	std::cout << "Local interface found -> " << ipver << ": " << ipstr << std::endl;
}

void    Server::init() {

}

void	Server::run() {

}
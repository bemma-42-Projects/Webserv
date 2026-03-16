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

void    Server::_init_addrinfo_params(struct addrinfo &addrinfo_params) {
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

    _init_addrinfo_params(addrinfo_params);
    if ((status = getaddrinfo(NULL, port_str.c_str(), &addrinfo_params, &res)) != 0) {
		throw GaiError("DNS/Setup Error", status);
	}
	std::cout << "Booting up server on port " << port_str << "..." << std::endl;

    return (res);
}

void    Server::_print_interface(struct addrinfo *p, char *ip_buffer) {
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

bool    Server::_setupSocket(struct addrinfo *p, const std::string &port_str) {
    int yes = 1;

    _server_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
	if (_server_socket == -1) {
	    std::cerr << "Failed to create socket: " << strerror(errno) << ". Moving to next..." << std::endl;
	    return (false);
	}
	std::cout << "Socket successfully created!" << std::endl;
	
    setsockopt(_server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	std::cout << "Attempting to bind to port " << port_str << "..." << std::endl;
	
    if (bind(_server_socket, p->ai_addr, p->ai_addrlen) == -1) {
		std::cerr << "Bind failed: " << strerror(errno) << " Closing socket..." << std::endl;
		close(_server_socket);
        _server_socket = -1;
		return (false) ;
	}
	std::cout << "Successfully bound to port " << port_str << "!" << std::endl;
	return (true) ;
}

void    Server::_bindSocketLoop(struct addrinfo *res, const std::string &port_str) {
    struct addrinfo		*p;
    char				ip_buffer[INET6_ADDRSTRLEN];

    for (p = res; p != NULL; p = p->ai_next)
	{
		_print_interface(p, ip_buffer);
        if (_setupSocket(p, port_str))
            break ;
    }
    freeaddrinfo(res);
	if (p == NULL)
		throw SystemError("Fatal error: Failed to bind to any of the local interfaces");
}

void    Server::_createAndBindSocket(const std::string &port_str) {
    struct addrinfo *res;

    res = _getAddrInfo(port_str);
    _bindSocketLoop(res, port_str);
}

void	Server::_startListening() {
	std::cout << "Setting up the listener..." << std::endl;
    if (listen(_server_socket, BACKLOG) == -1)
        throw SystemError("Fatal error: listen() failed");

    std::cout << "Server is now actively listening on port " << PORT << "! (Backlog: " << BACKLOG << ")" << std::endl;
}

void	Server::_initEpoll() {
	_epoll_fd = epoll_create(MAX_EVENTS);
    if (_epoll_fd == -1) {
        throw SystemError("Fatal error: epoll_create() failed");
    }

	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd = _server_socket;

    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _server_socket, &ev) == -1)
        throw SystemError("Fatal error: epoll_ctl() failed on _server_socket");
}

void    Server::init() {
    _createAndBindSocket(PORT);
	_startListening();
	_setNonBlocking(_server_socket);
	_initEpoll();
}


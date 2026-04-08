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

#define PORT "8080"
#define BACKLOG 128

#define MAX_TIMEOUT 10

Server::Server() : server_socket_(-1), epoll_fd_(-1) {

}

Server::~Server()
{
    if (server_socket_ != -1)
        close(server_socket_);
    if (epoll_fd_ != -1)
        close(epoll_fd_);
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

bool    Server::_setupSocket(struct addrinfo *p, const std::string &port_str) {
    int yes = 1;
    server_socket_ = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
	if (server_socket_ == -1) {
	    std::cerr << "Failed to create socket: " << strerror(errno) << ". Moving to next..." << std::endl;
	    return (false);
	}
	std::cout << "Socket successfully created!" << std::endl;
    if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
	 	std::cerr << "Failed to set socket option to SO_REUSEADDR." << std::endl;
	    return (false);
	}
	std::cout << "Attempting to bind to port " << port_str << "..." << std::endl;
    if (bind(server_socket_, p->ai_addr, p->ai_addrlen) == -1) {
		std::cerr << "Bind failed: " << strerror(errno) << " Closing socket..." << std::endl;
		close(server_socket_);
        server_socket_ = -1;
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
		_printInterface(p, ip_buffer);
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
    if (listen(server_socket_, BACKLOG) == -1)
        throw SystemError("Fatal error: listen() failed");
    std::cout << "Server is now actively listening on port " << PORT << "! (Backlog: " << BACKLOG << ")" << std::endl;
}

void	Server::_initEpoll() {
	epoll_fd_ = epoll_create(MAX_EVENTS);
    if (epoll_fd_ == -1) {
        throw SystemError("Fatal error: epoll_create() failed");
    }
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd = server_socket_;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, server_socket_, &ev) == -1)
        throw SystemError("Fatal error: epoll_ctl() failed on server_socket_");
}

void    Server::init() {
    _createAndBindSocket(PORT);
	_startListening();
	_setNonBlocking(server_socket_);
	_initEpoll();
}

void    Server::_handleTimeouts() {
    time_t current_time = std::time(NULL);
    for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ) {
        Client &client = it->second;
        if (std::difftime(current_time, client.getLastActivity()) > MAX_TIMEOUT) {
            std::cout << "Client on socket " << client.getSocketFd() << " timed out due to inactivity. Closing connection." << std::endl;
            client.setState(Client::DISCONNECTED);
            epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client.getSocketFd(), NULL);
            close(client.getSocketFd());
            _clients.erase(it++);
        }
        else {
            ++it;
        }
    }
}

void	Server::_handleClientDisconnect(int client_fd) {
	epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_fd, NULL);
    close(client_fd);
    _clients.erase(client_fd);
}

bool	Server::_addClientToEpoll(int client_fd) {
	struct epoll_event client_ev;

    memset(&client_ev, 0, sizeof(client_ev));
    client_ev.events = EPOLLIN;
    client_ev.data.fd = client_fd;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &client_ev) == -1) {
        std::cerr << "Error: epoll_ctl(ADD) failed on client_fd " << client_fd << ": " << std::strerror(errno) << std::endl;
        close(client_fd);
        _clients.erase(client_fd);
        return (false);
    }
	return (true);
}

void	Server::_logNewConnection(int client_fd) {
	std::cout << "CONNECTION ACCEPTED!" << std::endl;
    std::cout << "Client IP: " << _clients[client_fd].getIp() << std::endl;
    std::cout << "Communication is now open on new socket: " << client_fd << std::endl;
    std::cout << "Listening socket " << server_socket_ << " is still active in the background." << std::endl;
}

void    Server::_handleNewConnection() {
    struct sockaddr_storage client_addr;
    socklen_t addr_size;
	int client_fd;

	addr_size = sizeof(client_addr);
    client_fd = accept(server_socket_, reinterpret_cast<struct sockaddr *>(&client_addr), &addr_size);
    if (client_fd == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return ;
        std::cerr << "Error: accept() failed: " << std::strerror(errno) << std::endl;
        return ;
    }
    _setNonBlocking(client_fd);
    Client  client(client_fd, client_addr);
    client.updateLastActivity();
    client.setState(Client::READING_REQUEST);
    _clients[client_fd] = client;
    if (!_addClientToEpoll(client_fd)) {
        return ;
    }
	_logNewConnection(client_fd);
}

void	Server::_setSocketToWriteState(int client_fd) {
	struct epoll_event mod_ev;
    
	mod_ev.events = EPOLLIN | EPOLLOUT;
    mod_ev.data.fd = client_fd;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, client_fd, &mod_ev) == -1) {
        std::cerr << "Error: epoll_ctl(MOD) failed on socket " << client_fd << ": " << std::strerror(errno) << std::endl;
        _handleClientDisconnect(client_fd);
		return ;
	}
    std::cout << "Socket " << client_fd << " switched to EPOLLOUT. Waiting for network to be ready to send..." << std::endl;
}

bool	Server::_isRequestComplete(Client &client) {
    (void)client;
    return true;
}

std::string	Server::_buildHttpResponse(Client &client) {
    (void)client;
	return ("Good talking to you!\n");
}

void    Server::_bufferizeResponse(Client& client, const std::string& response) {
    (void)client;
    (void)response;
}

void	Server::_processClientRequest(int client_fd, const std::string& received_data) {
    (void)received_data;
	Client	&client = _clients[client_fd];

    if (_isRequestComplete(client)) {
        client.setState(Client::PROCESSING);
        std::string response = _buildHttpResponse(client);
		_bufferizeResponse(client, response);
        client.setState(Client::WRITING_RESPONSE);
		_setSocketToWriteState(client_fd);
	}
	else
		std::cout << "Socket " << client_fd << ": Request incomplete. Waiting for more data..." << std::endl;
}

void	Server::_handleClientRead(int client_fd) {
	char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytes_received;

	bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        if (bytes_received == 0)
            std::cout << "Client on socket " << client_fd << " closed the connection." << std::endl;
        else {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return ;
            std::cerr << "Error: recv() failed on socket " << client_fd << ": " << std::strerror(errno) << std::endl;
		}
		_handleClientDisconnect(client_fd);
		return ;
	}
    _clients[client_fd].updateLastActivity();
    std::string received_data(buffer, bytes_received);
    std::cout << "--- RECEIVED " << bytes_received << " BYTES FROM SOCKET " << client_fd << " ---\n" << received_data << std::endl;
	_processClientRequest(client_fd, received_data);
}

std::string	Server::_getResponseToSend(Client& client) {
    (void)client;
    return "Good talking to you!\n";
}

bool	Server::_isResponseFullySent(Client& client, ssize_t bytes_sent) {
    (void)client;
    (void)bytes_sent;
    return true;
}

void	Server::_clearClientBuffers(Client &client)
{
	client.setState(Client::READING_REQUEST);
}

void	Server::_setSocketToReadState(int client_fd) {
	struct epoll_event listen_ev;

    listen_ev.events = EPOLLIN;
    listen_ev.data.fd = client_fd;
	if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, client_fd, &listen_ev) == -1) {
        std::cerr << "Error: epoll_ctl(MOD) failed on socket " << client_fd << ": " << std::strerror(errno) << std::endl;
        _handleClientDisconnect(client_fd);
		return ;
	}
    std::cout << "Socket " << client_fd << " kept alive. Waiting for next request..." << std::endl;
}

void    Server::_handleClientWrite(int client_fd) {
    ssize_t 		bytes_sent;

	Client	&client = _clients[client_fd];
    std::string response_to_send = _getResponseToSend(client);
	bytes_sent = send(client_fd, response_to_send.c_str(), response_to_send.size(), 0);
    if (bytes_sent < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return ;
        std::cerr << "Error: send() failed on socket " << client_fd << ": " << std::strerror(errno) << std::endl;
		_handleClientDisconnect(client_fd);
		return ;
	}
    else if (bytes_sent == 0) {
        std::cout << "Notice: 0 bytes sent to socket " << client_fd << " (Network buffer full)" << std::endl;
        return ;
	}
    std::cout << "Successfully sent " << bytes_sent << " bytes back to socket " << client_fd << std::endl;
    client.updateLastActivity();
	if (_isResponseFullySent(client, bytes_sent))
	{
		_clearClientBuffers(client);
		_setSocketToReadState(client_fd);
	}
}

void	Server::run() {
    struct epoll_event  events[MAX_EVENTS];
	int	n_events;

	std::cout << "Entering the main server loop..." << std::endl;
    while (g_running) {
        _handleTimeouts();   	
		n_events = epoll_wait(epoll_fd_, events, MAX_EVENTS, 1000);
		if (n_events == -1)
		{
			if (errno == EINTR)
				break ;
            throw SystemError("Fatal error: epoll_wait() failed.");
		}
        for (int i = 0; i < n_events; i++) {
            int client_fd = events[i].data.fd;
            if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                if (client_fd == server_socket_) {
                    throw SystemError("Fatal error: Server socket encountered an error or hung up.");
                } else {
                    std::cerr << "Epoll error or hang up on client socket " << client_fd << std::endl;
                    _handleClientDisconnect(client_fd);
                }
			}
            else if (client_fd == server_socket_)
                _handleNewConnection();
			else if (events[i].events & EPOLLIN)
				_handleClientRead(client_fd);
			else if (events[i].events & EPOLLOUT)
				_handleClientWrite(client_fd);
		}
	}
}

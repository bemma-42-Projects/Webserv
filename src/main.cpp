#define _POSIX_C_SOURCE 200112L
#include <string.h>
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
#include <sys/epoll.h>
#include <errno.h>
#include <cstdio>

#include "Client.hpp"
#include "SystemError.hpp"
#include "GaiError.hpp"


#define PORT "8080"
#define BACKLOG 128
#define MAX_EVENTS 64
#define MAX_TIMEOUT 50

void set_nonblocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		throw SystemError("fcntl(F_GETFL) failed");
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		throw SystemError("fcntl(F_SETFL) failed");
}

void init_addrinfo_parm(struct addrinfo &addrinfo_param) {
	memset(&addrinfo_param, 0, sizeof addrinfo_param);
	addrinfo_param.ai_family = AF_UNSPEC;
	addrinfo_param.ai_socktype = SOCK_STREAM;
	addrinfo_param.ai_flags = AI_PASSIVE;
}

void print_interface(struct addrinfo *p, char *ip_buffer) {
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

int create_and_bind_socket_server(const std::string &port_str) {
	struct addrinfo		addrinfo_param;
	struct addrinfo		*res;
	struct addrinfo		*p;
	int					status;
	char				ip_buffer[INET6_ADDRSTRLEN];
	int					sock_fd = -1;
	int					yes = 1;

	init_addrinfo_parm(addrinfo_param);
	if ((status = getaddrinfo(NULL, port_str.c_str(), &addrinfo_param, &res)) != 0) {
		throw GaiError("DNS/Setup Error", status);
	}
	std::cout << "Booting up server on port " << port_str << "..." << std::endl;
	
	for (p = res; p != NULL; p = p->ai_next)
	{
		print_interface(p, ip_buffer);
		sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sock_fd == -1)
		{
			std::cerr << "Failed to create socket: " << strerror(errno) << ". Moving to next..." << std::endl;
			continue ;
		}
		std::cout << "Socket successfully created!" << std::endl;
		setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		std::cout << "Attempting to bind to port " << port_str << "..." << std::endl;
		if (bind(sock_fd, p->ai_addr, p->ai_addrlen) == -1)
		{
			std::cerr << "Bind failed: " << strerror(errno) << " Closing socket..." << std::endl;
			close(sock_fd);
			continue ;
		}
		std::cout << "Successfully bound to port " << port_str << "!" << std::endl;
		break ;
	}
	freeaddrinfo(res);
	if (p == NULL)
	{
		throw SystemError("Fatal error: Failed to bind to any of the local interfaces");
	}
	return (sock_fd);
}

void start_listening(int sock_fd) {
	std::cout << "Setting up the listener..." << std::endl;

		if (listen(sock_fd, BACKLOG) == -1) {
			throw SystemError("Fatal error: listen() failed");
		}
		set_nonblocking(sock_fd);
}

int setup_epoll(int sock_fd) {
	int epoll_fd = epoll_create(MAX_EVENTS);

	if (epoll_fd == -1) {
		throw SystemError("Fatal error: epoll_create() failed.\n");
	}

	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = sock_fd;

	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &ev) == -1) {
		close(epoll_fd);
		throw SystemError("Fatal error: epoll_ctl() failed.\n");
	}

	return (epoll_fd);
}

void	erase_client(int epoll_fd, int client_fd, std::map<int, Client> &clients) {
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
	close(client_fd);
	clients.erase(client_fd);
}

void	handle_new_connection(int sock_fd, int epoll_fd, std::map<int, Client> &clients) {
	std::cout << "Waiting for incoming connections... (Program is blocked here)" << std::endl;

	struct sockaddr_storage	client_addr;
	socklen_t				addr_size;
	int						client_fd;

	addr_size = sizeof(client_addr);
	client_fd = accept(sock_fd, reinterpret_cast<struct sockaddr *>(&client_addr), &addr_size);
	if (client_fd == -1) {
		std::cerr << "Error: accept() failed: " << strerror(errno) << std::endl;
		return ;
	}
	set_nonblocking(client_fd);
	Client  client(client_fd, client_addr);
	client.updateLastActivity();
	client.setState(Client::READING_REQUEST);
	clients[client_fd] = client;
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = client_fd;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == 1000) {
		perror("epoll_ctl: client_fd");
		close(client_fd);
		return ;
	}
	std::cout << "CONNECTION ACCEPTED!" << std::endl;
	std::cout << "Client IP: " << clients[client_fd].getIp() << std::endl;
	std::cout << "Communication is now open on new socket: " << clients[client_fd].getSocketFd() << std::endl;
	std::cout << "Listening socket " << sock_fd << " is still active in the background." << std::endl;
}


void handle_client_read(int epoll_fd, std::map<int, Client> &clients, int client_fd) {
	char buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
	if (bytes_received <= 0) {
		if (bytes_received == 0) {
			std::cout << "Client on socket " << client_fd << " closed the connection." << std::endl;
		}
		else {
			std::cerr << "Error: recv() failed on socket " << client_fd << ": " << strerror(errno) << std::endl;
		}
		erase_client(epoll_fd, client_fd, clients);
		return;
	}
	else {
		Client &current_client = clients[client_fd];
		current_client.updateLastActivity();
		std::cout << "--- RECEIVED " << bytes_received << " BYTES FROM CLIENT " << client_fd << " ---" << std::endl;
		bool is_request_complete = true;
		if (is_request_complete) {
			current_client.setState(Client::PROCESSING);
			std::string response = "Good talking to you!\n";
			current_client.setState(Client::WRITING_RESPONSE);
			struct epoll_event mod_ev;
			mod_ev.events = EPOLLOUT;
			mod_ev.data.fd = client_fd;
			if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &mod_ev) == -1)
			{
				std::cerr << "Error: epoll_ctl(MOD) failed on socket " << client_fd << ": " << strerror(errno) << std::endl;
				erase_client(epoll_fd, client_fd, clients);
				return;
			}
			std::string received_data(buffer, bytes_received);
			std::cout << received_data << std::endl;
		}
	}
}

void handle_client_write(int epoll_fd, int client_fd, std::map<int, Client> &clients) {
	Client &current_client = clients[client_fd];
	std::string response_to_send;
	response_to_send = "Good talking to you!\n";
	ssize_t bytes_sent = send(client_fd, response_to_send.c_str(), response_to_send.size(), 0);
	if (bytes_sent < 0) {
		std::cerr << "Error: send() failed on socket " << client_fd << ": " << strerror(errno) << std::endl;
	}
	else if (bytes_sent == 0) {
		std::cout << "Notice: 0 bytes sent to socket " << client_fd << " (Network buffer full)" << std::endl;
		return ;
	} else {
		std::cout << "Successfully sent " << bytes_sent << " bytes back to socket " << client_fd << std::endl;
		current_client.updateLastActivity();
	}
	struct epoll_event listen_ev;
	listen_ev.events = EPOLLIN;
	listen_ev.data.fd = client_fd;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &listen_ev) == 1000)
	{
		erase_client(epoll_fd, client_fd, clients);
	} else {
		current_client.setState(Client::READING_REQUEST);
		std::cout << "Socket " << client_fd << " kept alive. Waiting for next request..." << std::endl;
	}
}

void handle_timeout_clients(int epoll_fd, std::map<int, Client> &clients) {
	time_t current_time = std::time(NULL);

	for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); )
	{
		Client &client = it->second;

		if (std::difftime(current_time, client.getLastActivity()) > MAX_TIMEOUT) 
		{
			std::cout << "Client on socket " << client.getSocketFd() << " timed out due to inactivity. Closing connection." << std::endl;
			client.setState(Client::DISCONNECTED);
			epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client.getSocketFd(), NULL);
			close(client.getSocketFd());
			clients.erase(it++);
		}
		else
		{
			++it;
		}
	}
}

int main(void)
{
	int					sock_fd = -1;
	const std::string	port_str = PORT;
	int					epoll_fd = -1;

	try {
		sock_fd = create_and_bind_socket_server(port_str);
		start_listening(sock_fd);
		epoll_fd = setup_epoll(sock_fd);

		struct epoll_event events[MAX_EVENTS];

		std::cout << "Server is now actively listening on port " << port_str << "! (Backlog: 10)" << std::endl;
		std::cout << "Entering the main server loop..." << std::endl;
		std::map<int, Client> clients;

		while (1)
		{
			handle_timeout_clients(epoll_fd, clients);

			int n_events = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000);
			if (n_events == -1) {
				perror("epoll_wait");
				return 1;
			}

			// Traiter chaque événement
			for (int i = 0; i < n_events; i++)
			{
				if (events[i].data.fd == sock_fd) {
					handle_new_connection(sock_fd, epoll_fd, clients);
				}
				// CAS 2 : DONNÉES À LIRE D'UN CLIENT
				else if (events[i].events & EPOLLIN) {
					handle_client_read(epoll_fd, clients, events[i].data.fd);
				}

				// CAS 3 : PRÊT À ÉCRIRE LA RÉPONSE À UN CLIENT
				else if (events[i].events & EPOLLOUT) 
				{
					handle_client_write(epoll_fd, events[i].data.fd, clients);
				}
				// CAS 4 : ERREUR SUR LE SOCKET CLIENT
				else if (events[i].events & EPOLLERR)
				{
					int client_fd = events[i].data.fd;
					printf("Error on client %d\n", client_fd);
					// free_client(client_fd, epoll_fd);
				}
			}
		}
	}

	catch (const std::exception &e) {
		std::cerr << "Fatal error: " << e.what() << std::endl;
		if (sock_fd != -1) {
			close(sock_fd);
		}
		return (1);
	}
	std::cout << "Shutting down the server (sock_fd)." << std::endl;
	if (sock_fd != -1) {
		close(sock_fd);
	}
	return (0);
}

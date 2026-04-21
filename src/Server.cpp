#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#include "Server.hpp"
#include "utils.hpp"

#define PORT "8080"
#define BACKLOG 128

#define MAX_TIMEOUT 10

// constructeur par défaut, qui initialise le socket serveur et le fd de epoll à -1
Server::Server() : server_socket_(-1), epoll_fd_(-1) {
    
}

// destructeur
Server::~Server()
{
    if (server_socket_ != -1)
        close(server_socket_);
    if (epoll_fd_ != -1)
        close(epoll_fd_);
}

// initialise les hints pour getaddrinfo
// permet d'utiliser IPv4 et IPv6
void    Server::initAddrinfoParams_(struct addrinfo &addrinfo_params) {
    memset(&addrinfo_params, 0, sizeof(addrinfo_params));
	addrinfo_params.ai_family = AF_UNSPEC;
	addrinfo_params.ai_socktype = SOCK_STREAM;
	addrinfo_params.ai_flags = AI_PASSIVE;
}

// récupère les infos d'addresse système
// list chainee d'interfaces réseau sur lesquelles le serveur peut se binder
struct addrinfo *Server::getAddrInfo_(const std::string &port_str)
{
    struct addrinfo		addrinfo_params;
	struct addrinfo		*res;
	int					status;

    initAddrinfoParams_(addrinfo_params);
    if ((status = getaddrinfo(NULL, port_str.c_str(), &addrinfo_params, &res)) != 0) {
		throw std::runtime_error(std::string("DNS/Setup Error") + gai_strerror(status));
	}
	std::cout << "Booting up server on port " << port_str << "..." << std::endl;
    return (res);
}

// affiche l'adresse IP de l'interface réseau
void    Server::printInterface_(struct addrinfo *p, char *ip_buffer) {
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

// tente de créer un socket et configure ses options
bool    Server::setupSocket_(struct addrinfo *p, const std::string &port_str) {
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

// boucle sur les interfaces réseau pour binder le serveur
void    Server::bindSocketLoop_(struct addrinfo *res, const std::string &port_str) {
    struct addrinfo		*p;
    char				ip_buffer[INET6_ADDRSTRLEN];

    for (p = res; p != NULL; p = p->ai_next)
	{
		printInterface_(p, ip_buffer);
        if (setupSocket_(p, port_str))
            break ;
    }
    freeaddrinfo(res);
	if (p == NULL)
		throw std::runtime_error("Fatal error: Failed to bind to any of the local interfaces");
}

// orchestre la création et le bind du socket principal
void    Server::createAndBindSocket_(const std::string &port_str) {
    struct addrinfo *res;

    res = getAddrInfo_(port_str);
    bindSocketLoop_(res, port_str);
}

// met le socket serveur en mode écoute
void	Server::startListening_() {
	std::cout << "Setting up the listener..." << std::endl;
    if (listen(server_socket_, BACKLOG) == -1)
        throw std::runtime_error("Fatal error: listen() failed");
    std::cout << "Server is now actively listening on port " << PORT << "! (Backlog: " << BACKLOG << ")" << std::endl;
}

// intialise l'instance epoll
void	Server::initEpoll_() {
	epoll_fd_ = epoll_create(MAX_EVENTS);
    if (epoll_fd_ == -1) {
        throw std::runtime_error("Fatal error: epoll_create() failed");
    }
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd = server_socket_;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, server_socket_, &ev) == -1)
        throw std::runtime_error("Fatal error: epoll_ctl() failed on server_socket_");
}

// initalise l'infrastructure réseau du serveur
void    Server::init() {
    createAndBindSocket_(PORT);
	startListening_();
	setNonBlocking(server_socket_);
	initEpoll_();
}

// déconnecte les clients inactifs
void    Server::handleTimeouts_() {
    time_t current_time = std::time(NULL);
    for (std::map<int, Client>::iterator it = clients_.begin(); it != clients_.end(); ) {
        Client &client = it->second;
        if (std::difftime(current_time, client.getLastActivity()) > MAX_TIMEOUT) {
            std::cout << "Client on socket " << client.getSocketFd() << " timed out due to inactivity. Closing connection." << std::endl;
            client.setState(Client::DISCONNECTED);
            epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client.getSocketFd(), NULL);
            close(client.getSocketFd());
            clients_.erase(it++);
        }
        else {
            ++it;
        }
    }
}

// gère la déconnexion propre d'un client
void	Server::handleClientDisconnect_(int client_fd) {
	epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_fd, NULL);
    close(client_fd);
    clients_.erase(client_fd);
}

// ajoute un nouveau socket client à la surveillance epoll
bool	Server::addClientToEpoll_(int client_fd) {
	struct epoll_event client_ev;

    memset(&client_ev, 0, sizeof(client_ev));
    client_ev.events = EPOLLIN;
    client_ev.data.fd = client_fd;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &client_ev) == -1) {
        std::cerr << "Error: epoll_ctl(ADD) failed on client_fd " << client_fd << ": " << std::strerror(errno) << std::endl;
        close(client_fd);
        clients_.erase(client_fd);
        return (false);
    }
	return (true);
}

// log une nouvelle connection
void	Server::logNewConnection_(int client_fd) {
	std::cout << "CONNECTION ACCEPTED!" << std::endl;
    std::cout << "Client IP: " << clients_[client_fd].getIp() << std::endl;
    std::cout << "Communication is now open on new socket: " << client_fd << std::endl;
    std::cout << "Listening socket " << server_socket_ << " is still active in the background." << std::endl;
}

// accepte et configure une nouvelle connexion cliente
void    Server::handleNewConnection_() {
    struct sockaddr_storage client_addr;
    socklen_t addr_size;
	int client_fd;

	addr_size = sizeof(client_addr);
    client_fd = accept(server_socket_, reinterpret_cast<struct sockaddr *>(&client_addr), &addr_size);
    if (client_fd == -1) {
        std::cerr << "Error: accept() failed: " << std::strerror(errno) << std::endl;
        return ;
    }
    setNonBlocking(client_fd);
    Client  client(client_fd, client_addr);
    client.updateLastActivity();
    client.setState(Client::READING_REQUEST);
    clients_[client_fd] = client;
    if (!addClientToEpoll_(client_fd)) {
        return ;
    }
	logNewConnection_(client_fd);
}

// bascule la surveillance epoll d'un client en mode écriture
void	Server::setSocketToWriteState_(int client_fd) {
	struct epoll_event mod_ev;
    
	mod_ev.events = EPOLLIN | EPOLLOUT;
    mod_ev.data.fd = client_fd;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, client_fd, &mod_ev) == -1) {
        std::cerr << "Error: epoll_ctl(MOD) failed on socket " << client_fd << ": " << std::strerror(errno) << std::endl;
        handleClientDisconnect_(client_fd);
		return ;
	}
    std::cout << "Socket " << client_fd << " switched to EPOLLOUT. Waiting for network to be ready to send..." << std::endl;
}

// vérifie si la requête http du client est complètement reçue
bool	Server::isRequestComplete_(Client &client) {
    (void)client;
    return true;
}

// génère la réponse HTTP
std::string	Server::buildHttpResponse_(Client &client) {
    (void)client;
	return ("Good talking to you!\n");
}

// stocke la réponse générée dans le buffer d'écriture du client
void    Server::bufferizeResponse_(Client& client, const std::string& response) {
    (void)client;
    (void)response;
}

// traite les données brutes reçues d'un client
void	Server::processClientRequest_(int client_fd, const std::string& received_data) {
    //(void)received_data;
    std::cout << received_data << std::endl;
	Client	&client = clients_[client_fd];

    if (isRequestComplete_(client)) {
        client.setState(Client::PROCESSING);
        std::string response = buildHttpResponse_(client);
		bufferizeResponse_(client, response);
        client.setState(Client::WRITING_RESPONSE);
		setSocketToWriteState_(client_fd);
	}
	else
		std::cout << "Socket " << client_fd << ": Request incomplete. Waiting for more data..." << std::endl;
}

// gère l'évènement de lecture sur un socket client
void	Server::handleClientRead_(int client_fd) {
	char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytes_received;

	bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        if (bytes_received == 0)
            std::cout << "Client on socket " << client_fd << " closed the connection." << std::endl;
        else {
            std::cerr << "Error: recv() failed on socket " << client_fd << ": " << std::strerror(errno) << std::endl;
		}
		handleClientDisconnect_(client_fd);
		return ;
	}
    clients_[client_fd].updateLastActivity();
    std::string received_data(buffer, bytes_received);
    std::cout << "--- RECEIVED " << bytes_received << " BYTES FROM SOCKET " << client_fd << " ---\n" << received_data << std::endl;
	processClientRequest_(client_fd, received_data);
}

// récupère le prochain bloc de données à envoyer au client
std::string	Server::getResponseToSend_(Client& client) {
    (void)client;
    return "Good talking to you!\n";
}

// vérifie si la réponse entière a été transmise
bool	Server::isResponseFullySent_(Client& client, ssize_t bytes_sent) {
    (void)client;
    (void)bytes_sent;
    return true;
}

// réinitialise l'état du client après une transaction
void	Server::clearClientBuffers_(Client &client)
{
	client.setState(Client::READING_REQUEST);
}

// bascule la surveillance epoll d'un client en mode lecture
void	Server::setSocketToReadState_(int client_fd) {
	struct epoll_event listen_ev;

    listen_ev.events = EPOLLIN;
    listen_ev.data.fd = client_fd;
	if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, client_fd, &listen_ev) == -1) {
        std::cerr << "Error: epoll_ctl(MOD) failed on socket " << client_fd << ": " << std::strerror(errno) << std::endl;
        handleClientDisconnect_(client_fd);
		return ;
	}
    std::cout << "Socket " << client_fd << " kept alive. Waiting for next request..." << std::endl;
}

// gère l'évènement d'écriture sur un socket client
void    Server::handleClientWrite_(int client_fd) {
    ssize_t 		bytes_sent;

	Client	&client = clients_[client_fd];
    std::string response_to_send = getResponseToSend_(client);
	bytes_sent = send(client_fd, response_to_send.c_str(), response_to_send.size(), 0);
    if (bytes_sent < 0) {
        std::cerr << "Error: send() failed on socket " << client_fd << ": " << std::strerror(errno) << std::endl;
		handleClientDisconnect_(client_fd);
		return ;
	}
    else if (bytes_sent == 0) {
        std::cout << "Notice: 0 bytes sent to socket " << client_fd << " (Network buffer full)" << std::endl;
        return ;
	}
    std::cout << "Successfully sent " << bytes_sent << " bytes back to socket " << client_fd << std::endl;
    client.updateLastActivity();
	if (isResponseFullySent_(client, bytes_sent))
	{
		clearClientBuffers_(client);
		setSocketToReadState_(client_fd);
	}
}

// lance la boucle d'évènements principale du serveur
void	Server::run() {
    struct epoll_event  events[MAX_EVENTS];
	int	n_events;

	std::cout << "Entering the main server loop..." << std::endl;
    while (g_running) {
        handleTimeouts_();   	
		n_events = epoll_wait(epoll_fd_, events, MAX_EVENTS, 1000);
		if (n_events == -1)
            throw std::runtime_error("Fatal error: epoll_wait() failed.");
        for (int i = 0; i < n_events; i++) {
            int client_fd = events[i].data.fd;
            if (client_fd == server_socket_)
                handleNewConnection_();
			else if (events[i].events & EPOLLIN)
				handleClientRead_(client_fd);
			else if (events[i].events & EPOLLOUT)
				handleClientWrite_(client_fd);
		}
	}
}

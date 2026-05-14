#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h> // pour waitpid

#include "Client.hpp"
#include "Server.hpp"
#include "utils.hpp"
#include "RequestAnswer.hpp"
#include "Error.hpp"

//#define PORT "8080"
#define BACKLOG 128

#define MAX_TIMEOUT 10

// constructeur par défaut, qui initialise le socket serveur et le fd de epoll à -1
Server::Server(const std::vector<ServerConfig> &configs) : epoll_fd_(-1), configs_(configs) {
	
}

// destructeur
Server::~Server()
{
	std::map<int, ServerConfig*>::iterator	listen_it = listen_sockets_.begin();

	while (listen_it != listen_sockets_.end())
	{
		close(listen_it->first);
		++listen_it;
	}
	listen_sockets_.clear();

	if (epoll_fd_ != -1) {
		close(epoll_fd_);
	}

	std::map<int, Client*>::iterator	it = clients_.begin();
	while (it != clients_.end()) {
		close(it->first);
		delete it->second;
		++it;
	}
	clients_.clear();

	std::map<int, int>::iterator	cgi_it = cgi_to_client_.begin();
	while (cgi_it != cgi_to_client_.end()) {
		close(cgi_it->first);
		++cgi_it;
	}
	cgi_to_client_.clear();
}

// initialise les hints pour getaddrinfo
// permet d'utiliser IPv4 et IPv6
void	Server::initAddrinfoParams_(struct addrinfo &addrinfo_params) {
	memset(&addrinfo_params, 0, sizeof(addrinfo_params));
	addrinfo_params.ai_family = AF_UNSPEC;
	addrinfo_params.ai_socktype = SOCK_STREAM;
	addrinfo_params.ai_flags = AI_PASSIVE;
}

// récupère les infos d'addresse système
// list chainee d'interfaces réseau sur lesquelles le serveur peut se binder
struct addrinfo	*Server::getAddrInfo_(const std::string &ip, const std::string &port_str)
{
	struct addrinfo		addrinfo_params;
	struct addrinfo		*res;
	int					status;

	this->initAddrinfoParams_(addrinfo_params);
	if ((status = getaddrinfo(ip.c_str(), port_str.c_str(), &addrinfo_params, &res)) != 0) {
		throw std::runtime_error(std::string("DNS/Setup Error") + gai_strerror(status));
	}
	return (res);
}

// affiche l'adresse IP de l'interface réseau
void	Server::printInterface_(struct addrinfo *p, char *ip_buffer) {
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
}

// tente de créer un socket et configure ses options
int	Server::setupSocket_(struct addrinfo *p) {
	int	fd;
	int	yes = 1;

	fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
	if (fd == -1) {
		std::cerr << "Failed to create socket. Moving to next..." << std::endl;
		return (-1);
	}
	//std::cout << "Socket successfully created!" << std::endl;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
	 	std::cerr << "Failed to set socket option to SO_REUSEADDR." << std::endl;
		close(fd);
		return (-1);
	}
	//std::cout << "Attempting to bind to port " << port_str << "..." << std::endl;
	if (bind(fd, p->ai_addr, p->ai_addrlen) == -1) {
		std::cerr << "Bind failed. Closing socket..." << std::endl;
		close(fd);
		return (-1);
	}
	return (fd);
}

// boucle sur les interfaces réseau pour binder le serveur
int	Server::bindSocketLoop_(struct addrinfo *res) {
	struct addrinfo		*p;
	int					fd = -1;

	for (p = res; p != NULL; p = p->ai_next)
	{
		fd = this->setupSocket_(p);
		if (fd != -1)
			break ;
	}
	freeaddrinfo(res);

	return (fd);
}

// orchestre la création et le bind du socket principal
int	Server::createAndBindSocket_(const std::string &ip, const std::string &port_str) {
	struct addrinfo	*res;

	res = getAddrInfo_(ip, port_str);
	int	fd = bindSocketLoop_(res);
	if (fd == -1)
		throw std::runtime_error("Fatal: Failed to bind to " + ip + ":" + port_str);
	return (fd);
}

// met le socket serveur en mode écoute
void	Server::startListening_(int fd) {
	if (listen(fd, BACKLOG) == -1)
	{
		close(fd);
		throw std::runtime_error("Fatal error: listen() failed");
	}
}

void	Server::addListenSocketToEpoll_(int fd) {
	struct epoll_event	ev;
	memset(&ev, 0, sizeof(ev));
	ev.events = EPOLLIN;
	ev.data.fd = fd;
	if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) == -1)
		throw std::runtime_error("Fatal error: epoll_ctl() failed on server_socket_");
}

// intialise l'instance epoll
void	Server::initEpoll_() {
	epoll_fd_ = epoll_create(MAX_EVENTS);
	if (epoll_fd_ == -1) {
		throw std::runtime_error("Fatal error: epoll_create() failed");
	}	
}

// initalise l'infrastructure réseau du serveur
void	Server::init() {
	this->initEpoll_();
	for (size_t i = 0; i < configs_.size(); ++i)
	{
		const std::vector<Listen>	&listens = configs_[i].getListen();
		for (size_t j = 0; j < listens.size(); ++j)
		{
			std::string host = listens[j].ip;
			
			std::stringstream	ss_port;
			ss_port << listens[j].port;
			std::string	port = ss_port.str();

			int fd = this->createAndBindSocket_(listens[j].ip, port);

			this->startListening_(fd);
			setNonBlocking(fd);
			
			this->listen_sockets_[fd] = &configs_[i];
			this->addListenSocketToEpoll_(fd);

		}  
	}
}

// déconnecte les clients inactifs
void	Server::handleTimeouts_() {
	time_t	current_time = std::time(NULL);
	for (std::map<int, Client*>::iterator it = this->clients_.begin(); it != clients_.end(); ) {
		Client	*client = it->second;

		if (std::difftime(current_time, client->getLastActivity()) > MAX_TIMEOUT) {
			client->setState(Client::DISCONNECTED);
			epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client->getSocketFd(), NULL);
			close(client->getSocketFd());
			delete (client);
			clients_.erase(it++);
		}
		else {
			++it;
		}
	}
}

// gère la déconnexion propre d'un client
void	Server::handleClientDisconnect_(int client_fd) {
	std::map<int, int>::iterator	cgi_it = this->cgi_to_client_.begin();
	while (cgi_it != this->cgi_to_client_.end())
	{
		if (cgi_it->second == client_fd)
		{
			int	cgi_fd = cgi_it->first;
			epoll_ctl(this->epoll_fd_, EPOLL_CTL_DEL, cgi_fd, NULL);
			close(cgi_fd);
			this->cgi_to_client_.erase(cgi_it);
			break ;
		}
		++cgi_it;
	}

	std::map<int, Client*>::iterator	it = clients_.find(client_fd);
	if (it != this->clients_.end())
	{
		epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_fd, NULL);
		close(client_fd);
		delete (it->second);
		clients_.erase(it);
	}
}

// ajoute un nouveau socket client à la surveillance epoll
bool	Server::addClientToEpoll_(int client_fd) {
	struct epoll_event	client_ev;

	memset(&client_ev, 0, sizeof(client_ev));
	client_ev.events = EPOLLIN;
	client_ev.data.fd = client_fd;
	if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &client_ev) == -1) {
		std::cerr << "Error: epoll_ctl(ADD) failed on client_fd " << client_fd << std::endl;
		close(client_fd);
		clients_.erase(client_fd);
		return (false);
	}
	return (true);
}

// accepte et configure une nouvelle connexion cliente
void	Server::handleNewConnection_(int listen_fd) {
	struct sockaddr_storage	client_addr;
	socklen_t				addr_size;
	int						client_fd;

	addr_size = sizeof(client_addr);
	client_fd = accept(listen_fd, reinterpret_cast<struct sockaddr *>(&client_addr), &addr_size);
	if (client_fd == -1) {
		return ;
	}
	setNonBlocking(client_fd);

	Client	*new_client = NULL;
	try {
		const ServerConfig	*associated_config = this->listen_sockets_[listen_fd];

		new_client = new Client(client_fd, client_addr, associated_config);

		new_client->updateLastActivity();
		new_client->setState(Client::READING_REQUEST);
	
		if (!addClientToEpoll_(client_fd))
			throw std::runtime_error("Epoll addition failed");

		this->clients_[client_fd] = new_client;
	}
	catch (const std::exception &e)
	{
		std::cerr << "[CRITICAL] Failed to setup new connection: " << e.what() << std::endl;

		sendEmergencyError_(client_fd, 500, "Internal Server Error");
		
		if (!new_client) {
			delete (new_client);
			this->clients_.erase(client_fd);
		}
	}
	//logNewConnection_(client_fd);
}

// bascule la surveillance epoll d'un client en mode écriture
void	Server::setSocketToWriteState_(int client_fd) {
	if (this->clients_.count(client_fd) == 0)
		return ;
	
	Client	*client = this->clients_[client_fd];

	if (client->getState() == Client::DISCONNECTED)
		return ;

	struct epoll_event	mod_ev;
	
	mod_ev.events = EPOLLIN | EPOLLOUT;
	mod_ev.data.fd = client_fd;
	if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, client_fd, &mod_ev) == -1) {
		this->handleClientDisconnect_(client_fd);
		return ;
	}
}

void	Server::prepareForWriting_(int client_fd, Client &client)
{
	client.setState(Client::WRITING_RESPONSE);
	this->setSocketToWriteState_(client_fd);
}

void	Server::setupCgiEpoll_(int client_fd, Client &client)
{
	client.setState(Client::WAITING_CGI);

	int	cgi_fd = client.getAnswer().getCGIHandler()->getReadFd();
	struct epoll_event	ev;
	ev.events = EPOLLIN;
	ev.data.fd = cgi_fd;

	if (epoll_ctl(this->epoll_fd_, EPOLL_CTL_ADD, cgi_fd, &ev) == -1)
	{
		std::cerr << "[CRITICAL] CGI epoll_ctl failed" << std::endl;
		client.getAnswer().setCode(500);
		client.getAnswer().setMessage("Internal Server Error");
		client.getAnswer().setAnswer(client.getRequest()); 
		this->prepareForWriting_(client_fd, client);
		return ;
	}
	this->cgi_to_client_[cgi_fd] = client_fd;

	if (client.getRequest().getMethod() == "POST" && !client.getRequest().getBody().empty())
	{
		int	write_fd = client.getAnswer().getCGIHandler()->getWriteFd();
		
		if (write_fd == -1)
			return ;

		if (fcntl(write_fd, F_SETFL, O_NONBLOCK) == -1) {
			std::cerr << "[ERROR] fcntl O_NONBLOCK failed for write_fd" << std::endl;
		}

		struct epoll_event	ev_out;
		memset(&ev_out, 0, sizeof(ev_out));
		ev_out.events = EPOLLOUT;
		ev_out.data.fd = write_fd;

		if (epoll_ctl(this->epoll_fd_, EPOLL_CTL_ADD, write_fd, &ev_out) == -1) {
			client.getAnswer().setCode(500);
			client.getAnswer().setMessage("Internal Server Error");
			client.getAnswer().setAnswer(client.getRequest()); 
			this->prepareForWriting_(client_fd, client);
			return;
		}
		this->cgi_write_to_client_[write_fd] = client_fd;
	}
}

void	Server::processClientRequest_(int client_fd) {
	Client			&client = *(clients_[client_fd]);
	Request			&request = client.getRequest();
	RequestAnswer	&response = client.getAnswer();

	try {
		ParsingStatus	parsing_status = request.parsingHttp(client.getRequestData());

		if (parsing_status == PARSING_INCOMPLETE)
			return ;
		if (parsing_status == PARSING_FAILED)
		{
			int			err_code = request.getError();
			std::string	err_msg = request.getErrorMessage();
			
			if (err_code == 0) {
				err_code = 400;
				err_msg = "Bad Request";
			}
			
			response.setCode(err_code);
			response.setMessage(err_msg);

			response.setCloseConnection(true);

			response.fullAnswer();
			
			this->prepareForWriting_(client_fd, client);
			return ;
		}
		AnswerStatus	answer_status = response.setAnswer(request);

		if (answer_status == READY_TO_SEND || answer_status == ERROR)
			this->prepareForWriting_(client_fd, client);
		else if (answer_status == CGI_IN_PROGRESS)
			this->setupCgiEpoll_(client_fd, client);
	}
	catch (const std::exception &e)
	{
		std::cerr << "[CRITICAL] Exception during request processing: " << e.what() << std::endl;
		client.getAnswer().setCode(500);
		client.getAnswer().setMessage("Internal Server Error");
		client.getAnswer().fullAnswer();
		this->prepareForWriting_(client_fd, client);
	}
}

// gère l'évènement de lecture sur un socket client
void	Server::handleClientRead_(int client_fd) {
	char	buffer[4096];
	ssize_t	bytes_received;
	bool	data_read = false;

	if (clients_.find(client_fd) == clients_.end() || clients_[client_fd] == NULL) {
		std::cerr << "[RESEAU FATAL] Tentative de lecture sur un FD client inconnu (" << client_fd << ")" << std::endl;
		return ;
	}

	Client	*client = clients_[client_fd];

	try {
		while (true)
		{
			memset(buffer, 0, sizeof(buffer));
			bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
			
			if (bytes_received > 0)
			{
				std::string	chunk(buffer, bytes_received);
				client->appendRequestData(chunk);
				data_read = true;
			}
			else if (bytes_received == 0) {
				return (handleClientDisconnect_(client_fd));
			}
			else if (bytes_received == -1) {
				break ;
			}
		}
	
		if (data_read)
		{
			client->updateLastActivity();
			const ServerConfig	*config = client->getConfig();
			client->getRequest().setServerConfig(config);
			processClientRequest_(client_fd);
		}
	}
	catch (const std::exception &e) {
		this->sendEmergencyError_(client_fd, 500, "Internal Server Error");
		this->handleClientDisconnect_(client_fd);
	}
}

// bascule la surveillance epoll d'un client en mode lecture
void	Server::setSocketToReadState_(int client_fd) {
	if (this->clients_.count(client_fd) == 0)
		return ;

	struct epoll_event	listen_ev;

	listen_ev.events = EPOLLIN;
	listen_ev.data.fd = client_fd;
	if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, client_fd, &listen_ev) == -1) {
		std::cerr << "Error: epoll_ctl(MOD) failed on socket " << client_fd << std::endl;
		handleClientDisconnect_(client_fd);
		return ;
	}
}

// gère l'évènement d'écriture sur un socket client
void	Server::handleClientWrite_(int client_fd) {
	Client				*client = clients_[client_fd];
	RequestAnswer		&response = client->getAnswer();

	const std::string	&buffer = response.getAnswer();

	if (buffer.empty())
	{
		setSocketToReadState_(client_fd);
		return ;
	}

	ssize_t	bytes_sent = send(client_fd, buffer.c_str(), buffer.length(), MSG_NOSIGNAL);

	if (bytes_sent == -1)
		return (handleClientDisconnect_(client_fd));
	else if (bytes_sent == 0)
		return (handleClientDisconnect_(client_fd));
	
	client->updateLastActivity();
	response.eraseSentBytes(bytes_sent);

	if (response.isResponseFullySent())
	{
		client->clearBuffers();
		client->getRequest().clear(); 
		client->getAnswer().clear();
		client->setState(Client::READING_REQUEST);
		setSocketToReadState_(client_fd);
	}
}

void	Server::cleanCgiData_(int cgi_fd, std::map<int, int>::iterator it)
{
	epoll_ctl(this->epoll_fd_, EPOLL_CTL_DEL, cgi_fd, NULL);
	close(cgi_fd);
	if (it != this->cgi_to_client_.end())
		this->cgi_to_client_.erase(it);
}

void	Server::handleCgiRead_(int cgi_fd)
{
	std::map<int, int>::iterator	it = cgi_to_client_.find(cgi_fd);
	if (it == cgi_to_client_.end()) {
		return (cleanCgiData_(cgi_fd, it));
	}

	int		client_fd = it->second;
	Client	*client = clients_[client_fd];
	char	buffer[4096];
	ssize_t	bytes_read = read(cgi_fd, buffer, sizeof(buffer));

	if (bytes_read > 0)
	{
		client->getAnswer().getCGIHandler()->appendOutput(std::string(buffer, bytes_read));
		return ;
	}

	int	status;
	waitpid(client->getAnswer().getCGIHandler()->getPid(), &status, 0);

	bool	error_detected = false;
	if (bytes_read < 0 || client->getAnswer().getCGIHandler()->getRawOutput().empty())
		error_detected = true;

	if (error_detected) {
		client->getAnswer().setCode(500);
	}
	else {
		client->getAnswer().buildCGIResponse();
	}

	cleanCgiData_(cgi_fd, it);
	
	this->prepareForWriting_(client_fd, *client);
}

void	Server::handleCgiWrite_(int fd)
{
	int			client_fd = this->cgi_write_to_client_[fd];
	Client		*client = this->clients_[client_fd];
	CGIHandler	*cgi = client->getAnswer().getCGIHandler();

	cgi->handleWrite();

	if (cgi->getBytesSent() >= client->getRequest().getBody().size()) {
		epoll_ctl(this->epoll_fd_, EPOLL_CTL_DEL, fd, NULL);
		this->cgi_write_to_client_.erase(fd);
	}
}

// lance la boucle d'évènements principale du serveur
void	Server::run() {
	struct epoll_event	events[MAX_EVENTS];
	int					n_events;

	while (g_running) {
		handleTimeouts_();
		n_events = epoll_wait(epoll_fd_, events, MAX_EVENTS, 1000);
		
		if (n_events == -1) {
			if (!g_running)
				break ;
			continue ;
		}
		for (int i = 0; i < n_events; i++) {
			int	fd = events[i].data.fd;
			
			try {
				if (this->listen_sockets_.count(fd) > 0)
					handleNewConnection_(fd);
				else if (this->clients_.count(fd) > 0) {
					Client	*client = this->clients_[fd];

					if (client->getState() == Client::DISCONNECTED)
						continue ;
					
					if (events[i].events & EPOLLIN) {
						if (client->getState() == Client::READING_REQUEST)
							handleClientRead_(fd);
					}

					if (this->clients_.count(fd) > 0 && events[i].events & EPOLLOUT) {
						if (client->getState() == Client::WRITING_RESPONSE)
							handleClientWrite_(fd);
					}

					if (this->clients_.count(fd) > 0 && (events[i].events & (EPOLLERR | EPOLLHUP))) {
						this->handleClientDisconnect_(fd);
					}
				}

				else if (cgi_to_client_.count(fd) > 0)
					handleCgiRead_(fd);
				
				else if (cgi_write_to_client_.count(fd) > 0) {
					if (events[i].events & EPOLLOUT)
						handleCgiWrite_(fd);
					if (cgi_write_to_client_.count(fd) > 0 && (events[i].events & (EPOLLERR | EPOLLHUP)))
						close(fd);
				}
			}
			catch (const std::exception &e) {
				std::cerr << "[RUNTIME ERROR] Socket " << fd << ": " << e.what() << std::endl;
				if (this->clients_.count(fd) > 0)
					sendEmergencyError_(fd, 500, "Internal Server Error");
				else
					close(fd);
			}
		}
	}
}

void	Server::sendEmergencyError_(int client_fd, int code, const std::string& message) {
	std::stringstream	ss;
	std::stringstream	ss_code;
	ss_code << code;
	std::string	code_str = ss_code.str();
	std::string	body = "<html><body><h1>" + code_str + " " + message + "</h1></body></html>";
	
	ss << "HTTP/1.1 " << code << " " << message << "\r\n";
	ss << "Content-Type: text/html\r\n";
	ss << "Content-Length: " << body.length() << "\r\n";
	ss << "Connection: close\r\n\r\n";
	ss << body;
	
	std::string	response = ss.str();
	send(client_fd, response.c_str(), response.size(), 0);
	close(client_fd);
}

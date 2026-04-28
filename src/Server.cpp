#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/wait.h> // pour waitpid

#include "Server.hpp"
#include "utils.hpp"
#include "RequestAnswer.hpp"

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

// génère la réponse HTTP
// A supprimer, ceci est maintenant dans RequestAnswer
/*
std::string	Server::buildHttpResponse_(Request &request) {

    RequestAnswer   answer(request);
	return (answer.getAnswer());
}
*/

// traite les données brutes reçues d'un client
void	Server::processClientRequest_(int client_fd) {
	Client	        &client = clients_[client_fd];
    Request         &request = client.getRequest();
    RequestAnswer   &response = client.getAnswer();

    ParsingStatus   parsing_status = request.parsingHttp(client.getRequestData());

    if (parsing_status == PARSING_FAILED)
    {
        std::cout << "Error : " << request.getError() << std::endl;
        // recuperer le code d'erreur genere par le parser
        // et demander au constructeur de la reponse de generer le HTML de l'erreur
        //response.buildErrorPage(request.getError());

        // changer l'etat du client en WRITING_RESPONSE
        client.setState(Client::WRITING_RESPONSE);
        // dire a epoll d'arreter d'ecouter en IN et prevenir des 
        // que le client est reseau a envoyer la reponse (OUT)
        setSocketToWriteState_(client_fd);
        return ;
    }
    // si la requete est incomplete, on retourne directement
    // on attendra le prochain tour de boucle
    // comportement asynchrone, on ne bloque pas le serveur
    // le client sera mis de cote et les autres clients seront ecoutes alors
    if (parsing_status == PARSING_INCOMPLETE)
        return ;
    
    else if (parsing_status == PARSING_SUCCESS)
    {
        client.setState(Client::PROCESSING);

        AnswerStatus answer_status = response.setAnswer(request);

        if (answer_status == READY_TO_SEND)
        {
            client.setState(Client::WRITING_RESPONSE);
	    	setSocketToWriteState_(client_fd);   
        }
        else if (answer_status == ERROR)
        {
            // recuperer le code d'erreur genere par le parser
            // et demander au constructeur de la reponse de generer le HTML de l'erreur
            //response.buildErrorPage(request.getError());
            client.setState(Client::WRITING_RESPONSE);
            setSocketToWriteState_(client_fd);
        }
        else if (answer_status == CGI_IN_PROGRESS)
        {
            client.setState(Client::WAITING_CGI);

            int cgi_fd  = response.getCGIHandler()->getReadFd();

            struct  epoll_event ev;
            ev.events = EPOLLIN | EPOLLET;
            ev.data.fd = cgi_fd;

            if (epoll_ctl(this->epoll_fd_, EPOLL_CTL_ADD, cgi_fd, &ev) == -1)
            {
                std::cerr << "Eroor epoll_ctl CGI FD " << cgi_fd << std::endl;
            }
            else
            {
                this->cgi_to_client_[cgi_fd] = client_fd;
                std::cout << "CGI lancé sur le fd " << cgi_fd << "for the client " << client_fd << std::endl;
            }
        }
    }
}

// gère l'évènement de lecture sur un socket client
void	Server::handleClientRead_(int client_fd) {
	char    buffer[4096];
    ssize_t bytes_received;
    bool    data_read = false;

    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
	    bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_received > 0)
        {
            std::string chunk(buffer, bytes_received);
            clients_[client_fd].appendRequestData(chunk);
            data_read = true;
        }
        else if (bytes_received == 0)
        {
            std::cout << "Client on socket " << client_fd << " closed the connection." << std::endl;
            handleClientDisconnect_(client_fd);
		    return ;
        }
        else
            break ;
	}
    if (data_read)
    {
        clients_[client_fd].updateLastActivity();
	    processClientRequest_(client_fd);
    }
}

/*
// Plus nécessaire
// car si cette requete lance un CGI, RequestAnswer va créer un CGIHandler
// si l'objet est détruit à la fin de la fonction, le CGI tournera dans le vide
// et il y a aussi le probleme que la reponse peut arriver en chunks
// récupère le prochain bloc de données à envoyer au client
std::string	Server::getResponseToSend_(Request& request) {
    RequestAnswer   answer(request);

    if (answer.setAnswer() == 1)
		std::cout << "answer :" << std::endl;
    std::cout << answer.getAnswer() << std::endl;
	return (answer.getAnswer());
}
*/

/*
// A SUPPRIMER
// vérifie si la réponse entière a été transmise
bool	Server::isResponseFullySent_(Client& client, ssize_t bytes_sent) {
    client.getAnswer().eraseAnswer()(bytes_sent);

    if (client.getAnswer().isAnswerEmpty())
        return true;
    return (false);
}
*/

/*
// INTEGRER DANS LE CLIENT
// réinitialise l'état du client après une transaction

void	Server::clearClientBuffers_(Client &client)
{
	client.setState(Client::READING_REQUEST);
}
*/


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
    Client	&client = clients_[client_fd];
    RequestAnswer   &response = client.getAnswer();

    const std::string   &buffer = response.getAnswer();

    ssize_t 		bytes_sent = send(client_fd, buffer.c_str(), buffer.length(), 0);

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

    response.eraseSentBytes(bytes_sent);

	if (response.isResponseFullySent())
	{
		client.clearBuffers();
		setSocketToReadState_(client_fd);
	}
}

void    Server::handleCgiRead_(int cgi_fd)
{
    // on retrouve le client associé au fd du CGI
    std::map<int, int>::iterator    it = cgi_to_client_.find(cgi_fd);
    if (it == cgi_to_client_.end())
    {
        std::cerr << "Error" << std::endl;
        epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, cgi_fd, NULL);
        close(cgi_fd);
        return ;
    } 
    int client_fd = it->second;

    Client  &client = clients_[client_fd];

    char    buffer[4096];
    ssize_t bytes_read = read(cgi_fd, buffer, sizeof(buffer));

    if (bytes_read > 0)
    {
        std::string chunk(buffer, bytes_read);
        client.getAnswer().getCGIHandler()->appendOutput(chunk);
    }
    else if (bytes_read == 0)
    {
        int status;
        // on fait un waitpid
        // pour récupérer le zombie proprement
        waitpid(client.getAnswer().getCGIHandler()->getPid(), &status, 0);
        
        // on nettoie
        epoll_ctl(this->epoll_fd_, EPOLL_CTL_DEL, cgi_fd, NULL);
        close(cgi_fd);
        this->cgi_to_client_.erase(it);

        client.getAnswer().buildCGIResponse();
        client.setState(Client::WRITING_RESPONSE);
        setSocketToWriteState_(client_fd);
    }
    else
    {
        epoll_ctl(this->epoll_fd_, EPOLL_CTL_DEL, cgi_fd, NULL);
        close(cgi_fd);
        this->cgi_to_client_.erase(it);

        //client.getAnswer().buildErrorPage(500);
        client.setState(Client::WRITING_RESPONSE);
        setSocketToWriteState_(client_fd);
    }
}

// lance la boucle d'évènements principale du serveur
void	Server::run() {
    struct epoll_event  events[MAX_EVENTS];
	int	n_events;

	std::cout << "Entering the main server loop..." << std::endl;
    while (g_running) {
        //handleTimeouts_();
		n_events = epoll_wait(epoll_fd_, events, MAX_EVENTS, 1000);
		if (n_events == -1)
            throw std::runtime_error("Fatal error: epoll_wait() failed.");
        for (int i = 0; i < n_events; i++) {
            int fd = events[i].data.fd;
            // si c'est le serveur
            // c'est une nouvelle connection
            if (fd == this->server_socket_)
                handleNewConnection_();
            // si c'est un client
            else if (this->clients_.count(fd) > 0)
            {
                // en EPOLLIN
                // on lit
                if (events[i].events & EPOLLIN)
				    handleClientRead_(fd);
                // en EPOLLOUT
                // on écrit
			    else if (events[i].events & EPOLLOUT)
				    handleClientWrite_(fd);
            }
            // si c'est un pipe CGI
            else if (cgi_to_client_.count(fd) > 0)
                handleCgiRead_(fd);
		}
	}
}

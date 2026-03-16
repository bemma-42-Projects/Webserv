#include "Server.hpp"
#include <exception>

#define _POSIX_C_SOURCE 200112L

#include <cstring>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
//#include <fcntl.h>
//#include <vector>
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
#define MAX_TIMEOUT 10

void set_nonblocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		throw SystemError("fcntl(F_GETFL) failed");
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		throw SystemError("fcntl(F_SETFL) failed");
}

void init_addrinfo_params(struct addrinfo &addrinfo_params) {
	memset(&addrinfo_params, 0, sizeof(addrinfo_params));
	addrinfo_params.ai_family = AF_UNSPEC;
	addrinfo_params.ai_socktype = SOCK_STREAM;
	addrinfo_params.ai_flags = AI_PASSIVE;
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
	struct addrinfo		addrinfo_params;
	struct addrinfo		*res;
	struct addrinfo		*p;
	int					status;
	char				ip_buffer[INET6_ADDRSTRLEN];
	int					sock_fd = -1;
	int					yes = 1;

	init_addrinfo_params(addrinfo_params);
	if ((status = getaddrinfo(NULL, port_str.c_str(), &addrinfo_params, &res)) != 0) {
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

void    handle_timeouts(int epoll_fd, std::map<int, Client> &clients) {
    time_t current_time = std::time(NULL);
    
    for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ) {
        Client &client = it->second;
        if (std::difftime(current_time, client.getLastActivity()) > MAX_TIMEOUT) {
            std::cout << "Client on socket " << client.getSocketFd() << " timed out due to inactivity. Closing connection." << std::endl;
            client.setState(Client::DISCONNECTED);
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client.getSocketFd(), NULL);
            close(client.getSocketFd());
            clients.erase(it++);
        }
        else {
            ++it;
        }
    }
}

void    handle_new_connection(int sock_fd, int epoll_fd, std::map<int, Client> &clients) {
    struct sockaddr_storage client_addr;
    socklen_t addr_size = sizeof(client_addr);


    int client_fd = accept(sock_fd, reinterpret_cast<struct sockaddr *>(&client_addr), &addr_size);
    
    if (client_fd == -1) {
        std::cerr << "Error: accept() failed: " << std::strerror(errno) << std::endl;
        return ;
    }
    
    set_nonblocking(client_fd);

    Client  client(client_fd, client_addr);
    client.updateLastActivity();
    client.setState(Client::READING_REQUEST);
    clients[client_fd] = client;

    struct epoll_event client_ev;
    client_ev.events = EPOLLIN;
    client_ev.data.fd = client_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_ev) == -1) {
        std::cerr << "Error: epoll_ctl(ADD) failed on client_fd " << client_fd << ": " << std::strerror(errno) << std::endl;
        close(client_fd);
        clients.erase(client_fd);
        return ;
    }

    std::cout << "CONNECTION ACCEPTED!" << std::endl;
    std::cout << "Client IP: " << clients[client_fd].getIp() << std::endl;
    std::cout << "Communication is now open on new socket: " << client_fd << std::endl;
    std::cout << "Listening socket " << sock_fd << " is still active in the background." << std::endl;
}

int main(void)
{
    int					    sock_fd = -1;
    std::map<int, Client>   clients;
    const std::string	    port_str = PORT;

    try {
        sock_fd = create_and_bind_socket_server(port_str);

        std::cout << "Setting up the listener..." << std::endl;
        if (listen(sock_fd, BACKLOG) == -1) {
            throw SystemError("Fatal error: listen() failed");
        }
        std::cout << "Server is now actively listening on port " << PORT << "! (Backlog: 10)" << std::endl;

        set_nonblocking(sock_fd);

        int epoll_fd = epoll_create(MAX_EVENTS);
        if (epoll_fd == -1) {
            throw std::runtime_error("Fatal error: epoll_create() failed");
        }

        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = sock_fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &ev) == -1) {
            throw std::runtime_error("Fatal error: epoll_ctl() failed on sockfd");
        }
        struct epoll_event events[MAX_EVENTS];

        std::cout << "Entering the main server loop..." << std::endl;
        while (1) {
            handle_timeouts(epoll_fd, clients);
            int n_events = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000);
            if (n_events == -1) {
                if (errno == EINTR) {
                    continue;
                }
                throw std::runtime_error(std::string("Fatal error: epoll_wait() failed: ") + std::strerror(errno));
            }

            for (int i = 0; i < n_events; i++) {
                int active_fd = events[i].data.fd;
                if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                    std::cerr << "Epoll error or hang up on socket " << active_fd << std::endl;
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, active_fd, NULL);
                    close(active_fd);
                    clients.erase(active_fd);
                    continue; 
                }
                if (active_fd == sock_fd) {
                    handle_new_connection(sock_fd, epoll_fd, clients);
                    continue;
                }

                else if (events[i].events & EPOLLIN) {
                    char buffer[1024];
                    memset(buffer, 0, sizeof(buffer));
                    ssize_t bytes_received;
                    bytes_received = recv(events[i].data.fd, buffer, sizeof(buffer) - 1, 0);
                    
                    if (bytes_received <= 0) {
                        if (bytes_received == 0) {
                            std::cout << "Client on socket " << active_fd << " closed the connection." << std::endl;
                        }
                        else {
                            std::cerr << "Error: recv() failed on socket " << events[i].data.fd << ": " << std::strerror(errno) << std::endl;
                        }
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, active_fd, NULL);
                        close(active_fd);
                        clients.erase(active_fd);
                        continue;
                    } else {
                        Client &current_client = clients[active_fd];
                        current_client.updateLastActivity();
                        std::string received_data(buffer, bytes_received);
                        std::cout << "--- RECEIVED " << bytes_received << " BYTES FROM SOCKET " << active_fd << " ---\n" << received_data << std::endl;
                        // =========================================================================
                        // INTÉGRATION DU PARSEUR HTTP (À FAIRE PAR ROMANE)
                        // =========================================================================
                        // 1. Stocker 'received_data' dans un buffer cumulatif propre au client 
                        //    (ex: current_client.appendRequestString(received_data)).
                        // 2. Appeler le parseur pour analyser ce buffer.
                        // 3. Déterminer si la requête est complète (présence de "\r\n\r\n" ou fin du chunking).
                        
                        // Simulation du retour du parseur HTTP :
                        // - true  : La requête est entière, on peut la traiter.
                        // - false : Il manque des morceaux, on laisse epoll_wait nous réveiller au prochain tour.bool is_request_complete = true;
                        bool is_request_complete = true;
                        if (is_request_complete) {
                            current_client.setState(Client::PROCESSING);
                            // ROMANE : C'est ici qu'il faut construire la vraie réponse HTTP complète
                            // (En-têtes + Corps de la page). 
                            // Ex: "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>Hello</h1>"
                            // Cette réponse devra être sauvegardée dans l'objet Client.
                            std::string response = "Good talking to you!\n";
                            // =========================================================
                            // ROMANE : BUFFERISATION DE LA RÉPONSE HTTP
                            // =========================================================
                            // La réponse HTTP (headers + body) doit impérativement être 
                            // persistée dans l'instance du client via cette méthode.
                            // 
                            // Contexte technique (I/O asynchrone) : 
                            // Nous opérons sur des sockets non-bloquants pilotés par epoll. 
                            // Un appel immédiat à send() risquerait de bloquer le thread 
                            // principal (erreur EAGAIN/EWOULDBLOCK) si le buffer d'émission 
                            // du kernel est plein.
                            // 
                            // On sauvegarde donc l'état en mémoire, on bascule le descripteur 
                            // de fichier en EPOLLOUT, et on rend la main à l'Event Loop. 
                            // L'envoi effectif sera déclenché lors du prochain événement epoll.

                            // A DECOMMENTER POUR STOCKER LA REPONSE DANS LE BUFFER DU CLIENT
                            // current_client.setResponseBuffer(response);
                            current_client.setState(Client::WRITING_RESPONSE);
                            struct epoll_event mod_ev;
                            mod_ev.events = EPOLLOUT;
                            mod_ev.data.fd = active_fd;
                            if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, active_fd, &mod_ev) == -1) {
                                std::cerr << "Error: epoll_ctl(MOD) failed on socket " << active_fd << ": " << std::strerror(errno) << std::endl;
                                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, active_fd, NULL);
                                close(active_fd);
                                clients.erase(active_fd);
                                continue;
                            }
                            std::cout << "Socket " << active_fd << " successfully switched to EPOLLOUT. Waiting for network to be ready to send..." << std::endl;
                            std::cout << "Socket " << active_fd << " switched to EPOLLOUT. Waiting for network to be ready to send..." << std::endl;
                        }
                    }
                }        
                else if (events[i].events & EPOLLOUT) {
                    Client &current_client = clients[active_fd];
                    // =========================================================
                    // ROMANE : LA RÉCUPÉRATION DE LA RÉPONSE (La suite logique)
                    // =========================================================
                    // C'est ici que ton travail de l'étape précédente prend tout son sens !
                    // 
                    // Comme notre serveur ne bloque jamais, on est sortis 
                    // de l'événement de lecture (EPOLLIN) pour attendre que le réseau se libère.
                    // Du coup, toutes les variables locales qu'on avait créées ont été détruites 
                    // à la fin du tour de boucle.
                    // 
                    // C'est pour ça qu'on avait sauvegardé ta réponse finale à l'intérieur 
                    // de l'objet 'Client'. Maintenant qu'on a le feu vert pour écrire (EPOLLOUT), 
                    // on fait simplement appel à getResponseBuffer() pour récupérer ta string 
                    // intacte et l'envoyer avec send().
                    std::string response_to_send;

                    // A DECOMMENTER POUR RECUPERER LA REPONSE GENEREE PLUS HAUT
                    //response_to_send = current_client.getResponseBuffer();
                    // A COMMENTER POUR ENVOYER LA VRAIE REPONSE AU CLIENT
                    response_to_send = "Good talking to you!\n";

                    ssize_t bytes_sent = send(active_fd, response_to_send.c_str(), response_to_send.size(), 0);
                    if (bytes_sent < 0) {
                        std::cerr << "Error: send() failed on socket " << active_fd << ": " << std::strerror(errno) << std::endl;
                    }
                    else if (bytes_sent == 0) {
                        std::cout << "Notice: 0 bytes sent to socket " << active_fd << " (Network buffer full)" << std::endl;
                        continue;
                    // Succès partiel ou total de l'envoi
                    // ROMANE : vérifier avec une condition dans ce else si la réponse a été totalement envoyée
                    // Si la réponse n'a pas été envoyée totalement, ne pas faire le noettyage final (4. NETTOYAGE FINAL)
                    } else {
                        std::cout << "Successfully sent " << bytes_sent << " bytes back to socket " << active_fd << std::endl;
                        current_client.updateLastActivity();
                    }
                    struct epoll_event listen_ev;
                    listen_ev.events = EPOLLIN;
                    listen_ev.data.fd = active_fd;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, active_fd, &listen_ev) == 1000) {
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, active_fd, NULL);
                        close(active_fd);
                        clients.erase(active_fd);
                    } else {
                        // ROMANE : C'est ici qu'il faudra vider les buffers (requête et réponse) 
                        // pour ne pas mélanger l'ancienne requête avec la nouvelle.
                        // current_client.clearBuffers();
                        current_client.setState(Client::READING_REQUEST);
                        std::cout << "Socket " << active_fd << " kept alive. Waiting for next request..." << std::endl;
                    }
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
    std::cout << "Shutting down the server (sockfd)." << std::endl;
    if (sock_fd != -1) {
        close(sock_fd);
    }
    return (0);
}


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

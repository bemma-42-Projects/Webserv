// #define _POSIX_C_SOURCE 200112L
// #include <stdio.h>
// #include <string.h>
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <netdb.h>
// #include <arpa/inet.h>
// #include <netinet/in.h>
// #include <unistd.h>
// #include <fcntl.h>
// #include <sys/epoll.h>
// #include <errno.h>

// #define PORT "8080"
// #define BACKLOG 128
// #define MAX_EVENTS 64

// void set_nonblocking(int fd) {
// 	int flags = fcntl(fd, F_GETFL, 0);
// 	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
// }

// int main(void) {
// 	struct addrinfo hints;
// 	struct addrinfo *res;
// 	struct addrinfo *p;
// 	int status;
// 	char ipstr[INET6_ADDRSTRLEN];
// 	int sockfd;
// 	int yes = 1;

// 	memset(&hints, 0, sizeof hints);
// 	hints.ai_family = AF_UNSPEC;
// 	hints.ai_socktype = SOCK_STREAM;
// 	hints.ai_flags = AI_PASSIVE;

// 	if ((status = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
// 		fprintf(stderr, "DNS/Setup Error: %s\n", gai_strerror(status));
// 		return 1;
// 	}

// 	printf("Booting up server on port %s...\n\n", PORT);

// 	for (p = res; p != NULL; p = p->ai_next) {
// 		void *addr;
// 		const char *ipver;
// 		struct sockaddr_in *ipv4;
// 		struct sockaddr_in6 *ipv6;

// 		if (p->ai_family == AF_INET) {
// 			ipv4 = (struct sockaddr_in *)p->ai_addr;
// 			addr = &(ipv4->sin_addr);
// 			ipver = "IPv4";
// 		} else {
// 			ipv6 = (struct sockaddr_in6 *)p->ai_addr;
// 			addr = &(ipv6->sin6_addr);
// 			ipver = "IPv6";
// 		}

// 		inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
// 		printf("Local interface found -> %s: %s\n", ipver, ipstr);

// 		sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
// 		if (sockfd == -1) {
// 			printf("Failed to create socket. Moving to the next one...\n\n");
// 			continue;
// 		}
// 		printf("Socket successfully created!\n");

// 		setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

// 		printf("Attempting to bind to port %s...\n", PORT);
// 		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
// 			printf("Bind failed. Closing socket and moving to the next one...\n\n");
// 			close(sockfd);
// 			continue;
// 		}
// 		printf("Successfully bound to %s on port %s!\n\n", ipstr, PORT);
// 		break;
// 	}

// 	if (p == NULL) {
// 		fprintf(stderr, "Fatal error: Failed to bind to any of the local interfaces.\n");
// 		freeaddrinfo(res);
// 		return 2;
// 	}

// 	freeaddrinfo(res);

// 	printf("Setting up the listener...\n");
// 	if (listen(sockfd, BACKLOG) == -1) {
// 		fprintf(stderr, strerror(errno));
// 		close(sockfd);
// 		return 3;
// 	}

// 	set_nonblocking(sockfd);

// 	// CRÉATION D'EPOLL - À LAISSER TEL QUEL
// 	int epoll_fd = epoll_create(MAX_EVENTS);
// 	if (epoll_fd == -1) {
// 		fprintf(stderr, "Fatal error: epoll_create() failed.\n");
// 		close(sockfd);
// 		return 1;
// 	}

// 	struct epoll_event ev;
// 	ev.events = EPOLLIN;
// 	ev.data.fd = sockfd;
// 	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &ev) == -1) {
// 		fprintf(stderr, "Fatal error: epoll_ctl() failed.\n");
// 		close(sockfd);
// 		return 1;
// 	}

// 	struct epoll_event events[MAX_EVENTS];

// 	printf("Server is now actively listening on port %s! (Backlog: %d)\n\n", PORT, BACKLOG);

// 	// BOUCLE D'ÉVÉNEMENTS
// 	while (1) {
// 		// Attendre les événements
// 		int n_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

// 		if (n_events == -1) {
// 			perror("epoll_wait");
// 			return 1;
// 		}

// 		// Traiter chaque événement
// 		for (int i = 0; i < n_events; i++) {

// 			// CAS 1 : NOUVELLE CONNEXION SUR LE SERVEUR
// 			if (events[i].data.fd == sockfd) {
// 				int client_fd = accept(sockfd, NULL, NULL);
// 				if (client_fd == -1) {
// 					perror("accept");
// 					continue;
// 				}
// 				fcntl(client_fd, F_SETFL, O_NONBLOCK);

// 				struct epoll_event ev;
// 				ev.events = EPOLLIN;
// 				ev.data.fd = client_fd;

// 				if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
// 					perror("epoll_ctl: client_fd");
// 					close(client_fd);
// 					continue;
// 				}
				
// 				//APPEL À LA FONCTION POUR NOUVELLE CONNEXION
// 				// Elle doit:
// 				//   1. Accept la connexion
// 				//   2. Créer/initialiser la structure/class Client
// 				//   3. Ajouter le client à epoll
				
// 			}
// 			// CAS 2 : DONNÉES À LIRE D'UN CLIENT
// 			else if (events[i].events & EPOLLIN) {
// 				int client_fd = events[i].data.fd;

// 				// APPEL À LA FONCTION POUR LIRE
// 				// Elle doit:
// 				//   1. Lire les données du client
// 				//   2. Accumuler dans un buffer client
// 				//   3. Parser la requête HTTP quand complète
// 				//   4. Retourner 0 si requête incomplète
// 				//   5. Retourner 1 si requête complète
// 				//   6. Retourner -1 si client fermé/erreur
// 				int result = lafonction();
				
// 				if (result == 1) {
// 					// Requête complète, basculer à EPOLLOUT pour répondre
// 					ev.events = EPOLLOUT;
// 					ev.data.fd = client_fd;
// 					epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &ev);
// 				}
// 				else if (result == -1) {
// 					// Client fermé ou erreur
// 					// APPEL À LA FONCTION FREE CLIENT
// 					free_client(client_fd, epoll_fd);
// 				}
// 				// Si result == 0, on reste en EPOLLIN et on attend plus de données
// 			}

// 			// CAS 3 : PRÊT À ÉCRIRE LA RÉPONSE À UN CLIENT
// 			else if (events[i].events & EPOLLOUT) {
// 				int client_fd = events[i].data.fd;

// 				// APPEL À LA FONCTION POUR ECRIRE
// 				// Cette fonction doit:
// 				//   1. Générer la réponse HTTP (selon la requête parsée)
// 				//   2. Envoyer la réponse au client
// 				//   3. Retourner 0 si envoi incomplet
// 				//   4. Retourner 1 si envoi complété
// 				//   5. Retourner -1 si erreur
// 				int result = fonctionecrire(client_fd);
				
// 				if (result == 1) {
// 					// Réponse entièrement envoyée, fermer la connexion
// 					free_client(client_fd, epoll_fd);
// 				}
// 				else if (result == -1) {
// 					// Erreur lors de l'envoi
// 					free_client(client_fd, epoll_fd);
// 				}
// 				// Si result == 0, on reste en EPOLLOUT pour terminer l'envoi
// 			}

// 			// CAS 4 : ERREUR SUR LE SOCKET CLIENT
// 			else if (events[i].events & EPOLLERR) {
// 				int client_fd = events[i].data.fd;
// 				printf("Error on client %d\n", client_fd);
// 				free_client(client_fd, epoll_fd);
// 			}
// 		}
// 	}

// 	// Nettoyage (jamais atteint dans cette boucle infinie)
// 	close(epoll_fd);
// 	close(sockfd);
// 	return 0;
// }



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
#include "Client.hpp"       
#include "SystemError.hpp"  
#include "GaiError.hpp"
#include <sys/epoll.h>
#include <errno.h>
#include <cstdio>


#define PORT "8080"
#define BACKLOG 128
#define MAX_EVENTS 64
#define MAX_TIMEOUT 10

void set_nonblocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(void)
{
	struct addrinfo         hints;                          
	struct addrinfo         *res;                           
	struct addrinfo         *p;                             
	int                     status;                         
	char                    ip_buffer[INET6_ADDRSTRLEN];    
	int                     sock_fd = -1;                    
	int                     yes = 1;                        
	const std::string       port_str = PORT;

	try {
		
		memset(&hints, 0, sizeof hints);   
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
			sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
			if (sock_fd == -1) {
				std::cerr << "Failed to create socket: " << strerror(errno) << ". Moving to next..." << std::endl;
				continue ;
			}
			std::cout << "Socket successfully created!" << std::endl;
			setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
			std::cout << "Attempting to bind to port " << port_str << "..." << std::endl;
			if (bind(sock_fd, p->ai_addr, p->ai_addrlen) == -1) {
				std::cerr << "Bind failed: " << strerror(errno) << " Closing socket..." << std::endl;
				close(sock_fd);
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
		if (listen(sock_fd, BACKLOG) == -1) {
			throw SystemError("Fatal error: listen() failed");
		}

		set_nonblocking(sock_fd);

		// CRÉATION D'EPOLL - À LAISSER TEL QUEL
		int epoll_fd = epoll_create(MAX_EVENTS);
		if (epoll_fd == -1) {
			fprintf(stderr, "Fatal error: epoll_create() failed.\n");
			close(sock_fd);
			return 1;
		}

		struct epoll_event ev;
		ev.events = EPOLLIN;
		ev.data.fd = sock_fd;
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &ev) == -1) {
			fprintf(stderr, "Fatal error: epoll_ctl() failed.\n");
			close(sock_fd);
			return 1;
		}

		struct epoll_event events[MAX_EVENTS];

		std::cout << "Server is now actively listening on port " << port_str << "! (Backlog: 10)" << std::endl;
		std::cout << "Entering the main server loop..." << std::endl;
		std::map<int, Client> clients;

		while (1)
		{
			// time_t current_time = std::time(NULL);
			// for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ) {
			// 	Client &client = it->second;
			// 	if (std::difftime(current_time, client.getLastActivity()) > MAX_TIMEOUT) {
			// 		std::cout << "Client on socket " << client.getSocketFd() << " timed out due to inactivity. Closing connection." << std::endl;
			// 		client.setState(Client::DISCONNECTED);
			// 		close(client.getSocketFd());
			// 		clients.erase(it++);
			// 	}
			// 	else {
			// 		++it;
			// 	}
			// }

			int n_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
			if (n_events == -1) {
				perror("epoll_wait");
				return 1;
			}

			// Traiter chaque événement
			for (int i = 0; i < n_events; i++)
			{

				// CAS 1 : NOUVELLE CONNEXION SUR LE SERVEUR
				if (events[i].data.fd == sock_fd)
				{
					std::cout << "Waiting for incoming connections... (Program is blocked here)" << std::endl;

					struct sockaddr_storage client_addr;
					socklen_t               addr_size;
					int                     client_fd;
					addr_size = sizeof(client_addr);

					client_fd = accept(sock_fd, reinterpret_cast<struct sockaddr *>(&client_addr), &addr_size);
					if (client_fd == -1) {
						std::cerr << "Error: accept() failed: " << strerror(errno) << std::endl;
						continue ;
					}
					set_nonblocking(client_fd);

					Client  client(client_fd, client_addr);
					client.updateLastActivity();
					client.setState(Client::READING_REQUEST);
					clients[client_fd] = client;

					struct epoll_event ev;
					ev.events = EPOLLIN;
					ev.data.fd = client_fd;

					if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
						perror("epoll_ctl: client_fd");
						close(client_fd);
						continue;
					}

					std::cout << "CONNECTION ACCEPTED!" << std::endl;
					std::cout << "Client IP: " << clients[client_fd].getIp() << std::endl;
					std::cout << "Communication is now open on new socket: " << clients[client_fd].getSocketFd() << std::endl;
					std::cout << "Listening socket " << sock_fd << " is still active in the background." << std::endl;
				}

				// CAS 2 : DONNÉES À LIRE D'UN CLIENT
				else if (events[i].events & EPOLLIN)
				{
					int client_fd = events[i].data.fd;
					char buffer[1024];
					memset(buffer, 0, sizeof(buffer));
					ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

					if (bytes_received < 0) {
						std::cerr << "Error reading from socket." << std::endl;
					} 
					else if (bytes_received == 0) {
						std::cout << "Client unexpectedly closed the connection." << std::endl;
						clients[client_fd].setState(Client::DISCONNECTED);
						close(client_fd);
						clients.erase(client_fd);
					} 
					else {
						Client &current_client = clients[client_fd];
						current_client.updateLastActivity();
						std::cout << "--- RECEIVED " << bytes_received << " BYTES FROM CLIENT " << client_fd << " ---" << std::endl;
						std::string received_data(buffer, bytes_received);
						std::cout << received_data << std::endl;
					}
				}

				// CAS 3 : PRÊT À ÉCRIRE LA RÉPONSE À UN CLIENT
				else if (events[i].events & EPOLLOUT) 
				{
					int client_fd = events[i].data.fd;

					std::cout << "--------------------------------------" << std::endl;
					// current_client.setState(Client::PROCESSING);
					std::string response = "Good talking to you!\n";
					// current_client.setState(Client::WRITING_RESPONSE);
					ssize_t bytes_sent = send(client_fd, response.c_str(), response.size(), 0);
					if (bytes_sent == 0) {
						std::cout << "Successfully sent " << bytes_sent << " bytes back to the client " << client_fd << std::endl;
						clients[client_fd].updateLastActivity();
						
					} 
					else {
						std::cerr << "Error: sendind response to " << client_fd << std::endl;
					}
					// clients[client_fd].setState(Client::DISCONNECTED); 
					// close(client_fd);
					// clients.erase(client_fd);

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

//$(SRC_DIR)/main.cpp \
    //  $(SRC_DIR)/Client.cpp \
    //  $(SRC_DIR)/GaiError.cpp \
      $(SRC_DIR)/SystemError.cpp
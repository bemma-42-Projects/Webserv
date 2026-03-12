#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#define PORT "8080" // the ports users will be connecting to
#define BACKLOG 10  // how many pending connections queue holds

int main(void)
{
	struct addrinfo         hints;
	struct addrinfo         *res;
	struct addrinfo         *p;
	int                     status;
	char                    ipstr[INET6_ADDRSTRLEN];
	int                     sockfd;
	int                     yes = 1;    // setsockopt
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t               addr_size;
	int                     new_fd;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;    // use IPv4 or IPv6, whichever
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;    // fill in my IP for me

	if ((status = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
		fprintf(stderr, "DNS/Setup Error: %s\n", gai_strerror(status));
		return (1);
	}

	printf("Booting up server on port %s...\n\n", PORT);

	for (p = res; p != NULL; p = p->ai_next) {
		void                *addr;
		const char          *ipver;
		struct sockaddr_in  *ipv4;
		struct sockaddr_in6 *ipv6;

		if (p->ai_family == AF_INET) {  // IPv4
			ipv4 = (struct sockaddr_in *)p->ai_addr;
			addr = &(ipv4->sin_addr);
			ipver = "IPv4";
		} else {    // IPv6
			ipv6 = (struct sockaddr_in6 *)p->ai_addr;
			addr = &(ipv6->sin6_addr);
			ipver = "IPv6";
		}

		inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
		printf("Local interface found -> %s: %s\n", ipver, ipstr);

		// make a socket
		sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sockfd == -1) {
			printf("Failed to create socket. Moving to the next one...\n\n");
			continue ;
		}
		printf("Socket successfully created!\n");

		setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		
		// bind it
		printf("Attempting to bind to port %s...\n", PORT);
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			printf("Bind failed. Closing socket and moving to the next one...\n\n");
			close(sockfd);
			continue ;
		}
		printf("Successfully bound to %s on port %s!\n\n", ipstr, PORT);
		break ;
	}

	if (p == NULL) {
		fprintf(stderr, "Fatal error: Failed to bind to any of the local interfaces.\n");
		freeaddrinfo(res);
		return (2);
	}

	freeaddrinfo(res);

	// listen on it
	printf("Setting up the listener...\n");
	if (listen(sockfd, BACKLOG) == -1) {
		fprintf(stderr, "Fatal error: listen() failed.\n");
		close(sockfd);
		return (3);
	}
	printf("Server is now actively listening on port %s! (Backlog: 10)\n\n", PORT);

	// now accept incoming connection
	addr_size = sizeof(their_addr);
	new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);

	close(sockfd);
	return (0);
}





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
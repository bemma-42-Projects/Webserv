#define _POSIX_C_SOURCE 200112L
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <poll.h>
#include <vector>
#include "Client.hpp"

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

    // EMMA : Décommenter cette partie pour faire du non-blocking et du poll, et gérer plusieurs clients en même temps
    /*
    if (fcntl(sockfd, F_SETFL, O_NONBLOCK) == -1) {
        fprintf(stderr, "Fatal error: fcntl() failed to set listening socket to non-blocking mode.\n");
        close(sockfd);
        return (4);
    }
    printf("Server socket is now non-blocking!\n");
    */

    printf("Entering the main server loop...\n\n");

    while (true) {
        // EMMA : Implémenter le poll() ici pour gérer les connexions multiples
        // 1 : décommenter le fcntl() plus haut
        // 2 : Créer une structure de données pour stocker les clients connectés (ex: vector<struct pollfd>)
        // 3 : Appeler poll()
        // 4 : Parcourir les résultats pour savoir s'il faut accept() ou recv()

        // =========================================================
        // JULIEN : GESTION DES TIMEOUTS
        // 1. Parcourir la future map<int, Client> clients
        // 2. Si (temps_actuel - client.getLastActivity()) > MAX_TIMEOUT
        // 3. Appeler la procédure de fermeture propre (close + erase de la map)
        // =========================================================

        // EMMA : Code temporaire (bloquant) pour tester la partie accept() et recv() avant d'implémenter le poll()

        // now accept incoming connection
        printf("⏳ Waiting for incoming connections... (Program is blocked here)\n\n");

        struct sockaddr_storage client_addr; // connector's address information
        socklen_t               addr_size;
        int                     client_fd;

        addr_size = sizeof(client_addr);
        client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_size);

        if (client_fd == -1) {
            fprintf(stderr, "Fatal error: accept() failed.\n");
            continue ;
        }

        // =========================================================
        // JULIEN : PRÉPARER LE DOSSIER CLIENT
        // 1. Rendre client_fd non-bloquant avec fcntl (O_NONBLOCK)
        // 2. Instancier l'objet Client avec client_fd et client_addr
        // 3. Stocker l'objet dans la map : clients[client_fd] = client;
        // 4. Mettre à jour le timestamp d'activité : client.updateLastActivity()
        // =========================================================

        Client  client(client_fd, client_addr); // Create a Client object for this new connection

        char client_ip[INET6_ADDRSTRLEN];
        void *raw_ip_addr;
        const char *client_ipver;

        // client_addr is of type sockaddr_storage. We use ss_family to determine
        // if the client connected via IPv4 or IPv6.
        if (client_addr.ss_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)&client_addr;
            raw_ip_addr = &(ipv4->sin_addr);
            client_ipver = "IPv4";
        } else { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)&client_addr;
            raw_ip_addr = &(ipv6->sin6_addr);
            client_ipver = "IPv6";
        }

        inet_ntop(client_addr.ss_family, raw_ip_addr, client_ip, sizeof(client_ip));

        printf("CONNECTION ACCEPTED!\n");
        printf("Client IP: %s (%s)\n", client_ip, client_ipver);
        printf("Communication is now open on new socket: %d\n", client.getSocketFd());
        printf("Listening socket %d is still active in the background.\n\n", sockfd);

        // --- RECV (Reading the client's request) ---
        char buffer[1024]; // Allocate a buffer large enough for basic messages
        memset(buffer, 0, sizeof(buffer)); // Zero it out to prevent reading garbage memory

        // recv blocks until the client sends some data
        ssize_t bytes_received = recv(client.getSocketFd(), buffer, sizeof(buffer) - 1, 0);

        if (bytes_received < 0) {
            fprintf(stderr, "Error reading from socket.\n");
        } else if (bytes_received == 0) {
            // =========================================================
            // JULIEN : FERMETURE PROPRE (DÉCONNEXION CLIENT)
            // 1. Retirer le fd du vector poll_fds (Emma)
            // 2. Supprimer l'entrée de la map clients.erase(fd)
            // 3. close(fd)
            // =========================================================
            printf("Client unexpectedly closed the connection.\n");
        } else {
            printf("--- RECEIVED %zd BYTES FROM CLIENT ---\n", bytes_received);
            printf("%s\n", buffer);
            printf("--------------------------------------\n\n");

            // =========================================================
            // ROMANE : IMPLÉMENTER LE PARSING HTTP ICI
            // 1. Stocker le contenu de 'buffer' dans client._read_buffer
            //    (ex: client.appendReadBuffer(buffer, bytes_received))
            // 2. Vérifier si on a reçu une requête HTTP complète (présence de "\r\n\r\n")
            // 3. Analyser la requête (GET, POST, URI, Headers)
            // 4. Générer la réponse HTTP correspondante (200 OK, 404 Not Found, etc.)
            //    et la stocker dans un buffer d'écriture (ex: client._write_buffer)
            // 5. Modifier le client.state pour passer en WRITING_RESPONSE 
            //    (nécessaire une fois que poll sera en place)
            // =========================================================

            // --- SEND (Sending the response) ---
            // ROMANE : Temporairement, on envoie une réponse statique pour tester.
            // À terme, cette partie devra envoyer le contenu de client._write_buffer
            const char *response = "Good talking to you!\n";

            // =========================================================
            // JULIEN : STATE MACHINE ENVOI
            // Une fois que poll() indique POLLOUT :
            // 1. Envoyer une partie ou la totalité de client._write_buffer
            // 2. Supprimer les octets envoyés du buffer
            // 3. Si buffer vide -> repasser en READING_REQUEST ou FINISHED
            // =========================================================
            ssize_t bytes_sent = send(client.getSocketFd(), response, strlen(response), 0);
            
            if (bytes_sent < 0) {
                fprintf(stderr, "Error sending response.\n");
            } else {
                printf("Successfully sent %zd bytes back to the client.\n", bytes_sent);
            }
        }

        // Close the connection with this specific client
        printf("\nClosing the connection (client_fd).\n");
        close(client.getSocketFd());
    }

    // Shut down the main listening server socket
    printf("Shutting down the server (sockfd).\n");
    close(sockfd);
        
    return (0);
}

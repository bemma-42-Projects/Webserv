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
#include <map>
#include <ctime>
#include <iostream>
#include "Client.hpp"
#include "SystemError.hpp"
#include "GaiError.hpp"

#define PORT "8080"     // The port users will be connecting to
#define BACKLOG 10      // How many pending connections queue holds
#define MAX_TIMEOUT 10  // Maximum allowed inactivity time for clients (in seconds)

int main(void)
{
    struct addrinfo         hints;
    struct addrinfo         *res;
    struct addrinfo         *p;
    int                     status;
    char                    ipstr[INET6_ADDRSTRLEN];
    int                     sockfd;
    int                     yes = 1;    // setsockopt
    
    try {
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;    // use IPv4 or IPv6, whichever
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;    // fill in my IP for me

        if ((status = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
            throw GaiError("DNS/Setup Error", status);
        }

        std::cout << "Booting up server on port " << PORT << "..." << std::endl;

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
            std::cout << "Local interface found -> " << ipver << ": " << ipstr << std::endl;

            // make a socket
            sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (sockfd == -1) {
                std::cerr << "Failed to create socket: " << strerror(errno) << ". Moving to next..." << std::endl;
                continue ;
            }
            std::cout << "Socket successfully created!" << std::endl;

            setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
            
            // bind it
            std::cout << "Attempting to bind to port " << PORT << "..." << std::endl;
            if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                std::cerr << "Bind failed: " << strerror(errno) << " Closing socket..." << std::endl;
                close(sockfd);
                continue ;
            }
            std::cout << "Successfully bound to " << ipstr << " on port " << PORT << "!" << std::endl;
            break ;
        }

        if (p == NULL) {
            freeaddrinfo(res);
            throw SystemError("Fatal error: Failed to bind to any of the local interfaces");
        }

        freeaddrinfo(res);

        // listen on it
        std::cout << "Setting up the listener..." << std::endl;
        if (listen(sockfd, BACKLOG) == -1) {
            throw SystemError("Fatal error: listen() failed");
        }
        std::cout << "Server is now actively listening on port " << PORT << "! (Backlog: 10)" << std::endl;

        // EMMA : Décommenter cette partie pour faire du non-blocking et du poll, et gérer plusieurs clients en même temps
        /*
        if (fcntl(sockfd, F_SETFL, O_NONBLOCK) == -1) {
            throw SystemError("Fatal error: fcntl() failed to set listening socket to non-blocking mode");
            return (4);
        }
        std::cout << "Server socket is now non-blocking!" << std::endl;
        */

        std::cout << "Entering the main server loop..." << std::endl;

        std::map<int, Client> clients; // Map to store connected clients (key: socket fd, value: Client object)

        while (true) {
            // EMMA : Implémenter le poll() ici pour gérer les connexions multiples
            // 1 : décommenter le fcntl() plus haut
            // 2 : Créer une structure de données pour stocker les clients connectés (ex: vector<struct pollfd>)
            // 3 : Appeler poll()
            // 4 : Parcourir les résultats pour savoir s'il faut accept() ou recv()

            time_t current_time = time(NULL);

            for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ) {
                Client &client = it->second;
                if (difftime(current_time, client.getLastActivity()) > MAX_TIMEOUT) {
                    std::cout << "Client on socket " << client.getSocketFd() << " timed out due to inactivity. Closing connection." << std::endl;
                    client.setState(Client::DISCONNECTED);
                    close(client.getSocketFd());
                    clients.erase(it++); // erase returns the next iterator
                }
                else {
                    ++it; // Move to the next client
                }
            }
            // EMMA : Code temporaire (bloquant) pour tester la partie accept() et recv() avant d'implémenter le poll()

            // now accept incoming connection
            std::cout << "⏳ Waiting for incoming connections... (Program is blocked here)" << std::endl;

            struct sockaddr_storage client_addr; // connector's address information
            socklen_t               addr_size;
            int                     client_fd;

            addr_size = sizeof(client_addr);
            client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_size);

            if (client_fd == -1) {
                std::cerr << "Error: accept() failed: " << strerror(errno) << std::endl;
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

            clients[client_fd] = client;
        
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

            std::cout << "CONNECTION ACCEPTED!" << std::endl;
            std::cout << "Client IP: " << client_ip << " (" << client_ipver << ")" << std::endl;
            std::cout << "Communication is now open on new socket: " << client.getSocketFd() << std::endl;
            std::cout << "Listening socket " << sockfd << " is still active in the background." << std::endl;

            // --- RECV (Reading the client's request) ---
            char buffer[1024]; // Allocate a buffer large enough for basic messages
            memset(buffer, 0, sizeof(buffer)); // Zero it out to prevent reading garbage memory

            // recv blocks until the client sends some data
            ssize_t bytes_received = recv(client.getSocketFd(), buffer, sizeof(buffer) - 1, 0);

            if (bytes_received < 0) {
                std::cerr << "Error reading from socket." << std::endl;
            } else if (bytes_received == 0) {
                // =========================================================
                // JULIEN : FERMETURE PROPRE (DÉCONNEXION CLIENT)
                // 1. Retirer le fd du vector poll_fds (Emma)
                // 2. Supprimer l'entrée de la map clients.erase(fd)
                // 3. close(fd)
                // =========================================================
                std::cout << "Client unexpectedly closed the connection." << std::endl;
            } else {
                std::cout << "--- RECEIVED " << bytes_received << " BYTES FROM CLIENT ---" << std::endl;
                std::cout << buffer << std::endl;
                std::cout << "--------------------------------------" << std::endl;

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
                    std::cerr << "Error sending response." << std::endl;
                } else {
                    std::cout << "Successfully sent " << bytes_sent << " bytes back to the client." << std::endl;
                }
            }

            // Close the connection with this specific client
            //std::cout << "\nClosing the connection (client_fd).\n" << std::endl;
            //close(client.getSocketFd());
        }
    }
    catch (const std::exception &e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        if (sockfd != -1) {
            close(sockfd);
        }
        return (1);
    }
    // Shut down the main listening server socket
    std::cout << "Shutting down the server (sockfd)." << std::endl;
    close(sockfd);
        
    return (0);
}

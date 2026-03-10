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

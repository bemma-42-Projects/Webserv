#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    struct addrinfo hints;
    struct addrinfo *res;
    struct addrinfo *p;
    int             status;
    char            ipstr[INET6_ADDRSTRLEN];
    int             sockfd;

    if (argc != 2) {
        fprintf(stderr, "usage: %s hostname\n", argv[0]);
        return (1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(argv[1], "http", &hints, &res)) != 0) {
        fprintf(stderr, "DNS Error (getaddrinfo): %s\n", gai_strerror(status));
        return (2);
    }

    printf("Analyzing and setting up connection for %s:\n\n", argv[1]);

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
        printf("Target found -> %s: %s\n", ipver, ipstr);

        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
            printf("Failed to create socket. Moving to the next one...\n\n");
            continue ;
        }
        printf("Socket successfully created for this IP!\n");
        
        printf("Attempting to connect...\n");
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            printf("Connection failed. Closing socket and moving to the next IP...\n\n");
            close(sockfd);
            continue ;
        }
        printf("Successfully connected to %s!\n\n", ipstr);
        break ;
    }

    if (p == NULL) {
        fprintf(stderr, "Fatal error: Failed to connect to any of the IPs of %s.\n", argv[1]);
        freeaddrinfo(res);
        return (2);
    }

    printf("[INFO] Connection is live! Ready to send/receive data.\n");
    close(sockfd);
    
    freeaddrinfo(res);
    return (0);
}

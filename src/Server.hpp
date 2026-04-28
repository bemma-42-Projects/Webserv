#ifndef SERVER_HPP
# define SERVER_HPP

# include <string>
# include <map>
# include <sys/epoll.h>

# include "Client.hpp"
# include "Request.hpp"

# define MAX_EVENTS 64

extern bool g_running;

class   Server {
    public:
        Server();
        ~Server();
        void    init();
        void    run();

    private:
        Server(const Server &src);
        Server &operator=(const Server &rhs);
        void            initAddrinfoParams_(struct addrinfo &addrinfo_params);
        struct addrinfo *getAddrInfo_(const std::string &port_str);
        void            printInterface_(struct addrinfo *p, char *ip_buffer);
        std::string     getClientIpStr(struct sockaddr_storage *client_addr);
        bool            setupSocket_(struct addrinfo *p, const std::string &port_str);
        void            bindSocketLoop_(struct addrinfo *res, const std::string &port_str);
        void            createAndBindSocket_(const std::string &port_str);
        void            startListening_();
        void            initEpoll_();
        void            handleTimeouts_();
        void            handleClientDisconnect_(int client_fd);
        bool            addClientToEpoll_(int client_fd);
        void            logNewConnection_(int client_fd);
        void            handleNewConnection_();
        void            setSocketToWriteState_(int client_fd);
        void            processClientRequest_(int client_fd);
        void            handleClientRead_(int client_fd);
        void            setSocketToReadState_(int client_fd);
        void            handleClientWrite_(int client_fd);
        // pont asynchrone entre le script cgi qui tourne en arrière-plan et la mémoire du client
        // récupère le résultat du cgi et prépare l'envoi de la réponse
        // retrouve le client a qui le cgi_fd appartient
        // lit les données
        // stocke les données dans la réponse du client
        // cette fonction sera appelée plusieurs fois par epoll
        // et grace au retour de read, elle saura que le CGI est terminé
        // et qu'il a fermé son pipe
        // puis retire le cgi_fd de epoll
        // ferme le cgi_fd
        // change l'etat du client en READY_TO_SEND
        void            handleCgiRead_(int cgi_fd);

        clearClientBuffers a implementer !

        int server_socket_;
        int epoll_fd_;
        std::map<int, Client> clients_;
        // quand le script (php par exemple) fait un echo pour remplir le pipe
        // epoll_wait se reveille, et indique que le fd concerne a des données a lire
        // le probleme est que le serveur a "oublié" alors pour quel client le CGI travaillait
        // pour stocker la réponse générée, il faut retrouver le client associé à ce CGI
        // pour cela, on utilise cgi_to_client (map)
        // clé : le fd du pipe CGI
        // valeur : le FD du socket du client
        // a la creation du CFI, on ajoute une entree dans ce dictionnaire
        // puis la fonction handleCgiRead_() consultera ce dictionnaire
        // pour refaire le lien
        // a la fin, on ferme le pipe et on efface cette key-value du dictionnaire
        std::map<int, int> cgi_to_client_;
};

#endif
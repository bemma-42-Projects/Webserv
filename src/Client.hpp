#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <string>
# include <ctime>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>

class Client {
    public:
        enum State {
            READING_REQUEST,    // Le client est en train d'envoyer sa requête
            PROCESSING,         // Le serveur analyse et prépare la réponse
            WRITING_RESPONSE,   // Le serveur envoie la réponse au client
            DISCONNECTED        // Le client a été déconnecté (Timeout ou fermeture volontaire)
        };
        Client();
        Client(int socket_fd, struct sockaddr_storage addr);
        Client(const Client &copy);
        Client &operator=(const Client &src);
        ~Client();

        int             getSocketFd() const;
        State           getState() const;
        void            setState(State state);
        time_t          getLastActivity() const;
        void            updateLastActivity();
        std::string     getIp() const;

    private:
        int _socket_fd;                 // Le socket pour communiquer avec ce client
        struct sockaddr_storage _addr;  // L'adresse du client (IP et port)
        State _state;                   // L'état actuel du client
        time_t _last_activity;          // Le timestamp de la dernière activité du client
        std::string _ip_address;        // L'adresse IP du client sous forme de chaîne de caractères
    
        void    _initIpAddress(struct sockaddr_storage addr); // Méthode privée pour initialiser _ip_address à partir de _addr
    };

#endif
#ifndef CLIENT_HPP
# define CLIENT_HPP

//# include <string>
//# include <ctime>
//# include <sys/socket.h>
# include <netinet/in.h>    // Pour struct sockaddr_in

/*
enum class ClientState {
    READING_REQUEST,    // Le client est en train d'envoyer sa requête
    PROCESSING,         // Le serveur analyse et prépare la réponse
    WRITING_RESPONSE,   // Le serveur envoie la réponse au client
    DISCONNECTED        // Le client a été déconnecté (Timeout ou fermeture volontaire)
};
*/

class Client {
    public:
        Client();
        Client(int socket_fd, struct sockaddr_storage addr);
        Client(const Client &copy);
        Client &operator=(const Client &src);
        ~Client();

        int getSocketFd() const;

    private:
        int _socket_fd;             // Le socket pour communiquer avec ce client
        struct sockaddr_storage _addr;   // L'adresse du client (IP et port)
};

#endif
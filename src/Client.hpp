#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <string>
# include <ctime>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>

# include "Request.hpp"

// pour stocker answer_ en tant que type RequestAnswer et non plus en tant que std::string
// et pour pouvoir implémenter getAnswer qui retournera un requestAnswer
# include "RequestAnswer.hpp"


class Client {
    public:
        enum State {
            READING_REQUEST,    // Le client est en train d'envoyer sa requête

            // le serveur va devoir gérer la préparation de la réponse du CGI de facon specifique
            WAITING_CGI,        // Le serveur attend que le CGI génère sa réponse
            
            WRITING_RESPONSE,   // Le serveur envoie la réponse au client
            DISCONNECTED        // Le client a été déconnecté (timeout ou fermeture volontaire)
        };

        Client();
        Client(int socket_fd, struct sockaddr_storage addr);
        ~Client();

        int                     getSocketFd() const;
        State                   getState() const;
        void                    setState(State state);
        time_t                  getLastActivity() const;
        Request                 &getRequest();
        // retourne le RequestAnswer du client
        RequestAnswer           &getAnswer();
        void                    updateLastActivity();
        std::string             getIp() const;
        
        // concatene la request data (string)
        void                    appendRequestData(const std::string &data);
        const std::string       &getRequestData() const;
        const std::string       &getResponseData() const;
        
        // A mettre en place pour resoudre (peut-etre) le bug suivant :
        // si un client demande index.php : index.php s'affiche
        // puis, si il demande index.html, il s'affiche
        // mais si le client demande a nouveau index.php, c'est index.html qui s'affiche
        // il y a aussi sans doute des problemes de valgrind etc
        // car les buffers et response data ne sont pas clean apres coup
        // voir aussi avec le keep alive ?
        //void                    setResponseData(const std::string &data);
        //void                    eraseSentResponseData(ssize_t bytes_sent);
        void                    clearBuffers();

    private:
        // il faut interdire la copie d'un client !
        Client(const Client &src);
        Client  &operator=(const Client &rhs);
        void                    initIpAddress_(struct sockaddr_storage addr);

        int                     socket_fd_;
        struct sockaddr_storage addr_;
        State                   state_;
        time_t                  last_activity_;
        std::string             ip_address_;
        std::string             request_buffer_;
        std::string             response_buffer_;
        Request                 request_;
        // le requestAnswer du Client
        RequestAnswer           answer_;
};

#endif
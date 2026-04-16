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
            READING_REQUEST,
            PROCESSING,
            WRITING_RESPONSE,
            DISCONNECTED
        };

        Client();
        Client(int socket_fd, struct sockaddr_storage addr);
        Client(const Client &src);
        Client  &operator=(const Client &rhs);
        ~Client();

        int             getSocketFd() const;
        State           getState() const;
        void            setState(State state);
        time_t          getLastActivity() const;
        void            updateLastActivity();
        std::string     getIp() const;

    private:
        int                     socket_fd_;
        struct sockaddr_storage addr_;
        State                   state_;
        time_t                  last_activity_;
        std::string             ip_address_;
        std::string		        answer_;

        void    _initIpAddress(struct sockaddr_storage addr);
    };

#endif
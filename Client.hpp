#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <string>
# include <ctime>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>

class   Client {
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
        int _socket_fd;
        struct sockaddr_storage _addr;
        State _state;
        time_t _last_activity;
        std::string _ip_address;
    
        void    _initIpAddress(struct sockaddr_storage addr);
    };

#endif
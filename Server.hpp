#ifndef SERVER_HPP
# define SERVER_HPP

# include <string>
# include <map>
# include <sys/epoll.h>

# include "Client.hpp"

# define MAX_EVENTS 64

class   Server {
    public:
        Server();
        ~Server();

        void    init();
        void    run();

    private:
        Server(const Server &copy);
        Server &operator=(const Server &src);

        void            _setNonBlocking(int fd);
        void            _init_addrinfo_params(struct addrinfo &addrinfo_params);
        struct addrinfo *_getAddrInfo(const std::string &port_str);
        void            _print_interface(struct addrinfo *p, char *ip_buffer);
        bool            _setupSocket(struct addrinfo *p, const std::string &port_str);
        void            _bindSocketLoop(struct addrinfo *res, const std::string &port_str);
        void            _createAndBindSocket(const std::string &port_str);
        void            _startListening(void);
        void            _initEpoll(void);

        //void            _handleTimeouts(void);
        //void            _handleNewConnection(void);
        //void            _handleClientRead(int client_fd);
        //void            _handleClientWrite(int client_fd);

        int _server_socket;
        int _epoll_fd;
        std::map<int, Client> _clients;
        struct epoll_event  _events[MAX_EVENTS];
};

#endif
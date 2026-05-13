#ifndef SERVER_HPP
# define SERVER_HPP

# include <string>
# include <map>
# include <sys/epoll.h>
# include <vector>

# include "Client.hpp"
# include "Request.hpp"
# include "ServerConfig.hpp"


# define MAX_EVENTS 64

extern bool g_running;

class   Server {
    public:
        Server(const std::vector<ServerConfig> &configs);
        ~Server();
        void    init();
        void    run();

    private:
        Server(const Server &src);
        Server &operator=(const Server &rhs);
        void            initAddrinfoParams_(struct addrinfo &addrinfo_params);
        struct addrinfo *getAddrInfo_(const std::string &ip, const std::string &port_str);
        void            printInterface_(struct addrinfo *p, char *ip_buffer);
        std::string     getClientIpStr(struct sockaddr_storage *client_addr);
        int             setupSocket_(struct addrinfo *p);
        int             bindSocketLoop_(struct addrinfo *res);
        int             createAndBindSocket_(const std::string &ip, const std::string &port_str);
        void            startListening_(int fd);
        void            addListenSocketToEpoll_(int fd);
        void            initEpoll_();
        void            handleTimeouts_();
        void            handleClientDisconnect_(int client_fd);
        bool            addClientToEpoll_(int client_fd);
        void            logNewConnection_(int client_fd);
        void            handleNewConnection_(int listen_fd);
        void            setSocketToWriteState_(int client_fd);
        void            processClientRequest_(int client_fd);
        void            handleClientRead_(int client_fd);
        void            setSocketToReadState_(int client_fd);
        void            handleClientWrite_(int client_fd);
        void            handleCgiRead_(int cgi_fd);
        void            handleCgiWrite_(int fd);
        void            prepareForWriting_(int client_fd, Client &client);
        void            setupCgiEpoll_(int client_fd, Client &client);
        void            cleanCgiData_(int cgi_fd, std::map<int, int>::iterator it);
        void            sendEmergencyError_(int client_fd, int code, const std::string &message);

        std::map<int, ServerConfig*>    listen_sockets_;
        int                             epoll_fd_;
        std::map<int, Client*>          clients_;
        std::map<int, int>              cgi_to_client_;
        std::map<int, int>              cgi_write_to_client_;
        std::vector<ServerConfig>       configs_;

    };

#endif
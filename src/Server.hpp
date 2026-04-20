#ifndef SERVER_HPP
# define SERVER_HPP

# include <string>
# include <map>
# include <sys/epoll.h>

# include "Client.hpp"

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
        bool	        isRequestComplete_(Client &client);
        std::string     buildHttpResponse_(Client &client);
        void            bufferizeResponse_(Client& client, const std::string& response);
        void            processClientRequest_(int client_fd, const std::string& received_data);
        void            handleClientRead_(int client_fd);
        std::string     getResponseToSend_(Client& client);
        bool            isResponseFullySent_(Client& client, ssize_t bytes_sent);
        void            clearClientBuffers_(Client &client);
        void            setSocketToReadState_(int client_fd);
        void            handleClientWrite_(int client_fd);

        int server_socket_;
        int epoll_fd_;
        std::map<int, Client> clients_;
};

#endif
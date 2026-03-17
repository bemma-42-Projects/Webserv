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
        Server(const Server &src);
        Server &operator=(const Server &rhs);

        void            _setNonBlocking(int fd);
        void            _initAddrinfoParams(struct addrinfo &addrinfo_params);
        struct addrinfo *_getAddrInfo(const std::string &port_str);
        void            _printInterface(struct addrinfo *p, char *ip_buffer);
        bool            _setupSocket(struct addrinfo *p, const std::string &port_str);
        void            _bindSocketLoop(struct addrinfo *res, const std::string &port_str);
        void            _createAndBindSocket(const std::string &port_str);

        void            _startListening();
        void            _initEpoll();

        void            _handleTimeouts();
/*
        void            _handleClientDisconnect(int client_fd);
        bool            _addClientToEpoll(int client_fd);
        void            _logNewConnection(int client_fd);
        void            _handleNewConnection();
        void            _setSocketToWriteState(int client_fd);
        bool	        _isRequestComplete(Client &client);
        std::string     _buildHttpResponse(Client &client);
        void            _bufferizeResponse(Client& client, const std::string& response);
        void            _processClientRequest(int client_fd, const std::string& received_data);
        void            _handleClientRead(int client_fd);
        std::string     _getResponseToSend(Client& client);
        bool            _isResponseFullySent(Client& client, ssize_t bytes_sent);
        void            _clearClientBuffers(Client &client);
        void            _setSocketToReadState(int client_fd);
        void            _handleClientWrite(int client_fd);
*/
        int _server_socket;
        int _epoll_fd;
        std::map<int, Client> _clients;
};

#endif
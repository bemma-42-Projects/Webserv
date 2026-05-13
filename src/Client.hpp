#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <string>
# include <ctime>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>

# include "Request.hpp"
# include "RequestAnswer.hpp"
# include "ServerConfig.hpp"


class Client {
    public:
        enum State {
            READING_REQUEST,
            WAITING_CGI,
            WRITING_RESPONSE,
            DISCONNECTED
        };

        Client();
        Client(int socket_fd, struct sockaddr_storage addr, const ServerConfig *config);
        ~Client();

        int                     getSocketFd() const;
        State                   getState() const;
        void                    setState(State state);
        time_t                  getLastActivity() const;
        Request                 &getRequest();
        RequestAnswer           &getAnswer();
        const ServerConfig      *getConfig() const;
        std::string             getIp() const;
        void                    updateLastActivity();
        void                    appendRequestData(const std::string &data);
        const std::string       &getRequestData() const;
        const std::string       &getResponseData() const;
        void                    clearBuffers();

    private:
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
        RequestAnswer           answer_;
        const ServerConfig      *config_;
};

#endif
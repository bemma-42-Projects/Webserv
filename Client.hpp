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
        int socket_fd_;
        struct sockaddr_storage addr_;
        State state_;
        time_t last_activity_;
        std::string ip_address_;
        std::string request_buffer_;
        std::string response_buffer_;

        void                initIpAddress_(struct sockaddr_storage addr);
        void                appendRequestData_(const std::string &data);
        const std::string   &getRequestData_() const;
        void                setResponseData_(const std::string &data);
        void                eraseSentResponseData_(ssize_t bytes_sent);
        void                clearBuffers_();
    };

#endif
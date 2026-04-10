#include <string.h>
#include <netdb.h>
#include <exception>
#include <cstring>

#define PORT "8080"

int main()
{
    struct addrinfo hints;
    int             status;
    const std::string port_str(PORT);

    try {
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        if ((status = getaddrinfo(NULL, port_str.c_str(), &hints, &res)) != 0)
            throw std::runtime_error(gai_strerror(status));
    }
    catch (const std::exception &e)
    {

    }
}
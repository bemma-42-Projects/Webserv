#include "Client.hpp"
#include <cstring> // Pour memset

// Constructeur par défaut
// On initialise le socket à -1 pour indiquer qu'il n'est pas encore connecté
Client::Client() : _socket_fd(-1) {
    // Initialisation de l'adresse à zéro
    memset(&_addr, 0, sizeof(_addr));
}

// Constructeur paramétrique
Client::Client(int socket_fd, struct sockaddr_storage addr) : _socket_fd(socket_fd), _addr(addr) {   
}

// Constructeur par copie
Client::Client(const Client &copy) : _socket_fd(copy._socket_fd), _addr(copy._addr) {
}

// Surcharge de l'opérateur d'affectation
Client &Client::operator=(const Client &src) {
    if (this != &src) {
        _socket_fd = src._socket_fd;
        _addr = src._addr;
    }
    return (*this);
}

int Client::getSocketFd() const {
    return (this->_socket_fd);
}

// Destructeur
Client::~Client() {

}
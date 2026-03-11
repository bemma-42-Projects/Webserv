#include "Client.hpp"
#include <cstring> // Pour memset

// Constructeur par défaut
// On initialise le socket à -1 pour indiquer qu'il n'est pas encore connecté
Client::Client() : _socket_fd(-1), _state(READING_REQUEST), _last_activity(time(NULL)) {
    // Initialisation de l'adresse à zéro
    memset(&_addr, 0, sizeof(_addr));
}

// Constructeur paramétrique
Client::Client(int socket_fd, struct sockaddr_storage addr) : _socket_fd(socket_fd), _addr(addr), _state(READING_REQUEST), _last_activity(time(NULL)) {
}

// Constructeur par copie
Client::Client(const Client &copy) : _socket_fd(copy._socket_fd), _addr(copy._addr), _state(copy._state), _last_activity(copy._last_activity) {
}

// Surcharge de l'opérateur d'affectation
Client &Client::operator=(const Client &src) {
    if (this != &src) {
        _socket_fd = src._socket_fd;
        _addr = src._addr;
        _state = src._state;
        _last_activity = src._last_activity;
    }
    return (*this);
}

int Client::getSocketFd() const {
    return (this->_socket_fd);
}

Client::State Client::getState() const {
    return (this->_state);
}

void Client::setState(State state) {
    this->_state = state;
}

time_t Client::getLastActivity() const {
    return (this->_last_activity);
}

void Client::updateLastActivity() {
    this->_last_activity = time(NULL);
}

// Destructeur
Client::~Client() {

}
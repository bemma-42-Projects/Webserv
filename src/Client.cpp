#include "Client.hpp"
#include <cstring>

// constructeur par défaut
Client::Client() : socket_fd_(-1), state_(READING_REQUEST), last_activity_(time(NULL)), ip_address_("") {
    memset(&addr_, 0, sizeof(addr_));
}

// constructeur paramétrique
Client::Client(int socket_fd, struct sockaddr_storage addr) : socket_fd_(socket_fd), addr_(addr), state_(READING_REQUEST), last_activity_(time(NULL)) {
    initIpAddress_(addr);
    this->request_.setClientIP(this->ip_address_);
}

// constructeur par copie
Client::Client(const Client &src) : socket_fd_(src.socket_fd_), addr_(src.addr_), state_(src.state_), last_activity_(src.last_activity_), ip_address_(src.ip_address_), request_(src.request_) {

}

// opérateur d'assignation
Client &Client::operator=(const Client &rhs) {
    if (this != &rhs) {
        socket_fd_ = rhs.socket_fd_;
        addr_ = rhs.addr_;
        state_ = rhs.state_;
        last_activity_ = rhs.last_activity_;
        ip_address_ = rhs.ip_address_;
        request_ = rhs.request_;
    }
    return (*this);
}

// destructeur
Client::~Client() {

}

// convertit la structure d'adresse en IP lisible et la stocke dans ip_address_
void Client::initIpAddress_(struct sockaddr_storage addr) {
    char ip_buffer[INET6_ADDRSTRLEN];
    void *raw_ip_ptr;
    std::string ip_version;
    if (addr.ss_family == AF_INET) {
        raw_ip_ptr = &(reinterpret_cast<struct sockaddr_in *>(&addr)->sin_addr);
        ip_version = "IPv4";
    } else {
        raw_ip_ptr = &(reinterpret_cast<struct sockaddr_in6 *>(&addr)->sin6_addr);
        ip_version = "IPv6";
    }
    inet_ntop(addr.ss_family, raw_ip_ptr, ip_buffer, sizeof(ip_buffer));
    this->ip_address_ = std::string(ip_buffer) + " (" + ip_version + ")";
}


// récupère le descripteur de fichier du socket client
int Client::getSocketFd() const {
    return (this->socket_fd_);
}

// récupère l'état actuel du client
Client::State Client::getState() const {
    return (this->state_);
}

// modifie l'état actuel du client
void Client::setState(State state) {
    this->state_ = state;
}

// récupère le timestamp de la dernière action du client
time_t Client::getLastActivity() const {
    return (this->last_activity_);
}

// actualise le chronomètre d'activité du client
void Client::updateLastActivity() {
    this->last_activity_ = time(NULL);
}

// récupère l'adresse IP du client sous forme de texte
std::string Client::getIp() const {
    return (this->ip_address_);
}

Request &Client::getRequest()
{
    return (this->request_);
}

// ajoute les données reçues au buffer de la requête
void    Client::appendRequestData_(const std::string &data)
{
    this->request_buffer_ += data;
}

// récupère l'intégralité des données brutes de la requête
const std::string    &Client::getRequestData_() const {
    return (this->request_buffer_);
}

// stocke la réponse générée dans le buffer d'écriture du client
void    Client::setResponseData_(const std::string &data)
{
    this->response_buffer_ = data;
}

// récupère le prochain bloc de données à envoyer au client
void    Client::eraseSentResponseData_(ssize_t bytes_sent) {
    this->response_buffer_.erase(0, bytes_sent);
}

// récupère le prochain bloc de données à envoyer au client
void    Client::clearBuffers_() {
    this->request_buffer_.clear();
    this->response_buffer_.clear();
    setState(READING_REQUEST);
}

